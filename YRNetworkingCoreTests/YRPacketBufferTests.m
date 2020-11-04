//
// YRPacketBufferTests.m
//
// The MIT License (MIT)
//
// Copyright (c) 2020 Yurii Romanchenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#import <XCTest/XCTest.h>
#import "YRNetworking.h"

@interface YRPacketBufferTests : XCTestCase
@end

@implementation YRPacketBufferTests {
	char _buffer[UINT16_MAX];
}

- (void)testPacketBuffer {
	YRPacketBufferDebugInfo(_buffer);
	
	NSLog(@"%s", _buffer);
}

- (void)testPacketBufferCreateWithZeroBuffers {
	YRPacketBufferRef pb = YRPacketBufferCreate(0, 0);
		
	YRPacketBufferDump(pb, _buffer);
		
	XCTAssert(YRPacketBufferGetBufferSize(pb) >= YRMaxPacketHeaderSize, @"%s", _buffer);
	XCTAssert(YRPacketBufferGetBuffersCount(pb) > 0, @"%s", _buffer);
	
	YRPacketBufferDestroy(pb);
}

- (void)testPacketBufferCreate {
    for (YRPayloadLengthType bs = 0; bs < sizeof(YRPayloadLengthType) * 8; bs++) {
        for (uint8_t bc = 0; bc < sizeof(uint8_t) * 8; bc++) {
            YRPacketBufferRef pb = YRPacketBufferCreate(pow(2, bs), pow(2, bc));
			
			YRPayloadLengthType l = 0;
			bool inUse = false;
			
			void *buf = YRPacketBufferGetBuffer(pb, pow(2, bc) - 1, &l, &inUse);
			
			YRPacketBufferDump(pb, _buffer);
			
			XCTAssert(YRPacketBufferGetBufferSize(pb) >= bs, @"%s", _buffer);
			XCTAssert(YRPacketBufferGetBuffersCount(pb) >= bc, @"%s", _buffer);
			XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == 0, @"%s", _buffer);
			
			XCTAssert(buf != NULL, "%s", _buffer);
			XCTAssertEqual(l, 0);
			XCTAssertEqual(inUse, false);
            
			void *wrongBuf = YRPacketBufferGetBuffer(pb, pow(2, bc), &l, &inUse);
			XCTAssert(wrongBuf == NULL, "%s", _buffer);
			
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

	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
	
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", _buffer);
		
		bool inUse = false;
		YRPayloadLengthType actualLen = 0;
		void *buf = YRPacketBufferGetBuffer(pb, seq, &actualLen, &inUse);
		
		for (int i = 0; i < 32; i++) {
			*((int *)buf + i) = arc4random();
		}
		
		XCTAssert(inUse, @"%s", _buffer);
		XCTAssert(actualLen == MIN(bufferSize, len), @"%s", _buffer);
		XCTAssert(buf != NULL, @"%s", _buffer);
	}
		
	YRSequenceNumberType outOfRangeSeq = buffersCount;
	YRPacketBufferMarkBufferInUse(pb, outOfRangeSeq, 32);
	
	bool inUse = false;
	YRPayloadLengthType actualLen = 0;
	void *buf = YRPacketBufferGetBuffer(pb, outOfRangeSeq, &actualLen, &inUse);

	YRPacketBufferDump(pb, _buffer);
	XCTAssert(!inUse, @"%s", _buffer);
	XCTAssert(actualLen == 0, @"%s", _buffer);
	XCTAssert(buf == NULL, @"%s", _buffer);
	
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

	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);
		
	YRPacketBufferSetBase(pb, 65333);
	
	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == 0, @"%s", _buffer);
		
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

	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);

	YRPacketBufferAdvanceBase(pb, 1);
	
	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", _buffer);
	}

	// Advance once more
	[testData removeObjectAtIndex:0];
	YRPacketBufferAdvanceBase(pb, 1);
	
	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", _buffer);
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
	YRPacketBufferDump(pb, _buffer);
	XCTAssert(inUse == testData.count, @"%s", _buffer);
	
	for (NSArray *meta in [testData reverseObjectEnumerator]) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];

		YRPacketBufferUnmarkBufferInUse(pb, seq);
		inUse -= 1;
		
		YRPacketBufferDump(pb, _buffer);
		XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == inUse, @"%s", _buffer);
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

	YRPacketBufferDump(pb, _buffer);
	XCTAssert(YRPacketBufferGetBuffersInUseNum(pb) == testData.count, @"%s", _buffer);

	for (NSArray *meta in testData) {
		YRSequenceNumberType seq = [meta[0] unsignedIntValue];
		YRPayloadLengthType len = [meta[1] unsignedIntValue];
	
		XCTAssert(YRPacketBufferIsBufferInUse(pb, seq), @"%s", _buffer);
		
		bool inUse = false;
		YRPayloadLengthType actualLen = 0;
		void *buf = YRPacketBufferGetBuffer(pb, seq, &actualLen, &inUse);
		
		XCTAssert(inUse, @"%s", _buffer);
		XCTAssert(actualLen == MIN(bufferSize, len), @"%s", _buffer);
		XCTAssert(buf != NULL, @"%s", _buffer);
	}
	
	YRPacketBufferDestroy(pb);
}

@end
