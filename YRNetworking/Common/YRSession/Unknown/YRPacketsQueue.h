//
//  YRPacketsQueue.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/23/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YRPacketsQueue : NSObject

+ (instancetype)queueWithSize:(uint32_t)size;

- (void)insertPacket:(id)packet;
- (void)removePacket:(id)packet;
- (id)packetWithSequenceNumber:(uint32_t)number;

@end
