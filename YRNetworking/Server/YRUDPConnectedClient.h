//
//  YRUDPConnectedClient.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/19/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
    YRUDPConnectedClientNotConnectedState,
    YRUDPConnectedClientConnectingState,
    YRUDPConnectedClientConnectedState,
} YRUDPConnectedClientState;

@interface YRUDPConnectedClient : NSObject

@property (nonatomic) YRUDPConnectedClientState state;

@end
