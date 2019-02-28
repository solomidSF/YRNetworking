//
// YRSession.c
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

#pragma mark - Declarations

typedef struct YRSession {
    struct YRRef ref;
    YRSessionProtocolRef protocol;
} YRSession;

#pragma mark - Prototypes

void YRSessionDestroy(const struct YRRef *ref);

#pragma mark - Lifecycle

YRSessionRef YRSessionCreate(YRSessionProtocolRef protocol) {
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
    
    YRSessionInvalidate(session);
    YRSessionProtocolDestroy(session->protocol);
    
    session->protocol = NULL;
    
    free(session);
}

#pragma mark - Connection

void YRSessionConnect(YRSessionRef session) {
    YRSessionProtocolConnect(session->protocol);
}

void YRSessionWait(YRSessionRef session) {
    YRSessionProtocolWait(session->protocol);
}

void YRSessionClose(YRSessionRef session) {
    YRSessionProtocolClose(session->protocol);
}

void YRSessionInvalidate(YRSessionRef session) {
    YRSessionProtocolInvalidate(session->protocol);
}

#pragma mark - Communication

void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    YRSessionProtocolReceive(session->protocol, payload, length);
}
                                                                                        
void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    YRSessionProtocolSend(session->protocol, payload, length);
}
