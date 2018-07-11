//
//  YRSession.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/24/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

@import Foundation;

#import "YRPeer.h"
#import "YRConfiguration.h"

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

@class YRSessionOVERLOADED;

@interface YRSessionContext : NSObject

@property (nonatomic) NSData *peerAddress;
@property (nonatomic) YRConfiguration *sessionConfig;
@property (nonatomic, copy) void (^connectionStateCallout) (YRSessionOVERLOADED *session, YRSessionState state);
@property (nonatomic, copy) void (^sendCallout) (YRSessionOVERLOADED *session, NSData *data);
@property (nonatomic, copy) id receiveCallout;

@end

@interface YRSessionOVERLOADED : NSObject

@property (nonatomic, readonly) NSData* peerAddress;

/**
 *  Peer to which this session is related.
 */
@property (nonatomic, readonly) YRPeer *peer;
@property (nonatomic, readonly) YRSessionState state;
@property (nonatomic, readonly) YRConfiguration *sessionConfig;

+ (BOOL)isContextValid:(YRSessionContext *)context outErrorCode:(int32_t *)outError;

#pragma mark - Init

- (instancetype)initWithContext:(YRSessionContext *)context;

#pragma mark - Configuration

/**
 *  Acts as active session (aka client) that sends request to connect to its peer.
 */
- (void)connect;

/**
 *  Acts as passive session (aka server) that waits for connection request from its peer.
 */
- (void)wait;

/**
 *  Closes connection and notifies its peer about that.
 */
- (void)close;

/**
 *  Forcefully invalidates connection without notifying its peer.
 *  Should be called if peer can't be notified about this at all.
 */
- (void)invalidate;

#pragma mark - Communication

/**
 *  Send/receive are abstracted away and are not managed by session.
 *  These convenience functions should be called when by one when raw data received from peer or should be sent to peer.
 *  YRSession will call handlers passed on initialization providing real data to be sent/received.
 */
- (void)receive:(NSData *)data;
- (void)send:(NSData *)data;

@end
