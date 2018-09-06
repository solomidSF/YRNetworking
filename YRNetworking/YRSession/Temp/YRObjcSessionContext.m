//
//  YRObjcSessionContext.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/9/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRObjcSessionContext.h"

@implementation YRObjcSessionContext

- (id)copyWithZone:(NSZone *)zone {
    YRObjcSessionContext *cp = [[YRObjcSessionContext allocWithZone:zone] init];
    
    cp.peerAddress = self.peerAddress;
    cp.connectionStateCallout = self.connectionStateCallout;
    cp.sendCallout = self.sendCallout;
    cp.receiveCallout = self.receiveCallout;
    
    return cp;
}
@end
