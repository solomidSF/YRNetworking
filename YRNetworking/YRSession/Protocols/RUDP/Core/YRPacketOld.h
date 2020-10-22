//
//  YRPacket.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/21/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef __YRPacket__
#define __YRPacket__

#include <stdio.h>
#include "YRPacketHeader.h"
#include "YRLightweightInputStream.h"
#include "YRLightweightOutputStream.h"

typedef struct YRPacket *YRPacketRef;

#pragma mark - Data Structure Sizes

YRPayloadLengthType YRPacketSYNLength(void);
YRPayloadLengthType YRPacketRSTLength(void);
YRPayloadLengthType YRPacketNULLength(void);
YRPayloadLengthType YRPacketACKLength(void);
YRPayloadLengthType YRPacketEACKLength(YRSequenceNumberType *ioSequencesCount);
YRPayloadLengthType YRPacketEACKLengthWithPayload(YRSequenceNumberType *ioSequencesCount, YRPayloadLengthType payloadLength);
YRPayloadLengthType YRPacketLengthForPayload(YRPayloadLengthType payloadLength);

/**
 *  Returns size needed to maintain YRPacket data structure for given packet size.
 *  Typically used to allocate buffers for session.
 */
size_t YRPacketDataStructureLengthForPacketSize(YRPayloadLengthType packetSize);

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(YRConnectionConfiguration configuration,
                              YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              bool hasACK,
                              void *packetBuffer);

YRPacketRef YRPacketCreateRST(uint8_t errorCode,
                              YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              bool hasACK,
                              void *packetBuffer);

YRPacketRef YRPacketCreateNUL(YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              void *packetBuffer);

YRPacketRef YRPacketCreateACK(YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              void *packetBuffer);

YRPacketRef YRPacketCreateEACK(YRSequenceNumberType seqNumber,
                               YRSequenceNumberType ackNumber,
                               YRSequenceNumberType *sequences,
                               YRSequenceNumberType *ioSequencesCount,
                               void *packetBuffer);

YRPacketRef YRPacketCreateEACKWithPayload(YRSequenceNumberType seqNumber,
                                          YRSequenceNumberType ackNumber,
                                          YRSequenceNumberType *sequences,
                                          YRSequenceNumberType *ioSequencesCount,
                                          const void *payload,
                                          YRPayloadLengthType payloadLength,
                                          bool copyPayload,
                                          void *packetBuffer);

YRPacketRef YRPacketCreateWithPayload(YRSequenceNumberType seqNumber,
                                      YRSequenceNumberType ackNumber,
                                      const void *payload,
                                      YRPayloadLengthType payloadLength,
                                      bool copyPayload,
                                      void *packetBuffer);

void YRPacketCopy(YRPacketRef packet, void *whereTo);

void YRPacketDestroy(YRPacketRef packet);

#pragma mark - Introspection

YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet);
void *YRPacketGetPayload(YRPacketRef packet, YRPayloadLengthType *outPayloadSize);
YRPayloadLengthType YRPacketGetLength(YRPacketRef packet);

/**
 *  Checks if packet is logically valid.
 *  Header is not valid if protocol version doesn't match current one.
 *  Header is not valid if checksum doesn't match.
 *  Header is not valid if it has 2+ types listed: SYN NUL RST.
 *  Header is not valid if NUL packet doesn't contain ACK/EACK.
 *  Header is not valid if RST/SYN/NUL packet contains data.
 */
bool YRPacketIsLogicallyValid(YRPacketRef packet);
bool YRPacketCanDeserializeFromStream(YRLightweightInputStreamRef stream);

#pragma mark - Serialization

void YRPacketSerialize(YRPacketRef packet, YRLightweightOutputStreamRef buffer);

YRPacketRef YRPacketDeserialize(YRLightweightInputStreamRef stream);
YRPacketRef YRPacketDeserializeAt(YRLightweightInputStreamRef stream, void *packetBuffer);

/**
 *  Should be invoked only after YRPacketDeserialize/YRPacketDeserializeAt.
 *  Default deserialization functions just set pointer to packet payload to avoid memcpy.
 *  This is a good win in case of one-shot packets that will be directly passed on higher levels.
 *  But this one copies all data into YRPacketRef data structure for further processing.
 */
void YRPacketCopyPayloadInline(YRPacketRef packet);

#endif
