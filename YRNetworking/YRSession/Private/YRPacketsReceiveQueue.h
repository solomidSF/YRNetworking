//
//  YRPacketsReceiveQueue.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/20/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRPacketsReceiveQueue__
#define __YRPacketsReceiveQueue__

#include "YRTypes.h"

#include <stdio.h>

/**
 *  Simple data structure for storing incoming packets received out of sequence.
 */
typedef struct YRPacketsReceiveQueue *YRPacketsReceiveQueueRef;

#pragma mark - Lifecycle

YRPacketsReceiveQueueRef YRPacketsReceiveQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount);
void YRPacketsReceiveQueueDestroy(YRPacketsReceiveQueueRef queue);

#pragma mark - Base Segment

void YRPacketsReceiveQueueSetBaseSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType base);
YRSequenceNumberType YRPacketsReceiveQueueGetBaseSegment(YRPacketsReceiveQueueRef queue);
void YRPacketsReceiveQueueAdvanceBaseSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType by);

#pragma mark - Buffers

bool YRPacketsReceiveQueueHasBufferForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);
bool YRPacketsReceiveQueueIsBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);
/**
 *  Returns buffer regardless if it's in use or not.
 *  Returns NULL if given segment is not in range of base + buffersCount (overflow is taken into account).
 */
void *YRPacketsReceiveQueueBufferForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);

void YRPacketsReceiveQueueMarkBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);
void YRPacketsReceiveQueueUnmarkBufferInUseForSegment(YRPacketsReceiveQueueRef queue, YRSequenceNumberType segment);

uint8_t YRPacketsReceiveQueueBuffersInUse(YRPacketsReceiveQueueRef queue);
void YRPacketsReceiveQueueGetSegmentNumbersForBuffersInUse(YRPacketsReceiveQueueRef queue, YRSequenceNumberType *outSegments, uint8_t *inOutCount);

#endif /* YRPacketsReceiveQueue_h */
