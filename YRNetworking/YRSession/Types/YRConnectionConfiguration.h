//
//  YRConnectionConfiguration.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/13/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRConnectionConfiguration__
#define __YRConnectionConfiguration__

#include <stdio.h>

typedef struct {
    uint16_t options; // zero for now
    uint16_t retransmissionTimeoutValue; // ms
    uint16_t nullSegmentTimeoutValue; // ms
    uint16_t maximumSegmentSize;
    uint8_t maxNumberOfOutstandingSegments;
    uint8_t maxRetransmissions;
    //    uint8_t maxCumulativeAck;
    //    uint16_t cumulativeAckTimeoutValue; // ms
} YRConnectionConfiguration;

#endif
