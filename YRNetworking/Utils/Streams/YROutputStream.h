//
// YROutputStream.h
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

#ifndef __YROutputStream__
#define __YROutputStream__

typedef struct YROutputStream *YROutputStreamRef;

#pragma mark - Lifecycle

YROutputStreamRef YROutputStreamGetGlobal(void);
YROutputStreamRef YROutputStreamGet(void *buffer, uint16_t bufferSize);
YROutputStreamRef YROutputStreamCreateEmpty(void);
YROutputStreamRef YROutputStreamCreate(void *buffer, uint16_t bufferSize);
void YROutputStreamDestroy(YROutputStreamRef stream);

#pragma mark - Introspection

const void *YROutputStreamBufferStart(YROutputStreamRef stream);
const void *YROutputStreamGetBytes(YROutputStreamRef stream);
uint16_t YROutputStreamBytesWritten(YROutputStreamRef stream);
uint16_t YROutputStreamCurrentIndex(YROutputStreamRef stream);

#pragma mark - Interface

void YROutputStreamReset(YROutputStreamRef stream);
void YROutputStreamSetTo(YROutputStreamRef stream, void *buffer, uint16_t bufferSize);
bool YROutputStreamSetIndexTo(YROutputStreamRef stream, uint16_t index);

void YROutputStreamWriteUInt8(YROutputStreamRef stream, uint8_t value);
void YROutputStreamWriteUInt16(YROutputStreamRef stream, uint16_t value);
void YROutputStreamWriteUInt32(YROutputStreamRef stream, uint32_t value);
void YROutputStreamWriteUInt64(YROutputStreamRef stream, uint64_t value);
void YROutputStreamWriteFloat(YROutputStreamRef stream, float value);
void YROutputStreamWriteDouble(YROutputStreamRef stream, double value);
void YROutputStreamAppend(YROutputStreamRef stream, const void *payload, uint16_t size);

#endif /* __YROutputStream__ */
