//
//  YRLightweightOutputStream.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRLightweightOutputStream_h
#define YRLightweightOutputStream_h

#include <stdio.h>

typedef struct YRLightweightOutputStream *YRLightweightOutputStreamRef;

// Size for stream structure. Can be used to create stream on stack.
extern uint8_t const kYRLightweightOutputStreamSize;

YRLightweightOutputStreamRef YRLightweightOutputStreamCreate(void *buffer, uint16_t bufferSize);
// Creates stream and places it in outStreamRef
YRLightweightOutputStreamRef YRLightweightOutputStreamCreateAt(void *buffer, uint16_t bufferSize, void *outStreamRef);

void YRLightweightOutputStreamWriteInt8(YRLightweightOutputStreamRef streamRef, uint8_t value);
void YRLightweightOutputStreamWriteInt16(YRLightweightOutputStreamRef streamRef, uint16_t value);
void YRLightweightOutputStreamWriteInt32(YRLightweightOutputStreamRef streamRef, uint32_t value);

void *YRLightweightOutputStreamMemalign(YRLightweightOutputStreamRef stream);
void YRLightweightOutputStreamMemalignWriteBytes(YRLightweightOutputStreamRef streamRef, void *bytes, uint16_t size);

void *YRLightweightOutputStreamGetBytes(YRLightweightOutputStreamRef stream);

#endif /* YRLightweightOutputStream_h */
