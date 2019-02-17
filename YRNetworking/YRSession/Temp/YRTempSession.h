//
//  YRTempSession.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 2/4/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRTempSession_h
#define YRTempSession_h

#include "YRConnectionConfiguration.h"
#include "YRTypes.h"
#include "YRSessionState.h"

#include <Block.h>

#pragma mark - Declarations

typedef struct YRSession *YRSessionRef;

typedef void (^YRSessionConnectionStateCallout) (YRSessionRef session, YRSessionState newState);
typedef void (^YRSessionSendCallout) (YRSessionRef session, const void *payload, YRPayloadLengthType size);
typedef void (^YRSessionReceiveCallout) (YRSessionRef session, const void *payload, YRPayloadLengthType size);

typedef struct {
    YRSessionConnectionStateCallout connectionStateCallout;
    YRSessionSendCallout sendCallout;
    YRSessionReceiveCallout receiveCallout;
    // void *hasSpaceAvailableCallout
} YRSessionCallbacks;

typedef struct {
    // Send-related
    YRSequenceNumberType sendInitialSequenceNumber;
    YRSequenceNumberType sendNextSequenceNumber;
    YRSequenceNumberType sendLatestUnackSegment;
    
    // Receive-related
    YRSequenceNumberType rcvLatestAckedSegment;
    YRSequenceNumberType rcvInitialSequenceNumber;
} YRSessionInfo;

#pragma mark - Sizes

/**
 *  Returns minimum required size for session with given configuration.
 */
size_t YRSessionGetMinimumSize(YRConnectionConfiguration configuration);

#pragma mark - Lifecycle

YRSessionRef YRSessionCreateWithConfiguration(YRConnectionConfiguration configuration, YRSessionCallbacks callbacks);
void YRSessionDestroy(YRSessionRef session);

// Temporary solution
void YRSessionSetTimerProvider(YRSessionRef session, void *(^createTimer) (uint32_t seconds), void (^cancel) (void *));

//YRSessionRef YRSessionRetain(YRSessionRef session);
//YRSessionRef YRSessionRelease(YRSessionRef session);

#pragma mark - Configuration

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
 *  Returns true if session has space available for outgoing data to be sent.
 */
bool YRSessionCanSend(YRSessionRef session);

/**
 *  Send/receive are abstracted away and not managed by session.
 *  These convenience functions should be called by one when raw data received from peer or should be sent to peer.
 *  YRSession will call handlers passed on initialization providing real data to be sent/received.
 */
void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length);
void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length);

#pragma mark - State

YRSessionState YRSessionGetState(YRSessionRef session);
YRSessionInfo YRSessionGetSessionInfo(YRSessionRef session);
YRConnectionConfiguration YRSessionGetLocalConnectionInfo(YRSessionRef session);
YRConnectionConfiguration YRSessionGetRemoteConnectionInfo(YRSessionRef session);


#endif /* YRTempSession_h */
