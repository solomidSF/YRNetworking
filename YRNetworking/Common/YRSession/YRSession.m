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

// Temporary files
#import "YRSendOperation.h"
#import "YRReceiveOperation.h"

#import "YRSharedLogger.h"
// TODO: Temporary for developing purposes
#import "GCDAsyncUdpSocket.h"
#import "YRDebugUtils.h"

static uint32_t const kYRMaxPacketSize = 65536;

@implementation YRSession {
    YRLogger *_sessionLogger;
    
    YRConnectionConfiguration _localConfiguration;
    YRConnectionConfiguration _remoteConfiguration;
    
    YRSessionContext *_sessionContext;
    
    NSMutableArray *_sendQueue;
    NSMutableArray *_receiveQueue;
    NSMutableArray <NSNumber *> *_outOfSequenceSegmentsReceived;
    
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
        _localConfiguration = (YRConnectionConfiguration) {
            .options = 0,
            .retransmissionTimeoutValue = 1000,
            .nullSegmentTimeoutValue = 3000,
            .maximumSegmentSize = 1200,
            .maxNumberOfOutstandingSegments = 20,
            .maxRetransmissions = 5,
        };
        
        _sessionContext = [context copy];
        
        NSString *address = [NSString stringWithFormat:@"%@:%d", [GCDAsyncUdpSocket hostFromAddress:context.peerAddress], [GCDAsyncUdpSocket portFromAddress:context.peerAddress]];
        
        _sessionLogger = [[YRSharedLogger shared] loggerWithReporter:[NSString stringWithFormat:@"YRSession (%p)", self]];
        [_sessionLogger logInfo:@"[INIT]: Protocol version: %d. Peer address: %@. State callout: %d. Receive callout: %d. Send callout: %d", kYRProtocolVersion,
            address, context.connectionStateCallout != NULL, context.receiveCallout != NULL, context.sendCallout != NULL];
        
        _sendQueue = [NSMutableArray arrayWithCapacity:kYRMaxPacketSize];
        _receiveQueue = [NSMutableArray arrayWithCapacity:kYRMaxPacketSize];
        _outOfSequenceSegmentsReceived = [NSMutableArray new];
        
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
        _isInitiator = YES;
        [self transiteToState:kYRSessionStateInitiating];

        YRPacketRef packet = YRPacketCreateSYN(_localConfiguration, _sendNextSequenceNumber, 0, NO);
        _sendNextSequenceNumber++;
        
        [self sendPacketReliably:packet];
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
        [self transiteToState:kYRSessionStateDisconnecting];

        YRPacketRef packet = YRPacketCreateRST(0, _sendNextSequenceNumber, _rcvLatestAckedSegment, true);
        _sendNextSequenceNumber++;
        
        [self sendPacketReliably:packet];
        
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
        [self transiteToState:kYRSessionStateClosed];

        YRPacketRef packet = YRPacketCreateRST(0, _sendNextSequenceNumber, 0, false);

        [self sendPacketUnreliably:packet];
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
    
    if (data.length > _localConfiguration.maximumSegmentSize) {
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
    BOOL hasEACK = YRPacketHeaderHasEACK(receivedHeader);

    YRSequenceNumberType sequenceNumber = YRPacketHeaderGetSequenceNumber(receivedHeader);
    YRSequenceNumberType ackNumber = YRPacketHeaderGetAckNumber(receivedHeader);
    
    switch (self.state) {
        case kYRSessionStateClosed:
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRPacketRef rstPacket = YRPacketCreateRST(0, ackNumber + 1, 0, false);
                
                [self sendPacketUnreliably:rstPacket];
            } else {
                YRPacketRef rstPacket = YRPacketCreateRST(0, 0, sequenceNumber, true);

                [self sendPacketUnreliably:rstPacket];
            }
            
            break;
        case kYRSessionStateWaiting:
            // Waiting for SYN packet.
            if (isSYN) {
                _rcvLatestAckedSegment = sequenceNumber;
                _rcvInitialSequenceNumber = sequenceNumber;
                
                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
                
                _remoteConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
                
                [self transiteToState:kYRSessionStateConnecting];

                YRPacketRef synAckPacket = YRPacketCreateSYN(_localConfiguration, _sendNextSequenceNumber,
                    _rcvLatestAckedSegment, true);
                _sendNextSequenceNumber++;

                [self sendPacketReliably:synAckPacket];
                
                break;
            }
            
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRPacketRef rstPacket = YRPacketCreateRST(0, ackNumber + 1, 0, false);

                [self sendPacketUnreliably:rstPacket];
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
                
                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
                
                _remoteConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
                
                if (hasACK) {
                    for (uint16_t i = _sendLatestUnackSegment; i < ackNumber + 1; i++) {
                        if ([_sendQueue[i] isKindOfClass:[YRSendOperation class]]) {
                            [_sendQueue[i] end];
                            _sendQueue[i] = [NSNull null];
                        }
                    }

                    _sendLatestUnackSegment = ackNumber + 1;
                    
                    [self transiteToState:kYRSessionStateConnected];

                    [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
                } else {
                    [self transiteToState:kYRSessionStateConnecting];
                    
                    // TODO: This flow is strange as rfc describes.
                    // if SYN & SYN rcved on both sides, SYN/ACK will be ignored on both sides which will result in ACK sent by both peer = connected.
                    // if - & SYN/ACK rcvd = SYN/ACK side will resend SYN/ACK.
                    YRPacketRef synAckPacket = YRPacketCreateSYN(_localConfiguration, _sendInitialSequenceNumber,
                        _rcvLatestAckedSegment, true);
                    [self sendPacketReliably:synAckPacket];
                }
                
                break;
            }
            
            break;
        case kYRSessionStateConnecting: {
            YRSequenceNumberType expectedToReceive = _rcvLatestAckedSegment + 1;

            BOOL willWrapAround = (expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments) < expectedToReceive;

            BOOL canProcessPacket = expectedToReceive <= sequenceNumber && sequenceNumber <= (expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments);

            if (willWrapAround) {
                canProcessPacket = expectedToReceive <= sequenceNumber || sequenceNumber <= (expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments);
            }

            if (canProcessPacket) {
                // Can process further.
            } else {
                [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
                
                break;
            }
            
            if (isRST) {
                if (!_isInitiator) {
                    [self transiteToState:kYRSessionStateWaiting];
                } else {
                    [_sessionLogger logError:@"Connection refused!"];
                    [self transiteToState:kYRSessionStateClosed];
                }
                
                break;
            }
            
            if (isSYN) {
                [_sessionLogger logError:@"Connection reset!"];
                [self transiteToState:kYRSessionStateClosed];

                YRSequenceNumberType seqNumberToRespond = hasACK ? ackNumber + 1 : 0;
                YRPacketRef rstPacket = YRPacketCreateRST(0, seqNumberToRespond, 0, false);
                [self sendPacketUnreliably:rstPacket];
                
                break;
            }

            if (hasEACK) {
                [self transiteToState:kYRSessionStateClosed];
                
                YRSequenceNumberType seqNumberToRespond = hasACK ? ackNumber + 1 : 0;
                YRPacketRef rstPacket = YRPacketCreateRST(0, seqNumberToRespond, 0, false);
                [self sendPacketUnreliably:rstPacket];

                break;
            }
            
            if (hasACK) {
                if (ackNumber == _sendInitialSequenceNumber) {
                    for (uint16_t i = _sendLatestUnackSegment; i < ackNumber + 1; i++) {
                        if ([_sendQueue[i] isKindOfClass:[YRSendOperation class]]) {
                            [_sendQueue[i] end];
                            _sendQueue[i] = [NSNull null];
                        }
                    }

                    _sendLatestUnackSegment = ackNumber + 1;
                    
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    YRSequenceNumberType seqNumber = ackNumber + 1;
                    YRPacketRef rstPacket = YRPacketCreateRST(0, seqNumber, 0, false);
                    
                    [self sendPacketUnreliably:rstPacket];
                    
                    break;
                }
            }
            
            YRPayloadLengthType payloadLength = 0;
            
            if (YRPacketHeaderHasPayloadLength(receivedHeader)) {
                payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader);
            }
            
            if (isNUL || payloadLength > 0) {
                if (sequenceNumber == expectedToReceive) {
                    _rcvLatestAckedSegment = sequenceNumber;

                    if (payloadLength > 0) {
                        // Copy data
                        void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
                        
                        if (rawPayload) {
                            NSData *rawPacketData = [NSData dataWithBytesNoCopy:rawPayload length:payloadLength freeWhenDone:NO];
                            
                            _sessionContext.receiveCallout(self, rawPacketData);
                        } else {
                            // TODO: Packet is not valid? It must be valid if we get here!
                        }
                    }
                    
                    [self processOutOfSequencePacketsWithCallouts];
                } else {
                    if (![_receiveQueue[sequenceNumber] isKindOfClass:[YRReceiveOperation class]]) {
                        // Buffer data
                        // snd: EAK
                        YRPacketCopyPayloadInline(receivedPacket);
                        
                        _receiveQueue[sequenceNumber] = [[YRReceiveOperation alloc] initWithPacket:receivedPacket];
                        [_outOfSequenceSegmentsReceived addObject:@(sequenceNumber)];
                        
                        receivedPacket = NULL;
                    } else {
                        // Received duplicated out of sequence segment, ignore.
                    }
                }
                
                [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
            }
            }
            break;
        case kYRSessionStateConnected: {
            YRSequenceNumberType expectedToReceive = _rcvLatestAckedSegment + 1;
            
            YRSequenceNumberType maxSegmentThatCanBeReceived = expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments;
            BOOL willWrapAround = maxSegmentThatCanBeReceived < expectedToReceive;
            
            BOOL canProcessPacket = expectedToReceive <= sequenceNumber && sequenceNumber <= maxSegmentThatCanBeReceived;
            
            if (willWrapAround) {
                canProcessPacket = expectedToReceive <= sequenceNumber || sequenceNumber <= maxSegmentThatCanBeReceived;
            }
            
            if (canProcessPacket) {
                // Can process further.
            } else {
                [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
                
                break;
            }
            
            if (isSYN) {
                // call: connection reset
                [self transiteToState:kYRSessionStateClosed];
                
                YRSequenceNumberType seqNumber = hasACK ? ackNumber + 1 : 0;
                YRPacketRef rstPacket = YRPacketCreateRST(0, seqNumber, 0, false);
                [self sendPacketUnreliably:rstPacket];
                
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
            
            if (hasACK) {
//                if (_sendLatestUnackSegment <= ackNumber &&
//                    ackNumber < _sendNextSequenceNumber) {

                    // queue: flush.
                YRSequenceNumberType newLatestUnackSegment = ackNumber + 1;
                    for (YRSequenceNumberType i = _sendLatestUnackSegment; i != newLatestUnackSegment; i++) {
                        if ([_sendQueue[i] isKindOfClass:[YRSendOperation class]]) {
                            [_sendQueue[i] end];
                            _sendQueue[i] = [NSNull null];
                        }
                    }
                    
                    _sendLatestUnackSegment = newLatestUnackSegment;
//                }
            }
            
            if (hasEACK) {
                // queue: flush
                YRPacketHeaderEACKRef receivedEACKHeader = (YRPacketHeaderEACKRef)receivedHeader;
                
                YRSequenceNumberType eacksCount = 0;
                YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(receivedEACKHeader, &eacksCount);
                
                for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
                    YRSequenceNumberType sequence = eacks[i];
                    
                    if ([_sendQueue[sequence] isKindOfClass:[YRSendOperation class]]) {
                        [_sendQueue[sequence] end];
                        _sendQueue[sequence] = [NSNull null];
                    }
                }
            }

            if (isNUL) {
                // TODO: Next iteration: NUL segment.
                // We must ack it if it falls into acceptance window.
                // However we shouldn't set _rcvLatestAckedSegment to this seq number.
                _rcvLatestAckedSegment = sequenceNumber;
                [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
                
                break;
            }
            
            if (YRPacketHeaderHasPayloadLength(receivedHeader) &&
                YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader) > 0) {
                if (sequenceNumber == expectedToReceive) {
                    _rcvLatestAckedSegment = sequenceNumber;
                    
                    // Do callout with data.
                    YRPayloadLengthType payloadLength = 0;
                    void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
                    
                    if (rawPayload) {
                        NSData *rawPacketData = [NSData dataWithBytesNoCopy:rawPayload length:payloadLength freeWhenDone:NO];
                        
                        _sessionContext.receiveCallout(self, rawPacketData);
                    } else {
                        // TODO: Packet is not valid? It must be valid if we get here!
                    }
                    
                    [self processOutOfSequencePacketsWithCallouts];
                } else {
                    if (![_receiveQueue[sequenceNumber] isKindOfClass:[YRReceiveOperation class]]) {
                        // Buffer data
                        // snd: EAK
                        YRPacketCopyPayloadInline(receivedPacket);
                        
                        _receiveQueue[sequenceNumber] = [[YRReceiveOperation alloc] initWithPacket:receivedPacket];
                        [_outOfSequenceSegmentsReceived addObject:@(sequenceNumber)];
                        
                        receivedPacket = NULL;
                    } else {
                        // Received duplicated out of sequence segment, ignore.
                    }
                }
                
                [self doACKOrEACKWithSequenceNumber:_sendNextSequenceNumber ackNumber:_rcvLatestAckedSegment];
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

    _sendNextSequenceNumber++;
    
    [self sendPacketReliably:packet];
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

- (void)transiteToState:(YRSessionState)state {
    if (_state != state) {
        [_sessionLogger logInfo:@"[STATE_CHG]: (%@) -> (%@)", [self humanReadableState:_state], [self humanReadableState:state]];
        _state = state;
        
        _sessionContext.connectionStateCallout(self, state);
    }
}

- (void)doACKOrEACKWithSequenceNumber:(YRSequenceNumberType)seqNumber ackNumber:(YRSequenceNumberType)ackNumber {
    YRPacketRef packet = NULL;
    
    if (_outOfSequenceSegmentsReceived.count > 0) {
        YRSequenceNumberType sequencesCount = _outOfSequenceSegmentsReceived.count > 255 ? 255 : _outOfSequenceSegmentsReceived.count;
        YRSequenceNumberType sequences[sequencesCount];
        
        for (YRSequenceNumberType i = 0; i < sequencesCount; i++) {
            sequences[i] = [_outOfSequenceSegmentsReceived[i] intValue];
        }
        
        packet = YRPacketCreateEACK(seqNumber, ackNumber, sequences, &sequencesCount);
    } else {
        packet = YRPacketCreateACK(seqNumber, ackNumber);
    }
    
    [self sendPacketUnreliably:packet];
}

- (void)processOutOfSequencePacketsWithCallouts {
    YRSequenceNumberType nextSegment = _rcvLatestAckedSegment + 1;
    id operation = _receiveQueue[nextSegment];
    
    while ([operation isKindOfClass:[YRReceiveOperation class]]) {
        _rcvLatestAckedSegment = nextSegment;
        
        YRReceiveOperation *outOfSeqOperation = operation;
        YRPacketRef packet = outOfSeqOperation.packet;

        // Do callout with data.
        YRPayloadLengthType payloadLength = 0;
        void *rawPayload = YRPacketGetPayload(packet, &payloadLength);
        
        if (rawPayload) {
            NSData *rawPacketData = [NSData dataWithBytesNoCopy:rawPayload length:payloadLength freeWhenDone:NO];
            
            !_sessionContext.receiveCallout ?: _sessionContext.receiveCallout(self, rawPacketData);
        }

        _receiveQueue[nextSegment] = [NSNull null];
        [_outOfSequenceSegmentsReceived removeObject:@(nextSegment)];

        nextSegment++;
        
        operation = _receiveQueue[nextSegment];
    }
}

- (void)sendPacketReliably:(YRPacketRef)packet {
    YRPacketCopyPayloadInline(packet);
    
    YRSequenceNumberType seqNumber = YRPacketHeaderGetSequenceNumber(YRPacketGetHeader(packet));
    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:seqNumber];

    if ([_sendQueue[seqNumber] isKindOfClass:[YRSendOperation class]]) {
        [_sendQueue[seqNumber] end];
    }
    
    _sendQueue[seqNumber] = operation;
    
    [operation start];
    
    __typeof(self) __weak weakSelf = self;
    __typeof(YRLogger *) __weak weakLogger = _sessionLogger;
    
    operation.onTransmissionTimeout = ^(YRSendOperation *operation) {
        [weakLogger logInfo:@"[SEND_TIM]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(packet)]];
        
        [operation start];
        [weakSelf sendDataFromPacket:operation.packet];
    };
    
    [_sessionLogger logInfo:@"[SEND_REL]: Header: %@", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];
    
    [self sendDataFromPacket:packet];
}

- (void)sendPacketUnreliably:(YRPacketRef)packet {
    [_sessionLogger logInfo:@"[SEND_UNR]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(packet)]];
    
    [self sendDataFromPacket:packet];
}

- (void)sendDataFromPacket:(YRPacketRef)packet {
    YRPayloadLengthType packetLength = YRPacketGetLength(packet);
    
    uint8_t outputStreamBuffer[kYRLightweightOutputStreamSize];
    uint8_t packetBuffer[packetLength];
    
    YRLightweightOutputStreamRef outputStream = YRLightweightOutputStreamCreateAt(packetBuffer, packetLength, outputStreamBuffer);
    
    YRPacketSerialize(packet, outputStream);
    
    NSData *data = [NSData dataWithBytesNoCopy:YRLightweightOutputStreamGetBytes(outputStream) length:packetLength freeWhenDone:NO];
    
    _sessionContext.sendCallout(self, data);
}

@end
