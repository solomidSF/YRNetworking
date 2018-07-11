//
//  YRPacket.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/25/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRPacket.h"

@implementation YRPacket

- (instancetype)initWithHeader:(YRPacketHeader)header payload:(NSData *)payload {
    if (self = [super init]) {
        _header = header;
        _payload = payload;
    }
    
    return self;
}

@end
