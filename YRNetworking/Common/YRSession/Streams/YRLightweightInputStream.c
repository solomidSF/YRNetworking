//
//  YRLightweightInputStream.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRLightweightInputStream.h"
#include <stdlib.h>

#define YRMakeMultipleTo(what, to) (((uintptr_t)(what) + ((to) - 1)) & (~((to) - 1)))

typedef struct YRLightweightInputStream {
    uint8_t *data;
    uint16_t size;
    uint16_t index;
} YRLightweightInputStream;

uint8_t const kYRLightweightInputStreamSize = sizeof(YRLightweightInputStream);

YRLightweightInputStreamRef YRLightweightInputStreamCreate(const void *buffer, uint16_t bufferSize) {
    YRLightweightInputStreamRef stream = calloc(1, kYRLightweightInputStreamSize);
    
    YRLightweightInputStreamCreateAt(buffer, bufferSize, stream);
    
    return stream;
}

YRLightweightInputStreamRef YRLightweightInputStreamCreateAt(const void *buffer, uint16_t bufferSize, void *outStream) {
    YRLightweightInputStreamRef stream = (YRLightweightInputStreamRef)outStream;
    
    stream->data = (uint8_t *)buffer;
    stream->size = bufferSize;
    stream->index = 0;
    
    return stream;
}

void *YRLightweightInputStreamCurrentPointer(YRLightweightInputStreamRef stream, uint16_t *outSizeLeft) {
    if (outSizeLeft) {
        *outSizeLeft = stream->size - stream->index;
    }
    
    return stream->data + stream->index;
}

uint16_t YRLightweightInputStreamSize(YRLightweightInputStreamRef stream) {
    return stream->size;
}

uint16_t YRLightweightInputStreamCurrentIndex(YRLightweightInputStreamRef stream) {
    return stream->index;
}

// TODO: Transite to memcpy, unaligned pointers is UB.
uint8_t YRLightweightInputStreamReadInt8(YRLightweightInputStreamRef streamRef) {
    if (streamRef->index + sizeof(uint8_t) <= streamRef->size) {
        uint8_t value = 0;
        
        value = *(streamRef->data + streamRef->index);
        
        streamRef->index += sizeof(uint8_t);
        
        return value;
    } else {
        return 0;
    }
}

uint16_t YRLightweightInputStreamReadInt16(YRLightweightInputStreamRef streamRef) {
    if (streamRef->index + sizeof(uint16_t) <= streamRef->size) {
        uint16_t value = 0;
        
        value = ntohs(*((uint16_t *)(streamRef->data + streamRef->index)));
        
        streamRef->index += sizeof(uint16_t);
        
        return value;
    } else {
        return 0;
    }
}

uint32_t YRLightweightInputStreamReadInt32(YRLightweightInputStreamRef streamRef) {
    if (streamRef->index + sizeof(uint32_t) <= streamRef->size) {
        uint32_t value = 0;
        
        value = ntohl(*((uint32_t *)(streamRef->data + streamRef->index)));
        
        streamRef->index += sizeof(uint32_t);
        
        return value;
    } else {
        return 0;
    }
}

void YRLightweightInputSteamReset(YRLightweightInputStreamRef stream) {
    stream->index = 0;
}

void *YRLightweightInputSteamMemalignCurrentPointer(YRLightweightInputStreamRef stream, uint16_t *outSizeLeft) {
    uint16_t index = YRMakeMultipleTo(stream->index, 8);
    
    if (index <= stream->size) {
        stream->index = index;
    } else {
        // TODO: Report error somehow.
        return NULL;
    }
    
    return YRLightweightInputStreamCurrentPointer(stream, outSizeLeft);
}

bool YRLightweightInputStreamSetIndexTo(YRLightweightInputStreamRef stream, uint16_t index) {
    if (index <= stream->size) {
        stream->index = index;
        return true;
    }
    
    return false;
}

bool YRLightweightInputStreamAdvanceBy(YRLightweightInputStreamRef stream, uint16_t by) {
    if (stream->index + by <= stream->size) {
        stream->index += by;
        
        return true;
    } else {
        return false;
    }
}

uint16_t YRLightweightInputStreamBytesLeft(YRLightweightInputStreamRef stream) {
    return stream->size - stream->index;
}
