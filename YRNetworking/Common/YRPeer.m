//
//  YRPeer.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/24/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRPeer.h"

@implementation YRPeer

- (instancetype)initWithSYNPacket:(YRSYNPacketHeader)header {
    if (self = [super init]) {
        _maxNumberOfOutstangingSegments = header.maxNumberOfOutstangingSegments;
        _flags = header.flags;
        _maximumSegmentSize = header.maximumSegmentSize;
        _retransmissionTimeoutValue = header.retransmissionTimeoutValue;
        _cumulativeAckTimeoutValue = header.cumulativeAckTimeoutValue;
        _nullSegmentTimeoutValue = header.nullSegmentTimeoutValue;
        _maxRetransmissions = header.maxRetransmissions;
        _maxCumulativeAck = header.maxCumulativeAck;
        _maxOutOfSequence = header.maxOutOfSequence;
        _connectionIdentifier = header.connectionIdentifier;
    }
    
    return self;
}

@end
