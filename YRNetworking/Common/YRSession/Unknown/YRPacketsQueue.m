//
//  YRPacketsQueue.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/23/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRPacketsQueue.h"

@implementation YRPacketsQueue {
    uint32_t *_packetsArray;
    uint32_t _maximumSize;
}

#pragma mark - Initializers

+ (instancetype)queueWithSize:(uint32_t)size {
    return [[self alloc] initWithSize:size];
}

#pragma mark - Lifecycle

- (instancetype)initWithSize:(uint32_t)size {
    if (self = [super init]) {
        _maximumSize = size;
        _packetsArray = calloc(1, size * sizeof(uint32_t));
    }
    
    return self;
}

- (void)dealloc {
    if (_packetsArray) {
        free(_packetsArray);
        _packetsArray = NULL;
    }
}

- (void)insertPacket:(id)packet {
    
}

- (void)removePacket:(id)packet {
    
}

- (id)packetWithSequenceNumber:(uint32_t)number {
    if (number < _maximumSize) {
//        id packet = (id)_packetsArray[number];
//        return packet;
        return nil;
    } else {
        return nil;
    }
}

@end
