//
//  YRSessionContext.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/9/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRSessionContext.h"

@implementation YRSessionContext

- (id)copyWithZone:(NSZone *)zone {
    YRSessionContext *cp = [[YRSessionContext allocWithZone:zone] init];
    
    cp.peerAddress = self.peerAddress;
    cp.connectionStateCallout = self.connectionStateCallout;
    cp.sendCallout = self.sendCallout;
    cp.receiveCallout = self.receiveCallout;
    
    return cp;
}
@end
