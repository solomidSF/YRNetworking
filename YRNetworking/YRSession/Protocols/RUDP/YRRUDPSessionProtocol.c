//
// YRRUDPSessionProtocol.c
//
// The MIT License (MIT)
//
// Copyright (c) 2019 Yuri R.
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

#define YR_FP_INVOKE(fp, ...) !(fp) ?: (fp(__VA_ARGS__))
#define YR_FP_FORWARD(fp, ...) \
	do { \
		YRRUDPSessionProtocolRef rudp = (YRRUDPSessionProtocolRef)protocol; \
		YR_FP_INVOKE(rudp->currentState.fp, __VA_ARGS__); \
	} while(0)

#pragma mark - Declarations

typedef struct YRRUDPSessionProtocol {
    struct YRSessionProtocol rawProtocol;
	struct YRRUDPState currentState;
} YRRUDPSessionProtocol;

#pragma mark - Prototypes

void YRRUDPSessionEnterState(YRRUDPSessionProtocolRef protocol, struct YRRUDPState state);

#pragma mark - YRSessionProtocolLifecycleCallbacks

YR_FP_IMPL(invalidate, YRSessionProtocolRef protocol) {
	YR_FP_FORWARD(lifecycleCallbacks.invalidateCallback, protocol);
};

YR_FP_IMPL(destroy, YRSessionProtocolRef protocol) {
	YR_FP_FORWARD(lifecycleCallbacks.destroyCallback, protocol);
};

#pragma mark - YRSessionProtocolCallbacks

YR_FP_IMPL(rudpConnect, YRSessionProtocolRef protocol) {
	YR_FP_FORWARD(protocolCallbacks.connectCallback, protocol);
};

YR_FP_IMPL(rudpWait, YRSessionProtocolRef protocol) {
	YR_FP_FORWARD(protocolCallbacks.waitCallback, protocol);
};

YR_FP_IMPL(rudpClose, YRSessionProtocolRef protocol) {
	YR_FP_FORWARD(protocolCallbacks.closeCallback, protocol);
};

YR_FP_IMPL(rudpSend, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {
	YR_FP_FORWARD(protocolCallbacks.sendCallback, protocol, payload, length);
};

YR_FP_IMPL(rudpReceive, YRSessionProtocolRef protocol, const void *payload, uint16_t length) {
	YR_FP_FORWARD(protocolCallbacks.receiveCallback, protocol, payload, length);
};

static YRSessionProtocolLifecycleCallbacks rudpLifecycleCallbacks;
static YRSessionProtocolCallbacks rudpProtocolCallbacks;

#pragma mark - Interface

YRSessionRef YRRUDPSessionCreate(YRRUDPSessionProtocolCallbacks callbacks) {
	static bool isRUDPProtocolInitialized = false;

	if (!isRUDPProtocolInitialized) {
		rudpLifecycleCallbacks = (YRSessionProtocolLifecycleCallbacks) {
			invalidate,
			destroy
		};
		
		rudpProtocolCallbacks = (YRSessionProtocolCallbacks) {
			rudpConnect,
			rudpWait,
			rudpClose,
			rudpSend,
			rudpReceive
		};
		
		isRUDPProtocolInitialized = true;
	}
	
    YRRUDPSessionProtocolRef protocol = calloc(1, sizeof(YRRUDPSessionProtocol));

	YRSessionProtocolSetLifecycleCallbacks(&protocol->rawProtocol, rudpLifecycleCallbacks);
	YRSessionProtocolSetProtocolCallbacks(&protocol->rawProtocol, rudpProtocolCallbacks);
	YRSessionProtocolSetClientCallbacks(&protocol->rawProtocol, callbacks.clientCallbacks);
	
	YRRUDPSessionEnterState(protocol, YRRUDPStateForState(kYRRUDPSessionStateClosed));

	YRSessionRef session = YRSessionCreate(&protocol->rawProtocol);

    return session;
}

void YRRUDPSessionEnterState(YRRUDPSessionProtocolRef protocol, YRRUDPState state) {
	YR_FP_INVOKE(protocol->currentState.onExit, protocol, protocol->currentState);
	protocol->currentState = state;
	YR_FP_INVOKE(protocol->currentState.onEnter, protocol, state);
}
