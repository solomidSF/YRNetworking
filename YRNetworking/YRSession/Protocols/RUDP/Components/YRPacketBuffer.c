//
// YRPacketBuffer.c
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

#pragma mark - Declarations

typedef struct YRPacketBufferElement *YRPacketBufferElementRef;

typedef struct YRPacketBufferElement {
	YRPacketBufferElementRef next;
	YRPacketBufferElementRef prev;
	YRPayloadLengthType usedSize;
	//	uint8_t next;
	//	uint8_t prev;
	
	// Pointer is for alignment
	uint8_t *bufferBlob[];
} YRPacketBufferElement;

typedef struct YRPacketBuffer {
	YRPacketBufferElementRef headBufferInUse;
	YRPacketBufferElementRef tailBufferInUse;
	
	YRPayloadLengthType bufferSize;
	YRSequenceNumberType base;
	
	//    uint8_t headBufferInUse;
	//    uint8_t tailBufferInUse;
	
	uint8_t currentIndex;
	uint8_t buffersInUseCount;
	//    YRPacketBufferFlags flags;
	
	uint8_t buffersCount;
	
	YRPacketBufferElementRef elements[];
} YRPacketBuffer;

#pragma mark - Prototypes

static inline size_t YRPacketBufferElementSizeForBufferSize(YRPayloadLengthType bufferSize);
static inline size_t YRPacketBufferSizeForBufferSize(YRPayloadLengthType bufferSize, uint8_t buffersCount);
static inline bool YRPacketBufferIsBufferInUseInlined(YRPacketBufferRef buffer, YRPacketBufferElementRef element);
static inline YRPacketBufferElementRef YRPacketBufferElementForSeqInlined(
	YRPacketBufferRef buffer,
	YRSequenceNumberType segment
);
static inline YRSequenceNumberType YRPacketBufferSeqForElementInlined(
	YRPacketBufferRef buffer,
	YRPacketBufferElementRef element
);

#pragma mark - Lifecycle

YRPacketBufferRef YRPacketBufferCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount) {
	if (buffersCount == 0) {
		buffersCount = 1;
	}
	
	bufferSize = (bufferSize < YRMaxPacketHeaderSize) ? YRMaxPacketHeaderSize : bufferSize;
	
	YRPacketBufferRef buffer = calloc(1, YRPacketBufferSizeForBufferSize(bufferSize, buffersCount));

	// TODO: Use malloc?
//	YRPacketBufferRef buffer = malloc(YRPacketBufferSizeForBufferSize(bufferSize, buffersCount));
//	memset(buffer, 0, sizeof(YRPacketBuffer));

	buffer->bufferSize = bufferSize;
	buffer->buffersCount = buffersCount;
	
	return buffer;
}

void YRPacketBufferDestroy(YRPacketBufferRef buffer) {
	if (buffer) {
		free(buffer);
	}
}

#pragma mark - Introspection

YRPayloadLengthType YRPacketBufferGetBufferSize(YRPacketBufferRef buffer) {
	return buffer->bufferSize;
}

uint8_t YRPacketBufferGetBuffersCount(YRPacketBufferRef buffer) {
	return buffer->buffersCount;
}

uint8_t YRPacketBufferGetBuffersInUseNum(YRPacketBufferRef buffer) {
	return buffer->buffersInUseCount;
}

#pragma mark - Base

void YRPacketBufferSetBase(YRPacketBufferRef buffer, YRSequenceNumberType base) {
	buffer->base = base;
	buffer->currentIndex = 0;
	
	YRPacketBufferElementRef currentElement = buffer->headBufferInUse;
	
	while (currentElement != NULL) {
		YRPacketBufferElementRef elementToCleanup = currentElement;
		
		currentElement = currentElement->next;
		
		elementToCleanup->usedSize = 0;
		elementToCleanup->next = NULL;
		elementToCleanup->prev = NULL;
	}
	
	// Clear all in-use buffers.
	buffer->headBufferInUse = NULL;
	buffer->tailBufferInUse = NULL;
	buffer->buffersInUseCount = 0;
}

YRSequenceNumberType YRPacketBufferGetBase(YRPacketBufferRef buffer) {
	return buffer->base;
}

void YRPacketBufferAdvanceBase(YRPacketBufferRef buffer, YRSequenceNumberType by) {
	if (by >= buffer->buffersCount) {
		YRPacketBufferSetBase(buffer, buffer->base + by);
	} else {
		// Unmark possible buffers in use that fall in range base + by
		// Worst case scenario 'by == buffersCount == UINT8_MAX'
		// In this one we probably can check some treshold and request buffers in use and iterate through them.
		for (YRSequenceNumberType seq = 0; seq < by; seq++) {
			YRPacketBufferUnmarkBufferInUse(buffer, buffer->base + seq);
		}
		
		buffer->base += by;
		buffer->currentIndex += by;
	}
}

#pragma mark - Buffers

bool YRPacketBufferIsBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq) {
	return YRPacketBufferIsBufferInUseInlined(buffer, YRPacketBufferElementForSeqInlined(buffer, seq));
}

void *YRPacketBufferGetBuffer(YRPacketBufferRef buffer, YRSequenceNumberType seq, YRPayloadLengthType *length, bool *inUse) {
	YRPacketBufferElementRef element = YRPacketBufferElementForSeqInlined(buffer, seq);
	
	if (length) {
		*length = element ? element->usedSize : 0;
	}
	
	if (inUse) {
		*inUse = YRPacketBufferIsBufferInUseInlined(buffer, element);
	}
	
	return element ? element->bufferBlob : NULL;
}

void YRPacketBufferMarkBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq, YRPayloadLengthType len) {
	YRPacketBufferElementRef element = YRPacketBufferElementForSeqInlined(buffer, seq);
	
	if (element) {
		element->usedSize = len > buffer->bufferSize ? buffer->bufferSize : len;
	}
	
	if (element && !YRPacketBufferIsBufferInUseInlined(buffer, element)) {
		buffer->buffersInUseCount++;
		
		if (buffer->headBufferInUse == NULL) {
			// No elements in use currently.
			buffer->headBufferInUse = element;
			buffer->tailBufferInUse = element;
		} else {
			YRPacketBufferElementRef tail = buffer->tailBufferInUse;
			
			tail->next = element;
			element->prev = tail;
			
			buffer->tailBufferInUse = element;
		}
	}
}

void YRPacketBufferUnmarkBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq) {
	YRPacketBufferElementRef element = YRPacketBufferElementForSeqInlined(buffer, seq);
	
	if (YRPacketBufferIsBufferInUseInlined(buffer, element)) {
		buffer->buffersInUseCount--;
		
		YRPacketBufferElementRef nextElement = element->next;
		YRPacketBufferElementRef previousElement = element->prev;
		
		element->usedSize = 0;
		element->next = NULL;
		element->prev = NULL;
		
		if (nextElement != NULL) {
			nextElement->prev = previousElement;
		}
		
		if (previousElement != NULL) {
			previousElement->next = nextElement;
		}
		
		if (element == buffer->tailBufferInUse) {
			// We're unmarking last element in use.
			buffer->tailBufferInUse = previousElement;
		}
		
		if (element == buffer->headBufferInUse) {
			// We're unmarking first element in use.
			buffer->headBufferInUse = nextElement;
		}
	}
}

#pragma mark - Private

size_t YRPacketBufferElementSizeForBufferSize(YRPayloadLengthType bufferSize) {
	return YR_MAKE_MULTIPLE_TO((sizeof(YRPacketBufferElement) + bufferSize), sizeof(YRPacketBufferElementRef));
}

size_t YRPacketBufferSizeForBufferSize(YRPayloadLengthType bufferSize, uint8_t buffersCount) {
	return sizeof(YRPacketBuffer) + YR_MAKE_MULTIPLE_TO(
		(sizeof(YRPacketBufferElement) + bufferSize), sizeof(YRPacketBufferElementRef)
	) * buffersCount;
}

bool YRPacketBufferIsBufferInUseInlined(YRPacketBufferRef buffer, YRPacketBufferElementRef element) {
	if (!element) {
		// No element for given segment.
		return false;
	}
	
	return element->next != NULL || element->prev != NULL || element == buffer->headBufferInUse;
}

YRPacketBufferElementRef YRPacketBufferElementForSeqInlined(YRPacketBufferRef buffer, YRSequenceNumberType seq) {
	// Overflow is ok
	YRSequenceNumberType offset = seq - buffer->base;
	
	if (offset >= buffer->buffersCount) {
		// Segment is not in range.
		return NULL;
	}
	
	uint8_t index = (buffer->currentIndex + offset) % buffer->buffersCount;
	size_t elementSize = YRPacketBufferElementSizeForBufferSize(buffer->bufferSize);
	
	YRPacketBufferElementRef element = (YRPacketBufferElementRef)((uint8_t *)buffer->elements + (index * elementSize));
	
	return element;
}

YRSequenceNumberType YRPacketBufferSeqForElementInlined(YRPacketBufferRef buffer, YRPacketBufferElementRef element) {
	// Check if element is in range.
	size_t totalSize = YRPacketBufferSizeForBufferSize(buffer->bufferSize, buffer->buffersCount);
	ptrdiff_t diff = (uintptr_t)element - (uintptr_t)buffer;
	
	if (diff >= totalSize) {
		return -1;
	}
	
	uint8_t index = diff / YRPacketBufferElementSizeForBufferSize(buffer->bufferSize);
	
	if (buffer->currentIndex > index) {
		return buffer->base + index + buffer->buffersCount - buffer->currentIndex;
	} else {
		return buffer->base + index - buffer->currentIndex;
	}
}

#pragma mark - Utility

#define WRITE_BUF(s) \
	offset += sprintf(buffer + offset, s"\n")

#define WRITE_BUF_A(s, ...) \
	offset += sprintf(buffer + offset, s"\n", __VA_ARGS__)

void YRPacketBufferDebugInfo(char *buffer) {
	int offset = 0;
	
	WRITE_BUF("<YRPacketBuffer>");
	WRITE_BUF("Data structure sizes:");
	WRITE_BUF_A("\tYRPacketBuffer: %zu", sizeof(YRPacketBuffer));
	WRITE_BUF_A("\tYRPacketBufferElement: %zu", sizeof(YRPacketBufferElement));
}

void YRPacketBufferDump(YRPacketBufferRef bufferRef, char *buffer) {
	int offset = 0;
	
	WRITE_BUF_A("<YRPacketBuffer %p>", bufferRef);
	WRITE_BUF_A("\tBuffer size: %d", bufferRef->bufferSize);
	WRITE_BUF_A("\tActual buffer size: %zu", YRPacketBufferElementSizeForBufferSize(bufferRef->bufferSize));
	WRITE_BUF_A("\tBuffers count: %d", bufferRef->buffersCount);
	WRITE_BUF_A("\tBuffers in use: %d", bufferRef->buffersInUseCount);
	WRITE_BUF_A("\tBase: %d. Internal idx: %d", bufferRef->base, bufferRef->currentIndex);
	WRITE_BUF_A("\tTotal size: %zu", YRPacketBufferSizeForBufferSize(bufferRef->bufferSize, bufferRef->buffersCount));
	YRPacketBufferElementRef i = bufferRef->headBufferInUse;
	
	if (i) {
		WRITE_BUF("Buffers:");
	}
	
	while (i) {
		WRITE_BUF_A("\t<YRPacketBufferElement %p>", i);
		WRITE_BUF_A("\t\tRepresented seq#: %d", YRPacketBufferSeqForElementInlined(bufferRef, i));
		WRITE_BUF_A("\t\tSize: %d", i->usedSize);
		
		i = i->next;
	}
}
