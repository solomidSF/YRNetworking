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

#define STATE_PREFIX connected
//            // This will forcefully create receive queue, should we postpone this?
//            YRPacketsQueueRef receiveQueue = YRSessionGetReceiveQueue(session);
//            
//            // TODO: Wrong, rcv queue stores only out of seq data
//            if (!YRPacketsQueueHasBufferForSegment(receiveQueue, rcvSeqNumber)) {
//                // Can't process packet, because it's out of range.
//                YRSessionDoACKOrEACK(session);
//                break;
//            }
//            
//            YRSequenceNumberType expectedToReceive = session->sessionInfo.rcvLatestAckedSegment + 1;
//            
//            //            YRSequenceNumberType expectedToReceive = _rcvLatestAckedSegment + 1;
//            //
//            //            YRSequenceNumberType maxSegmentThatCanBeReceived = expectedToReceive + _localConfiguration.maxNumberOfOutstandingSegments;
//            //            BOOL willWrapAround = maxSegmentThatCanBeReceived < expectedToReceive;
//            //
//            //            BOOL canProcessPacket = expectedToReceive <= sequenceNumber && sequenceNumber <= maxSegmentThatCanBeReceived;
//            //
//            //            if (willWrapAround) {
//            //                canProcessPacket = expectedToReceive <= sequenceNumber || sequenceNumber <= maxSegmentThatCanBeReceived;
//            //            }
//            //
//            if (isSYN) {
//                // call: connection reset
//                YRSessionTransiteToState(session, kYRSessionStateClosed);
//                
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer,
//                                                     YRSequenceNumberType seqNumber,
//                                                     YRSequenceNumberType ackNumber) {
//                    YRSequenceNumberType seqNumberToRespond = hasACK ? rcvAckNumber + 1 : 0;
//                    YRPacketCreateRST(0, seqNumberToRespond, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//                
//                break;
//            }
//            
//            if (isRST) {
//                // signal: connection reset
//                //                [_disconnectingTimer invalidate];
//                //                _disconnectingTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self
//                //                                                                     selector:@selector(handleDisconnectTimeout:) userInfo:nil repeats:NO];
//                
//                YRSessionTransiteToState(session, kYRSessionStateDisconnecting);
//                break;
//            }
//            
//            if (hasACK) {
//                // queue: flush.
//                YRPacketsQueueRef sendQueue = YRSessionGetSendQueue(session);
//                
//                session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
//                
//                YRSequenceNumberType currentSegment = YRPacketsQueueGetBaseSegment(sendQueue);
//                YRPacketsQueueAdvanceBaseSegment(sendQueue, rcvAckNumber - currentSegment);
//                
//                if (YRPacketsQueueBuffersInUse(sendQueue) == 0) {
//                    // TODO: Cancel retransmission timer.
//                }
//            }
//            
//            if (hasEACK) {
//                // queue: flush
//                YRPacketHeaderEACKRef receivedEACKHeader = (YRPacketHeaderEACKRef)receivedHeader;
//                YRPacketsQueueRef sendQueue = YRSessionGetSendQueue(session);
//                
//                YRSequenceNumberType eacksCount = 0;
//                YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(receivedEACKHeader, &eacksCount);
//                
//                for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
//                    YRSequenceNumberType sequence = eacks[i];
//                    
//                    // TODO: What if we receive here eack that is really ack?
//                    YRPacketsQueueUnmarkBufferInUseForSegment(sendQueue, sequence);
//                }
//                
//                if (YRPacketsQueueBuffersInUse(sendQueue) == 0) {
//                    // TODO: Cancel retransmission timer.
//                }
//            }
//            
//            if (isNUL) {
//                // TODO: Next iteration: NUL segment.
//                // We must ack it if it falls into acceptance window.
//                // However we shouldn't set _rcvLatestAckedSegment to this seq number.
//                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
//                YRSessionDoACKOrEACK(session);
//                break;
//            }
//            
//            if (YRPacketHeaderHasPayloadLength(receivedHeader) &&
//                YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)receivedHeader) > 0) {
//                if (rcvSeqNumber == expectedToReceive) {
//                    session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
//                    
//                    // Do callout with data.
//                    YRPayloadLengthType payloadLength = 0;
//                    void *rawPayload = YRPacketGetPayload(receivedPacket, &payloadLength);
//                    
//                    !session->callbacks.receiveCallout ?: session->callbacks.receiveCallout(session, rawPayload, payloadLength);
//                    
//                    YRSessionProcessOutOfSequencePacketsIfAny(session);
//                } else {
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

YRRUDPState YRRUDPConnectedState() {
	return STATE_DECL(kYRRUDPSessionStateConnected);
}
