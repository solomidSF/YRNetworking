//
//  YRObjcSession.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

@import Foundation;

#import "YRObjcSessionContext.h"

// Types
#import "YRTypes.h"
#import "YRSessionState.h"

/**
 *  Represents session between two peers: local and remote.
 */
@interface YRObjcSession : NSObject

@property (nonatomic, readonly) NSData *peerAddress;
@property (nonatomic, readonly) YRSessionState state;

#pragma mark - Init

- (instancetype)initWithContext:(YRObjcSessionContext *)context;

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
 *  YRObjcSession will call handlers passed on initialization providing real data to be sent/received.
 */
- (void)receive:(NSData *)data;
- (void)send:(NSData *)data;

@end
