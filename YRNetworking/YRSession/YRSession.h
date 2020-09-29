//
// YRSession.h
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

#ifndef __YRSession__
#define __YRSession__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#include "YRBase.h"
#endif

#pragma mark - Declarations

typedef struct YRSession *YRSessionRef;

#pragma mark - Lifecycle

YRSessionRef YRSessionCreate(YRSessionProtocolRef protocol);

// TODO: Remove
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
void YRSessionReceive(YRSessionRef session, void *payload, uint16_t length);
void YRSessionSend(YRSessionRef session, void *payload, uint16_t length);

#endif
