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

void onEnterConnecting(YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	
}

void onExitConnecting(YRRUDPSessionProtocolRef protocol, YRRUDPState next) {
	
}

#pragma mark - YRSessionProtocolLifecycleCallbacks

YR_FP_IMPL(invalidateConnecting, YRSessionProtocolRef protocol) {
	YRRUDPSessionProtocolRef rudp = (YRRUDPSessionProtocolRef)protocol;
	
};

YR_FP_IMPL(destroyConnecting, YRSessionProtocolRef protocol) {
	
};

#pragma mark - YRSessionProtocolCallbacks

YR_FP_IMPL(connectConnecting, YRSessionProtocolRef protocol) {
	
};


YR_FP_IMPL(waitConnecting, YRSessionProtocolRef protocol) {

};

YR_FP_IMPL(closeConnecting, YRSessionProtocolRef protocol) {

};

YR_FP_IMPL(sendConnecting, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {

};

YR_FP_IMPL(receiveConnecting, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {

};

YRRUDPState YRRUDPConnectingState() {
	return (YRRUDPState) {
		kYRRUDPSessionStateConnecting,
		onEnterConnecting,
		onExitConnecting,
		{invalidateConnecting, destroyConnecting},
		{connectConnecting, waitConnecting, closeConnecting, sendConnecting, receiveConnecting},
	};
}
