//
//  YRPacketsQueueTests.m
//  YRNetworkingDemoTests
//
//  Created by Yuriy Romanchenko on 8/26/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>

#import "YRPacketsQueue.h"

@interface YRPacketsQueueTests : XCTestCase
@end

@implementation YRPacketsQueueTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testQueueCreation {
//    for (YRPayloadLengthType bufferSizeIterator = 0; bufferSizeIterator < (YRPayloadLengthType)(~0); bufferSizeIterator++) {
//        for (uint8_t buffersCountIterator = 0; buffersCountIterator < (uint8_t)(~0); buffersCountIterator++) {
//            YRPacketsQueueRef queue = YRPacketsQueueCreate(bufferSizeIterator, buffersCountIterator);
//
//
//
//            YRPacketsQueueDestroy(queue);
//        }
//    }
}

- (void)testQueueDestroy {
    YRPacketsQueueDestroy(NULL);

    YRPacketsQueueRef queue = YRPacketsQueueCreate(1000, 5);
    
    YRPacketsQueueDestroy(queue);
}

- (void)testQueueBaseWrapAround {
    YRPacketsQueueRef queue = YRPacketsQueueCreate(sizeof(int), 5);
    
    YRSequenceNumberType segmentBase = 150;
    
    YRPacketsQueueSetBaseSegment(queue, segmentBase);
    
    for (YRSequenceNumberType iterator = 0; iterator < 1000; iterator++) {
        XCTAssertTrue(YRPacketsQueueGetBaseSegment(queue) == segmentBase + iterator);

        YRPacketsQueueAdvanceBaseSegment(queue, 1);
    }
    
    YRSequenceNumberType halfOfMaxValue = (YRSequenceNumberType)(~0) / 2;
    
    segmentBase += halfOfMaxValue;
    YRPacketsQueueSetBaseSegment(queue, segmentBase);

    XCTAssertTrue(YRPacketsQueueGetBaseSegment(queue) == segmentBase);

    YRSequenceNumberType expectedSegmentBase = segmentBase;
    for (YRSequenceNumberType iterator = 0; iterator < (YRSequenceNumberType)(~0); iterator++) {
        XCTAssertTrue(YRPacketsQueueGetBaseSegment(queue) == expectedSegmentBase);
    
        YRSequenceNumberType randomQuarterAdvance = (arc4random() % halfOfMaxValue) / 2;
        
        YRPacketsQueueAdvanceBaseSegment(queue, halfOfMaxValue + randomQuarterAdvance);
        
        expectedSegmentBase += halfOfMaxValue + randomQuarterAdvance;
    }
    
    YRPacketsQueueDestroy(queue);
}

- (void)testCommonQueueBuffers {
    uint8_t buffersCount = 10;
    YRPacketsQueueRef queue = YRPacketsQueueCreate(sizeof(int), buffersCount);
    
    YRSequenceNumberType segmentBase = 150;
    
    YRPacketsQueueSetBaseSegment(queue, segmentBase);

    XCTAssertFalse(YRPacketsQueueHasBufferForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsQueueHasBufferForSegment(queue, segmentBase + buffersCount));

    XCTAssertFalse(YRPacketsQueueIsBufferInUseForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsQueueIsBufferInUseForSegment(queue, segmentBase + buffersCount));

    XCTAssertTrue(YRPacketsQueueBufferForSegment(queue, segmentBase - 1) == NULL);
    XCTAssertTrue(YRPacketsQueueBufferForSegment(queue, segmentBase + buffersCount) == NULL);
    
    YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase + buffersCount);

    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == 0);

    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount);

    for (uint8_t bufferIterator = 0; bufferIterator < buffersCount; bufferIterator++) {
        XCTAssertTrue(YRPacketsQueueHasBufferForSegment(queue, segmentBase + bufferIterator));
        
        void *buffer = YRPacketsQueueBufferForSegment(queue, segmentBase + bufferIterator);
        
        YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase + bufferIterator);
        
        *((int *)buffer) = bufferIterator;
    }
    
    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == buffersCount);
    
    // Unmark head and tail
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase);
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount - 1);

    uint8_t buffersInUse = YRPacketsQueueBuffersInUse(queue);
    YRSequenceNumberType seq[buffersInUse];
    memset(seq, 0, sizeof(YRSequenceNumberType) * buffersInUse);
    uint8_t zeroBuffersIn = 0;
    uint8_t tooMuchBuffersIn = buffersInUse + 1;
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, NULL, NULL);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, NULL);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &zeroBuffersIn);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &tooMuchBuffersIn);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &buffersInUse);
    
    XCTAssertTrue(tooMuchBuffersIn == buffersInUse);

    for (uint8_t i = 0; i < buffersInUse; i++) {
        XCTAssertTrue(seq[i] >= segmentBase && seq[i] < segmentBase + buffersCount);
        
        void *buffer = YRPacketsQueueBufferForSegment(queue, seq[i]);
        
        XCTAssertTrue(*(int *)buffer == seq[i] - segmentBase);
    }
    
    YRPacketsQueueAdvanceBaseSegment(queue, 2);
    
    YRPacketsQueueSetBaseSegment(queue, (YRSequenceNumberType)(~0));
    
    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == 0);

    YRPacketsQueueDestroy(queue);
}

- (void)testWrapAroundQueueBuffers {
    uint8_t buffersCount = 255;
    YRPacketsQueueRef queue = YRPacketsQueueCreate(sizeof(int), buffersCount);
    
    YRSequenceNumberType segmentBase = 65500;
    
    YRPacketsQueueSetBaseSegment(queue, segmentBase);
    YRPacketsQueueAdvanceBaseSegment(queue, 20);

    segmentBase += 20;
    
    XCTAssertFalse(YRPacketsQueueHasBufferForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsQueueHasBufferForSegment(queue, segmentBase + buffersCount));
    
    XCTAssertFalse(YRPacketsQueueIsBufferInUseForSegment(queue, segmentBase - 1));
    XCTAssertFalse(YRPacketsQueueIsBufferInUseForSegment(queue, segmentBase + buffersCount));
    
    XCTAssertTrue(YRPacketsQueueBufferForSegment(queue, segmentBase - 1) == NULL);
    XCTAssertTrue(YRPacketsQueueBufferForSegment(queue, segmentBase + buffersCount) == NULL);
    
    YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase + buffersCount);
    
    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == 0);
    
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase - 1);
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount);
    
    for (uint8_t bufferIterator = 0; bufferIterator < buffersCount; bufferIterator++) {
        XCTAssertTrue(YRPacketsQueueHasBufferForSegment(queue, segmentBase + bufferIterator));
        
        void *buffer = YRPacketsQueueBufferForSegment(queue, segmentBase + bufferIterator);
        
        YRPacketsQueueMarkBufferInUseForSegment(queue, segmentBase + bufferIterator);
        
        *((int *)buffer) = bufferIterator;
    }
    
    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == buffersCount);
    
    // Unmark head and tail
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase);
    YRPacketsQueueUnmarkBufferInUseForSegment(queue, segmentBase + buffersCount - 1);
    
    uint8_t buffersInUse = YRPacketsQueueBuffersInUse(queue);
    YRSequenceNumberType seq[buffersInUse];
    memset(seq, 0, sizeof(YRSequenceNumberType) * buffersInUse);
    uint8_t zeroBuffersIn = 0;
    uint8_t tooMuchBuffersIn = buffersInUse + 1;
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, NULL, NULL);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, NULL);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &zeroBuffersIn);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &tooMuchBuffersIn);
    YRPacketsQueueGetSegmentNumbersForBuffersInUse(queue, seq, &buffersInUse);
    
    XCTAssertTrue(tooMuchBuffersIn == buffersInUse);
    
    for (uint8_t i = 0; i < buffersInUse; i++) {
        void *buffer = YRPacketsQueueBufferForSegment(queue, seq[i]);
        
        XCTAssertTrue(*(int *)buffer == (YRSequenceNumberType)(seq[i] - segmentBase));
    }
    
    YRPacketsQueueAdvanceBaseSegment(queue, 200);
    
    YRPacketsQueueSetBaseSegment(queue, (YRSequenceNumberType)(~0));
    
    XCTAssertTrue(YRPacketsQueueBuffersInUse(queue) == 0);
    
    YRPacketsQueueDestroy(queue);
}

@end
