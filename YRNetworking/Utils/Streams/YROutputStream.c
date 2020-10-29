//
// YROutputStream.c
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

static YROutputStream gOutputStream;

#pragma mark - Lifecycle

YROutputStreamRef YROutputStreamGetGlobal(void) {
	return &gOutputStream;
}

YROutputStreamRef YROutputStreamGet(void *buffer, uint16_t bufferSize) {
	YROutputStreamRef stream = YROutputStreamGetGlobal();
	YROutputStreamSetTo(stream, buffer, bufferSize);
	return stream;
}

YROutputStreamRef YROutputStreamCreateEmpty(void) {
	return malloc(sizeof(YROutputStream));
}

YROutputStreamRef YROutputStreamCreate(void *buffer, uint16_t bufferSize) {
	YROutputStreamRef stream = YROutputStreamCreateEmpty();
	YROutputStreamSetTo(stream, buffer, bufferSize);
	return stream;
}

void YROutputStreamDestroy(YROutputStreamRef stream) {
	if (stream == &gOutputStream) {
		return;
	}
	
	free(stream);
}

#pragma mark - Introspection

void YROutputStreamReset(YROutputStreamRef stream) {
	stream->index = 0;
}

const void *YROutputStreamGetBytes(YROutputStreamRef stream) {
    return stream->data;
}

uint16_t YROutputStreamBytesWritten(YROutputStreamRef stream) {
	return YROutputStreamCurrentIndex(stream);
}

uint16_t YROutputStreamCurrentIndex(YROutputStreamRef stream) {
	return stream->index;
}

#pragma mark - Interface

void YROutputStreamSetTo(YROutputStreamRef stream, void *buffer, uint16_t bufferSize) {
	stream->data = buffer;
	stream->size = bufferSize;
	stream->index = 0;
}

bool YROutputStreamSetIndexTo(YROutputStreamRef stream, uint16_t index) {
    if (index <= stream->size) {
        stream->index = index;
        return true;
    }
    
    return false;
}

void YROutputStreamWriteUInt8(YROutputStreamRef stream, uint8_t value) {
    if (stream->index + sizeof(uint8_t) <= stream->size) {
        *(stream->data + stream->index) = value;
        
        stream->index += sizeof(uint8_t);
    }
}

void YROutputStreamWriteUInt16(YROutputStreamRef stream, uint16_t value) {
    if (stream->index + sizeof(uint16_t) <= stream->size) {
        uint16_t beValue = htons(value);
        memcpy(stream->data + stream->index, &beValue, sizeof(uint16_t));
        
        stream->index += sizeof(uint16_t);
    }
}

void YROutputStreamWriteUInt32(YROutputStreamRef stream, uint32_t value) {
    if (stream->index + sizeof(uint32_t) <= stream->size) {
        uint32_t beValue = htonl(value);
        memcpy(stream->data + stream->index, &beValue, sizeof(uint32_t));
        
        stream->index += sizeof(uint32_t);
    }
}

void YROutputStreamWriteUInt64(YROutputStreamRef stream, uint64_t value) {
    if (stream->index + sizeof(uint64_t) <= stream->size) {
        uint64_t beValue = htonll(value);
        memcpy(stream->data + stream->index, &beValue, sizeof(uint64_t));

        stream->index += sizeof(uint64_t);
    }
}

void YROutputStreamWriteFloat(YROutputStreamRef stream, float value) {
	union {
	  uint32_t i;
	  float f;
	 } u;

	u.f = value;
	
	YROutputStreamWriteUInt32(stream, u.i);
}

void YROutputStreamWriteDouble(YROutputStreamRef stream, double value) {
	union {
		uint64_t i;
		double d;
	} u;

	u.d = value;
	
	YROutputStreamWriteUInt64(stream, u.i);
}

void YROutputStreamAppend(YROutputStreamRef stream, const void *payload, uint16_t size) {
	if (stream->index + size <= stream->size) {
		memcpy(stream->data + stream->index, payload, size);
		
		stream->index += size;
	}
}
