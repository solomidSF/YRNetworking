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

#endif // __YRBase__
