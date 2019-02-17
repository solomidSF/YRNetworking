//
//  YRSession.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/17/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRInternal.h"
/////

//// Core
//#include "YRPacket.h"
//
//// Streams
//#import "YRLightweightInputStream.h"
//#import "YRLightweightOutputStream.h"
//
//// Private
//#include "YRPacketsQueue.h"

//////

#pragma mark - Declarations

typedef struct YRSession {
    struct YRRef ref;
    void* protocol;
} YRSession;

#pragma mark - Prototypes

void YRSessionDestroy(const struct YRRef* ref);

#pragma mark - Lifecycle

YRSessionRef YRSessionCreate(void *protocol) {
    YRSessionRef session = calloc(1, sizeof(YRSession));

    if (!session) {
        return NULL;
    }

    session->ref.destructor = &YRSessionDestroy;
    session->ref.count = 1;
    
    session->protocol = protocol;
    
    return session;
}

void YRSessionRetain(YRSessionRef session) {
    YRRefInc(&session->ref);
}

void YRSessionRelease(YRSessionRef session) {
    YRRefDec(&session->ref);
}

void YRSessionDestroy(const struct YRRef* ref) {
    YRSessionRef session = container_of(ref, struct YRSession, ref);
    free(session);
}

#pragma mark - Configuration

//void YRSessionSetCallbacks(YRSessionRef session, YRSessionCallbacks callbacks) {
//    if (callbacks.connectionStateCallout) {
//        callbacks.connectionStateCallout = _Block_copy(callbacks.connectionStateCallout);
//    }
//
//    if (callbacks.sendCallout) {
//        callbacks.sendCallout = _Block_copy(callbacks.sendCallout);
//    }
//
//    if (callbacks.receiveCallout) {
//        callbacks.receiveCallout = _Block_copy(callbacks.receiveCallout);
//    }
//
//    if (session->callbacks.connectionStateCallout) {
//        _Block_release(session->callbacks.connectionStateCallout);
//    }
//
//    if (session->callbacks.sendCallout) {
//        _Block_release(session->callbacks.sendCallout);
//    }
//
//    if (session->callbacks.receiveCallout) {
//        _Block_release(session->callbacks.receiveCallout);
//    }
//
//    session->callbacks = callbacks;
//}

#pragma mark - Connection

void YRSessionConnect(YRSessionRef session) {
    
}

void YRSessionWait(YRSessionRef session) {

}

void YRSessionClose(YRSessionRef session) {

}

void YRSessionInvalidate(YRSessionRef session) {
    
}

#pragma mark - Communication

void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length) {

}
                                                                                        
void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length) {

}
