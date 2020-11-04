//
// YRTimerInterfaceApple.m
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

#import <Foundation/Foundation.h>
#import "YRTimerInterfaceApple.h"

#pragma mark - Prototypes

YRTimerOrchestrator yrCreateOrchestrator_apple(void);
void yrDestroyOrchestrator_apple(YRTimerOrchestrator o);
YRTimerHandle yrSchedule_apple(
	YRTimerOrchestrator o,
	double timeout,
	void *context,
	void (*callback) (YRTimerOrchestrator o, YRTimerHandle h, void *context, bool *reschedule)
);
void yrCancel_apple(YRTimerOrchestrator o, YRTimerHandle handle);
void yrCancelAll_apple(YRTimerOrchestrator o);

typedef struct YRTimerFreeList {
	struct YRTimerFreeList *next;
	union {
		YRTimerOrchestrator o;
		YRTimerHandle h;
	} this;
} YRTimerFreeList;

// TODO: Rewrite using CFTypes
@interface YRTimerOrchestratorInfo : NSObject

- (YRTimerHandle)createHandle:(NSTimer *)timer;
- (void)cancelHandle:(YRTimerHandle)h;
- (void)cancelAll;

@end

@implementation YRTimerOrchestratorInfo {
	NSMutableDictionary <NSNumber *, NSTimer *> *_cache;
	YRTimerFreeList *_freeList;
	YRTimerHandle _next;
}

- (instancetype)init {
	if (self = [super init]) {
		_cache = [NSMutableDictionary new];
	}
	
	return self;
}

- (void)dealloc {
	[self cancelAll];
}

- (YRTimerHandle)createHandle:(NSTimer *)timer {
	YRTimerHandle res = 0;
	
	if (_freeList) {
		YRTimerFreeList *newHead = _freeList->next;
		res = _freeList->this.h;
		free(_freeList);

		_freeList = newHead;
	} else {
		if (_cache[@(_next)]) {
			NSLog(@"[YRTimerOrchestrator]: Couldn't associate new timer with id.");
			abort();
		}
		
		res = _next;
		_next++;
	}
	
	_cache[@(res)] = timer;
	
	return res;
}

- (void)cancelHandle:(YRTimerHandle)h {
	[self cancelHandle:h recycle:YES];
}

- (void)cancelHandle:(YRTimerHandle)h recycle:(BOOL)recycle {
	if (!_cache[@(h)]) {
		return;
	}
	
	[_cache[@(h)] invalidate];
	_cache[@(h)] = nil;

	if (recycle) {
		YRTimerFreeList *node = malloc(sizeof(YRTimerFreeList));
		if (!node) {
			NSLog(@"[YRTimerOrchestrator]: Couldn't move timer to free list.");
			abort();
		}
		node->this.h = h;
		node->next = _freeList;
		_freeList = node;
	}
}

- (void)cancelAll {
	for (NSNumber *handleN in [_cache copy]) {
		[self cancelHandle:[handleN unsignedIntValue] recycle:NO];
	}
	
	while (_freeList) {
		YRTimerFreeList *next = _freeList->next;
		
		free(_freeList);
		
		_freeList = next;
	}
	
	_next = 0;
}

@end

@interface YRTimerInterfaceApple : NSObject
+ (instancetype)shared;
@end

@implementation YRTimerInterfaceApple {
	NSMutableDictionary <NSNumber *, YRTimerOrchestratorInfo *> *_cache;
	YRTimerFreeList *freeOrchestratorsList;
	YRTimerOrchestrator _nextId;
}

#pragma mark - Singleton

+ (instancetype)shared {
	static YRTimerInterfaceApple *instance = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		if (!instance) {
			instance = [YRTimerInterfaceApple new];
		}
	});
	
	return instance;
}

- (instancetype)init {
	if (self = [super init]) {
		_cache = [NSMutableDictionary new];
	}
	
	return self;
}

- (YRTimerInterface)timerInterface {
	return (YRTimerInterface) {
		.createOrchestrator = &yrCreateOrchestrator_apple,
		.destroyOrchestrator = &yrDestroyOrchestrator_apple,
	
		.schedule = &yrSchedule_apple,
		.cancel = &yrCancel_apple,
		.cancelAll = &yrCancelAll_apple
	};
}

#pragma mark - Interface

- (YRTimerOrchestrator)createOrchestrator {
	YRTimerOrchestrator res = 0;
	
	if (freeOrchestratorsList) {
		YRTimerFreeList *newHead = freeOrchestratorsList->next;
		res = freeOrchestratorsList->this.o;
		free(freeOrchestratorsList);
		freeOrchestratorsList = newHead;
	} else {
		if (_cache[@(_nextId)]) {
			NSLog(@"[YRTimerInterface]: Couldn't create new timer orchestrator.");
			abort();
		}
		res = _nextId;
		_nextId++;
	}

	_cache[@(res)] = [YRTimerOrchestratorInfo new];

	return res;
}

- (void)destroyOrchestrator:(YRTimerOrchestrator)o {
	if (_cache[@(o)]) {
		[self cancelAll:o];

		YRTimerFreeList *node = malloc(sizeof(YRTimerFreeList));
		if (!node) {
			NSLog(@"[YRTimerInterface]: Couldn't move timer orchestrator to free list.");
			abort();
		}
		node->this.o = o;
		node->next = freeOrchestratorsList;
		freeOrchestratorsList = node;
	}

	_cache[@(o)] = nil;
}

- (YRTimerHandle)schedule:(YRTimerOrchestrator)o
	timeout:(double)timeout
	context:(void *)context
	callback:(void (*)(YRTimerOrchestrator, YRTimerHandle, void *, bool *))callback {
	if (!callback) {
		return 0;
	}
	
	if (!_cache[@(o)]) {
		NSLog(@"[YRTimerInterface]: Asked to schedule for non-existing orchestrator.");
		abort();
	}
	
	__block YRTimerHandle handle;
	
	NSTimer *t = [NSTimer timerWithTimeInterval:timeout / 1000 repeats:YES block:^(NSTimer *timer) {
		bool reschedule = false;
		callback(o, handle, context, &reschedule);
		
		if (!reschedule) {
			[self cancel:o handle:handle];
		}
	}];
	
	handle = [_cache[@(o)] createHandle:t];
	[[NSRunLoop mainRunLoop] addTimer:t forMode:NSRunLoopCommonModes];
	 
	return handle;
}

- (void)cancel:(YRTimerOrchestrator)o handle:(YRTimerHandle)handle {
	[_cache[@(o)] cancelHandle:handle];
}

- (void)cancelAll:(YRTimerOrchestrator)o {
	[_cache[@(o)] cancelAll];
	_cache[@(o)] = nil;
}

@end

YRTimerInterface YRTimerInterfaceGet(void) {
	return [[YRTimerInterfaceApple shared] timerInterface];
}

YRTimerOrchestrator yrCreateOrchestrator_apple(void) {
	return [[YRTimerInterfaceApple shared] createOrchestrator];
}

void yrDestroyOrchestrator_apple(YRTimerOrchestrator o) {
	return [[YRTimerInterfaceApple shared] destroyOrchestrator:o];
}

YRTimerHandle yrSchedule_apple(
	YRTimerOrchestrator o,
	double timeout,
	void *context,
	void (*callback) (YRTimerOrchestrator o, YRTimerHandle h, void *context, bool *reschedule)
) {
	return [[YRTimerInterfaceApple shared] schedule:o timeout:timeout context:context callback:callback];
}

void yrCancel_apple(YRTimerOrchestrator o, YRTimerHandle handle) {
	[[YRTimerInterfaceApple shared] cancel:o handle: handle];
}

void yrCancelAll_apple(YRTimerOrchestrator o) {
	[[YRTimerInterfaceApple shared] cancelAll:o];
}
