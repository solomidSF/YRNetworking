//
//  YRSession.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/17/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRSession__
#define __YRSession__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#include "YRBase.h"
#endif

#pragma mark - Declarations

typedef struct YRSession *YRSessionRef;

#if __has_extension(blocks)

typedef void (^YRSessionConnectCallback) (YRSessionRef session);
typedef void (^YRSessionWaitCallback) (YRSessionRef session);
typedef void (^YRSessionCloseCallback) (YRSessionRef session);
typedef void (^YRSessionInvalidateCallback) (YRSessionRef session);

typedef void (^YRSessionSendCallback) (YRSessionRef session);
typedef void (^YRSessionReceiveCallback) (YRSessionRef session, const void *payload, YRPayloadLengthType length);

#else

typedef void (*YRSessionConnectCallback) (YRSessionRef session);
typedef void (*YRSessionWaitCallback) (YRSessionRef session);
typedef void (*YRSessionCloseCallback) (YRSessionRef session);
typedef void (*YRSessionInvalidateCallback) (YRSessionRef session);

typedef void (*YRSessionSendCallback) (YRSessionRef session);
typedef void (*YRSessionReceiveCallback) (YRSessionRef session, const void *payload, YRPayloadLengthType length);

#endif // __has_extension(blocks)

typedef struct {
    YRSessionConnectCallback connectCallback;
    YRSessionWaitCallback waitCallback;
    YRSessionCloseCallback closeCallback;
    YRSessionInvalidateCallback invalidateCallback;
    YRSessionSendCallback sendCallback;
    YRSessionReceiveCallback receiveCallback;
} YRSessionProtocolCallbacks;

#pragma mark - Lifecycle

YRSessionRef YRSessionCreate(void *protocol);

void YRSessionRetain(YRSessionRef session);
void YRSessionRelease(YRSessionRef session);

#pragma mark - Connection

/**
 *  Acts as active session (aka client) that sends request to connect to its peer.
 */
void YRSessionConnect(YRSessionRef session);

/**
 *  Acts as passive session (aka server) that waits for connection request from its peer.
 */
void YRSessionWait(YRSessionRef session);

/**
 *  Closes connection and notifies its peer about that.
 */
void YRSessionClose(YRSessionRef session);

/**
 *  Forcefully invalidates connection without notifying its peer.
 *  Should be called if peer can't be notified about this at all.
 */
void YRSessionInvalidate(YRSessionRef session);

#pragma mark - Communication

/**
 *  Send/receive are abstracted away and not managed by session.
 *  These convenience functions should be called by one when raw data received from peer or should be sent to peer.
 */
void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length);
void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length);

#endif
