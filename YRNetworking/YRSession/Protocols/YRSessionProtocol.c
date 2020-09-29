//
// YRSessionProtocol.c
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

#define YR_DO_CALLOUT(callbacks, name, ...) \
    !protocol->callbacks.name ?: protocol->callbacks.name(__VA_ARGS__)

#define YR_DO_LIFECYCLE_CALLOUT(name, ...) YR_DO_CALLOUT(lifecycleCallbacks, name, __VA_ARGS__)
#define YR_DO_PROTOCOL_CALLOUT(name, ...) YR_DO_CALLOUT(protocolCallbacks, name, __VA_ARGS__)
#define YR_DO_CLIENT_CALLOUT(name, ...) YR_DO_CALLOUT(clientCallbacks, name, __VA_ARGS__)

#pragma mark - Lifecycle

void YRSessionProtocolInvalidate(YRSessionProtocolRef protocol) {
    YR_DO_LIFECYCLE_CALLOUT(invalidateCallback, protocol);
}

void YRSessionProtocolDestroy(YRSessionProtocolRef protocol) {
    YR_DO_LIFECYCLE_CALLOUT(destroyCallback, protocol);
}

#pragma mark - Configuration

void YRSessionProtocolSetCallbacks(YRSessionProtocolRef protocol,
                                   YRSessionProtocolLifecycleCallbacks lifecycleCallbacks,
                                   YRSessionProtocolCallbacks protocolCallbacks,
                                   YRSessionProtocolClientCallbacks clientCallbacks) {
    YRSessionProtocolSetLifecycleCallbacks(protocol, lifecycleCallbacks);
    YRSessionProtocolSetProtocolCallbacks(protocol, protocolCallbacks);
    YRSessionProtocolSetClientCallbacks(protocol, clientCallbacks);
}

void YRSessionProtocolSetLifecycleCallbacks(YRSessionProtocolRef protocol,
                                            YRSessionProtocolLifecycleCallbacks lifecycleCallbacks) {
    YR_COPY_FP(lifecycleCallbacks.invalidateCallback);
    YR_COPY_FP(lifecycleCallbacks.destroyCallback);

    YR_RELEASE_FP(protocol->lifecycleCallbacks.invalidateCallback);
    YR_RELEASE_FP(protocol->lifecycleCallbacks.destroyCallback);

    protocol->lifecycleCallbacks = lifecycleCallbacks;
}

void YRSessionProtocolSetProtocolCallbacks(YRSessionProtocolRef protocol,
                                           YRSessionProtocolCallbacks protocolCallbacks) {
    YR_COPY_FP(protocolCallbacks.connectCallback);
    YR_COPY_FP(protocolCallbacks.waitCallback);
    YR_COPY_FP(protocolCallbacks.closeCallback);
    YR_COPY_FP(protocolCallbacks.sendCallback);
    YR_COPY_FP(protocolCallbacks.receiveCallback);

    YR_RELEASE_FP(protocol->protocolCallbacks.connectCallback);
    YR_RELEASE_FP(protocol->protocolCallbacks.waitCallback);
    YR_RELEASE_FP(protocol->protocolCallbacks.closeCallback);
    YR_RELEASE_FP(protocol->protocolCallbacks.sendCallback);
    YR_RELEASE_FP(protocol->protocolCallbacks.receiveCallback);

    protocol->protocolCallbacks = protocolCallbacks;
}

void YRSessionProtocolSetClientCallbacks(YRSessionProtocolRef protocol,
                                         YRSessionProtocolClientCallbacks clientCallbacks) {
    YR_COPY_FP(clientCallbacks.sendCallback);
    YR_COPY_FP(clientCallbacks.receiveCallback);
    
    YR_RELEASE_FP(protocol->clientCallbacks.sendCallback);
    YR_RELEASE_FP(protocol->clientCallbacks.receiveCallback);
    
    protocol->clientCallbacks = clientCallbacks;
}

YRSessionProtocolLifecycleCallbacks YRSessionProtocolGetLifecycleCallbacks(YRSessionProtocolRef protocol) {
    return protocol->lifecycleCallbacks;
}

YRSessionProtocolCallbacks YRSessionProtocolGetCallbacks(YRSessionProtocolRef protocol) {
    return protocol->protocolCallbacks;
}
                                                                                       
YRSessionProtocolClientCallbacks YRSessionProtocolGetClientCallbacks(YRSessionProtocolRef protocol) {
    return protocol->clientCallbacks;
}

#pragma mark - Connection

void YRSessionProtocolConnect(YRSessionProtocolRef protocol) {
    YR_DO_PROTOCOL_CALLOUT(connectCallback, protocol);
}

void YRSessionProtocolWait(YRSessionProtocolRef protocol) {
    YR_DO_PROTOCOL_CALLOUT(waitCallback, protocol);
}

void YRSessionProtocolClose(YRSessionProtocolRef protocol) {
    YR_DO_PROTOCOL_CALLOUT(closeCallback, protocol);
}

#pragma mark - Communication

void YRSessionProtocolReceive(YRSessionProtocolRef protocol, void *payload, uint16_t length) {
    YR_DO_PROTOCOL_CALLOUT(receiveCallback, protocol, payload, length);
}

void YRSessionProtocolSend(YRSessionProtocolRef protocol, void *payload, uint16_t length) {
    YR_DO_PROTOCOL_CALLOUT(sendCallback, protocol, payload, length);
}

#pragma mark - Concepts

void YRSessionProtocolInstallModules(YRSessionProtocolRef protocol, void *modules) {
    // no-op
}
