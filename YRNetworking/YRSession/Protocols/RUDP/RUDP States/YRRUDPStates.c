//
//  YRRUDPStates.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 3/9/19.
//  Copyright © 2019 Yuriy Romanchenko. All rights reserved.
//

#include "YRInternal.h"

const YRRUDPState kRUDPNullState = {
	kYRRUDPSessionStateClosed, NULL, NULL, NULL, NULL
};

extern YRRUDPState YRRUDPClosedState(void);
extern YRRUDPState YRRUDPWaitingState(void);
extern YRRUDPState YRRUDPInitiatingState(void);
extern YRRUDPState YRRUDPConnectingState(void);
extern YRRUDPState YRRUDPConnectedState(void);
extern YRRUDPState YRRUDPDisconnectingState(void);

YRRUDPState YRRUDPStateForState(YRRUDPSessionState state) {
	switch (state) {
		case kYRRUDPSessionStateClosed: return YRRUDPClosedState();
		case kYRRUDPSessionStateWaiting: return YRRUDPWaitingState();
		case kYRRUDPSessionStateInitiating: return YRRUDPInitiatingState();
		case kYRRUDPSessionStateConnecting: return YRRUDPConnectingState();
		case kYRRUDPSessionStateConnected: return YRRUDPConnectedState();
		case kYRRUDPSessionStateDisconnecting: return YRRUDPDisconnectingState();
	}
	
	return kRUDPNullState;
}
