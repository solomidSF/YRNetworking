//
//  YRTempSession.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 2/4/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#include "YRTempSession.h"

// Core
#include "YRPacket.h"

// Streams
#import "YRLightweightInputStream.h"
#import "YRLightweightOutputStream.h"

// Private
#include "YRPacketsQueue.h"

#include <stdlib.h>

typedef void (^YRPacketBuilder) (void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber);

// TODO: Integrate
typedef enum {
    YRSessionFlagShouldKeepAlive = 1 << 0,
    YRSessionFlagHasSpace = 1 << 1,
    YRSessionFlagHasPeerConfiguration = 1 << 2
} YRSessionFlags;

typedef struct {
    double sendTime; // TODO: Think about type
    uint8_t retransmissions;
} YRSessionSendOperation;

typedef struct YRSession {
    YRSessionState state; // TODO: Reduce size of this one
    YRSessionFlags flags; // Some flags. We will need them, right?
    YRConnectionConfiguration localConnectionConfiguration;
    YRConnectionConfiguration remoteConnectionConfiguration;
    YRSessionCallbacks callbacks;
    
    void *(^createTimer) (void);
    void (^cancelTimer) (void *timer);
    
    // Determines if local peer should send NUL segments.
    bool shouldKeepAlive;
    
    YRSessionInfo sessionInfo;
    
    YRPacketsQueueRef sendQueue;
    YRPacketsQueueRef receiveQueue;
} YRSession;

static const YRSessionCallbacks kYRNullSessionCallbacks = {NULL, NULL, NULL};

#pragma mark - Prototypes

void YRSessionSetCallbacks(YRSessionRef session, YRSessionCallbacks callbacks);
void YRSessionTransiteToState(YRSessionRef session, YRSessionState state);
/**
 *  As session is created we cannot know in advance what configuration is for remote peer.
 */
// TODO: Let's go hardcore here, don't implement this, send queue will dynamically manage several regions of memory if it's needed.
size_t YRSessionGetFinalMinimumSize(YRConnectionConfiguration localConfiguration, YRConnectionConfiguration remoteConfiguration);

void YRSessionSchedulePacketSendReliably(YRSessionRef session, YRPacketRef packet);

void YRSessionProcessOutOfSequencePacketsIfAny(YRSessionRef session);

// Packet Queues
YRPacketsQueueRef YRSessionGetSendQueue(YRSessionRef session);
YRPacketsQueueRef YRSessionGetReceiveQueue(YRSessionRef session);

void YRSessionDoACKOrEACK(YRSessionRef session);

void YRSessionDoReliableSend(YRSessionRef session, YRPacketBuilder packetBuilder, YRPayloadLengthType packetLength);
void YRSessionDoReliableSendWithAutoIncrement(YRSessionRef session,
                                              YRPacketBuilder packetBuilder,
                                              YRPayloadLengthType packetLength,
                                              bool increment);

void YRSessionDoUnreliableSend(YRSessionRef session, YRPacketBuilder packetBuilder, YRPayloadLengthType packetLength);
void YRSessionSendPacket(YRSessionRef session, YRPacketRef packet);

#pragma mark - Sizes
#pragma mark - Lifecycle

//YRSessionRef YRSessionCreateWithConfiguration(YRConnectionConfiguration configuration, YRSessionCallbacks callbacks) {
//    YRSessionRef session = calloc(1, sizeof(YRSession));
//
//    if (!session) {
//        return NULL;
//    }
//
//    session->state = kYRSessionStateClosed;
//    session->localConnectionConfiguration = configuration;
//
//    YRSessionSetCallbacks(session, callbacks);
//
//    return session;
//}
//
//void YRSessionDestroy(YRSessionRef session) {
//    if (session) {
//        // TODO: Implement proper destruction
//        YRSessionSetCallbacks(session, kYRNullSessionCallbacks);
//
//        free(session);
//    }
//}

#pragma mark - Configuration

void YRSessionConnect(YRSessionRef session) {
    //    [_sessionLogger logInfo:@"[CONN_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (YRSessionGetState(session) == kYRSessionStateClosed) {
        session->shouldKeepAlive = true;
        
        YRSessionTransiteToState(session, kYRSessionStateInitiating);
        
        YRSessionDoReliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
            YRPacketCreateSYN(session->localConnectionConfiguration, seqNumber, 0, false, packetBuffer);
        }, YRPacketSYNLength());
    } else {
        //        [_sessionLogger logWarning:@"[CONN_REQ]: Trying to transite into '%@' from %@", [self humanReadableState:kYRSessionStateInitiating], [self humanReadableState:self.state]];
    }
}

void YRSessionWait(YRSessionRef session) {
    //    [_sessionLogger logInfo:@"[WAIT_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (session->state == kYRSessionStateClosed) {
        YRSessionTransiteToState(session, kYRSessionStateWaiting);
    } else {
        //        [_sessionLogger logWarning:@"[WAIT_REQ]: Trying to transite into '%@' from %@", [self humanReadableState:kYRSessionStateWaiting], [self humanReadableState:self.state]];
    }
}

void YRSessionClose(YRSessionRef session) {
    //    [_sessionLogger logInfo:@"[CLOSE_REQ] (%@)", [self humanReadableState:self.state]];
    //
    //    if (self.state == kYRSessionStateConnected) {
    //        [self transiteToState:kYRSessionStateDisconnecting];
    //
    //        YRPacketRef packet = YRPacketCreateRST(0, _sendNextSequenceNumber, _rcvLatestAckedSegment, true, NULL);
    //        _sendNextSequenceNumber++;
    //
    //        [self sendPacketReliably:packet];
    //
    //        [_disconnectingTimer invalidate];
    //        _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
    //                                                             selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
    //
    //        return;
    //    }
    //
    //    if (self.state == kYRSessionStateWaiting) {
    //        [self transiteToState:kYRSessionStateClosed];
    //        return;
    //    }
    //
    //    if (self.state == kYRSessionStateConnecting || self.state == kYRSessionStateInitiating) {
    //        [self transiteToState:kYRSessionStateClosed];
    //
    //        YRPacketRef packet = YRPacketCreateRST(0, _sendNextSequenceNumber, 0, false, NULL);
    //
    //        [self sendPacketUnreliably:packet];
    //        return;
    //    }
}

void YRSessionInvalidate(YRSessionRef session) {
    //    [_sessionLogger logInfo:@"[INVL_REQ] (%@): <NOT IMPLEMENTED YET>", [self humanReadableState:self.state]];
    //    // TODO:
    //
    //    [self transiteToState:kYRSessionStateClosed];
}

#pragma mark - Communication

bool YRSessionCanSend(YRSessionRef session) {
    if (session->state == kYRSessionStateConnected) {
        YRPacketsQueueRef queue = YRSessionGetSendQueue(session);
        
        return YRPacketsQueueBuffersInUse(queue) < session->remoteConnectionConfiguration.maxNumberOfOutstandingSegments;
    }
    
    return false;
}

void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    if (length > session->localConnectionConfiguration.maximumSegmentSize) {
        //        [_sessionLogger logWarning:@"[RCV_REQ]: Dropping large packet (%d bytes)", length];
        // Drop packets with large sizes.
        return;
    }
    
    // Create input stream on stack to read incoming packet.
    uint8_t bufferForStream[kYRLightweightInputStreamSize] __attribute__ ((__aligned__(sizeof(uintptr_t))));
    YRLightweightInputStreamRef stream = YRLightweightInputStreamCreateAt(payload, length, bufferForStream);
    
    bool canDeserialize = YRPacketCanDeserializeFromStream(stream);
    
    if (!canDeserialize) {
        // TODO: Error
        return;
    }
    
    // TODO: Deserialize packet on stack
    //    uint8_t bufferForPacket[kYRLightweightInputStreamSize] __attribute__ ((__aligned__(8)));
    YRPacketRef receivedPacket = YRPacketDeserialize(stream);
    YRPacketHeaderRef receivedHeader = YRPacketGetHeader(receivedPacket);
    
    //    BOOL shouldResetConnection = NO;
    bool isValidPacket = YRPacketIsLogicallyValid(receivedPacket);
    
    if (isValidPacket) {
        //        [_sessionLogger logInfo:@"[RCV_REQ] (%@) Packet: %@", [self humanReadableState:self.state], [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(receivedPacket)]];
    } else {
        //        [_sessionLogger logWarning:@"[RCV_REQ] (%@) Invalid Packet!\n%@", [self humanReadableState:self.state], [YRDebugUtils packetHeaderFullDescription:YRPacketGetHeader(receivedPacket)]];
        
        YRPacketDestroy(receivedPacket);
        return;
    }
    
    //    if (shouldResetConnection) {
    //        [self close];
    //        return;
    //    }
    
    bool isSYN = YRPacketHeaderIsSYN(receivedHeader);
    bool isRST = YRPacketHeaderIsRST(receivedHeader);
    bool isNUL = YRPacketHeaderIsNUL(receivedHeader);
    bool hasACK = YRPacketHeaderHasACK(receivedHeader);
    bool hasEACK = YRPacketHeaderHasEACK(receivedHeader);
    
    YRSequenceNumberType rcvSeqNumber = YRPacketHeaderGetSequenceNumber(receivedHeader);
    YRSequenceNumberType rcvAckNumber = YRPacketHeaderGetAckNumber(receivedHeader);
    
    switch (session->state) {
        case kYRSessionStateClosed:
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateRST(0, rcvAckNumber + 1, 0, false, packetBuffer);
                }, YRPacketRSTLength());
            } else {
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateRST(0, 0, rcvSeqNumber, true, packetBuffer);
                }, YRPacketRSTLength());
            }
            
            break;
        case kYRSessionStateWaiting:
            // Waiting for SYN packet.
            if (isSYN) {
                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
                session->sessionInfo.rcvInitialSequenceNumber = rcvSeqNumber;
                
                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
                
                session->remoteConnectionConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
                
                YRSessionTransiteToState(session, kYRSessionStateConnecting);
                
                YRSessionDoReliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateSYN(session->localConnectionConfiguration, seqNumber, ackNumber, true, packetBuffer);
                }, YRPacketSYNLength());
                
                break;
            }
            
            if (isRST) {
                break;
            }
            
            if (hasACK || isNUL) {
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateRST(0, rcvAckNumber + 1, 0, false, packetBuffer);
                }, YRPacketRSTLength());
                
                break;
            }
            
            break;
        case kYRSessionStateInitiating:
            // Waiting for SYN/ACK
            if (isRST && hasACK) {
                // err: Connection refused
                //                [_sessionLogger logError:@"Connection refused!"];
                YRSessionTransiteToState(session, kYRSessionStateClosed);
                
                break;
            }
            
            if (isSYN) {
                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
                session->sessionInfo.rcvInitialSequenceNumber = rcvSeqNumber;
                
                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
                
                session->remoteConnectionConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
                
                if (hasACK) {
                    // Normally we should remove all operations from send queue.
                    // But send queue is not created yet and we only have 1 packet that we send: SYN.
                    // And it's not stored in our send queue, so we only need to cancel retransmission timer.
                    session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
                    
                    YRSessionTransiteToState(session, kYRSessionStateConnected);
                    
                    YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                        YRPacketCreateACK(seqNumber, ackNumber, packetBuffer);
                    }, YRPacketACKLength());
                } else {
                    YRSessionTransiteToState(session, kYRSessionStateConnecting);
                    
                    // TODO: This flow is strange as rfc describes.
                    // if SYN & SYN rcved on both sides, SYN/ACK will be ignored on both sides which will result in ACK sent by both peer = connected.
                    // if - & SYN/ACK rcvd = SYN/ACK side will resend SYN/ACK.
                    YRSessionDoReliableSendWithAutoIncrement(session, ^(void *packetBuffer,
                                                                        YRSequenceNumberType seqNumber,
                                                                        YRSequenceNumberType ackNumber) {
                        YRPacketCreateSYN(session->localConnectionConfiguration,
                                          session->sessionInfo.sendInitialSequenceNumber,
                                          session->sessionInfo.rcvLatestAckedSegment,
                                          true,
                                          packetBuffer);
                    }, YRPacketSYNLength(), false);
                }
                
                break;
            }
            
            break;
        case kYRSessionStateConnecting: {
            YRSequenceNumberType expectedToReceive = session->sessionInfo.rcvLatestAckedSegment + 1;
            
            bool willWrapAround = (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments) < expectedToReceive;
            
            bool canProcessPacket = expectedToReceive <= rcvSeqNumber && rcvSeqNumber <= (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments);
            
            if (willWrapAround) {
                canProcessPacket = expectedToReceive <= rcvSeqNumber || rcvSeqNumber <= (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments);
            }
            
            if (!canProcessPacket) {
                YRSessionDoACKOrEACK(session);
                break;
            }
            
            if (isRST) {
                if (!session->shouldKeepAlive) {
                    YRSessionTransiteToState(session, kYRSessionStateWaiting);
                } else {
                    //                    [_sessionLogger logError:@"Connection refused!"];
                    YRSessionTransiteToState(session, kYRSessionStateClosed);
                }
                
                break;
            }
            
            if (isSYN) {
                //                [_sessionLogger logError:@"Connection reset!"];
                YRSessionTransiteToState(session, kYRSessionStateClosed);
                
                YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
                
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
                }, YRPacketRSTLength());
                
                break;
            }
            
            if (hasEACK) {
                YRSessionTransiteToState(session, kYRSessionStateClosed);
                
                YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
                }, YRPacketRSTLength());
                
                break;
            }
            
            if (hasACK) {
                if (rcvAckNumber == session->sessionInfo.sendInitialSequenceNumber) {
                    // Normally we should remove all operations from send queue.
                    // But send queue is not created yet and we only have 1 packet that we send: SYN.
                    // And it's not stored in our send queue, so we only need to cancel retransmission timer.
                    session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
                    
                    YRSessionTransiteToState(session, kYRSessionStateConnected);
                } else {
                    YRSequenceNumberType seqNumberToRespond = rcvAckNumber + 1;
                    
                    YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                        YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
                    }, YRPacketRSTLength());
                    
                    // We're half-open, this is not right. Needs polishing.
                    
                    break;
                }
            }
            
            YRPayloadLengthType payloadLength = 0;
            
            if (YRPacketHeaderHasPayloadLength(receivedHeader)) {
                payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader);
            }
            
            if (isNUL || payloadLength > 0) {
                if (rcvSeqNumber == expectedToReceive) {
                    session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
                    
                    if (payloadLength > 0) {
                        // Copy data
                        void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
                        
                        //                        assert(rawPayload);
                        
                        !session->callbacks.receiveCallout ?: session->callbacks.receiveCallout(session, rawPayload, payloadLength);
                    }
                    
                    YRSessionProcessOutOfSequencePacketsIfAny(session);
                } else {
                    // Received out-of-seq packet
                    YRPacketsQueueRef receiveQueue = YRSessionGetReceiveQueue(session);
                    
                    if (!YRPacketsQueueIsBufferInUseForSegment(receiveQueue, rcvSeqNumber)) {
                        // Buffer data
                        // snd: EAK
                        YRPacketCopyPayloadInline(receivedPacket);
                        
                        void *buffer = YRPacketsQueueBufferForSegment(receiveQueue, rcvSeqNumber);
                        
                        //                        assert(buffer);
                        
                        if (buffer) {
                            // This MUST be true always
                            // Move packet to heap
                            YRPacketCopy(receivedPacket, buffer);
                            
                            YRPacketsQueueMarkBufferInUseForSegment(receiveQueue, rcvSeqNumber);
                        }
                    } else {
                        // Received duplicated out of sequence segment, ignore.
                    }
                }
                
                YRSessionDoACKOrEACK(session);
            }
        }
            break;
        case kYRSessionStateConnected: {
            // This will forcefully create receive queue, should we postpone this?
            YRPacketsQueueRef receiveQueue = YRSessionGetReceiveQueue(session);
            
            // TODO: Wrong, rcv queue stores only out of seq data
            if (!YRPacketsQueueHasBufferForSegment(receiveQueue, rcvSeqNumber)) {
                // Can't process packet, because it's out of range.
                YRSessionDoACKOrEACK(session);
                break;
            }
            
            YRSequenceNumberType expectedToReceive = session->sessionInfo.rcvLatestAckedSegment + 1;
            
            //            YRSequenceNumberType expectedToReceive = _rcvLatestAckedSegment + 1;
            //
            //            YRSequenceNumberType maxSegmentThatCanBeReceived = expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments;
            //            BOOL willWrapAround = maxSegmentThatCanBeReceived < expectedToReceive;
            //
            //            BOOL canProcessPacket = expectedToReceive <= sequenceNumber && sequenceNumber <= maxSegmentThatCanBeReceived;
            //
            //            if (willWrapAround) {
            //                canProcessPacket = expectedToReceive <= sequenceNumber || sequenceNumber <= maxSegmentThatCanBeReceived;
            //            }
            //
            if (isSYN) {
                // call: connection reset
                YRSessionTransiteToState(session, kYRSessionStateClosed);
                
                YRSessionDoUnreliableSend(session, ^(void *packetBuffer,
                                                     YRSequenceNumberType seqNumber,
                                                     YRSequenceNumberType ackNumber) {
                    YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
                }, YRPacketRSTLength());
                
                break;
            }
            
            if (isRST) {
                // signal: connection reset
                //                [_disconnectingTimer invalidate];
                //                _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
                //                                                                     selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
                
                YRSessionTransiteToState(session, kYRSessionStateDisconnecting);
                break;
            }
            
            if (hasACK) {
                // queue: flush.
                YRPacketsQueueRef sendQueue = YRSessionGetSendQueue(session);
                
                session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
                
                YRSequenceNumberType currentSegment = YRPacketsQueueGetBaseSegment(sendQueue);
                YRPacketsQueueAdvanceBaseSegment(sendQueue, rcvAckNumber - currentSegment);
                
                if (YRPacketsQueueBuffersInUse(sendQueue) == 0) {
                    // TODO: Cancel retransmission timer.
                }
            }
            
            if (hasEACK) {
                // queue: flush
                YRPacketHeaderEACKRef receivedEACKHeader = (YRPacketHeaderEACKRef)receivedHeader;
                YRPacketsQueueRef sendQueue = YRSessionGetSendQueue(session);
                
                YRSequenceNumberType eacksCount = 0;
                YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(receivedEACKHeader, &eacksCount);
                
                for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
                    YRSequenceNumberType sequence = eacks[i];
                    
                    // TODO: What if we receive here eack that is really ack?
                    YRPacketsQueueUnmarkBufferInUseForSegment(sendQueue, sequence);
                }
                
                if (YRPacketsQueueBuffersInUse(sendQueue) == 0) {
                    // TODO: Cancel retransmission timer.
                }
            }
            
            if (isNUL) {
                // TODO: Next iteration: NUL segment.
                // We must ack it if it falls into acceptance window.
                // However we shouldn't set _rcvLatestAckedSegment to this seq number.
                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
                YRSessionDoACKOrEACK(session);
                break;
            }
            
            if (YRPacketHeaderHasPayloadLength(receivedHeader) &&
                YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader) > 0) {
                if (rcvSeqNumber == expectedToReceive) {
                    session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
                    
                    // Do callout with data.
                    YRPayloadLengthType payloadLength = 0;
                    void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
                    
                    !session->callbacks.receiveCallout ?: session->callbacks.receiveCallout(session, rawPayload, payloadLength);
                    
                    YRSessionProcessOutOfSequencePacketsIfAny(session);
                } else {
                    if (!YRPacketsQueueIsBufferInUseForSegment(receiveQueue, rcvSeqNumber)) {
                        // Buffer data
                        // snd: EAK
                        YRPacketCopyPayloadInline(receivedPacket);
                        
                        void *buffer = YRPacketsQueueBufferForSegment(receiveQueue, rcvSeqNumber);
                        
                        //                        assert(buffer);
                        
                        if (buffer) {
                            // This MUST be true always
                            // Move packet to heap
                            YRPacketCopy(receivedPacket, buffer);
                            
                            YRPacketsQueueMarkBufferInUseForSegment(receiveQueue, rcvSeqNumber);
                        }
                    } else {
                        // Received duplicated out of sequence segment, ignore.
                    }
                }
                
                YRSessionDoACKOrEACK(session);
            }
        }
            break;
        case kYRSessionStateDisconnecting:
            if (isRST) {
                //                [_disconnectingTimer invalidate];
                
                YRSessionTransiteToState(session, kYRSessionStateClosed);
            }
            break;
        default:
            //            [_sessionLogger logError:@"[RCV_REQ] <FATAL> Can't handle incoming packet because state is not defined: %d", self.state];
            //            NSAssert(NO, @"Can't handle incoming packet because state is not defined: %d", self.state);
            break;
    }
    
    YRPacketDestroy(receivedPacket);
}

void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    //    [_sessionLogger logInfo:@"[SEND_REQ] (%@)", [self humanReadableState:self.state]];
    
    if (length == 0) {
        // TODO: error: payload is empty
        return;
    }
    
    if (session->state == kYRSessionStateConnected) {
        if (YRSessionCanSend(session)) {
            YRSessionDoReliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
                YRPacketCreateWithPayload(seqNumber, ackNumber, payload, length, true, packetBuffer);
            }, YRPacketLengthForPayload(length));
        } else {
            // TODO: error: no space available
        }
    } else {
        // TODO: error: not connected
    }
}

#pragma mark - State

//YRSessionState YRSessionGetState(YRSessionRef session) {
//    return session->state;
//}
//
//YRSessionInfo YRSessionGetSessionInfo(YRSessionRef session) {
//    return session->sessionInfo;
//}
//
//YRConnectionConfiguration YRSessionGetLocalConnectionInfo(YRSessionRef session) {
//    return session->localConnectionConfiguration;
//}
//
//YRConnectionConfiguration YRSessionGetRemoteConnectionInfo(YRSessionRef session) {
//    return session->remoteConnectionConfiguration;
//}

#pragma mark - Private

void YRSessionInvalidateConnection(YRSessionRef session) {
    // TODO: Clean-up all packets to be sent, remove all timers, delete remote configuration and do callout.
}

//void YRSessionSetCallbacks(YRSessionRef session, YRSessionCallbacks callbacks) {
//    if (callbacks.connectionStateCallout) {
//        callbacks.connectionStateCallout = _Block_copy(callbacks.connectionStateCallout);
//    }
//
//    if (callbacks.sendCallout) {
//        callbacks.sendCallout = _Block_copy(callbacks.sendCallout);
//    }
//
//    if (callbacks.receiveCallout) {
//        callbacks.receiveCallout = _Block_copy(callbacks.receiveCallout);
//    }
//
//    if (session->callbacks.connectionStateCallout) {
//        _Block_release(session->callbacks.connectionStateCallout);
//    }
//
//    if (session->callbacks.sendCallout) {
//        _Block_release(session->callbacks.sendCallout);
//    }
//
//    if (session->callbacks.receiveCallout) {
//        _Block_release(session->callbacks.receiveCallout);
//    }
//
//    session->callbacks = callbacks;
//}

YRPacketsQueueRef YRSessionGetSendQueue(YRSessionRef session) {
    if (!session->sendQueue && session->remoteConnectionConfiguration.maximumSegmentSize > 0) {
        session->sendQueue = YRPacketsQueueCreate(session->remoteConnectionConfiguration.maximumSegmentSize,
                                                  session->remoteConnectionConfiguration.maxNumberOfOutstandingSegments);
        
        // assert send queue, or do graceful fallback EVERYWHERE
        
        YRPacketsQueueSetBaseSegment(session->sendQueue, session->sessionInfo.sendNextSequenceNumber);
    }
    
    return session->sendQueue;
}

YRPacketsQueueRef YRSessionGetReceiveQueue(YRSessionRef session) {
    if (!session->receiveQueue && session->localConnectionConfiguration.maximumSegmentSize > 0) {
        session->receiveQueue = YRPacketsQueueCreate(session->localConnectionConfiguration.maximumSegmentSize,
                                                     session->localConnectionConfiguration.maxNumberOfOutstandingSegments);
        
        // assert receive queue, or do graceful fallback EVERYWHERE
        
        // rcvLatestAckedSegment means that we expect next segment to have seg# + 1
        // This queue stores ONLY out of seq, so next out of seq is seq# + 2
        YRPacketsQueueSetBaseSegment(session->receiveQueue, session->sessionInfo.rcvLatestAckedSegment + 2);
    }
    
    return session->receiveQueue;
}

void YRSessionTransiteToState(YRSessionRef session, YRSessionState state) {
    if (session->state != state) {
        session->state = state;
        
        !session->callbacks.connectionStateCallout ?: session->callbacks.connectionStateCallout(session, state);
    }
}

void YRSessionSchedulePacketSendReliably(YRSessionRef session, YRPacketRef packet) {
    YRPacketCopyPayloadInline(packet);
    
    //    YRSequenceNumberType seqNumber = YRPacketHeaderGetSequenceNumber(YRPacketGetHeader(packet));
    //    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:seqNumber];
    //
    //    if ([_sendQueue[seqNumber] isKindOfClass:[YRSendOperation class]]) {
    //        [_sendQueue[seqNumber] end];
    //    }
    //
    //    _sendQueue[seqNumber] = operation;
    //
    //    [operation start];
    //
    //    __typeof(self) __weak weakSelf = self;
    //    __typeof(YRLogger *) __weak weakLogger = _sessionLogger;
    //
    //    operation.onTransmissionTimeout = ^(YRSendOperation *operation) {
    //        [weakLogger logInfo:@"[SEND_TIM]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(packet)]];
    //
    //        [operation start];
    //        [weakSelf sendDataFromPacket:operation.packet];
    //    };
    //
    //    [_sessionLogger logInfo:@"[SEND_REL]: Header: %@", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];
    //
    //    [self sendDataFromPacket:packet];
}

void YRSessionProcessOutOfSequencePacketsIfAny(YRSessionRef session) {
    //    YRSequenceNumberType nextSegment = _rcvLatestAckedSegment + 1;
    //    id operation = _receiveQueue[nextSegment];
    //
    //    while ([operation isKindOfClass:[YRReceiveOperation class]]) {
    //        _rcvLatestAckedSegment = nextSegment;
    //
    //        YRReceiveOperation *outOfSeqOperation = operation;
    //        YRPacketRef packet = outOfSeqOperation.packet;
    //
    //        // Do callout with data.
    //        YRPayloadLengthType payloadLength = 0;
    //        void *rawPayload = YRPacketGetPayload(packet, &payloadLength);
    //
    //        if (rawPayload) {
    //            NSData *rawPacketData = [NSData dataWithBytesNoCopy:rawPayload length:payloadLength freeWhenDone:NO];
    //
    //            !_sessionContext.receiveCallout ?: _sessionContext.receiveCallout(self, rawPacketData);
    //        }
    //
    //        _receiveQueue[nextSegment] = [NSNull null];
    //        [_outOfSequenceSegmentsReceived removeObject:@(nextSegment)];
    //
    //        nextSegment++;
    //
    //        operation = _receiveQueue[nextSegment];
    //    }
}

#pragma mark - ACK'ing

void YRSessionDoACKOrEACK(YRSessionRef session) {
    if (session->receiveQueue && YRPacketsQueueBuffersInUse(session->receiveQueue) > 0) {
        YRSequenceNumberType outOfSequenceReceived = YRPacketsQueueBuffersInUse(session->receiveQueue);
        YRPayloadLengthType packetLength = YRPacketEACKLength(&outOfSequenceReceived);
        
        YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
            uint8_t outOfSeqReceivedSmallInt = YRPacketsQueueBuffersInUse(session->receiveQueue);
            YRSequenceNumberType outOfSeq[outOfSeqReceivedSmallInt];
            
            YRPacketsQueueGetSegmentNumbersForBuffersInUse(session->receiveQueue, outOfSeq, &outOfSeqReceivedSmallInt);
            YRSequenceNumberType outOfSeqReceived = outOfSeqReceivedSmallInt;
            
            YRPacketCreateEACK(seqNumber, ackNumber, outOfSeq, &outOfSeqReceived, packetBuffer);
        }, packetLength);
    } else {
        // We don't have receive queue yet, so do simple ack
        YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
            YRPacketCreateACK(seqNumber, ackNumber, packetBuffer);
        }, YRPacketACKLength());
    }
}

#pragma mark - Sending

void YRSessionDoReliableSend(YRSessionRef session, YRPacketBuilder packetBuilder, YRPayloadLengthType packetLength) {
    YRSessionDoReliableSendWithAutoIncrement(session, packetBuilder, packetLength, true);
}

void YRSessionDoReliableSendWithAutoIncrement(YRSessionRef session,
                                              YRPacketBuilder packetBuilder,
                                              YRPayloadLengthType packetLength,
                                              bool increment) {
    if (session->state == kYRSessionStateConnected) {
        YRPacketsQueueRef queue = YRSessionGetSendQueue(session);
        
        //        if (packetLength > // buffer size in queue) {
        //            // Can't send packet because packet length exceeds maximum payload size to be received by remote
        //            return;
        //        }
        
        void *buffer = YRPacketsQueueBufferForSegment(queue, session->sessionInfo.sendNextSequenceNumber);
        
        if (buffer) {
            packetBuilder(buffer, session->sessionInfo.sendNextSequenceNumber, session->sessionInfo.rcvLatestAckedSegment);
            
            YRPacketsQueueMarkBufferInUseForSegment(queue, session->sessionInfo.sendNextSequenceNumber);
            
            if (increment) {
                session->sessionInfo.sendNextSequenceNumber++;
            }
        } else {
            // Insufficient space to store data for sending!
            return;
        }
        
        // Copy payload if any as we're going to send reliably
        YRPacketCopyPayloadInline((YRPacketRef)buffer);
        
        YRSessionSendPacket(session, (YRPacketRef)buffer);
    } else {
        // This branch is exclusively taken when we're not connected and that means only SYN segment will hit this.
        uint8_t buffer[packetLength] __attribute__ ((__aligned__(8)));
        
        packetBuilder(buffer, session->sessionInfo.sendNextSequenceNumber, session->sessionInfo.rcvLatestAckedSegment);
        
        YRSessionSendPacket(session, (YRPacketRef)buffer);
    }
}

void YRSessionDoUnreliableSend(YRSessionRef session, YRPacketBuilder packetBuilder, YRPayloadLengthType packetLength) {
    uint8_t buffer[packetLength] __attribute__ ((__aligned__(8)));
    
    packetBuilder(buffer, session->sessionInfo.sendNextSequenceNumber, session->sessionInfo.rcvLatestAckedSegment);
    
    YRSessionSendPacket(session, (YRPacketRef)buffer);
}

void YRSessionSendPacket(YRSessionRef session, YRPacketRef packet) {
    YRPayloadLengthType packetLength = YRPacketGetLength(packet);
    
    uint8_t outputStreamBuffer[kYRLightweightOutputStreamSize] __attribute__ ((__aligned__(sizeof(uintptr_t))));
    uint8_t packetBuffer[packetLength] __attribute__ ((__aligned__(8)));
    
    YRLightweightOutputStreamRef outputStream = YRLightweightOutputStreamCreateAt(packetBuffer, packetLength, outputStreamBuffer);
    
    YRPacketSerialize(packet, outputStream);
    
    !session->callbacks.sendCallout ?: session->callbacks.sendCallout(session, YRLightweightOutputStreamGetBytes(outputStream), packetLength);
}
