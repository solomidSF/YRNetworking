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

#define STATE_PREFIX waiting

#pragma mark - Lifecycle

void STATE_FP_PREFIX(onEnter) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {}
void STATE_FP_PREFIX(onExit) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {}

#pragma mark - YRSessionProtocolLifecycleCallbacks

STATE_FP_IMPL(invalidate, YRSessionProtocolRef protocol) {

};

STATE_FP_IMPL(destroy, YRSessionProtocolRef protocol) {

};

#pragma mark - YRSessionProtocolCallbacks

STATE_FP_IMPL(connect, YRSessionProtocolRef protocol) {
};

STATE_FP_IMPL(wait, YRSessionProtocolRef protocol) {
};

STATE_FP_IMPL(close, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef sessionProtocol = (YRRUDPSessionProtocolRef)protocol;
	YRRUDPSessionEnterState(sessionProtocol, YRRUDPStateForState(kYRRUDPSessionStateClosed));
};

STATE_FP_IMPL(send, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
	// TODO: Warning <Send not handled in this state>
};

STATE_FP_IMPL(receive, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
};

#pragma mark - YRPacketHandlers

//            // Waiting for SYN packet.
//            if (isSYN) {
//                session->sessionInfo.rcvLatestAckedSegment = rcvSeqNumber;
//                session->sessionInfo.rcvInitialSequenceNumber = rcvSeqNumber;
//
//                YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)receivedHeader;
//
//                session->remoteConnectionConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
//
//                YRSessionTransiteToState(session, kYRSessionStateConnecting);
//
//                YRSessionDoReliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateSYN(session->localConnectionConfiguration, seqNumber, ackNumber, true, packetBuffer);
//                }, YRPacketSYNLength());
//
//                break;
//            }
//
//            if (isRST) {
//                break;
//            }
//

void STATE_FP_PREFIX(syn) (void *context, YRPacketHeaderSYNRef header) {
	YRPacketHeaderRef base = YRPacketHeaderSYNGetBaseHeader(header);
	YRRUDPSessionProtocolRef protocol = (YRRUDPSessionProtocolRef)context;
	
	YRRUDPSessionInfo info = YRRUDPSessionGetInfo(protocol);
	info.rcvLatestAckedSegment = YRPacketHeaderGetSequenceNumber(base);
	info.remoteConnectionConfiguration = YRPacketHeaderSYNGetConfiguration(header);

	YRRUDPSessionUpdateInfo(protocol, info);
	
	YRRUDPSessionEnterState(protocol, YRRUDPStateForState(kYRRUDPSessionStateConnecting));
};

void STATE_FP_PREFIX(rst) (void *context, YRPacketHeaderRSTRef header) {
	// no-op
};

void STATE_FP_PREFIX(nul) (void *context, YRPacketHeaderRef header) {
//            if (hasACK || isNUL) {
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, rcvAckNumber + 1, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//
//                break;
//            }
};

void STATE_FP_PREFIX(eack) (void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength) {
	// no-op?
};

void STATE_FP_PREFIX(regular) (void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength) {
//            if (hasACK || isNUL) {
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, rcvAckNumber + 1, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//
//                break;
//            }
};

void STATE_FP_PREFIX(invalid) (void *context, YRRUDPError protocol) {

};

YRRUDPState YRRUDPWaitingState() {
	return STATE_DECL(kYRRUDPSessionStateWaiting);
}
