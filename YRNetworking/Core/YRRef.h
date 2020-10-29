//
//  YRRef.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 2/7/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

// TODO: <RF> Remove
#ifndef __YRRef__
#define __YRRef__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#include "YRBase.h"
#endif

#define container_of(ptr, type, member) \
((type *)((char *)(ptr) - offsetof(type, member)))

/**
 *  Simple reference counting implemetation.
 */
struct YRRef {
    void (*destructor) (const struct YRRef *ref);
    int32_t count;
};

static inline void
YRRefInc(struct YRRef *ref) {
    __sync_add_and_fetch((int32_t *)&ref->count, 1);
}

static inline void
YRRefDec(struct YRRef *ref) {
    if (__sync_sub_and_fetch((int32_t *)&ref->count, 1) == 0) {
        ref->destructor(ref);
    }
}

#endif // __YRRef__
