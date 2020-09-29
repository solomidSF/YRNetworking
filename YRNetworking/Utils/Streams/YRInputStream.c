//
// YRInputStream.c
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

typedef struct YRInputStream {
    uint8_t *data;
    uint16_t size;
    uint16_t index;
} YRInputStream;

static YRInputStream gInputStream;

#pragma mark - Lifecycle

YRInputStreamRef YRInputStreamGetGlobal(void) {
	return &gInputStream;
}

YRInputStreamRef YRInputStreamGet(const void *buffer, uint16_t bufferSize) {
	YRInputStreamRef stream = YRInputStreamGetGlobal();
	YRInputStreamSetTo(stream, buffer, bufferSize);
	return stream;
}

YRInputStreamRef YRInputStreamCreateEmpty(void) {
	return malloc(sizeof(YRInputStream));
}

YRInputStreamRef YRInputStreamCreate(const void *buffer, uint16_t bufferSize) {
	YRInputStreamRef stream = YRInputStreamCreateEmpty();
	YRInputStreamSetTo(stream, buffer, bufferSize);
	return stream;
}

void YRInputSteamDestroy(YRInputStreamRef stream) {
	YRInputSteamReset(stream);
	
	if (stream == &gInputStream) {
		return;
	}
	
	free(stream);
}

void YRInputSteamReset(YRInputStreamRef stream) {
	stream->index = 0;
}

#pragma mark - Introspection

void *YRInputStreamCurrentPointer(YRInputStreamRef stream, uint16_t *outSizeLeft) {
    if (outSizeLeft) {
        *outSizeLeft = stream->size - stream->index;
    }
    
    return stream->data + stream->index;
}

uint16_t YRInputStreamSize(YRInputStreamRef stream) {
    return stream->size;
}

uint16_t YRInputStreamCurrentIndex(YRInputStreamRef stream) {
    return stream->index;
}

uint16_t YRInputStreamBytesLeft(YRInputStreamRef stream) {
    return stream->size - stream->index;
}

#pragma mark - Interface

void YRInputStreamSetTo(YRInputStreamRef stream, const void *buffer, uint16_t bufferSize) {
	stream->data = (uint8_t *)buffer;
	stream->size = bufferSize;
	stream->index = 0;
}

uint8_t YRInputStreamReadUInt8(YRInputStreamRef stream) {
	if (stream->index + sizeof(uint8_t) <= stream->size) {
        uint8_t value = 0;
        
        value = *(stream->data + stream->index);
        
        stream->index += sizeof(uint8_t);
        
        return value;
    } else {
        return 0;
    }
}

uint16_t YRInputStreamReadUInt16(YRInputStreamRef stream) {
    if (stream->index + sizeof(uint16_t) <= stream->size) {
        uint16_t value = 0;
        
        memcpy(&value, stream->data + stream->index, sizeof(uint16_t));

        value = ntohs(value);
        
        stream->index += sizeof(uint16_t);
        
        return value;
    } else {
        return 0;
    }
}

uint32_t YRInputStreamReadUInt32(YRInputStreamRef stream) {
    if (stream->index + sizeof(uint32_t) <= stream->size) {
        uint32_t value = 0;
        
        memcpy(&value, stream->data + stream->index, sizeof(uint32_t));
        
        value = ntohl(value);
        
        stream->index += sizeof(uint32_t);
        
        return value;
    } else {
        return 0;
    }
}

bool YRInputStreamSetIndexTo(YRInputStreamRef stream, uint16_t index) {
    if (index <= stream->size) {
        stream->index = index;
        return true;
    }
    
    return false;
}

bool YRInputStreamAdvanceBy(YRInputStreamRef stream, uint16_t by) {
    if (stream->index + by <= stream->size) {
        stream->index += by;
        
        return true;
    } else {
        return false;
    }
}
