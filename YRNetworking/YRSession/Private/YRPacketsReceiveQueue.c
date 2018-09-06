//
//  YRPacketsReceiveQueue.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/20/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRPacketsReceiveQueue.h"

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

enum YRPacketsReceiveQueueFlags {
    kYRPacketsReceiveQueueFlagIsBaseSet = 1 << 0,
};

typedef uint8_t YRPacketsReceiveQueueFlags;

typedef struct YRPacketsReceiveQueue {
    YRQueueElementRef headBufferInUse;
    YRQueueElementRef tailBufferInUse;
    
    YRSequenceNumberType base;
    YRPayloadLengthType bufferSize;
    uint8_t currentIndex;
    uint8_t buffersCount;
    uint8_t buffersInUseCount;
    YRPacketsReceiveQueueFlags flags;
    
    YRQueueElementRef elements[];
} YRPacketsReceiveQueue;

#pragma mark - Prototypes

static inline size_t YRPacketsReceiveQueueElementSizeForBufferSize(YRPayloadLengthType bufferSize);
static inline bool YRPacketsReceiveQueueHasBufferForSegmentInlined(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);
YRQueueElementRef YRPacketsReceiveQueueElementForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);

#pragma mark - Lifecycle

YRPacketsReceiveQueueRef YRPacketsReceiveQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount) {
    // TODO: Try several regions in case of large allocations e.g. 1300 * 255
    size_t elementsSize = YRPacketsReceiveQueueElementSizeForBufferSize(bufferSize) * buffersCount;
    YRPacketsReceiveQueueRef queue = calloc(1, sizeof(YRPacketsReceiveQueue) + elementsSize);

    queue->bufferSize = bufferSize;
    queue->buffersCount = buffersCount;
    
    return queue;
}

void YRPacketsReceiveQueueDestroy(YRPacketsReceiveQueueRef queue) {
    if (queue) {
        free(queue);
    }
}

#pragma mark - Base Segment

void YRPacketsReceiveQueueSetBaseSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType base) {
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
    
    queue->flags |= kYRPacketsReceiveQueueFlagIsBaseSet;
}

YRSequenceNumberType YRPacketsReceiveQueueGetBaseSegment(YRPacketsReceiveQueueRef queue) {
    return queue->base;
}

void YRPacketsReceiveQueueAdvanceBaseSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType by) {
    if (by > queue->buffersCount) {
        YRPacketsReceiveQueueSetBaseSegment(queue, queue->base + by);
    } else {
        // Unmark possible buffers in use that fall in range base + by
        // Worst case scenario 'by == buffersCount == UINT8_MAX'
        // In this one we probably can check some treshold and request buffers in use and iterate through them.
        for (YRSequenceNumberType segment = queue->base; segment < queue->base + by; segment++) {
            YRPacketsReceiveQueueUnmarkBufferInUseForSegment(queue, segment);
        }
        
        queue->base += by;
        queue->currentIndex += by;
    }
}

#pragma mark - Buffers

bool YRPacketsReceiveQueueHasBufferForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    return YRPacketsReceiveQueueHasBufferForSegmentInlined(queue, segment);
}

bool YRPacketsReceiveQueueIsBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    YRQueueElementRef element = YRPacketsReceiveQueueElementForSegment(queue, segment);
    
    if (!element) {
        // No element for given segment.
        return false;
    }
    
    return element->next != NULL || element->prev != NULL || element == queue->headBufferInUse;
}

void *YRPacketsReceiveQueueBufferForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsReceiveQueueHasBufferForSegmentInlined(queue, segment)) {
        return YRPacketsReceiveQueueElementForSegment(queue, segment)->bufferBlob;
    } else {
        return NULL;
    }
}

void YRPacketsReceiveQueueMarkBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsReceiveQueueHasBufferForSegmentInlined(queue, segment) &&
        !YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segment)) {
        queue->buffersInUseCount++;
        
        YRQueueElementRef element = YRPacketsReceiveQueueElementForSegment(queue, segment);
        
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

void YRPacketsReceiveQueueUnmarkBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    if (YRPacketsReceiveQueueHasBufferForSegmentInlined(queue, segment) &&
        YRPacketsReceiveQueueIsBufferInUseForSegment(queue, segment)) {
        queue->buffersInUseCount--;
        
        YRQueueElementRef element = YRPacketsReceiveQueueElementForSegment(queue, segment);

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

uint8_t YRPacketsReceiveQueueBuffersInUse(YRPacketsReceiveQueueRef queue) {
    return queue->buffersInUseCount;
}

void YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(YRPacketsReceiveQueueRef queue, YRSequenceNumberType *outSegments, uint8_t *inOutCount) {
    if (!inOutCount || !outSegments) {
        return;
    }
    
    uint8_t buffersInUse = YRPacketsReceiveQueueBuffersInUse(queue);
    uint8_t buffersToProvide = *inOutCount;
    
    if (buffersInUse < buffersToProvide) {
        buffersToProvide = buffersInUse;
    }
    
    YRQueueElementRef queueElement = queue->headBufferInUse;
    size_t elementSize = YRPacketsReceiveQueueElementSizeForBufferSize(queue->bufferSize);
    
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

size_t YRPacketsReceiveQueueElementSizeForBufferSize(YRPayloadLengthType bufferSize) {
    return YRMakeMultipleTo((sizeof(YRQueueElement) + bufferSize), sizeof(YRQueueElementRef));
}

bool YRPacketsReceiveQueueHasBufferForSegmentInlined(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    // Overflow is ok
    YRSequenceNumberType offset = segment - queue->base;
    
    return offset < queue->buffersCount;
}

YRQueueElementRef YRPacketsReceiveQueueElementForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment) {
    // Overflow is ok
    YRSequenceNumberType offset = segment - queue->base;
    
    if (offset >= queue->buffersCount) {
        // Segment is not in range.
        return NULL;
    }
    
    uint8_t index = (queue->currentIndex + offset) % queue->buffersCount;
    size_t elementSize = YRPacketsReceiveQueueElementSizeForBufferSize(queue->bufferSize);
    
    YRQueueElementRef element = (YRQueueElementRef)((uint8_t *)queue->elements + (index * elementSize));
    
    return element;
}
