//
// YRPacketBuffer.h
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

#ifndef __YRPacketBuffer__
#define __YRPacketBuffer__

/**
 *  Simple data structure for storing packets.
 */
typedef struct YRPacketBuffer *YRPacketBufferRef;

#pragma mark - Lifecycle

YRPacketBufferRef YRPacketBufferCreate(YRPayloadLengthType bufferSize, uint8_t buffersCount);
void YRPacketBufferDestroy(YRPacketBufferRef buffer);

#pragma mark - Introspection

YRPayloadLengthType YRPacketBufferGetBufferSize(YRPacketBufferRef buffer);
uint8_t YRPacketBufferGetBuffersCount(YRPacketBufferRef buffer);
uint8_t YRPacketBufferGetBuffersInUseNum(YRPacketBufferRef buffer);

#pragma mark - Base

void YRPacketBufferSetBase(YRPacketBufferRef buffer, YRSequenceNumberType base);
YRSequenceNumberType YRPacketBufferGetBase(YRPacketBufferRef buffer);
void YRPacketBufferAdvanceBase(YRPacketBufferRef buffer, YRSequenceNumberType by);

#pragma mark - Buffers

/** Returns true if buffer marked 'in-use' for given sequence number */
bool YRPacketBufferIsBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq);
/** Marks buffer 'in-use' for given sequence number and sets buffer length to len */
void YRPacketBufferMarkBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq, YRPayloadLengthType len);
/** Unmarks buffer 'in-use' for given sequence number */
void YRPacketBufferUnmarkBufferInUse(YRPacketBufferRef buffer, YRSequenceNumberType seq);
/** Returns buffer with it's length or NULL and whether it's in use for given sequence number */
void *YRPacketBufferGetBuffer(YRPacketBufferRef buffer, YRSequenceNumberType seq, YRPayloadLengthType *length, bool *inUse);

#pragma mark - Utility

void YRPacketBufferDebugInfo(char *buffer);
void YRPacketBufferDump(YRPacketBufferRef bufferRef, char *buffer);

#endif // __YRPacketBuffer__
