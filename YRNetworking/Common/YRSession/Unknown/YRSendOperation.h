//
//  YRSendOperation.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YRSendOperation : NSObject

@property (nonatomic, readonly) uint32_t sequenceNumber;
@property (nonatomic, readonly) NSData *data;

@property (nonatomic, readonly) uint8_t retransmissionCount;

@property (nonatomic, copy) void (^onTransmissionTimeout) (YRSendOperation *operation);

- (instancetype)initWithData:(NSData *)data sequenceNumber:(uint32_t)sequenceNumber;

- (void)start;
- (void)end;

@end
