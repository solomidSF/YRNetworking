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

void onEnterConnected(YRRUDPSessionProtocolRef protocol, YRRUDPState prev) {
	
}

void onExitConnected(YRRUDPSessionProtocolRef protocol, YRRUDPState next) {
	
}

#pragma mark - YRSessionProtocolLifecycleCallbacks

YR_FP_IMPL(invalidateConnected, YRSessionProtocolRef protocol) {
	
};

YR_FP_IMPL(destroyConnected, YRSessionProtocolRef protocol) {
	
};

#pragma mark - YRSessionProtocolCallbacks

YR_FP_IMPL(connectConnected, YRSessionProtocolRef protocol) {
	
};


YR_FP_IMPL(waitConnected, YRSessionProtocolRef protocol) {

};

YR_FP_IMPL(closeConnected, YRSessionProtocolRef protocol) {

};

YR_FP_IMPL(sendConnected, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {

};

YR_FP_IMPL(receiveConnected, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {

};

YRRUDPState YRRUDPConnectedState() {
	return (YRRUDPState) {
		kYRRUDPSessionStateConnected,
		onEnterConnected,
		onExitConnected,
		{invalidateConnected, destroyConnected},
		{connectConnected, waitConnected, closeConnected, sendConnected, receiveConnected},
	};
}
