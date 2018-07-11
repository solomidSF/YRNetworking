//
//  YRSessionContext.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/9/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

@import Foundation;

@class YRSession;

@interface YRSessionContext : NSObject <NSCopying>

@property (nonatomic) NSData *peerAddress;
@property (nonatomic, copy) void (^sendCallout) (YRSession *session, NSData *data);
@property (nonatomic, copy) void (^receiveCallout) (YRSession *session, NSData *data);

@end
