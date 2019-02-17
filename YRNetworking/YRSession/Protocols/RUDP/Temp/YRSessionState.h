//
//  YRSessionState.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/14/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRSessionState__
#define __YRSessionState__

typedef enum {
    // Session is closed.
    kYRSessionStateClosed,
    // Session is waiting for incoming connection request from it's peer.
    kYRSessionStateWaiting,
    // Session is initiating connection.
    kYRSessionStateInitiating,
    // Session is establishing connection.
    kYRSessionStateConnecting,
    // Session has established connection.
    kYRSessionStateConnected,
    // Session is disconnecting from it's peer.
    kYRSessionStateDisconnecting
} YRSessionState;

#endif
