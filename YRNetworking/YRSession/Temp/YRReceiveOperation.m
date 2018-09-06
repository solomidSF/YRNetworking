//
//  YRReceiveOperation.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/2/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRReceiveOperation.h"

@implementation YRReceiveOperation

- (instancetype)initWithPacket:(YRPacketRef)packet {
    if (self = [super init]) {
        _packet = packet;
    }
    
    return self;
}

- (void)dealloc {
    YRPacketDestroy(self.packet);
    
    _packet = NULL;
}

@end
