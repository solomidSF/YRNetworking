//
//  YRConfiguration.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/25/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YRConfiguration : NSObject

@property (nonatomic, readonly) uint8_t maxNumberOfOutstangingSegments;
@property (nonatomic, readonly) uint16_t flags;
// Not used
@property (nonatomic, readonly) uint16_t maximumSegmentSize;
@property (nonatomic, readonly) uint16_t retransmissionTimeoutValue; // ms
@property (nonatomic, readonly) uint16_t cumulativeAckTimeoutValue; // ms
@property (nonatomic, readonly) uint16_t nullSegmentTimeoutValue; // ms
@property (nonatomic, readonly) uint8_t maxRetransmissions;
@property (nonatomic, readonly) uint8_t maxCumulativeAck;
@property (nonatomic, readonly) uint8_t maxOutOfSequence;
@property (nonatomic, readonly) uint32_t connectionIdentifier;

@end
