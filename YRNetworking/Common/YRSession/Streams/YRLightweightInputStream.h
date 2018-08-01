//
//  YRLightweightInputStream.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRLightweightInputStream_h
#define YRLightweightInputStream_h

#include <stdio.h>
#include <stdbool.h>

typedef struct YRLightweightInputStream *YRLightweightInputStreamRef;

// Size for stream structure. Can be used to create stream on stack.
extern uint8_t const kYRLightweightInputStreamSize;

YRLightweightInputStreamRef YRLightweightInputStreamCreate(const void *buffer, uint16_t bufferSize);
// Creates stream and places it in outStreamRef
YRLightweightInputStreamRef YRLightweightInputStreamCreateAt(const void *buffer, uint16_t bufferSize, void *outStreamRef);

void *YRLightweightInputSteamCurrentPointer(YRLightweightInputStreamRef stream, uint16_t *outSizeLeft);
uint16_t YRLightweightInputStreamSize(YRLightweightInputStreamRef stream);

uint8_t YRLightweightInputStreamReadInt8(YRLightweightInputStreamRef stream);
uint16_t YRLightweightInputStreamReadInt16(YRLightweightInputStreamRef stream);
uint32_t YRLightweightInputStreamReadInt32(YRLightweightInputStreamRef stream);

void YRLightweightInputSteamReset(YRLightweightInputStreamRef stream);
void *YRLightweightInputSteamMemalignCurrentPointer(YRLightweightInputStreamRef stream, uint16_t *outSizeLeft);

bool YRLightweightInputStreamAdvanceBy(YRLightweightInputStreamRef stream, uint16_t by);
uint16_t YRLightweightInputStreamBytesLeft(YRLightweightInputStreamRef stream);

#endif /* YRLightweightInputStream_h */
