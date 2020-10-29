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

#define STATE_PREFIX closed

#pragma mark - Prototypes

void YRRUDPClosedStateUpdateSessionInfo(YRRUDPSessionProtocolRef protocol, YRSequenceNumberType seqNumber, bool keepAlive);

#pragma mark - Lifecycle

void STATE_FP_PREFIX(onEnter) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	// TODO: Cleanup
}

void STATE_FP_PREFIX(onExit) (YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	// no-op
}

#pragma mark - YRSessionProtocolLifecycleCallbacks

STATE_FP_IMPL(invalidate, YRSessionProtocolRef protocol) {
	// no-op
};

STATE_FP_IMPL(destroy, YRSessionProtocolRef protocol) {
	// no-op
};

#pragma mark - YRSessionProtocolCallbacks

STATE_FP_IMPL(connect, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef sessionProtocol = (YRRUDPSessionProtocolRef)protocol;

	YRRUDPClosedStateUpdateSessionInfo(sessionProtocol, arc4random(), false);

	YRRUDPSessionEnterState(sessionProtocol, YRRUDPStateForState(kYRRUDPSessionStateInitiating));
};

STATE_FP_IMPL(wait, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef sessionProtocol = (YRRUDPSessionProtocolRef)protocol;
	
	YRRUDPClosedStateUpdateSessionInfo(sessionProtocol, arc4random(), true);
	
	YRRUDPSessionEnterState(sessionProtocol, YRRUDPStateForState(kYRRUDPSessionStateWaiting));
};

STATE_FP_IMPL(close, YRSessionProtocolRef protocol) {
	// no-op
};

STATE_FP_IMPL(send, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
	// TODO: Warning <Send not handled in this state>
};

STATE_FP_IMPL(receive, YRSessionProtocolRef protocol, const void *payload, YRPayloadLengthType payloadLength) {
	// no-op
};

#pragma mark - YRPacketHandlers

void STATE_FP_PREFIX(syn) (void *context, YRPacketHeaderSYNRef header) {

};

void STATE_FP_PREFIX(rst) (void *context, YRPacketHeaderRSTRef header) {
	// no-op
};

void STATE_FP_PREFIX(nul) (void *context, YRPacketHeaderRef header) {

};

void STATE_FP_PREFIX(eack) (void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength) {
};

void STATE_FP_PREFIX(regular) (void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength) {
};

void STATE_FP_PREFIX(invalid) (void *context, YRRUDPError protocol) {

};

//
//            if (hasACK || isNUL) {
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, rcvAckNumber + 1, 0, false, packetBuffer);
//                }, YRPacketRSTLength());
//            } else {
//                YRSessionDoUnreliableSend(session, ^(void *packetBuffer, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
//                    YRPacketCreateRST(0, 0, rcvSeqNumber, true, packetBuffer);
//                }, YRPacketRSTLength());
//            }
//
//            break;

YRRUDPState YRRUDPClosedState() {
	return STATE_DECL(kYRRUDPSessionStateClosed);
}

#pragma mark - Private

void YRRUDPClosedStateUpdateSessionInfo(YRRUDPSessionProtocolRef protocol, YRSequenceNumberType seqNumber, bool keepAlive) {
	YRRUDPSessionInfo info = YRRUDPSessionGetInfo(protocol);
	info.sendInitialSequenceNumber = seqNumber;
	info.sendNextSequenceNumber = seqNumber;
	info.shouldKeepAlive = keepAlive;
	YRRUDPSessionUpdateInfo(protocol, info);
}

void YRRUDPClosedStateSendRST(YRPacketHeaderRef header) {
	if (YRPacketHeaderHasACK(header) || YRPacketHeaderIsNUL(header)) {
//		YRPacketCreateRST(<#uint8_t errorCode#>, <#YRSequenceNumberType seqNumber#>, <#YRSequenceNumberType ackNumber#>, <#bool hasACK#>, <#YROutputStreamRef stream#>, <#YRPayloadLengthType *packetLength#>)
	} else {
		
	}
}
