//
//  YRPacketBufferTests.m
//  YRNetworkingCoreTests
//
//  Created by Yurii Romanchenko on 10/28/20.
//  Copyright © 2020 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "YRNetworking.h"

@interface YRPacketBufferTests : XCTestCase
@end

@implementation YRPacketBufferTests {
	char buffer[UINT16_MAX];
}

- (void)testPacketBuffer {
	YRPacketBufferDebugInfo(buffer);
	
	NSLog(@"%s", buffer);
}

- (void)testPacketBufferCreateWithZeroBuffers {
	YRPacketBufferRef pb = YRPacketBufferCreate(0, 0);
		
	YRPacketBufferDump(pb, buffer);
		
	XCTAssert(YRPacketBufferGetBufferSize(pb) >= YRMaxPacketHeaderSize, @"%s", buffer);
	XCTAssert(YRPacketBufferGetBuffersCount(pb) > 0, @"%s", buffer);
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferCreate {
    for (YRPayloadLengthType bs = 0; bs < sizeof(YRPayloadLengthType) * 8; bs++) {
        for (uint8_t bc = 0; bc < sizeof(uint8_t) * 8; bc++) {
            YRPacketBufferRef pb = YRPacketBufferCreate(pow(2, bs), pow(2, bc));
			
			YRPayloadLengthType l = 0;
			bool inUse = false;
			
			void *buf = YRPacketBufferGetBuffer(pb, pow(2, bc) - 1, &l, &inUse);
			
			YRPacketBufferDump(pb, buffer);
			
			XCTAssert(YRPacketBufferGetBufferSize(pb) >= bs, @"%s", buffer);
			XCTAssert(YRPacketBufferGetBuffersCount(pb) >= bc, @"%s", buffer);
			XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == 0, @"%s", buffer);
			
			XCTAssert(buf != NULL, "%s", buffer);
			XCTAssertEqual(l, 0);
			XCTAssertEqual(inUse, false);
            
			void *wrongBuf = YRPacketBufferGetBuffer(pb, pow(2, bc), &l, &inUse);
			XCTAssert(wrongBuf == NULL, "%s", buffer);
			
			YRPacketBufferDestroy(pb);
        }
    }
}

- (void)testPacketBufferDestroy {
    YRPacketBufferDestroy(NULL);

    YRPacketBufferRef buffer = YRPacketBufferCreate(1024, 5);

	YRPacketBufferDestroy(buffer);
}

- (void)testPacketBufferBase {
	YRPacketBufferRef pb = YRPacketBufferCreate(1024, 5);
	
	YRSequenceNumberType base = 43022;
	YRPacketBufferSetBase(pb, base);
	
	XCTAssertEqual(base, YRPacketBufferGetBase(pb));
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferAdvanceBase {
	uint8_t buffersCount = 5;
	YRPacketBufferRef pb = YRPacketBufferCreate(1024, buffersCount);
	
	// Set base
	YRSequenceNumberType base = 43022;
	YRPacketBufferSetBase(pb, base);
	
	XCTAssertEqual(base, YRPacketBufferGetBase(pb));

	// Advance base within buffers count
	YRSequenceNumberType by = buffersCount - 1;
	YRPacketBufferAdvanceBase(pb, by);
	
	XCTAssertEqual(base + by, YRPacketBufferGetBase(pb));
	
	// Advance base by number exceeding buffers count
	base += by;
	by = buffersCount + 1;
	YRPacketBufferAdvanceBase(pb, by);
	
	XCTAssertEqual(base + by, YRPacketBufferGetBase(pb));
	
	// Advance base leading to wrap around
	base = (YRSequenceNumberType)(~0) - 1;
	by = buffersCount - 1;
	YRPacketBufferSetBase(pb, base);
	
	YRPacketBufferAdvanceBase(pb, by);
	XCTAssertEqual((YRSequenceNumberType)(base + by), YRPacketBufferGetBase(pb));
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferMarkInUse {
	uint8_t buffersCount = 5;
	YRPayloadLengthType bufferSize = 1024;
	YRPacketBufferRef pb = YRPacketBufferCreate(bufferSize, buffersCount);
	
	NSArray *testData = @[
		@[@1, @512],
		@[@3, @256],
		@[@4, @1025],
	];
	
	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
		
		YRPacketBufferMarkBufferInUse(pb, seq, len);
	}

	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
	
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", buffer);
		
		bool inUse = false;
		YRPayloadLengthType actualLen = 0;
		void *buf = YRPacketBufferGetBuffer(pb, seq, &actualLen, &inUse);
		
		for (int i = 0; i < 32; i++) {
			*((int *)buf + i) = arc4random();
		}
		
		XCTAssert(inUse, @"%s", buffer);
		XCTAssert(actualLen == MIN(bufferSize, len), @"%s", buffer);
		XCTAssert(buf != NULL, @"%s", buffer);
	}
		
	YRSequenceNumberType outOfRangeSeq = buffersCount;
	YRPacketBufferMarkBufferInUse(pb, outOfRangeSeq, 32);
	
	bool inUse = false;
	YRPayloadLengthType actualLen = 0;
	void *buf = YRPacketBufferGetBuffer(pb, outOfRangeSeq, &actualLen, &inUse);

	YRPacketBufferDump(pb, buffer);
	XCTAssert(!inUse, @"%s", buffer);
	XCTAssert(actualLen == 0, @"%s", buffer);
	XCTAssert(buf == NULL, @"%s", buffer);
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferSetBaseUnmarksBuffersInUse {
	uint8_t buffersCount = 5;
	YRPayloadLengthType bufferSize = 1024;
	YRPacketBufferRef pb = YRPacketBufferCreate(bufferSize, buffersCount);
	
	NSArray *testData = @[
		@[@1, @512],
		@[@3, @256],
		@[@4, @1025],
	];
	
	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
		
		YRPacketBufferMarkBufferInUse(pb, seq, len);
	}

	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);
		
	YRPacketBufferSetBase(pb, 65333);
	
	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == 0, @"%s", buffer);
		
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferAdvanceBaseUnmarksBuffersInUse {
	uint8_t buffersCount = 5;
	YRPayloadLengthType bufferSize = 1024;
	YRPacketBufferRef pb = YRPacketBufferCreate(bufferSize, buffersCount);
	
	NSMutableArray *testData = [@[
		@[@1, @512],
		@[@2, @512],
		@[@3, @256],
		@[@4, @1025],
	] mutableCopy];
	
	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
		
		YRPacketBufferMarkBufferInUse(pb, seq, len);
	}

	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);

	YRPacketBufferAdvanceBase(pb, 1);
	
	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", buffer);
	}

	// Advance once more
	[testData removeObjectAtIndex:0];
	YRPacketBufferAdvanceBase(pb, 1);
	
	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", buffer);
	}

	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferUnmarkInUse {
	uint8_t buffersCount = 5;
	YRPayloadLengthType bufferSize = 1024;
	YRPacketBufferRef pb = YRPacketBufferCreate(bufferSize, buffersCount);
	
	NSArray *testData = @[
		@[@1, @512],
		@[@3, @256],
		@[@4, @1025],
	];
	
	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
		
		YRPacketBufferMarkBufferInUse(pb, seq, len);
	}

	YRPacketBufferUnmarkBufferInUse(pb, buffersCount);

	YRSequenceNumberType inUse = YRPacketBufferGetBuffersInUseNum(pb);
	YRPacketBufferDump(pb, buffer);
	XCTAssert(inUse == testData.count, @"%s", buffer);
	
	for (NSArray *meta in [testData reverseObjectEnumerator]) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];

		YRPacketBufferUnmarkBufferInUse(pb, seq);
		inUse -= 1;
		
		YRPacketBufferDump(pb, buffer);
		XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == inUse, @"%s", buffer);
	}
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferAdvanceAndMarkBuffer {
	uint8_t buffersCount = 5;
	YRPayloadLengthType bufferSize = 1024;
	YRPacketBufferRef pb = YRPacketBufferCreate(bufferSize, buffersCount);
	
	YRPacketBufferAdvanceBase(pb, 4);
	
	NSMutableArray *testData = [@[
		@[@4, @512],
		@[@5, @512],
		@[@6, @256],
		@[@7, @1025],
	] mutableCopy];
	
	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
		
		YRPacketBufferMarkBufferInUse(pb, seq, len);
	}

	YRPacketBufferDump(pb, buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
	
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", buffer);
		
		bool inUse = false;
		YRPayloadLengthType actualLen = 0;
		void *buf = YRPacketBufferGetBuffer(pb, seq, &actualLen, &inUse);
		
		XCTAssert(inUse, @"%s", buffer);
		XCTAssert(actualLen == MIN(bufferSize, len), @"%s", buffer);
		XCTAssert(buf != NULL, @"%s", buffer);
	}
	
	YRPacketBufferDestroy(pb);
}

@end
