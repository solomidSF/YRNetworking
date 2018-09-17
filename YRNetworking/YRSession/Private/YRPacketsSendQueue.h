//
//  YRPacketsSendQueue.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/19/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRPacketsSendQueue__
#define __YRPacketsSendQueue__

#include "YRTypes.h"

#include <stdio.h>

typedef struct YRPacketsSendQueue *YRPacketsSendQueueRef;

YRPacketsSendQueueRef YRPacketsSendQueueCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount);
void YRPacketsSendQueueDestroy(YRPacketsSendQueueRef queue);

#pragma mark - Base Segment

void YRPacketsSendQueueSetBaseSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType base);
YRSequenceNumberType YRPacketsSendQueueGetBaseSegment(YRPacketsSendQueueRef queue);
void YRPacketsSendQueueAdvanceBaseSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType by);

#pragma mark - Buffers

bool YRPacketsSendQueueHasBufferForSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType segment);
bool YRPacketsSendQueueIsBufferInUseForSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType segment);
/**
 *  Returns buffer regardless if it's in use or not.
 *  Returns NULL if given segment is not in range of base + buffersCount (overflow is taken into account).
 */
void *YRPacketsSendQueueBufferForSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType segment);

void YRPacketsSendQueueMarkBufferInUseForSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType segment);
void YRPacketsSendQueueUnmarkBufferInUseForSegment(YRPacketsSendQueueRef queue, YRSequenceNumberType segment);

uint8_t YRPacketsSendQueueBuffersInUse(YRPacketsSendQueueRef queue);
void YRPacketsSendQueueGetSegmentNumbersForBuffersInUse(YRPacketsSendQueueRef queue, YRSequenceNumberType *outSegments, uint8_t *inOutCount);

#endif
