//
// YRTransmissionQueue.h
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

#ifndef __YRTransmissionQueue__
#define __YRTransmissionQueue__

/**
 *  Simple data structure to encapsulate work with timers related to packet ttl.
 */
typedef struct YRTransmissionQueue *YRTransmissionQueueRef;

typedef struct {
	time_t startTime;
	YRTimerHandle h;
	uint8_t transmissionsSoFar;
	bool inUse;
} YRTransmissionQueueEntry;

#pragma mark - Lifecycle

YRTransmissionQueueRef YRTransmissionQueueCreate(
	void *context,
	void (*transmitHandler) (
		YRTransmissionQueueRef tq,
		YRTransmissionQueueEntry qe,
		YRSequenceNumberType seq,
		void *context,
		bool *reschedule
	),
	YRTimerOrchestrator o,
	uint8_t entriesCount
);

void YRTransmissionQueueDestroy(YRTransmissionQueueRef tq);

#pragma mark - Base

void YRTransmissionQueueSetBase(YRTransmissionQueueRef tq, YRSequenceNumberType base);
YRSequenceNumberType YRTransmissionQueueGetBase(YRTransmissionQueueRef tq);
void YRTransmissionQueueAdvanceBase(YRTransmissionQueueRef tq, YRSequenceNumberType by);

#pragma mark - Entries

uint8_t YRTransmissionQueueGetEntriesCount(YRTransmissionQueueRef tq);
bool YRTransmissionQueueAddEntry(YRTransmissionQueueRef tq, YRSequenceNumberType seq, double timeout);
/**
 *	Removes previously scheduled entry for given seq #.
 *	Must be called after AddEntry, otherwise - behavior is undefined.
 */
void YRTransmissionQueueRemoveEntry(YRTransmissionQueueRef tq, YRSequenceNumberType seq);

#pragma mark - Utility

void YRTransmissionQueueDebugInfo(char *buffer);
void YRTransmissionQueueDump(YRTransmissionQueueRef tq, char *buffer);

#endif // __YRTransmissionQueue__
