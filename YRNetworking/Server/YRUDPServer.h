//
//  YRConnectionService.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/20/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "YRUDPConnectedClient.h"

typedef enum {
    YRUDPServerNotReady,
    YRUDPServerReady,
} YRUDPServerState;

@interface YRUDPServer : NSObject

@property (nonatomic, readonly) YRUDPServerState state;
@property (nonatomic) uint32_t maxConnections;

@property (nonatomic, copy) void (^onSocketDidClose) (YRUDPServer *server, NSError *error);
@property (nonatomic, copy) void (^onClientConnected) (YRUDPServer *server, YRUDPConnectedClient *client);
@property (nonatomic, copy) void (^onClientRejected) (YRUDPServer *server, YRUDPConnectedClient *client);
@property (nonatomic, copy) void (^onClientDisconnected) (YRUDPServer *server, YRUDPConnectedClient *client);

@property (nonatomic, readonly) BOOL isAcceptingNewConnections;
@property (nonatomic, readonly) NSArray <YRUDPConnectedClient *> *clients;

+ (instancetype)server;
+ (instancetype)serverWithPort:(uint32_t)port;

#pragma mark - Public

- (BOOL)setup:(NSError *__autoreleasing *)error;

- (void)continueAcceptingNewConnections;
- (void)stopAcceptingNewConnections;

- (void)disconnectClient:(YRUDPConnectedClient *)client;

#pragma mark - Sending

- (void)sendReliably:(id)data to:(YRUDPConnectedClient *)client;
- (void)sendUnreliably:(id)data to:(YRUDPConnectedClient *)client;

@end
