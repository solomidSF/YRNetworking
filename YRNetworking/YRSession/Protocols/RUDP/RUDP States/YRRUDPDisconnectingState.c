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

#define STATE_PREFIX disconnecting
//            if (isRST) {
//                //                [_disconnectingTimer invalidate];
//
//                YRSessionTransiteToState(session, kYRSessionStateClosed);
//            }
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

YRRUDPState YRRUDPDisconnectingState() {
	return STATE_DECL(kYRRUDPSessionStateDisconnecting);
}
