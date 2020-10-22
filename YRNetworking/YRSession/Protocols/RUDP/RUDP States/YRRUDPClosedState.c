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

#pragma mark - Lifecycle

void onEnterClosed(YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {}

void onExitClosed(YRRUDPSessionProtocolRef protocol, YRRUDPState next) {}

#pragma mark - YRSessionProtocolLifecycleCallbacks

YR_FP_IMPL(invalidateClosed, YRSessionProtocolRef protocol) {

};

YR_FP_IMPL(destroyClosed, YRSessionProtocolRef protocol) {
	
};

#pragma mark - YRSessionProtocolCallbacks

YR_FP_IMPL(connectClosed, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef rudp = (YRRUDPSessionProtocolRef)protocol;
	YRRUDPSessionEnterState(rudp, YRRUDPStateForState(kYRRUDPSessionStateInitiating));
};

YR_FP_IMPL(waitClosed, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef rudp = (YRRUDPSessionProtocolRef)protocol;
	YRRUDPSessionEnterState(rudp, YRRUDPStateForState(kYRRUDPSessionStateWaiting));
};

YR_FP_IMPL(closeClosed, YRSessionProtocolRef protocol) {
	// no-op
};

YR_FP_IMPL(sendClosed, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {
	// TODO: Warning <Send not handled in this state>
};

YR_FP_IMPL(receiveClosed, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {
	// TODO: Send RST to peer
};

YRRUDPState YRRUDPClosedState() {
	return (YRRUDPState) {
		kYRRUDPSessionStateClosed,
		onEnterClosed,
		onExitClosed,
		{invalidateClosed, destroyClosed},
		{connectClosed, waitClosed, closeClosed, sendClosed, receiveClosed},
	};
}
