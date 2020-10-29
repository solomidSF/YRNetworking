//
// YRSessionProtocol.h
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

// TODO: Rename to YRSessionProtocolInterface
#ifndef __YRSessionProtocol__
#define __YRSessionProtocol__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#include "YRBase.h"
#endif

#pragma mark - Declarations

typedef struct YRSessionProtocol *YRSessionProtocolRef;

YR_TYPEDEF_FP(YRSessionProtocolInvalidateCallback, YRSessionProtocolRef protocol);
YR_TYPEDEF_FP(YRSessionProtocolDestroyCallback, YRSessionProtocolRef protocol);
YR_TYPEDEF_FP(YRSessionProtocolConnectCallback, YRSessionProtocolRef protocol);
YR_TYPEDEF_FP(YRSessionProtocolWaitCallback, YRSessionProtocolRef protocol);
YR_TYPEDEF_FP(YRSessionProtocolCloseCallback, YRSessionProtocolRef protocol);
YR_TYPEDEF_FP(YRSessionProtocolSendCallback, YRSessionProtocolRef protocol, const void *payload, uint16_t length);
YR_TYPEDEF_FP(YRSessionProtocolReceiveCallback, YRSessionProtocolRef protocol, const void *payload, uint16_t length);

typedef struct {
    YRSessionProtocolInvalidateCallback invalidateCallback;
    YRSessionProtocolDestroyCallback destroyCallback;
} YRSessionProtocolLifecycleCallbacks;

typedef struct {
    YRSessionProtocolConnectCallback connectCallback;
    YRSessionProtocolWaitCallback waitCallback;
    YRSessionProtocolCloseCallback closeCallback;
    YRSessionProtocolSendCallback sendCallback;
    YRSessionProtocolReceiveCallback receiveCallback;
} YRSessionProtocolCallbacks;

typedef struct {
    YRSessionProtocolSendCallback sendCallback;
    YRSessionProtocolReceiveCallback receiveCallback;
} YRSessionProtocolClientCallbacks;

// TODO: Make internal
typedef struct YRSessionProtocol {
    YRSessionProtocolLifecycleCallbacks lifecycleCallbacks;
    YRSessionProtocolCallbacks protocolCallbacks;
    YRSessionProtocolClientCallbacks clientCallbacks;
} YRSessionProtocol;

#pragma mark - Lifecycle

void YRSessionProtocolInvalidate(YRSessionProtocolRef protocol);
void YRSessionProtocolDestroy(YRSessionProtocolRef protocol);

#pragma mark - Configuration

void YRSessionProtocolSetCallbacks(
	YRSessionProtocolRef protocol,
	YRSessionProtocolLifecycleCallbacks lifecycleCallbacks,
	YRSessionProtocolCallbacks protocolCallbacks,
	YRSessionProtocolClientCallbacks clientCallbacks
);

void YRSessionProtocolSetLifecycleCallbacks(
	YRSessionProtocolRef protocol,
	YRSessionProtocolLifecycleCallbacks lifecycleCallbacks
);

void YRSessionProtocolSetProtocolCallbacks(
	YRSessionProtocolRef protocol,
	YRSessionProtocolCallbacks protocolCallbacks
);

void YRSessionProtocolSetClientCallbacks(
	YRSessionProtocolRef protocol,
	YRSessionProtocolClientCallbacks clientCallbacks
);

YRSessionProtocolLifecycleCallbacks YRSessionProtocolGetLifecycleCallbacks(YRSessionProtocolRef protocol);
YRSessionProtocolCallbacks YRSessionProtocolGetCallbacks(YRSessionProtocolRef protocol);
YRSessionProtocolClientCallbacks YRSessionProtocolGetClientCallbacks(YRSessionProtocolRef protocol);

#pragma mark - Connection

void YRSessionProtocolConnect(YRSessionProtocolRef protocol);
void YRSessionProtocolWait(YRSessionProtocolRef protocol);
void YRSessionProtocolClose(YRSessionProtocolRef protocol);

#pragma mark - Communication

void YRSessionProtocolReceive(YRSessionProtocolRef protocol, void *payload, uint16_t length);
void YRSessionProtocolSend(YRSessionProtocolRef protocol, void *payload, uint16_t length);

// TODO:
#pragma mark - Concepts

void YRSessionProtocolInstallModules(YRSessionProtocolRef protocol, void *modules);

#endif
