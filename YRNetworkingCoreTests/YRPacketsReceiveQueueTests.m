//
//  YRPacketsReceiveQueueTests.m
//  YRNetworkingDemoTests
//
//  Created by Yuriy Romanchenko on 8/26/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>

#import "YRPacketsReceiveQueue.h"

@interface YRPacketsReceiveQueueTests : XCTestCase
@end

@implementation YRPacketsReceiveQueueTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testQueueCreation {
//    for (YRPayloadLengthType bufferSizeIterator = 0; bufferSizeIterator < (YRPayloadLengthType)(~0); bufferSizeIterator++) {
//        for (uint8_t buffersCountIterator = 0; buffersCountIterator < (uint8_t)(~0); buffersCountIterator++) {
//            YRPacketsReceiveQueueRef queue = YRPacketsReceiveQueueCreate(bufferSizeIterator, buffersCountIterator);
//
//
//
//            YRPacketsReceiveQueueDestroy(queue);
//        }
//    }
}

- (void)testQueueDestroy {
    YRPacketsReceiveQueueDestroy(NULL);

    YRPacketsReceiveQueueRef queue = YRPacketsReceiveQueueCreate(1000, 5);
    
    YRPacketsReceiveQueueDestroy(queue);
}

- (void)testQueueBaseWrapAround {
    YRPacketsReceiveQueueRef queue = YRPacketsReceiveQueueCreate(sizeof(int), 5);
    
    YRSequenceNumberType segmentBase = 150;
    
    YRPacketsReceiveQueueSetBaseSegment(queue, segmentBase);
    
    for (YRSequenceNumberType iterator = 0; iterator < 1000; iterator++) {
        XCTAssertTrue(YRPacketsReceiveQueueGetBaseSegment(queue) == segmentBase + iterator);

        YRPacketsReceiveQueueAdvanceBaseSegment(queue, 1);
    }
    
    YRSequenceNumberType halfOfMaxValue = (YRSequenceNumberType)(~0) / 2;
    
    segmentBase += halfOfMaxValue;
    YRPacketsReceiveQueueSetBaseSegment(queue, segmentBase);

    XCTAssertTrue(YRPacketsReceiveQueueGetBaseSegment(queue) == segmentBase);

    YRSequenceNumberType expectedSegmentBase = segmentBase;
    for (YRSequenceNumberType iterator = 0; iterator < (YRSequenceNumberType)(~0); iterator++) {
        XCTAssertTrue(YRPacketsReceiveQueueGetBaseSegment(queue) == expectedSegmentBase);
    
        YRSequenceNumberType randomQuarterAdvance = (arc4random() % halfOfMaxValue) / 2;
        
        YRPacketsReceiveQueueAdvanceBaseSegment(queue, halfOfMaxValue + randomQuarterAdvance);
        
        expectedSegmentBase += halfOfMaxValue + randomQuarterAdvance;
    }
    
    YRPacketsReceiveQueueDestroy(queue);
}

- (void)testCommonQueueBuffers {
    uint8_t buffersCount = 10;
    YRPacketsReceiveQueueRef queue = YRPacketsReceiveQueueCreate(sizeof(int), buffersCount);
    
    YRSequenceNumberType segmentBase = 150;
    
    YRPacketsReceiveQueueSetBaseSegment(queue, segmentBase);

    XCTAssertFalse(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase + buffersCount));

    XCTAssertFalse(YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segmentBase + buffersCount));

    XCTAssertTrue(YRPacketsReceiveQueueBufferForSegment(queue, segmentBase - 1) == NULL);
    XCTAssertTrue(YRPacketsReceiveQueueBufferForSegment(queue, segmentBase + buffersCount) == NULL);
    
    YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase + buffersCount);

    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == 0);

    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount);

    for (uint8_t bufferIterator = 0; bufferIterator < buffersCount; bufferIterator++) {
        XCTAssertTrue(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase + bufferIterator));
        
        void *buffer = YRPacketsReceiveQueueBufferForSegment(queue, segmentBase + bufferIterator);
        
        YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase + bufferIterator);
        
        *((int *)buffer) = bufferIterator;
    }
    
    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == buffersCount);
    
    // Unmark head and tail
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase);
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount - 1);

    uint8_t buffersInUse = YRPacketsReceiveQueueBuffersInUse(queue);
    YRSequenceNumberType seq[buffersInUse];
    memset(seq, 0, sizeof(YRSequenceNumberType) * buffersInUse);
    uint8_t zeroBuffersIn = 0;
    uint8_t tooMuchBuffersIn = buffersInUse + 1;
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, NULL, NULL);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, NULL);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &zeroBuffersIn);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &tooMuchBuffersIn);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &buffersInUse);
    
    XCTAssertTrue(tooMuchBuffersIn == buffersInUse);

    for (uint8_t i = 0; i < buffersInUse; i++) {
        XCTAssertTrue(seq[i] >= segmentBase && seq[i] < segmentBase + buffersCount);
        
        void *buffer = YRPacketsReceiveQueueBufferForSegment(queue, seq[i]);
        
        XCTAssertTrue(*(int *)buffer == seq[i] - segmentBase);
    }
    
    YRPacketsReceiveQueueAdvanceBaseSegment(queue, 2);
    
    YRPacketsReceiveQueueSetBaseSegment(queue, (YRSequenceNumberType)(~0));
    
    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == 0);

    YRPacketsReceiveQueueDestroy(queue);
}

- (void)testWrapAroundQueueBuffers {
    uint8_t buffersCount = 255;
    YRPacketsReceiveQueueRef queue = YRPacketsReceiveQueueCreate(sizeof(int), buffersCount);
    
    YRSequenceNumberType segmentBase = 65500;
    
    YRPacketsReceiveQueueSetBaseSegment(queue, segmentBase);
    YRPacketsReceiveQueueAdvanceBaseSegment(queue, 20);

    segmentBase += 20;
    
    XCTAssertFalse(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase + buffersCount));
    
    XCTAssertFalse(YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segmentBase + buffersCount));
    
    XCTAssertTrue(YRPacketsReceiveQueueBufferForSegment(queue, segmentBase - 1) == NULL);
    XCTAssertTrue(YRPacketsReceiveQueueBufferForSegment(queue, segmentBase + buffersCount) == NULL);
    
    YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase + buffersCount);
    
    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == 0);
    
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount);
    
    for (uint8_t bufferIterator = 0; bufferIterator < buffersCount; bufferIterator++) {
        XCTAssertTrue(YRPacketsReceiveQueueHasBufferForSegment(queue, segmentBase + bufferIterator));
        
        void *buffer = YRPacketsReceiveQueueBufferForSegment(queue, segmentBase + bufferIterator);
        
        YRPacketsReceiveQueueMarkBufferInUseForSegment(queue, segmentBase + bufferIterator);
        
        *((int *)buffer) = bufferIterator;
    }
    
    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == buffersCount);
    
    // Unmark head and tail
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase);
    YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount - 1);
    
    uint8_t buffersInUse = YRPacketsReceiveQueueBuffersInUse(queue);
    YRSequenceNumberType seq[buffersInUse];
    memset(seq, 0, sizeof(YRSequenceNumberType) * buffersInUse);
    uint8_t zeroBuffersIn = 0;
    uint8_t tooMuchBuffersIn = buffersInUse + 1;
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, NULL, NULL);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, NULL);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &zeroBuffersIn);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &tooMuchBuffersIn);
    YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(queue, seq, &buffersInUse);
    
    XCTAssertTrue(tooMuchBuffersIn == buffersInUse);
    
    for (uint8_t i = 0; i < buffersInUse; i++) {
        void *buffer = YRPacketsReceiveQueueBufferForSegment(queue, seq[i]);
        
        XCTAssertTrue(*(int *)buffer == (YRSequenceNumberType)(seq[i] - segmentBase));
    }
    
    YRPacketsReceiveQueueAdvanceBaseSegment(queue, 2);
    
    YRPacketsReceiveQueueSetBaseSegment(queue, (YRSequenceNumberType)(~0));
    
    XCTAssertTrue(YRPacketsReceiveQueueBuffersInUse(queue) == 0);
    
    YRPacketsReceiveQueueDestroy(queue);

}

@end
