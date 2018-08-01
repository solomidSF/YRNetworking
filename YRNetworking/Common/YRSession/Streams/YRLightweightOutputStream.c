//
//  YRLightweightOutputStream.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRLightweightOutputStream.h"
#include <stdlib.h>
#include <string.h>

#define YRMakeMultipleTo(what, to) (((uintptr_t)(what) + ((to) - 1)) & (~((to) - 1)))

typedef struct YRLightweightOutputStream {
    uint8_t *data;
    uint16_t size;
    uint16_t index;
} YRLightweightOutputStream;

uint8_t const kYRLightweightOutputStreamSize = sizeof(YRLightweightOutputStream);

YRLightweightOutputStreamRef YRLightweightOutputStreamCreate(void *buffer, uint16_t bufferSize) {
    YRLightweightOutputStreamRef streamRef = calloc(1, kYRLightweightOutputStreamSize);
    
    YRLightweightOutputStreamCreateAt(buffer, bufferSize, streamRef);
    
    return streamRef;
}

YRLightweightOutputStreamRef YRLightweightOutputStreamCreateAt(void *buffer, uint16_t bufferSize, void *outStreamRef) {
    YRLightweightOutputStreamRef streamRef = (YRLightweightOutputStreamRef)outStreamRef;
    
    streamRef->data = (uint8_t *)buffer;
    streamRef->size = bufferSize;
    streamRef->index = 0;
    
    return streamRef;
}

void YRLightweightOutputStreamWriteInt8(YRLightweightOutputStreamRef streamRef, uint8_t value) {
    if (streamRef->index + sizeof(uint8_t) <= streamRef->size) {
        *(streamRef->data + streamRef->index) = value;
        
        streamRef->index += sizeof(uint8_t);
    }
}

void YRLightweightOutputStreamWriteInt16(YRLightweightOutputStreamRef streamRef, uint16_t value) {
    if (streamRef->index + sizeof(uint16_t) <= streamRef->size) {
        *((uint16_t *)(streamRef->data + streamRef->index)) = htons(value);
        
        streamRef->index += sizeof(uint16_t);
    }
}

void YRLightweightOutputStreamWriteInt32(YRLightweightOutputStreamRef streamRef, uint32_t value) {
    if (streamRef->index + sizeof(uint32_t) <= streamRef->size) {
        *((uint32_t *)(streamRef->data + streamRef->index)) = htonl(value);
        
        streamRef->index += sizeof(uint32_t);
    }
}

void *YRLightweightOutputStreamMemalign(YRLightweightOutputStreamRef stream) {
    uint16_t index = YRMakeMultipleTo(stream->index, 8);

    if (index < stream->size) {
        stream->index = index;
        
        return stream->data + index;
    } else {
        return NULL;
    }
}

void YRLightweightOutputStreamMemalignWriteBytes(YRLightweightOutputStreamRef streamRef, void *bytes, uint16_t size) {
    void *dataPointer = YRLightweightOutputStreamMemalign(streamRef);
    
    if (!dataPointer) {
        // No space.
        return;
    }
    
    if (streamRef->index + size <= streamRef->size) {
        memcpy(dataPointer, bytes, size);
        streamRef->index += size;
    } else {
        // No space.
        return;
    }
}

void *YRLightweightOutputStreamGetBytes(YRLightweightOutputStreamRef stream) {
    return stream->data;
}
