//
//  YRReceiveOperation.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/2/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "YRPacket.h"

@interface YRReceiveOperation : NSObject

@property (nonatomic, readonly) YRPacketRef packet;

- (instancetype)initWithPacket:(YRPacketRef)packet;

@end
