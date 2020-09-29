//
//  YRRUDPStates.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 3/9/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRRUDPStates__
#define __YRRUDPStates__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

typedef struct YRRUDPState YRRUDPState;

struct YRRUDPState {
	YRRUDPSessionState representedState;
    void (*onEnter) (YRRUDPSessionProtocolRef protocol, YRRUDPState prevState);
    void (*onExit) (YRRUDPSessionProtocolRef protocol, YRRUDPState nextState);
	
	YRSessionProtocolLifecycleCallbacks lifecycleCallbacks;
	YRSessionProtocolCallbacks protocolCallbacks;
};

YRRUDPState YRRUDPStateForState(YRRUDPSessionState state);

#endif
