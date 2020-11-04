//
// YRRuntimeEnvironmentTests.m
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
#include "YRNetworking.h"

@class YRRuntimeEnvironmentTests;

typedef struct {
	YRRuntimeEnvironmentTests *instance;
	void *context;
} YRTimerTestContext;

@interface YRRuntimeEnvironmentTests : XCTestCase
@end

@implementation YRRuntimeEnvironmentTests {
	YRTimerInterface _timerInterface;
	
	void (^_timerHandle) (YRTimerOrchestrator o, YRTimerHandle h, void *ctx, bool *reschedule);
}

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    _timerInterface = YRRuntimeEnvironmentGetTimerInterface();
    
    _timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *ctx, bool *reschedule) {};
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)testTimerOrchestratorLifecycle {
	YRTimerOrchestrator o1 = _timerInterface.createOrchestrator();
	_timerInterface.destroyOrchestrator(o1);
	_timerInterface.destroyOrchestrator(o1);
	YRTimerOrchestrator o2 = _timerInterface.createOrchestrator();
	_timerInterface.destroyOrchestrator(o2);
}

- (void)testTimerSchedule {
	YRTimerOrchestrator o1 = _timerInterface.createOrchestrator();

	int context = 42;
	YRTimerTestContext ctx = {
		self,
		&context
	};
	
	double timeout = 100;
	YRTimerHandle handle = _timerInterface.schedule(o1, timeout, &ctx, &callout);
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		*reschedule = true;
		
		XCTAssert(*(int *)actualCtx == context);
		XCTAssert(h == handle);
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.31]];
	
	_timerInterface.cancel(o1, handle);
}

- (void)testTimerScheduleWithoutCallback {
	YRTimerOrchestrator o1 = _timerInterface.createOrchestrator();
	
	YRTimerTestContext ctx = {
		self,
		NULL
	};
	
	double timeout = 100;
	YRTimerHandle handle = _timerInterface.schedule(o1, timeout, &ctx, NULL);
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		XCTFail(@"Shouldn't be called");
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.31]];
	
	_timerInterface.cancel(o1, handle);
	_timerInterface.destroyOrchestrator(o1);
}

- (void)testTimerCancel {
	YRTimerOrchestrator o1 = _timerInterface.createOrchestrator();
	
	int context = 22;
	YRTimerTestContext ctx = {
		self,
		&context
	};
	
	__block int retries = 4;
	double timeout = 50;
	YRTimerHandle handle1 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	__weak typeof(self) weakSelf = self;
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		retries -= 1;
		
		*reschedule = retries > 0;
		
		__strong typeof(self) strongSelf = weakSelf;
		
		if (strongSelf && retries <= 0) {
			strongSelf->_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *ctx, bool *r) {
				XCTFail(@"Shouldn't be called");
			};
		}
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.31]];
	
	YRTimerHandle handle2 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		XCTAssert(h == handle2);
		XCTAssert(context == *(int *)actualCtx);
		*reschedule = true;
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.31]];

	_timerInterface.cancel(o1, handle1);
	_timerInterface.cancel(o1, handle2);
}

- (void)testTimerCancelAll {
	YRTimerOrchestrator o1 = _timerInterface.createOrchestrator();
	YRTimerOrchestrator o2 = _timerInterface.createOrchestrator();
	
	int context = 22;
	YRTimerTestContext ctx = {
		self,
		&context
	};
	
	double timeout = 50;

	YRTimerHandle o1h1 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	YRTimerHandle o1h2 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	YRTimerHandle o1h3 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	YRTimerHandle o1h4 = _timerInterface.schedule(o1, timeout, &ctx, callout);
	YRTimerHandle o2h1 = _timerInterface.schedule(o2, timeout, &ctx, callout);
	YRTimerHandle o2h2 = _timerInterface.schedule(o2, timeout, &ctx, callout);
	YRTimerHandle o2h3 = _timerInterface.schedule(o2, timeout, &ctx, callout);

	__block int o1h1c = 0;
	__block int o1h2c = 0;
	__block int o1h3c = 0;
	__block int o1h4c = 0;
	__block int o2h1c = 0;
	__block int o2h2c = 0;
	__block int o2h3c = 0;
	
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		*reschedule = true;
		
		if (o == o1) {
			if (h == o1h1) {
				o1h1c += 1;
			} else if (h == o1h2) {
				o1h2c += 1;
			} else if (h == o1h3) {
				o1h3c += 1;
			} else if (h == o1h4) {
				o1h4c += 1;
			}
		}
		
		if (o == o2) {
			if (h == o2h1) {
				o2h1c += 1;
			} else if (h == o2h2) {
				o2h2c += 1;
			} else if (h == o2h3) {
				o2h3c += 1;
			}
		}
	};

	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];

	XCTAssert(o1h1c > 0);
	XCTAssert(o1h2c > 0);
	XCTAssert(o1h3c > 0);
	XCTAssert(o1h4c > 0);
	XCTAssert(o2h1c > 0);
	XCTAssert(o2h2c > 0);
	XCTAssert(o2h3c > 0);
	
	_timerInterface.cancel(o1, o1h2);
	_timerInterface.cancel(o1, o1h3);
	_timerInterface.cancelAll(o1);
	
	o2h1c = 0;
	o2h2c = 0;
	o2h3c = 0;

	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		*reschedule = true;
		
		XCTAssert(o != o1);
		
		if (h == o2h1) {
			o2h1c += 1;
		} else if (h == o2h2) {
			o2h2c += 1;
		} else if (h == o2h3) {
			o2h3c += 1;
		}
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
	
	XCTAssert(o2h1c > 0);
	XCTAssert(o2h2c > 0);
	XCTAssert(o2h3c > 0);

	_timerInterface.cancelAll(o2);
	
	_timerHandle = ^(YRTimerOrchestrator o, YRTimerHandle h, void *actualCtx, bool *reschedule) {
		*reschedule = true;
		
		XCTAssert(o != o1);
		XCTAssert(o != o2);
		XCTFail(@"Shouldn't be called");
	};
	
	[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
}

#pragma mark - Private

void callout(YRTimerOrchestrator o, YRTimerHandle h, void *context, bool *reschedule) {
	YRTimerTestContext *ctx = context;
	ctx->instance->_timerHandle(o, h, ctx->context, reschedule);
}

@end
