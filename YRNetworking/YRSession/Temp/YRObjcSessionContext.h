//
//  YRObjcSessionContext.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/9/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

@import Foundation;

#import "YRSessionState.h"

@class YRObjcSession;

@interface YRObjcSessionContext : NSObject <NSCopying>

@property (nonatomic) NSData *peerAddress;
@property (nonatomic, copy) void (^connectionStateCallout) (YRObjcSession *session, YRSessionState newState);
@property (nonatomic, copy) void (^sendCallout) (YRObjcSession *session, NSData *data);
@property (nonatomic, copy) void (^receiveCallout) (YRObjcSession *session, NSData *data);

@end
