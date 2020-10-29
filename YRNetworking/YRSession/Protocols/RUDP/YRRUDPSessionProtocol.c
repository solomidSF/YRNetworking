//
// YRRUDPSessionProtocol.c
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

// Invokes function if it's present
#define FP_INVOKE(fp, ...) !(fp) ?: (fp(__VA_ARGS__))

// Calls given function with passed arguments on current state
#define FORWARD_TO_STATE(fp, ...) \
	do { \
		YRRUDPSessionProtocolRef rudp = (YRRUDPSessionProtocolRef)protocol; \
		FP_INVOKE(rudp->currentState.fp, __VA_ARGS__); \
	} while(0)
	
// Appends prefix to function name
#define FORWARDING_FP(name) yr_session_##name

// Declares function that forwards call to current state
#define DECL_FORWARDING(name, fp) \
	YR_FP_IMPL(FORWARDING_FP(name), YRSessionProtocolRef protocol) { \
		FORWARD_TO_STATE(fp, protocol); \
	}
	
// Declares function that forwards call with payload to current state
#define DECL_FORWARDING_WITH_PAYLOAD(name, fp) \
	YR_FP_IMPL(FORWARDING_FP(name), YRSessionProtocolRef protocol, const void *payload, uint16_t payloadLength) { \
		FORWARD_TO_STATE(fp, protocol, payload, payloadLength); \
	}
	
#pragma mark - Declarations

typedef struct YRRUDPSessionProtocol {
    YRSessionProtocol rawProtocol;
	YRRUDPState currentState;
	YRPacketHandlers packetHandlers;
	YRRUDPSessionInfo sessionInfo;
	YRInputStream iStream;
	YROutputStream oStream;
	YRSessionProtocolConfig config;
} YRRUDPSessionProtocol;

#pragma mark - YRSessionProtocolLifecycleCallbacks

DECL_FORWARDING(invalidate, lifecycleCallbacks.invalidateCallback);
DECL_FORWARDING(destroy, lifecycleCallbacks.destroyCallback);

#pragma mark - YRSessionProtocolCallbacks

DECL_FORWARDING(connect, protocolCallbacks.connectCallback);
DECL_FORWARDING(wait, protocolCallbacks.waitCallback);
DECL_FORWARDING(close, protocolCallbacks.closeCallback);
DECL_FORWARDING_WITH_PAYLOAD(send, protocolCallbacks.sendCallback);

YR_FP_IMPL(FORWARDING_FP(receive), YRSessionProtocolRef protocol, const void *payload, uint16_t payloadLength) {
	YRRUDPSessionProtocol *sessionProtocol = (YRRUDPSessionProtocol *)protocol;

	YRInputStreamSetTo(&sessionProtocol->iStream, payload, payloadLength);
	YRPacketDeserialize(&sessionProtocol->iStream, sessionProtocol->packetHandlers, protocol);
};

#pragma mark - YRPacketHandlers

// Declares packet handler function which forwards call to current state
#define DECL_FORWARING_DESERIALIZE(name, arg1_type) \
	void FORWARDING_FP(name)(void *context, arg1_type arg) { \
		YRRUDPSessionProtocol *protocol = (YRRUDPSessionProtocol *)context; \
		FP_INVOKE(protocol->currentState.packetHandlers.name, context, arg); \
	}
	
// Declares packet handler function which forwards call with payload to current state
#define DECL_FORWARING_DESERIALIZE_WITH_PAYLOAD(name, header_type) \
	void FORWARDING_FP(name)(void *context, header_type header, const void *payload, YRPayloadLengthType payloadLength) { \
		YRRUDPSessionProtocol *protocol = (YRRUDPSessionProtocol *)context; \
		FP_INVOKE(protocol->currentState.packetHandlers.name, context, header, payload, payloadLength); \
	}
	
DECL_FORWARING_DESERIALIZE(syn, YRPacketHeaderSYNRef);
DECL_FORWARING_DESERIALIZE(rst, YRPacketHeaderRSTRef);
DECL_FORWARING_DESERIALIZE(nul, YRPacketHeaderRef);
DECL_FORWARING_DESERIALIZE_WITH_PAYLOAD(eack, YRPacketHeaderEACKRef);
DECL_FORWARING_DESERIALIZE_WITH_PAYLOAD(regular, YRPacketPayloadHeaderRef);
DECL_FORWARING_DESERIALIZE(invalid, YRRUDPError);

#pragma mark - Interface

YRSessionRef YRRUDPSessionCreate(YRRUDPSessionProtocolCallbacks callbacks, YRSessionProtocolConfig config) {
	static YRSessionProtocolLifecycleCallbacks rudpLifecycleCallbacks;
	static YRSessionProtocolCallbacks rudpProtocolCallbacks;
	static YRPacketHandlers packetHandlers;
	static bool isRUDPProtocolInitialized = false;

	if (!isRUDPProtocolInitialized) {
		rudpLifecycleCallbacks = (YRSessionProtocolLifecycleCallbacks) {
			FORWARDING_FP(invalidate),
			FORWARDING_FP(destroy)
		};
		
		rudpProtocolCallbacks = (YRSessionProtocolCallbacks) {
			FORWARDING_FP(connect),
			FORWARDING_FP(wait),
			FORWARDING_FP(close),
			FORWARDING_FP(send),
			FORWARDING_FP(receive)
		};
		
		packetHandlers = (YRPacketHandlers) {
			FORWARDING_FP(syn),
			FORWARDING_FP(rst),
			FORWARDING_FP(nul),
			FORWARDING_FP(eack),
			FORWARDING_FP(regular),
			FORWARDING_FP(invalid)
		};
		
		isRUDPProtocolInitialized = true;
	}
	
    YRRUDPSessionProtocolRef protocol = calloc(1, sizeof(YRRUDPSessionProtocol));

	YRSessionProtocolSetLifecycleCallbacks(&protocol->rawProtocol, rudpLifecycleCallbacks);
	YRSessionProtocolSetProtocolCallbacks(&protocol->rawProtocol, rudpProtocolCallbacks);
	YRSessionProtocolSetClientCallbacks(&protocol->rawProtocol, callbacks.clientCallbacks);
	
	protocol->packetHandlers = packetHandlers;
	protocol->config = config;
	
	YRRUDPSessionEnterState(protocol, YRRUDPStateForState(kYRRUDPSessionStateClosed));

	YRSessionRef session = YRSessionCreate(&protocol->rawProtocol);

    return session;
}

#pragma mark - Internal

void YRRUDPSessionEnterState(YRRUDPSessionProtocolRef protocol, YRRUDPState state) {
	FP_INVOKE(protocol->currentState.onExit, protocol, protocol->currentState);
	protocol->currentState = state;
	FP_INVOKE(protocol->currentState.onEnter, protocol, state);
}

YRRUDPSessionInfo YRRUDPSessionGetInfo(YRRUDPSessionProtocolRef protocol) {
	return protocol->sessionInfo;
}

void YRRUDPSessionUpdateInfo(YRRUDPSessionProtocolRef protocol, YRRUDPSessionInfo info) {
	protocol->sessionInfo = info;
}

void YRRUDPSessionSendSYN(YRRUDPSessionProtocolRef protocol) {
//	YRPacketCreateSYN(
//		protocol->sessionInfo.localConnectionConfiguration,
//		protocol->sessionInfo.sendInitialSequenceNumber,
//		<#YRSequenceNumberType ackNumber#>,
//		<#bool hasACK#>,
//		<#YROutputStreamRef stream#>,
//		<#YRPayloadLengthType *packetLength#>
//	)
}
