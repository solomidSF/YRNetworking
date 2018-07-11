//
//  YRPacket.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/25/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

@import Foundation;

#import "YRTypes.h"

@interface YRPacket : NSObject

@property (nonatomic, readonly) YRPacketHeader header;
@property (nonatomic, readonly) NSData *payload;

@end
