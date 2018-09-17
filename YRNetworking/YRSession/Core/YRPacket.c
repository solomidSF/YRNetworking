//
//  YRPacket.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/21/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRPacket.h"
#include <stdlib.h>
#include <string.h> // For memcpy

// Network Packets Layout:

// SYN Segment:
// 0             7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| | | |A| | | | |               |
//|1|0|0|C|0|0|0|0|       20      |
//| | | |K| | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|            Checksum           |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|         Options (zero)        |
//+---------------+---------------+
//| Retransmission Timeout Value  |
//+---------------+---------------+
//|   Null Segment Timeout Value  |
//+---------------+---------------+
//|      Maximum Segment Size     |
//+---------------+---------------+
//|  Max Retrans  | Max Out of Seq|
//+---------------+---------------+

// RST segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| | | |A| | | | |               |
//|0|1|0|C|0|0|0|0|       11      |
//| | | |K| | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|         Header Checksum       |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|   Error Code  |   Alignment   |
//+---------------+---------------+

// NUL segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|0|1|1|0|0|0|0|       10      |
//+-+-+-+-+-+-+-+-+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|            Checksum           |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+

// ACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|0|0|1|0|0|0|0|      12       |
//+-+-+-+-+-+-+-+-+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|         Payload Length        |
//+---------------+---------------+
//|            Checksum           |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|           Alignment           |
//+                               +
//|     (32 bits to match 64)     |
//+---------------+---------------+
//|            Payload            |
//+---------------+---------------+

// EACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|0|0|1|1|0|0|0|   2 * N + 12  |
//+-+-+-+-+-+-+-+-+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|            Checksum           |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|         Payload Length        |
//+---------------+---------------+
//|   1st out of seq ack number   |
//+---------------+---------------+
//|             . . .             |
//+---------------+---------------+
//|   Nth out of seq ack number   |
//+---------------+---------------+
//|           Alignment           |
//+                               +
//|         (to match 64)         |
//+---------------+---------------+
//|            Payload            |
//+---------------+---------------+

enum YRPacketFlags {
    // If set - payload start in packet actually is a pointer to real payload.
    YRPacketFlagPayloadIsByRef = 1 << 0,
    YRPacketFlagIsCustomlyAllocated = 1 << 1,
};

typedef uint8_t YRPacketFlags;

typedef struct YRPacket {
    YRPacketFlags flags;
    void *header;
    // padding to 64-bit boundary in case of payload;
    // payload;
} YRPacket;

static const uint8_t kYRAlignmentWithPayloadInBytes = 8;
static const uint8_t kYRAlignmentWithoutPayloadInBytes = 4;

#pragma mark - Prototypes

static inline YRPayloadLengthType YRPacketGenericLength(void);
static inline YRPayloadLengthType YRPacketGenericAlignedLength(void);

static inline YRPacketRef YRPacketConstruct(void *whereAt,
                                            size_t packetSize,
                                            YRSequenceNumberType seqNumber,
                                            YRSequenceNumberType ackNumber,
                                            bool hasACK,
                                            YRHeaderLengthType headerLength,
                                            void (^packetSpecificConstructor) (YRPacketRef packet, YRPacketHeaderRef header));

void YRPacketFinalize(YRPacketRef packet);
static inline YRChecksumType YRPacketCalculateChecksum(YRPacketRef packet);
void *YRPacketGetPayloadPointer(YRPacketRef packet);
void *YRPacketGetPayloadStart(YRPacketRef packet);

#pragma mark - Data Structure Sizes

#define kYRPacketStructureLength (sizeof(YRPacket) - sizeof(void *))

YRPayloadLengthType YRPacketGenericLength() {
    return kYRPacketHeaderGenericLength + kYRPacketStructureLength;
}

YRPayloadLengthType YRPacketWithPayloadLength() {
    return kYRPacketPayloadHeaderLength + kYRPacketStructureLength;
}

YRPayloadLengthType YRPacketGenericAlignedLength() {
    return YRMakeMultipleTo(YRPacketGenericLength(), kYRAlignmentWithoutPayloadInBytes);
}

YRPayloadLengthType YRPacketWithPayloadAlignedLength() {
    return YRMakeMultipleTo(YRPacketWithPayloadLength(), kYRAlignmentWithoutPayloadInBytes);
}

YRPayloadLengthType YRPacketSYNLength() {
    return YRMakeMultipleTo((kYRPacketHeaderSYNLength + kYRPacketStructureLength), kYRAlignmentWithoutPayloadInBytes);
}

YRPayloadLengthType YRPacketRSTLength() {
    return YRMakeMultipleTo((kYRPacketHeaderRSTLength + kYRPacketStructureLength), kYRAlignmentWithoutPayloadInBytes);
}

YRPayloadLengthType YRPacketNULLength() {
    return YRPacketGenericAlignedLength();
}

YRPayloadLengthType YRPacketACKLength() {
    return YRPacketWithPayloadAlignedLength();
}

YRPayloadLengthType YRPacketEACKLength(YRSequenceNumberType *ioSequencesCount) {
    return YRPacketEACKLengthWithPayload(ioSequencesCount, 0);
}

YRPayloadLengthType YRPacketEACKLengthWithPayload(YRSequenceNumberType *ioSequencesCount, YRPayloadLengthType payloadLength) {
    YRHeaderLengthType headerLength = YRPacketHeaderEACKLength(ioSequencesCount);

    if (payloadLength > 0) {
        return YRMakeMultipleTo(kYRPacketStructureLength + headerLength, kYRAlignmentWithPayloadInBytes) + payloadLength;
    } else {
        return YRMakeMultipleTo(kYRPacketStructureLength + headerLength, kYRAlignmentWithoutPayloadInBytes);
    }
}

YRPayloadLengthType YRPacketLengthForPayload(YRPayloadLengthType payloadLength) {
    if (payloadLength > 0) {
        return YRMakeMultipleTo(YRPacketWithPayloadLength(), kYRAlignmentWithPayloadInBytes) + payloadLength;
    } else {
        return YRPacketWithPayloadAlignedLength();
    }
}

size_t YRPacketDataStructureLengthForPacketSize(YRPayloadLengthType packetSize) {
    size_t maximumBytesThatPacketCanTake = packetSize + kYRPacketStructureLength + (kYRAlignmentWithoutPayloadInBytes - 1);
    
    return YRMakeMultipleTo(maximumBytesThatPacketCanTake, kYRAlignmentWithPayloadInBytes);
}

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(YRConnectionConfiguration configuration,
                              YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              bool hasACK,
                              void *packetBuffer) {
    return YRPacketConstruct(packetBuffer, YRPacketSYNLength(), seqNumber, ackNumber, hasACK,
        kYRPacketHeaderSYNLength, ^(YRPacketRef packet, YRPacketHeaderRef header) {
            YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)header;
            
            YRPacketHeaderSetSYN(header);
            YRPacketSYNHeaderSetConfiguration(synHeader, configuration);
        });
}

YRPacketRef YRPacketCreateRST(uint8_t errorCode,
                              YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              bool hasACK,
                              void *packetBuffer) {
    return YRPacketConstruct(packetBuffer, YRPacketRSTLength(), seqNumber, ackNumber, hasACK,
        kYRPacketHeaderRSTLength, ^(YRPacketRef packet, YRPacketHeaderRef header) {
            YRPacketHeaderRSTRef rstHeader = (YRPacketHeaderRSTRef)header;

            YRPacketHeaderSetRST(header);
            YRPacketRSTHeaderSetErrorCode(rstHeader, errorCode);
        });
}

YRPacketRef YRPacketCreateNUL(YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              void *packetBuffer) {
    return YRPacketConstruct(packetBuffer, YRPacketNULLength(), seqNumber, ackNumber, true,
        kYRPacketHeaderGenericLength, ^(YRPacketRef packet, YRPacketHeaderRef header) {
            YRPacketHeaderSetNUL(header);
        });
}

YRPacketRef YRPacketCreateACK(YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              void *packetBuffer) {
    return YRPacketConstruct(packetBuffer, YRPacketACKLength(), seqNumber, ackNumber, true,
        kYRPacketPayloadHeaderLength, NULL);
}

YRPacketRef YRPacketCreateEACK(YRSequenceNumberType seqNumber,
                               YRSequenceNumberType ackNumber,
                               YRSequenceNumberType *sequences,
                               YRSequenceNumberType *ioSequencesCount,
                               void *packetBuffer) {
    return YRPacketCreateEACKWithPayload(seqNumber, ackNumber, sequences, ioSequencesCount, NULL, 0, false, packetBuffer);
}

YRPacketRef YRPacketCreateEACKWithPayload(YRSequenceNumberType seqNumber,
                                          YRSequenceNumberType ackNumber,
                                          YRSequenceNumberType *sequences,
                                          YRSequenceNumberType *ioSequencesCount,
                                          const void *payload,
                                          YRPayloadLengthType payloadLength,
                                          bool copyPayload,
                                          void *packetBuffer) {
    size_t packetSize = YRPacketEACKLengthWithPayload(ioSequencesCount, payloadLength);
    YRHeaderLengthType headerSize = YRPacketHeaderEACKLength(ioSequencesCount);
    
    return YRPacketConstruct(packetBuffer, packetSize, seqNumber,
        ackNumber, true, headerSize, ^(YRPacketRef packet, YRPacketHeaderRef header) {
            YRPacketPayloadHeaderRef payloadHeader = (YRPacketPayloadHeaderRef)header;
            YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)header;

            if (ioSequencesCount && *ioSequencesCount > 0) {
                YRPacketHeaderSetEACKs(eackHeader, sequences, *ioSequencesCount);
            }

            YRPacketHeaderSetPayloadLength(payloadHeader, payloadLength);

            if (payloadLength > 0) {
                YRPacketHeaderSetCHK(header);

                if (copyPayload) {
                    memcpy(YRPacketGetPayloadPointer(packet), payload, payloadLength);
                } else {
                    *((uintptr_t *)(YRPacketGetPayloadPointer(packet))) = (uintptr_t)payload;

                    packet->flags |= YRPacketFlagPayloadIsByRef;
                }
            }
        });
}

YRPacketRef YRPacketCreateWithPayload(YRSequenceNumberType seqNumber,
                                      YRSequenceNumberType ackNumber,
                                      const void *payload,
                                      YRPayloadLengthType payloadLength,
                                      bool copyPayload,
                                      void *packetBuffer) {
    size_t packetSize = YRPacketLengthForPayload(payloadLength);

    return YRPacketConstruct(packetBuffer, packetSize, seqNumber, ackNumber, true,
        kYRPacketPayloadHeaderLength, ^(YRPacketRef packet, YRPacketHeaderRef header) {
            YRPacketPayloadHeaderRef payloadHeader = (YRPacketPayloadHeaderRef)header;
            
            YRPacketHeaderSetPayloadLength(payloadHeader, payloadLength);

            if (payloadLength > 0) {
                YRPacketHeaderSetCHK(header);
                
                if (copyPayload) {
                    memcpy(YRPacketGetPayloadPointer(packet), payload, payloadLength);
                } else {
                    *((uintptr_t *)(YRPacketGetPayloadPointer(packet))) = (uintptr_t)payload;

                    packet->flags |= YRPacketFlagPayloadIsByRef;
                }
            }
        });
}

void YRPacketDestroy(YRPacketRef packet) {
    // TODO: Improve
    if (packet) {
//        memset(packet, 0, YRPacketGetLength(packet));
        
        if (!(packet->flags & YRPacketFlagIsCustomlyAllocated)) {
            // Not freeing memory if packet was allocated customly.
            free(packet);
        }
    }
}

#pragma mark - Introspection

YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet) {
    return (YRPacketHeaderRef)(&packet->header);
}

void *YRPacketGetPayload(YRPacketRef packet, YRPayloadLengthType *outPayloadSize) {
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRPayloadLengthType payloadLength = 0;
    
    if (YRPacketHeaderHasPayloadLength(header)) {
        payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header);
    }
    
    if (outPayloadSize) {
        *outPayloadSize = payloadLength;
    }
    
    if (payloadLength > 0) {
        return YRPacketGetPayloadStart(packet);
    } else {
        return NULL;
    }
}

// TODO: Data structure length
YRPayloadLengthType YRPacketGetDataStructureLength(YRPacketRef packet) {
    return 0;
}

YRPayloadLengthType YRPacketGetLength(YRPacketRef packet) {
    // TODO: This is network length
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
    YRHeaderLengthType payloadLength = 0;
    
    if (YRPacketHeaderHasPayloadLength(header)) {
        payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header);
    }
    
    if (payloadLength > 0) {
        return YRMakeMultipleTo(headerLength, 8) + payloadLength;
    } else {
        return headerLength;
    }
}

bool YRPacketIsLogicallyValid(YRPacketRef packet) {
    // TODO: Return error codes
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    if (YRPacketHeaderGetProtocolVersion(header) != kYRProtocolVersion) {
        return false;
    }
    
    if (YRPacketHeaderIsSYN(header)) {
        // TODO: Additional check for SYN header
        return !YRPacketHeaderIsRST(header) && !YRPacketHeaderIsNUL(header) && (!YRPacketHeaderHasEACK(header));
    }
    
    if (YRPacketHeaderIsRST(header)) {
        return !YRPacketHeaderIsSYN(header) && !YRPacketHeaderIsNUL(header) && (!YRPacketHeaderHasEACK(header));
    }
    
    if (YRPacketHeaderIsNUL(header)) {
        if (!YRPacketHeaderHasACKOrEACK(header)) {
            return false;
        } else {
            return !YRPacketHeaderIsSYN(header) && !YRPacketHeaderIsRST(header);
        }
    }
    
    if (YRPacketCalculateChecksum(packet) != 0) {
        return false;
    }

    return true;
}

bool YRPacketCanDeserializeFromStream(YRLightweightInputStreamRef stream) {
    YRLightweightInputSteamReset(stream);
    
    // 1. Check if protocol version matches.
    YRPacketDescriptionType packetDescription = YRLightweightInputStreamReadInt8(stream);
    uint8_t protocolVersion = (packetDescription & YRPacketDescriptionProtocolVersionMask) >> kYRProtocolVersionOffset;
    
    if (protocolVersion != kYRProtocolVersion) {
        return false;
    }
    
    // 2. Check if header length is consistent.
    YRHeaderLengthType headerLength = YRLightweightInputStreamReadInt8(stream);
    
    if (headerLength < kYRPacketHeaderGenericLength ||
        headerLength > YRLightweightInputStreamSize(stream)) {
        return false;
    }
    
    // Seq# && Ack#
    YRLightweightInputStreamAdvanceBy(stream, sizeof(YRSequenceNumberType) * 2);
    
    // TODO: Packet-specific validation??
    
    // 3. Check if payload length matches.
    // TODO: payload length now not contained in every packet
//    YRPayloadLengthType payloadLength = YRLightweightInputStreamReadInt16(stream);
//
//    // Must always return true, because we checked that header length is not greater than stream size.
//    YRLightweightInputStreamSetIndexTo(stream, headerLength);
//
//    YRPayloadLengthType realPayloadLength = 0;
//    YRLightweightInputSteamMemalignCurrentPointer(stream, &realPayloadLength);
//
//    if (payloadLength != realPayloadLength) {
//        return false;
//    }
    
    return true;
}

#pragma mark - Serialization

void YRPacketSerialize(YRPacketRef packet, YRLightweightOutputStreamRef stream) {
    // Serialize header.
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRPacketDescriptionType packetDescription = YRPacketHeaderGetPacketDescription(header);
    YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
    YRSequenceNumberType seqNumber = YRPacketHeaderGetSequenceNumber(header);
    YRSequenceNumberType ackNumber = YRPacketHeaderGetAckNumber(header);
    YRChecksumType checksum = YRPacketHeaderGetChecksum(header);

    YRLightweightOutputStreamWriteInt8(stream, packetDescription);
    YRLightweightOutputStreamWriteInt8(stream, headerLength);
    YRLightweightOutputStreamWriteInt16(stream, seqNumber);
    YRLightweightOutputStreamWriteInt16(stream, ackNumber);
    YRLightweightOutputStreamWriteInt32(stream, checksum);
    
    YRPayloadLengthType payloadLength = 0;

    // TODO: Packet-specific serializing
    if (YRPacketHeaderHasPayloadLength(header)) {
        payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header);
        
        YRLightweightOutputStreamWriteInt16(stream, payloadLength);
    }

    if (YRPacketHeaderIsSYN(header)) {
        YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)header;
        YRConnectionConfiguration configuration = YRPacketSYNHeaderGetConfiguration(synHeader);
        
        YRLightweightOutputStreamWriteInt16(stream, configuration.options);
        YRLightweightOutputStreamWriteInt16(stream, configuration.retransmissionTimeoutValue);
        YRLightweightOutputStreamWriteInt16(stream, configuration.maximumSegmentSize);
        YRLightweightOutputStreamWriteInt8(stream, configuration.maxNumberOfOutstandingSegments);
        YRLightweightOutputStreamWriteInt8(stream, configuration.maxRetransmissions);
    } else if (YRPacketHeaderHasEACK(header)) {
        YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)header;
        YRSequenceNumberType eacksCount = 0;
        YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(eackHeader, &eacksCount);
        
        for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
            YRLightweightOutputStreamWriteInt16(stream, eacks[i]);
        }
    } else if (YRPacketHeaderIsRST(header)) {
        YRPacketHeaderRSTRef rstHeader = (YRPacketHeaderRSTRef)header;
        
        YRLightweightOutputStreamWriteInt8(stream, YRPacketRSTHeaderGetErrorCode(rstHeader));
    }
    
    if (payloadLength > 0) {
        // Serialize data.
        YRLightweightOutputStreamMemalignWriteBytes(stream, YRPacketGetPayloadStart(packet), payloadLength);
    }
}

YRPacketRef YRPacketDeserialize(YRLightweightInputStreamRef stream) {
//    size_t requiredSize = YRPacketDataStructureLengthForPacketSize(YRLightweightInputStreamSize(stream));
    return YRPacketDeserializeAt(stream, calloc(1, 1024));
//    return YRPacketDeserializeAt(stream, calloc(1, YRLightweightInputStreamSize(stream)));
}

YRPacketRef YRPacketDeserializeAt(YRLightweightInputStreamRef stream, void *packetBuffer) {
    YRPacketRef packet = (YRPacketRef)packetBuffer;
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRLightweightInputSteamReset(stream);

    YRPacketDescriptionType packetDescription = YRLightweightInputStreamReadInt8(stream);
    YRHeaderLengthType headerLength = YRLightweightInputStreamReadInt8(stream);
    YRSequenceNumberType seqNumber = YRLightweightInputStreamReadInt16(stream);
    YRSequenceNumberType ackNumber = YRLightweightInputStreamReadInt16(stream);
    YRChecksumType checksum = YRLightweightInputStreamReadInt32(stream);
    
    YRPacketHeaderSetPacketDescription(header, packetDescription);
    YRPacketHeaderSetHeaderLength(header, headerLength);
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    
    if (YRPacketHeaderHasACK(header)) {
        YRPacketHeaderSetAckNumber(header, ackNumber);
    }
  
    YRPayloadLengthType payloadLength = 0;
    
    if (YRPacketHeaderHasPayloadLength(header)) {
        payloadLength = YRLightweightInputStreamReadInt16(stream);
        
        YRPacketHeaderSetPayloadLength((YRPacketPayloadHeaderRef)header, payloadLength);
    }
    
    YRPacketHeaderSetChecksum(header, checksum);
    
    // TODO: Packet-specific parsing.
    if (YRPacketHeaderIsSYN(header)) {
        // Stream currently points in SYN area.
        YRConnectionConfiguration configuration = {0};
        
        configuration.options = YRLightweightInputStreamReadInt16(stream);
        configuration.retransmissionTimeoutValue = YRLightweightInputStreamReadInt16(stream);
        configuration.maximumSegmentSize = configuration.nullSegmentTimeoutValue = YRLightweightInputStreamReadInt16(stream);
        configuration.maxNumberOfOutstandingSegments = YRLightweightInputStreamReadInt8(stream);
        configuration.maxRetransmissions = YRLightweightInputStreamReadInt8(stream);
        
        YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)header;

        YRPacketSYNHeaderSetConfiguration(synHeader, configuration);
    } else if (YRPacketHeaderIsRST(header)) {
        YRPacketHeaderRSTRef rstHeader = (YRPacketHeaderRSTRef)header;
        
        YRPacketRSTHeaderSetErrorCode(rstHeader, YRLightweightInputStreamReadInt8(stream));
    } else if (YRPacketHeaderHasEACK(header)) {
        // Stream currently points in eack area.
        YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)header;
        YRSequenceNumberType eacksCount = YRPacketHeaderEACKsCountThatFit(headerLength - YRLightweightInputStreamCurrentIndex(stream));
        YRSequenceNumberType eacks[eacksCount];
        
        for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
            eacks[i] = YRLightweightInputStreamReadInt16(stream);
        }

        YRPacketHeaderSetEACKs(eackHeader, eacks, eacksCount);
    }

    if (payloadLength > 0) {
        if (!YRLightweightInputStreamSetIndexTo(stream, headerLength)) {
            // Failed to advance index to payload area while header reports that we have payload.
            return NULL;
        }
        
        YRPayloadLengthType realPayloadLengthLeft = 0;
        uintptr_t *rawPayloadAddress = YRLightweightInputSteamMemalignCurrentPointer(stream, &realPayloadLengthLeft);
        
        if (realPayloadLengthLeft == payloadLength) {
            // Lazily set payload pointer 
            *((uintptr_t *)(YRPacketGetPayloadPointer(packet))) = (uintptr_t)rawPayloadAddress;
            
            packet->flags |= YRPacketFlagPayloadIsByRef;
        } else {
            return NULL;
        }
    }

    return packet;
}

void YRPacketCopyPayloadInline(YRPacketRef packet) {
    if (packet->flags & YRPacketFlagPayloadIsByRef) {
        void *payloadStart = YRPacketGetPayloadStart(packet);
        
        YRPayloadLengthType payloadLength = 0;
        
        YRPacketHeaderRef header = YRPacketGetHeader(packet);
        
        if (YRPacketHeaderHasPayloadLength(header)) {
            payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header);
        }
        
        memcpy(YRPacketGetPayloadPointer(packet), payloadStart, payloadLength);
        
        packet->flags &= ~YRPacketFlagPayloadIsByRef;
    }
}

#pragma mark - Private

YRPacketRef YRPacketConstruct(void *whereAt,
                              size_t packetSize,
                              YRSequenceNumberType seqNumber,
                              YRSequenceNumberType ackNumber,
                              bool hasACK,
                              YRHeaderLengthType headerLength,
                              void (^packetSpecificConstructor) (YRPacketRef packet, YRPacketHeaderRef header)) {
    YRPacketRef packet = whereAt ? (YRPacketRef)whereAt : calloc(1, packetSize);
    
    if (whereAt) {
        memset(packet, 0, packetSize);
    }
    
    YRPacketHeaderRef header = YRPacketGetHeader(packet);

    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    
    if (hasACK) {
        YRPacketHeaderSetAckNumber(header, ackNumber);
    }
    
    YRPacketHeaderSetHeaderLength(header, headerLength);

    !packetSpecificConstructor ?: packetSpecificConstructor(packet, header);
    
    YRPacketFinalize(packet);

    if (whereAt) {
        packet->flags |= YRPacketFlagIsCustomlyAllocated;
    }
    
    return packet;
}

void YRPacketFinalize(YRPacketRef packet) {
    YRPacketHeaderSetProtocolVersion(YRPacketGetHeader(packet), kYRProtocolVersion);
    YRPacketHeaderSetChecksum(YRPacketGetHeader(packet), YRPacketCalculateChecksum(packet));
}

inline YRChecksumType YRPacketCalculateChecksum(YRPacketRef packet) {
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
    
    YRChecksumType resultingChecksum = 0;
    uint64_t sum = 0;
    
    // Iterate through header
    for (YRChecksumType *iterator = (YRChecksumType *)header;
         iterator < (YRChecksumType *)((uint8_t *)header + headerLength);
         iterator++) {
        sum += *iterator;
    }

    void *payloadStart = YRPacketGetPayloadStart(packet);
    YRPayloadLengthType payloadLength = 0;
    
    if (YRPacketHeaderHasPayloadLength(header)) {
        payloadLength = YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header);
    }
    
    if (YRPacketHeaderHasCHK(header)) {
        // Iterate through payload
        for (YRChecksumType *iterator = YRPacketGetPayloadStart(packet);
             iterator < (YRChecksumType *)((uint8_t *)payloadStart + payloadLength);
             iterator++) {
            sum += *iterator;
        }
    }
    
    // TODO: Make more independent of type size
    while (sum >> (sizeof(YRChecksumType) * 8)) {
        sum = (sum & (YRChecksumType)(~0)) + (sum >> (sizeof(YRChecksumType) * 8));
    }
    
    resultingChecksum = (YRChecksumType)(~sum);
    
    return resultingChecksum;
}

void *YRPacketGetPayloadPointer(YRPacketRef packet) {
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    YRPayloadLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
    
    return (void *)YRMakeMultipleTo((uint8_t *)header + headerLength, 8);
}

void *YRPacketGetPayloadStart(YRPacketRef packet) {
    uintptr_t *payloadPointer = YRPacketGetPayloadPointer(packet);
    
    if (packet->flags & YRPacketFlagPayloadIsByRef) {
        return (void *)(*payloadPointer);
    } else {
        return payloadPointer;
    }
}
