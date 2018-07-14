//
//  YRSession.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRSession.h"
#import "YRSendOperation.h"

static uint32_t const kYRMaxSequenceCount = 65536;
static uint16_t const kYRProtocolVersion = 0x0001;

@implementation YRSession {
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
        
        _sendQueue = [NSMutableArray arrayWithCapacity:kYRMaxSequenceCount];
        _receiveQueue = [NSMutableArray arrayWithCapacity:kYRMaxSequenceCount];
        
        for (uint32_t i = 0; i < kYRMaxSequenceCount; i++) {
            _sendQueue[i] = [NSNull null];
            _receiveQueue[i] = [NSNull null];
        }
    }
    
    return self;
}

- (void)dealloc {
    
}

#pragma mark - Dynamic Properties

- (NSData *)peerAddress {
    return _sessionContext.peerAddress;
}

#pragma mark - Configuration

- (void)connect {
    if (self.state == kYRSessionStateClosed) {
        YRPacketHeader header = {0};

        header.packetDescription = YRPacketDescriptionSYN | kYRProtocolVersion << 6;
        header.headerLength = sizeof(header);
        header.sequenceNumber = _sendNextSequenceNumber;
        header.checksum = [self calculateChecksum:&header];

        _sendNextSequenceNumber++;
        
        [self transiteToState:kYRSessionStateInitiating];
        
        NSMutableData *packetData = [NSMutableData new];
        
        [packetData appendBytes:&header length:sizeof(header)];
        
        YRSendOperation *operation = [[YRSendOperation alloc] initWithData:[packetData copy] sequenceNumber:header.sequenceNumber];
        
        [self sendReliably:operation];
    } else {
        NSLog(@"[YRSession]: Trying to transite into 'Initiating' state from %d", self.state);
    }
}

- (void)wait {
    if (self.state == kYRSessionStateClosed) {
        [self transiteToState:kYRSessionStateWaiting];
    } else {
        NSLog(@"[YRSession]: Trying to transite into 'Waiting' state from %d", self.state);
    }
}

- (void)close {
    YRPacketHeader outHeader = {0};
    
    outHeader.packetDescription = YRPacketDescriptionRST | YRPacketDescriptionACK | (kYRProtocolVersion << 6);
    outHeader.sequenceNumber = _sendNextSequenceNumber;
    outHeader.ackNumber = _rcvLatestAckedSegment;
    outHeader.headerLength = sizeof(YRPacketHeader);
    
    outHeader.checksum = [self calculateChecksum:&outHeader];
    
    _sendNextSequenceNumber++;
    
    [self sendReliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
        sequenceNumber:outHeader.sequenceNumber]];
    
    [self transiteToState:kYRSessionStateDisconnecting];
    
    [_disconnectingTimer invalidate];
    _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
        selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
}

- (void)invalidate {
    // TODO:
}

#pragma mark - Communication

- (void)receive:(NSData *)data {
    YRPacketHeader header = {0};
    BOOL shouldResetConnection = NO;
    BOOL isValidPacket = [self checkIfPacketValid:data outHeader:&header shouldResetConnection:&shouldResetConnection];
    
    if (!isValidPacket) {
        NSLog(@"[YRSession]: Received invalid packet!");
        return;
    }

    if (shouldResetConnection) {
        // TODO:
        return;
    }
    
    switch (self.state) {
        case kYRSessionStateClosed:
            if (header.packetDescription & YRPacketDescriptionRST) {
                return;
            }
            
            if ((header.packetDescription & YRPacketDescriptionACK) ||
                (header.packetDescription & YRPacketDescriptionNUL)) {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = header.ackNumber + 1;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
            } else {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = 0;
                outHeader.ackNumber = header.sequenceNumber;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
            }
            
            break;
        case kYRSessionStateWaiting:
            // Waiting for SYN packet.
            if (header.packetDescription & YRPacketDescriptionSYN) {
                _rcvLatestAckedSegment = header.sequenceNumber;
                _rcvInitialSequenceNumber = header.sequenceNumber;
                
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionSYN | YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = _sendNextSequenceNumber;
                outHeader.headerLength = sizeof(YRPacketHeader);
                outHeader.ackNumber = _rcvLatestAckedSegment;
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                _sendNextSequenceNumber++;
                
                YRSendOperation *operation = [[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber];
                [self sendReliably:operation];
                
                [self transiteToState:kYRSessionStateConnecting];
                
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionRST) {
                return;
            }
            
            if ((header.packetDescription & YRPacketDescriptionACK) ||
                (header.packetDescription & YRPacketDescriptionNUL)) {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = header.ackNumber + 1;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
                return;
            }
            
            break;
        case kYRSessionStateInitiating:
            // Waiting for SYN/ACK
            if ((header.packetDescription & YRPacketDescriptionRST) &&
                (header.packetDescription & YRPacketDescriptionACK)) {
                // err: Connection refused
                NSLog(@"[YRSession]: <Error> Connection refused!");
                [self transiteToState:kYRSessionStateClosed];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                _rcvLatestAckedSegment = header.sequenceNumber;
                _rcvInitialSequenceNumber = header.sequenceNumber;
                
                if (header.packetDescription & YRPacketDescriptionACK) {
                    for (uint16_t i = _sendLatestUnackSegment; i < header.ackNumber + 1; i++) {
                        [_sendQueue[i] end];
                        _sendQueue[i] = [NSNull null];
                    }

                    _sendLatestUnackSegment = header.ackNumber + 1;
                    
                    YRPacketHeader outHeader = {0};
                    
                    outHeader.packetDescription = YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                    outHeader.sequenceNumber = _sendNextSequenceNumber;
                    outHeader.headerLength = sizeof(YRPacketHeader);
                    outHeader.ackNumber = _rcvLatestAckedSegment;
                    
                    outHeader.checksum = [self calculateChecksum:&outHeader];
                    
                    [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                        sequenceNumber:outHeader.sequenceNumber]];
                    
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    YRPacketHeader outHeader = {0};
                    
                    outHeader.packetDescription = YRPacketDescriptionSYN | YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                    outHeader.sequenceNumber = _sendNextSequenceNumber;
                    outHeader.headerLength = sizeof(YRPacketHeader);
                    outHeader.ackNumber = _rcvLatestAckedSegment;
                    
                    outHeader.checksum = [self calculateChecksum:&outHeader];
                    
                    [self sendReliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                        sequenceNumber:outHeader.sequenceNumber]];
                    
                    [self transiteToState:kYRSessionStateConnecting];
                }
                
                return;
            }
            
            break;
        case kYRSessionStateConnecting:
            if (_rcvInitialSequenceNumber == header.sequenceNumber - 1) {
                // Can process further.
            } else {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = _sendNextSequenceNumber;
                outHeader.headerLength = sizeof(YRPacketHeader);
                outHeader.ackNumber = _rcvLatestAckedSegment;
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
                
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = (header.packetDescription & YRPacketDescriptionACK) ? header.ackNumber + 1 : 0;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];

                NSLog(@"[YRSession]: <Error> Connection reset!");
                
                [self transiteToState:kYRSessionStateClosed];
                return;
            }

            // Not supported in v1
//            if (header.packetDescription & YRPacketDescriptionEAK) {
//                // snd: RST
//                return;
//            }
            
            if (header.packetDescription & YRPacketDescriptionACK) {
                if (header.ackNumber == _sendInitialSequenceNumber) {
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    YRPacketHeader outHeader = {0};
                    
                    outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                    outHeader.sequenceNumber = header.ackNumber + 1;
                    outHeader.headerLength = sizeof(YRPacketHeader);
                    
                    outHeader.checksum = [self calculateChecksum:&outHeader];
                    
                    [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                        sequenceNumber:outHeader.sequenceNumber]];
                }
            } else {
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionNUL) {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = (header.packetDescription & YRPacketDescriptionACK) ? header.ackNumber + 1 : 0;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
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
            if (_rcvLatestAckedSegment == header.sequenceNumber - 1) {
                // Can process further.
            } else {
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = _sendNextSequenceNumber;
                outHeader.headerLength = sizeof(YRPacketHeader);
                outHeader.ackNumber = _rcvLatestAckedSegment;
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionRST) {
                // signal: connection reset
                [_disconnectingTimer invalidate];
                _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
                    selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
                
                [self transiteToState:kYRSessionStateDisconnecting];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionNUL) {
                _rcvLatestAckedSegment = header.sequenceNumber;
                
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = _sendNextSequenceNumber;
                outHeader.headerLength = sizeof(YRPacketHeader);
                outHeader.ackNumber = _rcvLatestAckedSegment;
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];

                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                // + snd: RST
                YRPacketHeader outHeader = {0};
                
                outHeader.packetDescription = YRPacketDescriptionRST | (kYRProtocolVersion << 6);
                outHeader.sequenceNumber = (header.packetDescription & YRPacketDescriptionACK) ? header.ackNumber + 1 : 0;
                outHeader.headerLength = sizeof(YRPacketHeader);
                
                outHeader.checksum = [self calculateChecksum:&outHeader];
                
                [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                    sequenceNumber:outHeader.sequenceNumber]];

                // call: connection reset
                [self transiteToState:kYRSessionStateClosed];
                
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionACK) {
                if (_sendLatestUnackSegment <= header.ackNumber &&
                    header.ackNumber < _sendNextSequenceNumber) {

                    // queue: flush.
                    for (uint16_t i = _sendLatestUnackSegment; i < header.ackNumber + 1; i++) {
                        [_sendQueue[i] end];
                        _sendQueue[i] = [NSNull null];
                    }
                    
                    _sendLatestUnackSegment = header.ackNumber + 1;
                }
            }

            // Not supported in v1
//            if (header.packetDescription & YRPacketDescriptionEAK) {
//                // queue: flush
//            }
            
            if (header.dataLength > 0) {
                if (header.sequenceNumber == _rcvLatestAckedSegment + 1) {
                    _rcvLatestAckedSegment = header.sequenceNumber;
                    // + snd: ACK
                    YRPacketHeader outHeader = {0};
                    
                    outHeader.packetDescription = YRPacketDescriptionACK | (kYRProtocolVersion << 6);
                    outHeader.sequenceNumber = _sendNextSequenceNumber;
                    outHeader.headerLength = sizeof(YRPacketHeader);
                    outHeader.ackNumber = _rcvLatestAckedSegment;
                    
                    outHeader.checksum = [self calculateChecksum:&outHeader];
                    
                    [self sendUnreliably:[[YRSendOperation alloc] initWithData:[NSData dataWithBytes:&outHeader length:sizeof(outHeader)]
                        sequenceNumber:outHeader.sequenceNumber]];
                    
                    // Copy data
                    NSData *rawPacketData = [data subdataWithRange:(NSRange){outHeader.headerLength, outHeader.dataLength}];
                    _sessionContext.receiveCallout(self, rawPacketData);
                } else {
                    // Not supported in v1
                    // Buffer data
                    // snd: EAK
                }
            }
            
            break;
        case kYRSessionStateDisconnecting:
            if (header.packetDescription & YRPacketDescriptionRST) {
                [_disconnectingTimer invalidate];
                
                [self transiteToState:kYRSessionStateClosed];
            }
            break;
        default:
            NSAssert(NO, @"Can't handle incoming packet because state is not defined: %d", self.state);
            break;
    }
}

- (void)send:(NSData *)data {
    if (data.length == 0) {
        // TODO: Error
        return;
    }
    
    YRPacketHeader header = {0};
    
    header.packetDescription = YRPacketDescriptionACK | kYRProtocolVersion << 6;
    header.headerLength = sizeof(header);
    header.sequenceNumber = _sendNextSequenceNumber;
    header.ackNumber = _rcvLatestAckedSegment;
    header.dataLength = data.length;

    header.checksum = [self calculateChecksum:&header];
    
    NSMutableData *rawData = [NSMutableData new];
    
    [rawData appendBytes:&header length:sizeof(header)];
    [rawData appendData:data];

    YRSendOperation *operation = [[YRSendOperation alloc] initWithData:[rawData copy] sequenceNumber:_sendNextSequenceNumber];

    _sendNextSequenceNumber++;
    
    [self sendReliably:operation];
}

#pragma mark - Callbacks

- (void)handleDisconnectTimeout:(NSTimer *)timer {
    [self transiteToState:kYRSessionStateClosed];
}

#pragma mark - Private

/**
 *  This ensures:
 *  Packet size doesn't exceed maximum segment size /=> RST.
 *  Packet has header info > header minimum size /=> ignore.
 *  Calculates checksum /=> ignore packet.
 *  Ensures that protocol version is the same /=> RST.
 *  Ensures that data length matches correctly /=> ignore.
 *  Strictly speaking that ensures that packet is valid and can't break RUDP logic.
 */
- (BOOL)checkIfPacketValid:(NSData *)packet outHeader:(YRPacketHeader *)outHeader shouldResetConnection:(BOOL *)shouldReset {
    // Fill buffer with zeroes
    char packetBuffer[kYRMaxSequenceCount] = {0};
    char headerBuffer[kYRMaxSequenceCount] = {0};
    
    BOOL stubShouldReset = NO;
    BOOL *ensuredShouldResetPointer = shouldReset;

    if (!ensuredShouldResetPointer) {
        ensuredShouldResetPointer = &stubShouldReset;
    }

    // Verify that received packet size doesn't exceed maximum packet size.
    if (packet.length > kYRMaxSequenceCount) {
        // Packet size exceeds maximum segment size.
        *ensuredShouldResetPointer = YES;
        return NO;
    }

    // Verify that packet size is not less than header size.
    if (packet.length < YRMinimumPacketHeaderSize) {
        // Received packet that has less bytes than regular packet header.
        return NO;
    }

    // Get raw packet data.
    [packet getBytes:packetBuffer range:(NSRange){0, packet.length}];

    uint8_t headerLength = ((YRPacketHeader *)packetBuffer)->headerLength;

    if (headerLength < YRMinimumPacketHeaderSize) {
        // Received packet has incorrect header length specified.
        return 0;
    }

    [packet getBytes:headerBuffer range:(NSRange){0, headerLength}];

    YRPacketHeader *header = (YRPacketHeader *)headerBuffer;

    // Verify checksum.
    uint32_t checksum = [self calculateChecksum:header];

    if (checksum != header->checksum) {
        // Checksum mismatch.
        return NO;
    }

    // Verify protocol version.
    if (((header->packetDescription & YRPacketDescriptionProtocolVersionMask) >> 6) != kYRProtocolVersion) {
        // Protocol version doesn't match.
        *ensuredShouldResetPointer = YES;

        return NO;
    }

    // Verify data length.
    if ((header->headerLength + header->dataLength) > packet.length) {
        // Invalid sizes provided.
        return NO;
    }

    if (outHeader) {
        memcpy(outHeader, header, headerLength);
    }

    return YES;
}

- (uint32_t)calculateChecksum:(YRPacketHeader *)header {
    uint32_t candidateChecksum = header->checksum;
    header->checksum = 0;
    
    uint32_t checksum = 0;
    uint32_t *headerPointer = (uint32_t *)header;
    
    for (; headerPointer < (uint32_t *)((char *)header + header->headerLength); headerPointer++) {
        checksum += *headerPointer;
        checksum <<= 1;
    }
    
    header->checksum = candidateChecksum;
    
    return checksum;
}

- (void)transiteToState:(YRSessionState)state {
    if (_state != state) {
        _state = state;
        
        _sessionContext.connectionStateCallout(self, state);
    }
}

- (void)sendReliably:(YRSendOperation *)operation {
    _sendQueue[operation.sequenceNumber] = operation;

    [operation start];
    
    __typeof(self) __weak weakSelf = self;
    
    operation.onTransmissionTimeout = ^(YRSendOperation *operation) {
        [weakSelf sendReliably:operation];
    };
    
    _sessionContext.sendCallout(self, operation.data);
}

- (void)sendUnreliably:(YRSendOperation *)operation {
    _sessionContext.sendCallout(self, operation.data);
}

@end
