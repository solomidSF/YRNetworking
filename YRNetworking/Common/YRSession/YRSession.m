//
//  YRSession.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRSession.h"
#import "YRPacket.h"
#import "YRLightweightInputStream.h"
#import "YRLightweightOutputStream.h"

// Will be removed
#import "YRSendOperation.h"

#import "YRSharedLogger.h"
// TODO: Temporary for developing purposes
#import "GCDAsyncUdpSocket.h"
#import "YRDebugUtils.h"

static uint32_t const kYRMaxPacketSize = 65536;

@implementation YRSession {
    YRLogger *_sessionLogger;
    
    YRSessionContext *_sessionContext;
    
    NSMutableArray *_sendQueue;
    NSMutableArray *_receiveQueue;
    
    // Tells if given session was initiating the connection request.
    BOOL _isInitiator;
    
    // Send-related
    uint16_t _sendInitialSequenceNumber;
    uint16_t _sendNextSequenceNumber;
    uint16_t _sendLatestUnackSegment;
    
    // Receiver-related
    uint16_t _rcvLatestAckedSegment;
    uint16_t _rcvInitialSequenceNumber;
    
    NSTimer *_disconnectingTimer;
}

#pragma mark - Initialization

- (instancetype)initWithContext:(YRSessionContext *)context {
    if (self = [super init]) {
        _sessionContext = [context copy];
        
        NSString *address = [NSString stringWithFormat:@"%@:%d", [GCDAsyncUdpSocket hostFromAddress:context.peerAddress], [GCDAsyncUdpSocket portFromAddress:context.peerAddress]];
        
        _sessionLogger = [[YRSharedLogger shared] loggerWithReporter:[NSString stringWithFormat:@"YRSession (%p)", self]];
        [_sessionLogger logInfo:@"[INIT]: Protocol version: %d. Peer address: %@. State callout: %d. Receive callout: %d. Send callout: %d", kYRProtocolVersion,
            address, context.connectionStateCallout != NULL, context.receiveCallout != NULL, context.sendCallout != NULL];
        
        _sendQueue = [NSMutableArray arrayWithCapacity:kYRMaxPacketSize];
        _receiveQueue = [NSMutableArray arrayWithCapacity:kYRMaxPacketSize];
        
        for (uint32_t i = 0; i < kYRMaxPacketSize; i++) {
            _sendQueue[i] = [NSNull null];
            _receiveQueue[i] = [NSNull null];
        }
    }
    
    return self;
}

- (void)dealloc {
    [_sessionLogger logInfo:@"[DEALLOC]"];
}

#pragma mark - Dynamic Properties

- (NSData *)peerAddress {
    return _sessionContext.peerAddress;
}

#pragma mark - Configuration

- (void)connect {
    [_sessionLogger logInfo:@"[CONN_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (self.state == kYRSessionStateClosed) {
        YRPacketRef packet = YRPacketCreateSYN(_sendNextSequenceNumber, 0, NO);
        YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:_sendNextSequenceNumber];

        _sendNextSequenceNumber++;
        
        [self sendReliably:operation];
        
        _isInitiator = YES;
        [self transiteToState:kYRSessionStateInitiating];
    } else {
        [_sessionLogger logWarning:@"[CONN_REQ]: Trying to transite into '%@' from %@", [self humanReadableState:kYRSessionStateInitiating], [self humanReadableState:self.state]];
    }
}

- (void)wait {
    [_sessionLogger logInfo:@"[WAIT_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (self.state == kYRSessionStateClosed) {
        [self transiteToState:kYRSessionStateWaiting];
    } else {
        [_sessionLogger logWarning:@"[WAIT_REQ]: Trying to transite into '%@' from %@", [self humanReadableState:kYRSessionStateWaiting], [self humanReadableState:self.state]];
    }
}

- (void)close {
    [_sessionLogger logInfo:@"[CLOSE_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (self.state == kYRSessionStateConnected) {
        YRPacketRef packet = YRPacketCreateRST(_sendNextSequenceNumber, _rcvLatestAckedSegment, true);
        YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:_sendNextSequenceNumber];

        _sendNextSequenceNumber++;
        
        [self sendReliably:operation];
        
        [self transiteToState:kYRSessionStateDisconnecting];
        
        [_disconnectingTimer invalidate];
        _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
            selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
        
        return;
    }
    
    if (self.state == kYRSessionStateWaiting) {
        [self transiteToState:kYRSessionStateClosed];
        return;
    }
    
    if (self.state == kYRSessionStateConnecting || self.state == kYRSessionStateInitiating) {
        YRPacketRef packet = YRPacketCreateRST(_sendNextSequenceNumber, 0, false);
        YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:_sendNextSequenceNumber];

        [self sendUnreliably:operation];
        
        [self transiteToState:kYRSessionStateClosed];
        return;
    }
}

- (void)invalidate {
    [_sessionLogger logInfo:@"[INVL_REQ] (%@): <NOT IMPLEMENTED YET>", [self humanReadableState:self.state]];
    // TODO:
    
    [self transiteToState:kYRSessionStateClosed];
}

#pragma mark - Communication

- (void)receive:(NSData *)data {
    if (data.length > kYRMaxPacketSize) {
        [_sessionLogger logWarning:@"[RCV_REQ]: Dropping large packet (%d bytes)", (uint32_t)data.length];
        // Drop packets with large sizes.
        return;
    }
    
    // Create input stream on stack to read incoming packet.
    uint8_t bufferForStream[kYRLightweightInputStreamSize];
    YRLightweightInputStreamRef stream = YRLightweightInputStreamCreateAt(data.bytes, data.length, bufferForStream);
    
    bool canDeserialize = YRPacketCanDeserializeFromStream(stream);
    
    if (!canDeserialize) {
        // TODO: Error
        return;
    }
    
    YRPacketRef receivedPacket = YRPacketDeserialize(stream);
    YRPacketHeaderRef receivedHeader = YRPacketGetHeader(receivedPacket);
    
//    BOOL shouldResetConnection = NO;
    BOOL isValidPacket = YRPacketIsLogicallyValid(receivedPacket);

    if (isValidPacket) {
        [_sessionLogger logInfo:@"[RCV_REQ] (%@) Packet: %@", [self humanReadableState:self.state], [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(receivedPacket)]];
    } else {
        [_sessionLogger logWarning:@"[RCV_REQ] (%@) Invalid Packet!\n%@", [self humanReadableState:self.state], [YRDebugUtils packetHeaderFullDescription:YRPacketGetHeader(receivedPacket)]];
        
        YRPacketDestroy(receivedPacket);
        return;
    }

//    if (shouldResetConnection) {
//        [self close];
//        return;
//    }
    
    BOOL isSYN = YRPacketHeaderIsSYN(receivedHeader);
    BOOL isRST = YRPacketHeaderIsRST(receivedHeader);
    BOOL isNUL = YRPacketHeaderIsNUL(receivedHeader);
    BOOL hasACK = YRPacketHeaderHasACK(receivedHeader);

    YRSequenceNumberType sequenceNumber = YRPacketHeaderGetSequenceNumber(receivedHeader);
    YRSequenceNumberType ackNumber = YRPacketHeaderGetAckNumber(receivedHeader);
    
    switch (self.state) {
        case kYRSessionStateClosed:
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRPacketRef rstPacket = YRPacketCreateRST(ackNumber + 1, 0, false);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:ackNumber + 1];
                
                [self sendUnreliably:operation];
            } else {
                YRPacketRef rstPacket = YRPacketCreateRST(0, sequenceNumber, true);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:0];

                [self sendUnreliably:operation];
            }
            
            break;
        case kYRSessionStateWaiting:
            // Waiting for SYN packet.
            if (isSYN) {
                _rcvLatestAckedSegment = sequenceNumber;
                _rcvInitialSequenceNumber = sequenceNumber;
                
                YRPacketRef synAckPacket = YRPacketCreateSYN(_sendNextSequenceNumber, _rcvLatestAckedSegment, true);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:synAckPacket sequenceNumber:_sendNextSequenceNumber];
                _sendNextSequenceNumber++;

                [self sendReliably:operation];
                
                [self transiteToState:kYRSessionStateConnecting];
                
                break;
            }
            
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRPacketRef rstPacket = YRPacketCreateRST(ackNumber + 1, 0, false);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:_sendNextSequenceNumber];

                [self sendUnreliably:operation];
                break;
            }
            
            break;
        case kYRSessionStateInitiating:
            // Waiting for SYN/ACK
            if (isRST && hasACK) {
                // err: Connection refused
                [_sessionLogger logError:@"Connection refused!"];
                [self transiteToState:kYRSessionStateClosed];
                break;
            }
            
            if (isSYN) {
                _rcvLatestAckedSegment = sequenceNumber;
                _rcvInitialSequenceNumber = sequenceNumber;
                
                if (hasACK) {
                    for (uint16_t i = _sendLatestUnackSegment; i < ackNumber + 1; i++) {
                        [_sendQueue[i] end];
                        _sendQueue[i] = [NSNull null];
                    }

                    _sendLatestUnackSegment = ackNumber + 1;
                    
                    YRPacketRef ackPacket = YRPacketCreateACK(_sendNextSequenceNumber, _rcvLatestAckedSegment);
                    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:ackPacket sequenceNumber:_sendNextSequenceNumber];
                    [self sendUnreliably:operation];
                    
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    YRPacketRef synAckPacket = YRPacketCreateSYN(_sendNextSequenceNumber, _rcvLatestAckedSegment, true);
                    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:synAckPacket sequenceNumber:_sendNextSequenceNumber];
                    [self sendReliably:operation];
                    
                    [self transiteToState:kYRSessionStateConnecting];
                }
                
                break;
            }
            
            break;
        case kYRSessionStateConnecting:
            if (_rcvInitialSequenceNumber == sequenceNumber - 1) {
                // Can process further.
            } else {
                YRPacketRef ackPacket = YRPacketCreateACK(_sendNextSequenceNumber, _rcvLatestAckedSegment);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:ackPacket sequenceNumber:_sendNextSequenceNumber];
                
                [self sendUnreliably:operation];
                
                break;
            }
            
            if (isSYN) {
                YRSequenceNumberType seqNumberToRespond = hasACK ? ackNumber + 1 : 0;
                YRPacketRef rstPacket = YRPacketCreateRST(seqNumberToRespond, 0, false);
                
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:seqNumberToRespond];
                [self sendUnreliably:operation];

                [_sessionLogger logError:@"Connection reset!"];
                
                [self transiteToState:kYRSessionStateClosed];
                break;
            }

            // Not supported in v1
//            if (header.packetDescription & YRPacketDescriptionEAK) {
//                // snd: RST
//                return;
//            }
            
            if (hasACK) {
                if (ackNumber == _sendInitialSequenceNumber) {
                    for (uint16_t i = _sendLatestUnackSegment; i < ackNumber + 1; i++) {
                        [_sendQueue[i] end];
                        _sendQueue[i] = [NSNull null];
                    }

                    _sendLatestUnackSegment = ackNumber + 1;
                    
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    YRSequenceNumberType seqNumber = ackNumber + 1;
                    YRPacketRef rstPacket = YRPacketCreateRST(seqNumber, 0, false);
                    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:seqNumber];
                    
                    [self sendUnreliably:operation];
                }
            } else {
                break;
            }
            
            if (isNUL) {
                YRSequenceNumberType seqNumber = ackNumber + 1;
                YRPacketRef rstPacket = YRPacketCreateRST(seqNumber, 0, false);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:seqNumber];
                
                [self sendUnreliably:operation];
            }
            
            // No data should be received in this state. (v1)
            // FOR NOW
//            if ((header.packetDescription & YRPacketDescriptionNUL) || header.dataLength > 0) {
//                if (header.sequenceNumber == _rcvLatestAckedSegment + 1) {
//                    _rcvLatestAckedSegment = header.sequenceNumber;
//
//                    if (header.dataLength > 0) {
//                        // Copy data
//                    }
//
//                    // snd: ACK
//                } else {
//                    // Buffer it and do EACK
//                }
//            }
            
            break;
        case kYRSessionStateConnected:
            if (_rcvLatestAckedSegment == sequenceNumber - 1) {
                // Can process further.
            } else {
                YRPacketRef ackPacket = YRPacketCreateACK(_sendNextSequenceNumber, _rcvLatestAckedSegment);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:ackPacket sequenceNumber:_sendNextSequenceNumber];
                
                [self sendUnreliably:operation];
                
                break;
            }
            
            if (isRST) {
                // signal: connection reset
                [_disconnectingTimer invalidate];
                _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
                    selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
                
                [self transiteToState:kYRSessionStateDisconnecting];
                break;
            }
            
            if (isNUL) {
                _rcvLatestAckedSegment = sequenceNumber;
                YRPacketRef ackPacket = YRPacketCreateACK(_sendNextSequenceNumber, _rcvLatestAckedSegment);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:ackPacket sequenceNumber:_sendNextSequenceNumber];

                [self sendUnreliably:operation];

                break;
            }
            
            if (isSYN) {
                // + snd: RST
                YRSequenceNumberType seqNumber = hasACK ? ackNumber + 1 : 0;
                YRPacketRef rstPacket = YRPacketCreateRST(seqNumber, 0, false);
                YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:rstPacket sequenceNumber:seqNumber];
                
                [self sendUnreliably:operation];
                
                // call: connection reset
                [self transiteToState:kYRSessionStateClosed];
                
                break;
            }
            
            if (hasACK) {
                if (_sendLatestUnackSegment <= ackNumber &&
                    ackNumber < _sendNextSequenceNumber) {

                    // queue: flush.
                    for (uint16_t i = _sendLatestUnackSegment; i < ackNumber + 1; i++) {
                        [_sendQueue[i] end];
                        _sendQueue[i] = [NSNull null];
                    }
                    
                    _sendLatestUnackSegment = ackNumber + 1;
                }
            }

            // Not supported in v1
//            if (header.packetDescription & YRPacketDescriptionEAK) {
//                // queue: flush
//            }
            
            if (YRPacketHeaderGetPayloadLength(receivedHeader) > 0) {
                if (sequenceNumber == _rcvLatestAckedSegment + 1) {
                    _rcvLatestAckedSegment = sequenceNumber;
                    // + snd: ACK
                    YRPacketRef ackPacket = YRPacketCreateACK(_sendNextSequenceNumber, _rcvLatestAckedSegment);
                    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:ackPacket sequenceNumber:_sendNextSequenceNumber];
                    
                    [self sendUnreliably:operation];
                    
                    // Do callout with data.
                    YRPayloadLengthType payloadLength = 0;
                    void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
                    
                    if (rawPayload) {
                        NSData *rawPacketData = [NSData dataWithBytesNoCopy:rawPayload length:payloadLength];
                        
                        _sessionContext.receiveCallout(self, rawPacketData);
                    } else {
                        // TODO: Packet is not valid? It must be valid if we get here!
                    }
                } else {
                    // Not supported in v1
                    // Buffer data
                    // snd: EAK
                }
            }
            
            break;
        case kYRSessionStateDisconnecting:
            if (isRST) {
                [_disconnectingTimer invalidate];
                
                [self transiteToState:kYRSessionStateClosed];
            }
            break;
        default:
            [_sessionLogger logError:@"[RCV_REQ] <FATAL> Can't handle incoming packet because state is not defined: %d", self.state];
            NSAssert(NO, @"Can't handle incoming packet because state is not defined: %d", self.state);
            break;
    }
    
    YRPacketDestroy(receivedPacket);
}

- (void)send:(NSData *)data {
    if (data.length == 0) {
        // TODO: Error
        return;
    }

    [_sessionLogger logInfo:@"[SEND_REQ] (%@)", [self humanReadableState:self.state]];

    YRPacketRef packet = YRPacketCreateWithPayload(_sendNextSequenceNumber, _rcvLatestAckedSegment, data.bytes, data.length);
    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:_sendNextSequenceNumber];

    _sendNextSequenceNumber++;
    
    [self sendReliably:operation];
}

#pragma mark - Callbacks

- (void)handleDisconnectTimeout:(NSTimer *)timer {
    [_sessionLogger logInfo:@"[DSC_TIM]: Disconnect timeout reached."];
    
    [self transiteToState:kYRSessionStateClosed];
}

#pragma mark - Private

- (NSString *)humanReadableState:(YRSessionState)state {
    return @[
        @"Closed",
        @"Waiting",
        @"Initiating",
        @"Connecting",
        @"Connected",
        @"Disconnecting"
    ][state];
}
         
/**
 *  This ensures:
 *  Packet size doesn't exceed maximum segment size /=> RST.
 *  Packet has header info > header minimum size /=> ignore.
 *  Calculates checksum /=> ignore packet.
 *  Ensures that protocol version is the same /=> RST.
 *  Ensures that data length matches correctly /=> ignore.
 *  Strictly speaking that ensures that packet is valid and can't break RUDP logic.
 */
//- (BOOL)checkIfPacketValid:(NSData *)packet outHeader:(YRPacketHeader *)outHeader shouldResetConnection:(BOOL *)shouldReset {
//    // Fill buffer with zeroes
//    char packetBuffer[kYRMaxPacketSize] = {0};
//    char headerBuffer[kYRMaxPacketSize] = {0};
//
//    BOOL stubShouldReset = NO;
//    BOOL *ensuredShouldResetPointer = shouldReset;
//
//    if (!ensuredShouldResetPointer) {
//        ensuredShouldResetPointer = &stubShouldReset;
//    }
//
//    // Verify that received packet size doesn't exceed maximum packet size.
//    if (packet.length > kYRMaxPacketSize) {
//        // Packet size exceeds maximum segment size.
//        [_sessionLogger logError:@"[PACK_VAL]: Packet size exceeds maximum segment size!"];
//
//        *ensuredShouldResetPointer = YES;
//        return NO;
//    }
//
//    // Verify that packet size is not less than header size.
//    if (packet.length < YRMinimumPacketHeaderSize) {
//        // Received packet that has less bytes than regular packet header.
//        [_sessionLogger logWarning:@"[PACK_VAL]: Packet size less than packet header!"];
//
//        return NO;
//    }
//
//    // Get raw packet data.
//    [packet getBytes:packetBuffer range:(NSRange){0, packet.length}];
//
//    uint8_t headerLength = ((YRPacketHeader *)packetBuffer)->headerLength;
//
//    if (headerLength < YRMinimumPacketHeaderSize) {
//        // Received packet has incorrect header length specified.
//        [_sessionLogger logWarning:@"[PACK_VAL]: Header length is incorrect!"];
//
//        return 0;
//    }
//
//    [packet getBytes:headerBuffer range:(NSRange){0, headerLength}];
//
//    YRPacketHeader *header = (YRPacketHeader *)headerBuffer;
//
//    // Verify checksum.
//    uint32_t checksum = [self calculateChecksum:header];
//
//    if (checksum != header->checksum) {
//        // Checksum mismatch.
//        [_sessionLogger logWarning:@"[PACK_VAL]: Checksum mismatch!"];
//
//        return NO;
//    }
//
//    // Verify protocol version.
//    if (((header->packetDescription & YRPacketDescriptionProtocolVersionMask) >> 6) != kYRProtocolVersion) {
//        // Protocol version doesn't match.
//        [_sessionLogger logError:@"[PACK_VAL]: Protocol mismatch!"];
//
//        *ensuredShouldResetPointer = YES;
//        return NO;
//    }
//
//    // Verify data length.
//    if ((header->headerLength + header->dataLength) > packet.length) {
//        // Invalid sizes provided.
//
//        [_sessionLogger logWarning:@"[PACK_VAL]: Header/data sizes mismatch!"];
//
//        return NO;
//    }
//
//    if (outHeader) {
//        memcpy(outHeader, header, headerLength);
//    }
//
//    return YES;
//}

- (void)transiteToState:(YRSessionState)state {
    if (_state != state) {
        [_sessionLogger logInfo:@"[STATE_CHG]: (%@) -> (%@)", [self humanReadableState:_state], [self humanReadableState:state]];
        _state = state;
        
        _sessionContext.connectionStateCallout(self, state);
    }
}

- (void)sendReliably:(YRSendOperation *)operation {
    YRPacketCopyPayloadInline(operation.packet);
    
    _sendQueue[operation.sequenceNumber] = operation;

    [operation start];
    
    __typeof(self) __weak weakSelf = self;
    __typeof(YRLogger *) __weak weakLogger = _sessionLogger;
    
    operation.onTransmissionTimeout = ^(YRSendOperation *operation) {
        [weakLogger logInfo:@"[SEND_TIM]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];
        
        [weakSelf sendReliably:operation];
    };
    
    [_sessionLogger logInfo:@"[SEND_REL]: Header: %@", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];
    
    [self doSend:operation];
}

- (void)sendUnreliably:(YRSendOperation *)operation {
    [_sessionLogger logInfo:@"[SEND_UNR]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];

    [self doSend:operation];
}

- (void)doSend:(YRSendOperation *)operation {
    YRPayloadLengthType packetLength = YRPacketGetLength(operation.packet);
    
    uint8_t outputStreamBuffer[kYRLightweightOutputStreamSize];
    uint8_t packetBuffer[packetLength];
    
    YRLightweightOutputStreamRef outputStream = YRLightweightOutputStreamCreateAt(packetBuffer, packetLength, outputStreamBuffer);
    
    YRPacketSerialize(operation.packet, outputStream);
    
    NSData* data = [NSData dataWithBytesNoCopy:YRLightweightOutputStreamGetBytes(outputStream) length:packetLength freeWhenDone:NO];
    
    _sessionContext.sendCallout(self, data);
}

@end
