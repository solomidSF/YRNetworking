//
//  YRInternal.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 2/18/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRInternal__
#define __YRInternal__

#include "YRNetworking.h"

#define YR_MAKE_MULTIPLE_TO(what, to) (((what) + ((to) - 1)) & (~((to) - 1)))

#include "YRUtils+Internal.h"
#include "YRRUDPSessionProtocol+Internal.h"
#include "YRRUDPStates+Internal.h"
#include "YRRuntimeEnvironment+Internal.h"

#define __YRNETWORKING_INDIRECT__
#include "YRRef.h"
#undef __YRNETWORKING_INDIRECT__

#endif
