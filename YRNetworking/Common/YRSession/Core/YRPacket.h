//
//  YRPacket.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/21/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRPacket_h
#define YRPacket_h

#include <stdio.h>
#include "YRPacketHeader.h"
#include "YRLightweightInputStream.h"
#include "YRLightweightOutputStream.h"

typedef struct YRPacket *YRPacketRef;

#pragma mark - Data Structure Minimum Sizes

YRPayloadLengthType YRPacketSYNLength(void);
YRPayloadLengthType YRPacketRSTLength(void);
YRPayloadLengthType YRPacketNULLength(void);
YRPayloadLengthType YRPacketACKLength(void);
YRPayloadLengthType YRPacketEACKLength(YRSequenceNumberType *ioSequencesCount);
YRPayloadLengthType YRPacketEACKLengthWithPayload(YRSequenceNumberType *ioSequencesCount, YRPayloadLengthType payloadLength);
YRPayloadLengthType YRPacketLengthForPayload(YRPayloadLengthType payloadLength);

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(YRSequenceNumberType seqNumber, YRConnectionConfiguration configuration, YRSequenceNumberType ackNumber, bool hasACK);
YRPacketRef YRPacketCreateRST(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, bool hasACK);
YRPacketRef YRPacketCreateNUL(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber);
YRPacketRef YRPacketCreateACK(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber);
YRPacketRef YRPacketCreateEACK(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, YRSequenceNumberType *sequences, YRSequenceNumberType *ioSequencesCount);
YRPacketRef YRPacketCreateEACKWithPayload(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, YRSequenceNumberType *sequences,
    YRSequenceNumberType *ioSequencesCount, const void *payload, YRPayloadLengthType payloadLength);
YRPacketRef YRPacketCreateWithPayload(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, const void *payload, YRPayloadLengthType payloadLength);
// TODO: Add option to create with payload byref or by copy.
// Byref can be used to send packets unreliably.

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

// === Lazy deserialization ===
YRPacketRef YRPacketDeserializeHeader(YRLightweightInputStreamRef stream);
YRPacketRef YRPacketDeserializePayload(YRLightweightInputStreamRef stream);

YRPacketRef YRPacketDeserializeHeaderAt(YRLightweightInputStreamRef stream, void *packetBuffer);
YRPacketRef YRPacketDeserializePayloadAt(YRPacketRef packet, YRLightweightInputStreamRef stream);

#endif /* YRPacket_h */
