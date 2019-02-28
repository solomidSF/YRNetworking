//
//  YRBase.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 2/17/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRBase__
#define __YRBase__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

#ifndef __has_feature        
#define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif
#ifndef __has_extension
#define __has_extension __has_feature // Compatibility with pre-3.0 compilers.
#endif

typedef uint16_t YRPayloadLengthType;

#if __has_extension(blocks)

#include <Block.h>

#define YR_DECLARE_FP(name, ...) typedef void (^(name)) (__VA_ARGS__)

#define YR_COPY_FP(fp) \
    do { \
        if ((fp)) { \
            _Block_copy((fp)); \
        } \
    } while (0)

#define YR_RELEASE_FP(fp) \
    do { \
        if ((fp)) { \
            _Block_release((fp)); \
        } \
    } while (0)

#else

#define YR_DECLARE_FP(name, ...) typedef void (*(name)) (__VA_ARGS__)

#define YR_COPY_FP(fp)
#define YR_RELEASE_FP(fp)

#endif // __has_extension(blocks)

#endif // __YRBase__
