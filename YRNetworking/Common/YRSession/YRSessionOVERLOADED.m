//
//  YRSession.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/24/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

// YRSession
#import "YRSessionOVERLOADED.h"
#import "YRPacketsQueue.h"

static uint16_t const kYRProtocolVersion = 0x0001;

@implementation YRSessionOVERLOADED {
    YRSessionContext *_ctx;
    
    void *_incomingPacketBuffer;
    void *_headerPacketBuffer;
    
    YRPacketsQueue *_sendingQueue;
    YRPacketsQueue *_receivingQueue;
    
    // Send
    uint16_t _initialSequenceNumber;
    uint16_t _nextSequenceNumber;
    uint16_t _latestUnackSegment;
    
    // Receive
    uint16_t _latestAckedSegment;
    uint16_t _receiverInitialSequenceNumber;
    // out of seq received
}

+ (BOOL)isContextValid:(YRSessionContext *)context outErrorCode:(int32_t *)outError {
    return YES;
}

#pragma mark - Lifecycle

- (instancetype)initWithContext:(YRSessionContext *)context {
    if (self = [super init]) {
        int errorCode = 0;
        
        if (![self validateContext:context outErrorCode:&errorCode]) {
            NSLog(@"[YRSession]: Couldn't create YRSession because context is invalid.");
            return nil;
        }
        
        _ctx = [context copy];
        
        _incomingPacketBuffer = calloc(1, _ctx.sessionConfig.maximumSegmentSize);
        _headerPacketBuffer = calloc(1, _ctx.sessionConfig.maximumSegmentSize);
    }
    
    return self;
}

- (void)dealloc {
    if (_incomingPacketBuffer) {
        free(_incomingPacketBuffer);
    }
    
    if (_headerPacketBuffer) {
        free(_headerPacketBuffer);
    }
}

#pragma mark - Dynamic Properties

- (YRConfiguration *)sessionConfig {
    return _ctx.sessionConfig;
}

#pragma mark - Configuration

- (void)connect {
    if (self.state == kYRSessionStateClosed) {
        // snd: SYN
        [self transiteToState:kYRSessionStateInitiating];
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
    // TODO:
}

- (void)invalidate {
    // TODO:
}

#pragma mark - Communication

- (void)receive:(NSData *)data {
    YRPacketHeader header = {0};
    BOOL isValidPacket = [self checkIfPacketValid:data outHeader:&header shouldResetConnection:NULL];
    
    if (!isValidPacket) {
        NSLog(@"[YRSession]: Received invalid packet!");
        return;
    }
    
//    // Synchronization segment.
//    YRPacketDescriptionSYN = 1 << 0,
//    // Indicates acknowledgment number in the header is valid.
//    YRPacketDescriptionACK = 1 << 1,
//    // Indicates extended acknowledge segment is present.
//    YRPacketDescriptionEAK = 1 << 2,
//    // Indicates the packet is a reset segment.
//    YRPacketDescriptionRST = 1 << 3,
//    // Indicates the packet is a null segment.
//    YRPacketDescriptionNUL = 1 << 4,

    switch (self.state) {
        case kYRSessionStateClosed:
            if (header.packetDescription & YRPacketDescriptionRST) {
                return;
            }
            
            if ((header.packetDescription & YRPacketDescriptionACK) ||
                (header.packetDescription & YRPacketDescriptionNUL)) {
                // snd: RST <SEQ: SEG.ACK + 1>
            } else {
                // snd: RST <SEQ=0 RST, ACK: SEG.SEQ>
            }
            
            break;
        case kYRSessionStateWaiting:
            if (header.packetDescription & YRPacketDescriptionRST) {
                return;
            }
            
            if ((header.packetDescription & YRPacketDescriptionACK) ||
                (header.packetDescription & YRPacketDescriptionNUL)) {
                // snd: RST
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                _receiverInitialSequenceNumber = header.sequenceNumber;
                _latestAckedSegment = header.sequenceNumber;
                
                // snd: SYN/ACK
                
                [self transiteToState:kYRSessionStateConnecting];
            }
            
            break;
        case kYRSessionStateInitiating:
            if ((header.packetDescription & YRPacketDescriptionRST) &&
                (header.packetDescription & YRPacketDescriptionACK)) {
                // err: Connection refused
                [self transiteToState:kYRSessionStateClosed];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                _receiverInitialSequenceNumber = header.sequenceNumber;
                _latestAckedSegment = header.sequenceNumber;
                
                if (header.packetDescription & YRPacketDescriptionACK) {
                    _latestUnackSegment = header.ackNumber + 1;
                    
                    [self transiteToState:kYRSessionStateConnected];

                    // snd: ACK
                } else {
                    [self transiteToState:kYRSessionStateConnecting];
                    // snd: SYN/ACK
                }
                
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionRST) {
                if (header.packetDescription & YRPacketDescriptionACK) {
                    // err: Connection refused
                    [self transiteToState:kYRSessionStateClosed];
                }
                
                return;
            }
            
            break;
        case kYRSessionStateConnecting:
            if ((_receiverInitialSequenceNumber < header.sequenceNumber) &&
                (header.sequenceNumber <= (_latestAckedSegment + _ctx.sessionConfig.maxNumberOfOutstangingSegments))) {
                // Can process further.
            } else {
                // Can't receive, buffer this? (our buffer is reached max)
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                // snd: RST
                
                [self transiteToState:kYRSessionStateClosed];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionEAK) {
                // snd: RST
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionACK) {
                if (header.ackNumber == _initialSequenceNumber) {
                    [self transiteToState:kYRSessionStateConnected];
                } else {
                    // snd: RST
                }
            } else {
                return;
            }
            
            if ((header.packetDescription & YRPacketDescriptionNUL) || header.dataLength > 0) {
                if (header.sequenceNumber == _latestAckedSegment + 1) {
                    _latestAckedSegment = header.sequenceNumber;
                    
                    if (header.dataLength > 0) {
                        // Copy data
                    }
                    
                    // snd: ACK
                } else {
                    // Buffer it and do EACK
                }
            }
            
            break;
        case kYRSessionStateConnected:
            if ((_latestAckedSegment < header.sequenceNumber) &&
                (header.sequenceNumber <= (_latestAckedSegment + _ctx.sessionConfig.maxNumberOfOutstangingSegments))) {
                // Can process further.
            } else {
                // snd: ACK
                // Can't receive, buffer this? (our buffer is reached max)
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionRST) {
                // call: connection reset
                [self transiteToState:kYRSessionStateDisconnecting];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionNUL) {
                // TODO: Validate that no data is sent.
                _latestAckedSegment = header.sequenceNumber;
                
                // snd: ACK
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionSYN) {
                // snd: RST
                // call: connection reset
                [self transiteToState:kYRSessionStateClosed];
                return;
            }
            
            if (header.packetDescription & YRPacketDescriptionACK) {
                if (_latestUnackSegment <= header.ackNumber < _nextSequenceNumber) {
                    _latestUnackSegment = header.ackNumber + 1;
                    // queue: flush.
                }
            }
            
            if (header.packetDescription & YRPacketDescriptionEAK) {
                // queue: flush
            }
            
            if (header.dataLength > 0) {
                if (header.sequenceNumber == _latestAckedSegment + 1) {
                    _latestAckedSegment = header.sequenceNumber;
                    // snd: ACK
                    // Copy data
                } else {
                    // Buffer data
                    // snd: EAK
                }
            }
            
            break;
        case kYRSessionStateDisconnecting:
            if (header.packetDescription & YRPacketDescriptionRST) {
                // cancel: timer

                [self transiteToState:kYRSessionStateClosed];
            }
            break;
        default:
            NSAssert(NO, @"Can't handle incoming packet because state is not defined: %d", self.state);
            break;
    }
}

- (void)send:(NSData *)data {
    if (self.state == kYRSessionStateConnected) {
        
    } else {
        // err: connection not open.
    }
}

#pragma mark - Validation

- (BOOL)validateContext:(YRSessionContext *)context outErrorCode:(int *)outErrorCode {
    return YES;
}

- (BOOL)parsePacket:(NSData*)packet outHeader:(YRPacketHeader *)header outData:(NSData *)data {
    return NO;
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
- (BOOL)checkIfPacketValid:(NSData *)packet outHeader:(YRPacketHeader *)outHeader shouldResetConnection:(BOOL *)shouldReset {
    // Fill buffer with zeroes
    memset(_incomingPacketBuffer, 0, _ctx.sessionConfig.maximumSegmentSize);
    memset(_headerPacketBuffer, 0, _ctx.sessionConfig.maximumSegmentSize);
    
    BOOL stubShouldReset = NO;
    BOOL *ensuredShouldResetPointer = shouldReset;
    
    if (!ensuredShouldResetPointer) {
        ensuredShouldResetPointer = &stubShouldReset;
    }
    
    // Verify that received packet size doesn't exceed maximum packet size.
    if (packet.length > _ctx.sessionConfig.maximumSegmentSize) {
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
    [packet getBytes:_incomingPacketBuffer range:(NSRange){0, packet.length}];

    uint8_t headerLength = ((YRPacketHeader *)_incomingPacketBuffer)->headerLength;
    
    if (headerLength < YRMinimumPacketHeaderSize) {
        // Received packet has incorrect header length specified.
        return 0;
    }
    
    [packet getBytes:_headerPacketBuffer range:(NSRange){0, headerLength}];
    
    YRPacketHeader *header = (YRPacketHeader *)_headerPacketBuffer;
    
    // Verify checksum.
    uint32_t checksum = [self calculateChecksum:header];
    
    if (checksum != header->checksum) {
        // Checksum mismatch.
        return NO;
    }
    
    // Verify protocol version.
    if ((header->packetDescription & YRPacketDescriptionProtocolVersionMask) != kYRProtocolVersion) {
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
        outHeader = header;
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

#pragma mark - Private

- (void)transiteToState:(YRSessionState)state {
    if (_state != state) {
        _state = state;
        
        !_ctx.connectionStateCallout ?: _ctx.connectionStateCallout(self, state);
    }
}

@end
