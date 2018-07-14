//
//  YRSendOperation.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRSendOperation.h"

@implementation YRSendOperation {
    NSTimer *_transmissionTimer;
}

- (instancetype)initWithData:(NSData *)data sequenceNumber:(uint32_t)sequenceNumber {
    if (self = [super init]) {
        _data = data;
        _sequenceNumber = sequenceNumber;
    }
    
    return self;
}

- (void)start {
    __typeof(self) __weak weakSelf = self;
    
    _transmissionTimer = [NSTimer timerWithTimeInterval:2 repeats:NO block:^(NSTimer * _Nonnull timer) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (strongSelf) {
            !strongSelf.onTransmissionTimeout ?: strongSelf.onTransmissionTimeout(strongSelf);
        }
    }];
    
    [[NSRunLoop currentRunLoop] addTimer:_transmissionTimer forMode:NSRunLoopCommonModes];
}

- (void)handleTransmissionTimeout:(NSTimer *)timer {
    !self.onTransmissionTimeout ?: self.onTransmissionTimeout(self);
}

- (void)end {
    [_transmissionTimer invalidate];
}

@end
