//
//  YRPacketsQueue.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 9/7/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRPacketsQueue__
#define __YRPacketsQueue__

#include "YRTypes.h"

#include <stdio.h>

/**
 *  Simple data structure for storing packets data.
 */
typedef struct YRPacketsQueue *YRPacketsQueueRef;

#pragma mark - Lifecycle

YRPacketsQueueRef YRPacketsQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount);
// TODO: Investigate this idea.
//YRPacketsQueueRef YRPacketsQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount, uint8_t buffersAlignment);
void YRPacketsQueueDestroy(YRPacketsQueueRef queue);

#pragma mark - Base Segment

void YRPacketsQueueSetBaseSegment(YRPacketsQueueRef queue, YRSequenceNumberType base);
YRSequenceNumberType YRPacketsQueueGetBaseSegment(YRPacketsQueueRef queue);
void YRPacketsQueueAdvanceBaseSegment(YRPacketsQueueRef queue, YRSequenceNumberType by);

#pragma mark - Buffers

bool YRPacketsQueueHasBufferForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);
bool YRPacketsQueueIsBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);
/**
 *  Returns buffer regardless if it's in use or not.
 *  Returns NULL if given segment is not in range of base + buffersCount (overflow is taken into account).
 */
void *YRPacketsQueueBufferForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);

void YRPacketsQueueMarkBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);
void YRPacketsQueueUnmarkBufferInUseForSegment(YRPacketsQueueRef queue, YRSequenceNumberType segment);

uint8_t YRPacketsQueueBuffersInUse(YRPacketsQueueRef queue);
void YRPacketsQueueGetSegmentNumbersForBuffersInUse(YRPacketsQueueRef queue, YRSequenceNumberType *outSegments, uint8_t *inOutCount);

#endif
