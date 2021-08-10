//
//  YRTransmissionQueueTests.m
//  YRNetworkingCoreTests
//
//  Created by Yurii Romanchenko on 11/18/20.
//  Copyright © 2020 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "YRNetworking.h"

@class YRTransmissionQueueTests;

typedef struct {
	YRTransmissionQueueTests *tests;
	void *context;
} YRTQContext;

@interface YRTransmissionQueueTests : XCTestCase
@end

@implementation YRTransmissionQueueTests {
	char _buffer[UINT16_MAX];
	
	YRTimerInterface _timerInterface;
	void (^_timerHandle) (
		YRTransmissionQueueRef tq,
		YRTransmissionQueueEntry qe,
		YRSequenceNumberType seq,
		void *ctx,
		bool *reschedule
	);
}

- (void)setUp {
	[super setUp];
	
	_timerInterface = YRRuntimeEnvironmentGetTimerInterface();
	_timerHandle = ^(
		YRTransmissionQueueRef tq,
		YRTransmissionQueueEntry qe,
		YRSequenceNumberType seq,
		void *ctx,
		bool *reschedule
	) {};
}

- (void)testTransmissionQueue {
	YRTransmissionQueueDebugInfo(_buffer);
	
	NSLog(@"%s", _buffer);
}

- (void)testTransmissionQueueCreateWithZeroEntries {
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(NULL, NULL, 0, 0);
	
	YRTransmissionQueueDump(tq, _buffer);
		
	XCTAssert(YRTransmissionQueueGetEntriesCount(tq) > 0, @"%s", _buffer);
	
	YRTransmissionQueueDestroy(tq);
}

- (void)testTransmissionQueueDestroy {
	YRTransmissionQueueDestroy(NULL);
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(NULL, NULL, 0, 1);

	YRTransmissionQueueDestroy(tq);
}

- (void)testTransmissionQueueBase {
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(NULL, NULL, 0, 1);
	
	YRSequenceNumberType base = 43022;
	YRTransmissionQueueSetBase(tq, base);
	
	XCTAssertEqual(base, YRTransmissionQueueGetBase(tq));
	
	YRTransmissionQueueDestroy(tq);
}

- (void)testTransmissionQueueAdvanceBase {
	uint8_t buffersCount = 5;
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(NULL, NULL, 0, 1);
	
	// Set base
	YRSequenceNumberType base = 43022;
	YRTransmissionQueueSetBase(tq, base);
	
	XCTAssertEqual(base, YRTransmissionQueueGetBase(tq));

	// Advance base within buffers count
	YRSequenceNumberType by = buffersCount - 1;
	YRTransmissionQueueAdvanceBase(tq, by);
	
	XCTAssertEqual(base + by, YRTransmissionQueueGetBase(tq));
	
	// Advance base by number exceeding buffers count
	base += by;
	by = buffersCount + 1;
	YRTransmissionQueueAdvanceBase(tq, by);
	
	XCTAssertEqual(base + by, YRTransmissionQueueGetBase(tq));
	
	// Advance base leading to wrap around
	base = (YRSequenceNumberType)(~0) - 1;
	by = buffersCount - 1;
	YRTransmissionQueueSetBase(tq, base);
	
	YRTransmissionQueueAdvanceBase(tq, by);
	XCTAssertEqual((YRSequenceNumberType)(base + by), YRTransmissionQueueGetBase(tq));
	
	YRTransmissionQueueDestroy(tq);
}

- (void)testTransmissionQueueAddEntry {
	double timeout = 50;
	uint8_t buffersCount = 5;
	int v = 42;
	YRTimerOrchestrator o = _timerInterface.createOrchestrator();
	YRTQContext ctx = {
		self,
		&v
	};
	
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(
		&ctx,
		tq_callout,
		o,
		buffersCount
	);

	YRSequenceNumberType base = 65533;
	YRTransmissionQueueSetBase(tq, base);

	__block BOOL didReceiveTimerCallout = NO;
	
	_timerHandle = ^(
		 YRTransmissionQueueRef tq,
		 YRTransmissionQueueEntry qe,
		 YRSequenceNumberType seq,
		 void *ctx,
		 bool *reschedule
	) {
		didReceiveTimerCallout = YES;
		XCTAssertTrue((YRSequenceNumberType)(seq - base) >= 0 && (YRSequenceNumberType)(seq - base) < buffersCount);
		*reschedule = true;
	};
	
	for (int i = 0; i < buffersCount; i++) {
		XCTAssertTrue(YRTransmissionQueueAddEntry(tq, base + i, timeout));
	}
	
	XCTAssertFalse(YRTransmissionQueueAddEntry(tq, base + buffersCount, timeout));
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
	XCTAssertTrue(didReceiveTimerCallout);
	
	YRTransmissionQueueDestroy(tq);
	
	_timerHandle = ^(
		 YRTransmissionQueueRef tq,
		 YRTransmissionQueueEntry qe,
		 YRSequenceNumberType seq,
		 void *ctx,
		 bool *reschedule
	) {
		XCTFail(@"Shouldn't be called");
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
}

- (void)testTransmissionQueueRemoveEntry {
	double timeout = 50;
	uint8_t buffersCount = 1;
	int v = 42;
	YRTimerOrchestrator o = _timerInterface.createOrchestrator();
	YRTQContext ctx = {
		self,
		&v
	};
	
	YRTransmissionQueueRef tq = YRTransmissionQueueCreate(
		&ctx,
		tq_callout,
		o,
		buffersCount
	);

	YRSequenceNumberType base = 65535;
	YRTransmissionQueueSetBase(tq, base);

	__block BOOL didReceiveTimerCallout = NO;
	
	_timerHandle = ^(
		 YRTransmissionQueueRef tq,
		 YRTransmissionQueueEntry qe,
		 YRSequenceNumberType seq,
		 void *ctx,
		 bool *reschedule
	) {
		didReceiveTimerCallout = YES;
		XCTAssertTrue((YRSequenceNumberType)(seq - base) >= 0 && (YRSequenceNumberType)(seq - base) < buffersCount);
		*reschedule = true;
	};
	
	for (int i = 0; i < buffersCount; i++) {
		XCTAssertTrue(YRTransmissionQueueAddEntry(tq, base + i, timeout));
	}
	
	XCTAssertFalse(YRTransmissionQueueAddEntry(tq, base + buffersCount, timeout));
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
	XCTAssertTrue(didReceiveTimerCallout);
	
	YRTransmissionQueueRemoveEntry(tq, base);
	
	_timerHandle = ^(
		 YRTransmissionQueueRef tq,
		 YRTransmissionQueueEntry qe,
		 YRSequenceNumberType seq,
		 void *ctx,
		 bool *reschedule
	) {
		XCTFail(@"Shouldn't be called");
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
	
	YRTransmissionQueueDestroy(tq);
}

#pragma mark - Private

void tq_callout(
	YRTransmissionQueueRef tq,
	YRTransmissionQueueEntry qe,
	YRSequenceNumberType seq,
	void *context,
	bool *reschedule
) {
	YRTQContext *ctx = context;
	ctx->tests->_timerHandle(tq, qe, seq, ctx->context, reschedule);
}

@end
