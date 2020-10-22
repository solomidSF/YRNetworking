//
//  YRPacketsQueue.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 9/7/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRPacketsQueue.h"

#include <stdlib.h>
#include <assert.h>

//

typedef struct YRQueueElement *YRQueueElementRef;

typedef struct YRQueueElement {
    YRQueueElementRef next;
    YRQueueElementRef prev;
    
    uint8_t bufferBlob[];
} YRQueueElement;

//

enum YRPacketsQueueFlags {
    kYRPacketsQueueFlagIsBaseSet = 1 << 0,
};

typedef uint8_t YRPacketsQueueFlags;

typedef struct YRPacketsQueue {
    YRQueueElementRef headBufferInUse;
    YRQueueElementRef tailBufferInUse;
    
    YRSequenceNumberType base;
    YRPayloadLengthType bufferSize;
    uint8_t currentIndex;
    uint8_t buffersCount;
    uint8_t buffersInUseCount;
    YRPacketsQueueFlags flags;
    
    YRQueueElementRef elements[];
} YRPacketsQueue;

#pragma mark - Prototypes

static inline size_t YRPacketsQueueElementSizeForBufferSize(YRPayloadLengthType bufferSize);
static inline bool YRPacketsQueueHasBufferForSegmentInlined(YRPacketsQueueRef queue, YRSequenceNumberType segment);
YRQueueElementRef YRPacketsQueueElementForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);

#pragma mark - Lifecycle

YRPacketsQueueRef YRPacketsQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount) {
    // TODO: Try several regions in case of large allocations e.g. 1300 * 255
    if (buffersCount == 0) {
        buffersCount = 1;
    }
    
    size_t elementsSize = YRPacketsQueueElementSizeForBufferSize(bufferSize) * buffersCount;
    YRPacketsQueueRef queue = calloc(1, sizeof(YRPacketsQueue) + elementsSize);
    
    queue->bufferSize = bufferSize;
    queue->buffersCount = buffersCount;
    
    return queue;
}

void YRPacketsQueueDestroy(YRPacketsQueueRef queue) {
    if (queue) {
        free(queue);
    }
}

#pragma mark - Base Segment

void YRPacketsQueueSetBaseSegment(YRPacketsQueueRef queue, YRSequenceNumberType base) {
    queue->base = base;
    queue->currentIndex = 0;
    
    YRQueueElementRef currentElement = queue->headBufferInUse;
    
    while (currentElement != NULL) {
        YRQueueElementRef elementToCleanup = currentElement;
        
        currentElement = currentElement->next;
        
        elementToCleanup->next = NULL;
        elementToCleanup->prev = NULL;
    }
    
    // Clear all in-use buffers.
    queue->headBufferInUse = NULL;
    queue->tailBufferInUse = NULL;
    queue->buffersInUseCount = 0;
    
    queue->flags |= kYRPacketsQueueFlagIsBaseSet;
}

YRSequenceNumberType YRPacketsQueueGetBaseSegment(YRPacketsQueueRef queue) {
    return queue->base;
}

void YRPacketsQueueAdvanceBaseSegment(YRPacketsQueueRef queue, YRSequenceNumberType by) {
    if (by > queue->buffersCount) {
        YRPacketsQueueSetBaseSegment(queue, queue->base + by);
    } else {
        // Unmark possible buffers in use that fall in range base + by
        // Worst case scenario 'by == buffersCount == UINT8_MAX'
        // In this one we probably can check some treshold and request buffers in use and iterate through them.
        for (YRSequenceNumberType segment = queue->base; segment < (YRSequenceNumberType)(queue->base + by); segment++) {
            YRPacketsQueueUnmarkBufferInUseForSegment(queue, segment);
        }
        
        queue->base += by;
        queue->currentIndex += by;
    }
}

#pragma mark - Buffers

bool YRPacketsQueueHasBufferForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    return YRPacketsQueueHasBufferForSegmentInlined(queue, segment);
}

bool YRPacketsQueueIsBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    YRQueueElementRef element = YRPacketsQueueElementForSegment(queue, segment);
    
    if (!element) {
        // No element for given segment.
        return false;
    }
    
    return element->next != NULL || element->prev != NULL || element == queue->headBufferInUse;
}

void *YRPacketsQueueBufferForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsQueueHasBufferForSegmentInlined(queue, segment)) {
        return YRPacketsQueueElementForSegment(queue, segment)->bufferBlob;
    } else {
        return NULL;
    }
}

void YRPacketsQueueMarkBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsQueueHasBufferForSegmentInlined(queue, segment) &&
        !YRPacketsQueueIsBufferInUseForSegment(queue, segment)) {
        queue->buffersInUseCount++;
        
        YRQueueElementRef element = YRPacketsQueueElementForSegment(queue, segment);
        
        if (queue->headBufferInUse == NULL) {
            // No elements in use currently.
            queue->headBufferInUse = element;
            queue->tailBufferInUse = element;
        } else {
            YRQueueElementRef currentTail = queue->tailBufferInUse;
            
            currentTail->next = element;
            
            element->prev = currentTail;
            
            queue->tailBufferInUse = element;
        }
    }
}

void YRPacketsQueueUnmarkBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsQueueHasBufferForSegmentInlined(queue, segment) &&
        YRPacketsQueueIsBufferInUseForSegment(queue, segment)) {
        queue->buffersInUseCount--;
        
        YRQueueElementRef element = YRPacketsQueueElementForSegment(queue, segment);
        
        YRQueueElementRef nextElement = element->next;
        YRQueueElementRef previousElement = element->prev;
        
        element->next = NULL;
        element->prev = NULL;
        
        if (nextElement != NULL) {
            nextElement->prev = previousElement;
        }
        
        if (previousElement != NULL) {
            previousElement->next = nextElement;
        }
        
        if (element == queue->tailBufferInUse) {
            // We're unmarking last element in use.
            queue->tailBufferInUse = previousElement;
        }
        
        if (element == queue->headBufferInUse) {
            // We're unmarking first element in use.
            queue->headBufferInUse = nextElement;
        }
    }
}

uint8_t YRPacketsQueueBuffersInUse(YRPacketsQueueRef queue) {
    return queue->buffersInUseCount;
}

void YRPacketsQueueGetSegmentNumbersForBuffersInUse(YRPacketsQueueRef queue, YRSequenceNumberType *outSegments, uint8_t *inOutCount) {
    if (!inOutCount || !outSegments) {
        return;
    }
    
    uint8_t buffersInUse = YRPacketsQueueBuffersInUse(queue);
    uint8_t buffersToProvide = *inOutCount;
    
    if (buffersInUse < buffersToProvide) {
        buffersToProvide = buffersInUse;
    }
    
    YRQueueElementRef queueElement = queue->headBufferInUse;
    size_t elementSize = YRPacketsQueueElementSizeForBufferSize(queue->bufferSize);
    
    for (uint8_t i = 0; i < buffersToProvide; i++) {
        assert(queueElement != NULL);
        
        uintptr_t elementAddress = (uintptr_t)queueElement;
        uintptr_t diff = elementAddress - (uintptr_t)queue->elements;
        
        assert(diff % elementSize == 0);
        
        uint8_t index = diff / elementSize;
        uint8_t indexDiff = index - queue->currentIndex;
        
        outSegments[i] = queue->base + indexDiff;
        
        queueElement = queueElement->next;
    }
    
    *inOutCount = buffersToProvide;
}

#pragma mark - Private

size_t YRPacketsQueueElementSizeForBufferSize(YRPayloadLengthType bufferSize) {
	return sizeof(YRQueueElement) + bufferSize;
//    return YRMakeMultipleTo((sizeof(YRQueueElement) + bufferSize), sizeof(YRQueueElementRef));
}

bool YRPacketsQueueHasBufferForSegmentInlined(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    // Overflow is ok
    YRSequenceNumberType offset = segment - queue->base;
    
    return offset < queue->buffersCount;
}

YRQueueElementRef YRPacketsQueueElementForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment) {
    // Overflow is ok
    YRSequenceNumberType offset = segment - queue->base;
    
    if (offset >= queue->buffersCount) {
        // Segment is not in range.
        return NULL;
    }
    
    uint8_t index = (queue->currentIndex + offset) % queue->buffersCount;
    size_t elementSize = YRPacketsQueueElementSizeForBufferSize(queue->bufferSize);
    
    YRQueueElementRef element = (YRQueueElementRef)((uint8_t *)queue->elements + (index * elementSize));
    
    return element;
}
