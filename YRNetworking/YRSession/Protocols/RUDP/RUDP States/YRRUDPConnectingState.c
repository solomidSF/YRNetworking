//
// YRRUDPClosedState.c
//
// The MIT License (MIT)
//
// Copyright (c) 2020 Yurii Romanchenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "YRInternal.h"

#define STATE_PREFIX connecting
//            YRSequenceNumberType expectedToReceive = session->sessionInfo.rcvLatestAckedSegment + 1;
//            
//            bool willWrapAround = (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments) < expectedToReceive;
//            
//            bool canProcessPacket = expectedToReceive <= rcvSeqNumber && rcvSeqNumber <= (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments);
//            
//            if (willWrapAround) {
//                canProcessPacket = expectedToReceive <= rcvSeqNumber || rcvSeqNumber <= (expectedToReceive + session->localConnectionConfiguration.maxNumberOfOutstandingSegments);
//            }
//            
//            if (!canProcessPacket) {
//                YRSessionDoACKOrEACK(session);
//                break;
//            }
//            
//            if (isRST) {
//                if (!session->shouldKeepAlive) {
//                    YRSessionTransiteToState(session, kYRSessionStateWaiting);
//                } else {
//                    //                    [_sessionLogger logError:@"Connection refused!"];
//                    YRSessionTransiteToState(session, kYRSessionStateClosed);
//                }
//                
//                break;
//            }
//            
//            if (isSYN) {
//                //                [_sessionLogger logError:@"Connection reset!"];
//                YRSessionTransiteToState(session, kYRSessionStateClosed);
//                
//                YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
//                
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//                
//                break;
//            }
//            
//            if (hasEACK) {
//                YRSessionTransiteToState(session, kYRSessionStateClosed);
//                
//                YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//                
//                break;
//            }
//            
//            if (hasACK) {
//                if (rcvAckNumber == session->sessionInfo.sendInitialSequenceNumber) {
//                    // Normally we should remove all operations from send queue.
//                    // But send queue is not created yet and we only have 1 packet that we send: SYN.
//                    // And it's not stored in our send queue, so we only need to cancel retransmission timer.
//                    session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
//                    
//                    YRSessionTransiteToState(session, kYRSessionStateConnected);
//                } else {
//                    YRSequenceNumberType seqNumberToRespond = rcvAckNumber + 1;
//                    
//                    YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                        YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
//                    }, YRPacketRSTLength());
//                    
//                    // We're half-open, this is not right. Needs polishing.
//                    
//                    break;
//                }
//            }
//            
//            YRPayloadLengthType payloadLength = 0;
//            
//            if (YRPacketHeaderHasPayloadLength(receivedHeader)) {
//                payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader);
//            }
//            
//            if (isNUL || payloadLength > 0) {
//                if (rcvSeqNumber == expectedToReceive) {
//                    session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
//                    
//                    if (payloadLength > 0) {
//                        // Copy data
//                        void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
//                        
//                        //                        assert(rawPayload);
//                        
//                        !session->callbacks.receiveCallout ?: session->callbacks.receiveCallout(session, rawPayload, payloadLength);
//                    }
//                    
//                    YRSessionProcessOutOfSequencePacketsIfAny(session);
//                } else {
//                    // Received out-of-seq packet
//                    YRPacketsQueueRef receiveQueue = YRSessionGetReceiveQueue(session);
//                    
//                    if (!YRPacketsQueueIsBufferInUseForSegment(receiveQueue, rcvSeqNumber)) {
//                        // Buffer data
//                        // snd: EAK
//                        YRPacketCopyPayloadInline(receivedPacket);
//                        
//                        void *buffer = YRPacketsQueueBufferForSegment(receiveQueue, rcvSeqNumber);
//                        
//                        //                        assert(buffer);
//                        
//                        if (buffer) {
//                            // This MUST be true always
//                            // Move packet to heap
//                            YRPacketCopy(receivedPacket, buffer);
//                            
//                            YRPacketsQueueMarkBufferInUseForSegment(receiveQueue, rcvSeqNumber);
//                        }
//                    } else {
//                        // Received duplicated out of sequence segment, ignore.
//                    }
//                }
//                
//                YRSessionDoACKOrEACK(session);
//            }
//        }
//            break;
#pragma mark - Lifecycle

void STATE_FP_PREFIX(onEnter) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	YRRUDPSessionSendSYN(protocol);
}

void STATE_FP_PREFIX(onExit) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {

}

#pragma mark - YRSessionProtocolLifecycleCallbacks

STATE_FP_IMPL(invalidate, YRSessionProtocolRef protocol) {

};

STATE_FP_IMPL(destroy, YRSessionProtocolRef protocol) {};

#pragma mark - YRSessionProtocolCallbacks

STATE_FP_IMPL(connect, YRSessionProtocolRef protocol) {
};

STATE_FP_IMPL(wait, YRSessionProtocolRef protocol) {
};

STATE_FP_IMPL(close, YRSessionProtocolRef protocol) {
};

STATE_FP_IMPL(send, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
};

STATE_FP_IMPL(receive, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
};

#pragma mark - YRPacketHandlers

void STATE_FP_PREFIX(syn) (void *context, YRPacketHeaderSYNRef header) {

};

void STATE_FP_PREFIX(rst) (void *context, YRPacketHeaderRSTRef header) {

};

void STATE_FP_PREFIX(nul) (void *context, YRPacketHeaderRef header) {

};

void STATE_FP_PREFIX(eack) (void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength) {
};

void STATE_FP_PREFIX(regular) (void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength) {
};

void STATE_FP_PREFIX(invalid) (void *context, YRRUDPError protocol) {

};

YRRUDPState YRRUDPConnectingState() {
	return STATE_DECL(kYRRUDPSessionStateConnecting);
}
