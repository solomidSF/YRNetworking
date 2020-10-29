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

#define STATE_PREFIX initiating

//            // Waiting for SYN/ACK
//            if (isRST && hasACK) {
//                // err: Connection refused
//                //                [_sessionLogger logError:@"Connection refused!"];
//                YRSessionTransiteToState(session, kYRSessionStateClosed);
//
//                break;
//            }
//
//            if (isSYN) {
//                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
//                session->sessionInfo.rcvInitialSequenceNumber = rcvSeqNumber;
//
//                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
//
//                session->remoteConnectionConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
//
//                if (hasACK) {
//                    // Normally we should remove all operations from send queue.
//                    // But send queue is not created yet and we only have 1 packet that we send: SYN.
//                    // And it's not stored in our send queue, so we only need to cancel retransmission timer.
//                    session->sessionInfo.sendLatestUnackSegment = rcvAckNumber + 1;
//
//                    YRSessionTransiteToState(session, kYRSessionStateConnected);
//
//                    YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                        YRPacketCreateACK(seqNumber, ackNumber, packetBuffer);
//                    }, YRPacketACKLength());
//                } else {
//                    YRSessionTransiteToState(session, kYRSessionStateConnecting);
//
//                    // TODO: This flow is strange as rfc describes.
//                    // if SYN & SYN rcved on both sides, SYN/ACK will be ignored on both sides which will result in ACK sent by both peer = connected.
//                    // if - & SYN/ACK rcvd = SYN/ACK side will resend SYN/ACK.
//                    YRSessionDoReliableSendWithAutoIncrement(session, ^(void *packetBuffer,
//                                                                        YRSequenceNumberType seqNumber,
//                                                                        YRSequenceNumberType ackNumber) {
//                        YRPacketCreateSYN(session->localConnectionConfiguration,
//                                          session->sessionInfo.sendInitialSequenceNumber,
//                                          session->sessionInfo.rcvLatestAckedSegment,
//                                          true,
//                                          packetBuffer);
//                    }, YRPacketSYNLength(), false);
//                }
//
//                break;
//            }
//
//            break;

#pragma mark - Lifecycle

void STATE_FP_PREFIX(onEnter) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	// TODO: Send SYN
	YRRUDPSessionSendSYN(protocol);
//	YRPacketCreateSYN(<#YRRUDPConnectionConfiguration configuration#>, <#YRSequenceNumberType seqNumber#>, <#YRSequenceNumberType ackNumber#>, <#bool hasACK#>, <#YROutputStreamRef stream#>, <#YRPayloadLengthType *packetLength#>)
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

YRRUDPState YRRUDPInitiatingState() {
	return STATE_DECL(kYRRUDPSessionStateInitiating);
}
