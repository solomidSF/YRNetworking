//
// YRTransmissionQueue.c
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

#include "YRInternal.h"

typedef struct YRTransmissionQueue {
	void *context;
	void (*transmitHandler) (
		YRTransmissionQueueRef tq,
		YRTransmissionQueueEntry qe,
		YRSequenceNumberType seq,
		void *context,
		bool *reschedule
	);
	YRTimerOrchestrator o;
	YRSequenceNumberType base;
	uint8_t currentIndex;
	uint8_t entriesCount;
	uint8_t entriesUsedSoFar;
	YRTransmissionQueueEntry entries[];
} YRTransmissionQueue;

#pragma mark - Prototypes

void YRTransmissionQueueInvalidateEntries(YRTransmissionQueueRef tq, YRSequenceNumberType howMuch);
YRTransmissionQueueEntry *YRTransmissionQueueEntryForSeq(YRTransmissionQueueRef tq, YRSequenceNumberType seq);
void YRTransmissionQueueTimerCallback(
	YRTimerOrchestrator o,
	YRTimerHandle h,
	void *context1,
	void *context2,
	bool *reschedule
);

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
) {
	YRTransmissionQueueRef tq = malloc(sizeof(YRTransmissionQueue) + sizeof(YRTransmissionQueueEntry) * entriesCount);
		
	tq->context = context;
	tq->transmitHandler = transmitHandler;
	tq->o = o;
	tq->base = 0;
	tq->currentIndex = 0;
	tq->entriesCount = entriesCount > 0 ? entriesCount : 1;
	tq->entriesUsedSoFar = 0;
	
	return tq;
}

void YRTransmissionQueueDestroy(YRTransmissionQueueRef tq) {
	if (tq) {
		YRTransmissionQueueInvalidateEntries(tq, tq->entriesUsedSoFar);
		
		free(tq);
	}
}

#pragma mark - Base

void YRTransmissionQueueSetBase(YRTransmissionQueueRef tq, YRSequenceNumberType base) {
	YRTransmissionQueueInvalidateEntries(tq, tq->entriesUsedSoFar);
	
	tq->base = base;
	tq->currentIndex = 0;
	tq->entriesUsedSoFar = 0;
}

YRSequenceNumberType YRTransmissionQueueGetBase(YRTransmissionQueueRef tq) {
	return tq->base;
}

void YRTransmissionQueueAdvanceBase(YRTransmissionQueueRef tq, YRSequenceNumberType by) {
	uint8_t howMuchToInvalidate = by < tq->entriesUsedSoFar ? by : tq->entriesUsedSoFar;
	
	YRTransmissionQueueInvalidateEntries(tq, howMuchToInvalidate);
	
	tq->base += by;
	tq->currentIndex += by;
}

#pragma mark - Entries

uint8_t YRTransmissionQueueGetEntriesCount(YRTransmissionQueueRef tq) {
	return tq->entriesCount;
}

bool YRTransmissionQueueAddEntry(YRTransmissionQueueRef tq, YRSequenceNumberType seq, double timeout) {
	YRTransmissionQueueEntry *entry = YRTransmissionQueueEntryForSeq(tq, seq);

	if (!entry) {
		return false;
	}

	tq->entriesUsedSoFar = (tq->entriesUsedSoFar + 1) > tq->entriesCount ? tq->entriesCount : (tq->entriesUsedSoFar + 1);
	entry->inUse = true;
	entry->transmissionsSoFar = 0;
	entry->startTime = time(NULL);
	
	uintptr_t convertedSeq = seq;
	
	YRTimerHandle h = YRRuntimeEnvironmentGetTimerInterface().schedule(
		tq->o,
		timeout,
		tq,
		(void *)convertedSeq,
		true,
		&YRTransmissionQueueTimerCallback
	);
	
	entry->h = h;
	
	return true;
}

void YRTransmissionQueueRemoveEntry(YRTransmissionQueueRef tq, YRSequenceNumberType seq) {
	YRTransmissionQueueEntry *entry = YRTransmissionQueueEntryForSeq(tq, seq);
	
	if (!entry) {
		return;
	}
	
	if (entry->inUse) {
		entry->inUse = false;
		
		YRRuntimeEnvironmentGetTimerInterface().cancel(tq->o, entry->h);
	}
}

#pragma mark - Utility

#define WRITE_BUF(s) \
	offset += sprintf(buffer + offset, s"\n")

#define WRITE_BUF_A(s, ...) \
	offset += sprintf(buffer + offset, s"\n", __VA_ARGS__)

void YRTransmissionQueueDebugInfo(char *buffer) {
	int offset = 0;

	WRITE_BUF("<YRTransmissionQueue>");
	WRITE_BUF("Data structure sizes:");
	WRITE_BUF_A("\tYRTransmissionQueue: %zu", sizeof(YRTransmissionQueue));
	WRITE_BUF_A("\tYRTransmissionQueueEntry: %zu", sizeof(YRTransmissionQueueEntry));
}

void YRTransmissionQueueDump(YRTransmissionQueueRef tq, char *buffer) {
	int offset = 0;
	
	WRITE_BUF_A("<YRTransmissionQueue %p>", tq);
	// TODO:
}

#pragma mark - Private

void YRTransmissionQueueInvalidateEntries(YRTransmissionQueueRef tq, YRSequenceNumberType howMuch) {
	YRSequenceNumberType iterations = howMuch < tq->entriesUsedSoFar ? howMuch : tq->entriesUsedSoFar;

	for (YRSequenceNumberType i = 0; i < iterations; i++) {
		YRTransmissionQueueRemoveEntry(tq, tq->base + i);
	}
}

YRTransmissionQueueEntry *YRTransmissionQueueEntryForSeq(YRTransmissionQueueRef tq, YRSequenceNumberType seq) {
	// Overflow is ok
	YRSequenceNumberType offset = seq - tq->base;
	
	if (offset >= tq->entriesCount) {
		// Segment is not in range.
		return NULL;
	}
	
	uint8_t index = (tq->currentIndex + offset) % tq->entriesCount;

	return &tq->entries[index];
}

void YRTransmissionQueueTimerCallback(
	YRTimerOrchestrator o,
	YRTimerHandle h,
	void *context1,
	void *context2,
	bool *reschedule
) {
	YRTransmissionQueueRef tq = context1;
	YRSequenceNumberType seq = (YRSequenceNumberType)((uintptr_t)context2);
	YRTransmissionQueueEntry *entry = YRTransmissionQueueEntryForSeq(tq, seq);

	if (entry) {
		!tq->transmitHandler ?: tq->transmitHandler(tq, *entry, seq, tq->context, reschedule);
		
		entry->transmissionsSoFar += 1;
	}
}
