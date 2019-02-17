//
//  YRDebugUtils.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "YRPacketHeader.h"

@interface YRDebugUtils : NSObject

#pragma mark - YRPacketHeader

+ (NSString *)packetHeaderTypeDescription:(YRPacketHeaderRef)header;
+ (NSString *)packetHeaderShortDescription:(YRPacketHeaderRef)header;
+ (NSString *)packetHeaderFullDescription:(YRPacketHeaderRef)header;

@end
