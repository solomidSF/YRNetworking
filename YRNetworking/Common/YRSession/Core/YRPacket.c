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
//|         Payload Length        |
//+---------------+---------------+
//|            Checksum           |
//+                               +
//|      (32 bits in length)      |
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
    YRPacketFlagIsByRef = 1 << 0,
    YRPacketFlagIsCustomlyAllocated = 1 << 1,
};

typedef uint8_t YRPacketFlags;

typedef struct YRPacket {
    YRPacketFlags flags;
    void *header;
    // padding to 64-bit boundary in case of payload;
    // payload;
} YRPacket;

#pragma mark - Prototypes

static inline YRPayloadLengthType YRPacketGenericLength(void);
static inline YRPayloadLengthType YRPacketGenericAlignedLength(void);
void YRPacketFinalize(YRPacketRef packet);
static inline YRChecksumType YRPacketCalculateChecksum(YRPacketRef packet);
void *YRPacketGetPayloadPointer(YRPacketRef packet);
void *YRPacketGetPayloadStart(YRPacketRef packet);

#pragma mark - Convenience Methods

#define kYRPacketStructureLength (sizeof(YRPacket) - sizeof(void *))

YRPayloadLengthType YRPacketGenericLength() {
    return kYRPacketHeaderGenericLength + kYRPacketStructureLength;
}

YRPayloadLengthType YRPacketGenericAlignedLength() {
    return YRMakeMultipleTo(YRPacketGenericLength(), 4);
}

YRPayloadLengthType YRPacketSYNLength() {
    return YRMakeMultipleTo((kYRPacketHeaderSYNLength + kYRPacketStructureLength), 4);
}

YRPayloadLengthType YRPacketRSTLength() {
    return YRMakeMultipleTo((kYRPacketHeaderRSTLength + kYRPacketStructureLength), 4);
}

YRPayloadLengthType YRPacketNULLength() {
    return YRPacketGenericAlignedLength();
}

YRPayloadLengthType YRPacketACKLength() {
    return YRPacketGenericAlignedLength();
}

YRPayloadLengthType YRPacketEACKLength(YRSequenceNumberType *ioSequencesCount) {
    return YRPacketEACKLengthWithPayload(ioSequencesCount, 0);
}

YRPayloadLengthType YRPacketEACKLengthWithPayload(YRSequenceNumberType *ioSequencesCount, YRPayloadLengthType payloadLength) {
    YRHeaderLengthType headerLength = YRPacketHeaderEACKLength(ioSequencesCount);

    if (payloadLength > 0) {
        return YRMakeMultipleTo(kYRPacketStructureLength + headerLength, 8) + payloadLength;
    } else {
        return YRMakeMultipleTo(kYRPacketStructureLength + headerLength, 4);
    }
}

YRPayloadLengthType YRPacketLengthForPayload(YRPayloadLengthType payloadLength) {
    if (payloadLength > 0) {
        return YRMakeMultipleTo(YRPacketGenericLength(), 8) + payloadLength;
    } else {
        return YRPacketGenericAlignedLength();
    }
}

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(YRConnectionConfiguration configuration, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, bool hasACK) {
    YRPacketRef packet = calloc(1, YRPacketSYNLength());
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)header;
    
    YRPacketHeaderSetSYN(header);
    YRPacketSYNHeaderSetConfiguration(synHeader, configuration);
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
 
    if (hasACK) {
        YRPacketHeaderSetAckNumber(header, ackNumber);
    }
    
    YRPacketHeaderSetHeaderLength(header, kYRPacketHeaderSYNLength);
    YRPacketFinalize(packet);
    
    return packet;
}

YRPacketRef YRPacketCreateRST(uint8_t errorCode, YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, bool hasACK) {
    YRPacketRef packet = calloc(1, YRPacketRSTLength());
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRPacketHeaderSetRST(header);
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    
    YRPacketHeaderRSTRef rstHeader = (YRPacketHeaderRSTRef)header;
    YRPacketRSTHeaderSetErrorCode(rstHeader, errorCode);
    
    if (hasACK) {
        YRPacketHeaderSetAckNumber(header, ackNumber);
    }
    
    YRPacketHeaderSetHeaderLength(header, kYRPacketHeaderGenericLength);
    YRPacketFinalize(packet);
    
    return packet;
}

YRPacketRef YRPacketCreateNUL(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
    YRPacketRef packet = calloc(1, YRPacketNULLength());
    YRPacketHeaderRef header = YRPacketGetHeader(packet);

    YRPacketHeaderSetNUL(header);
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    YRPacketHeaderSetAckNumber(header, ackNumber);
    YRPacketHeaderSetHeaderLength(header, kYRPacketHeaderGenericLength);
    YRPacketFinalize(packet);
    
    return packet;
}

YRPacketRef YRPacketCreateACK(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber) {
    YRPacketRef packet = calloc(1, YRPacketACKLength());
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    YRPacketHeaderSetAckNumber(header, ackNumber);
    YRPacketHeaderSetHeaderLength(header, kYRPacketHeaderGenericLength);
    YRPacketFinalize(packet);
    
    return packet;
}

YRPacketRef YRPacketCreateEACK(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, YRSequenceNumberType *sequences, YRSequenceNumberType *ioSequencesCount) {
    return YRPacketCreateEACKWithPayload(seqNumber, ackNumber, sequences, ioSequencesCount, NULL, 0);
}

YRPacketRef YRPacketCreateEACKWithPayload(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, YRSequenceNumberType *sequences,
    YRSequenceNumberType *ioSequencesCount, const void *payload, YRPayloadLengthType payloadLength) {
    YRPacketRef packet = calloc(1, YRPacketEACKLengthWithPayload(ioSequencesCount, payloadLength));
    YRPacketHeaderRef commonHeader = YRPacketGetHeader(packet);
    YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)commonHeader;
    
    YRPacketHeaderSetSequenceNumber(commonHeader, seqNumber);
    YRPacketHeaderSetAckNumber(commonHeader, ackNumber);
    YRPacketHeaderSetHeaderLength(commonHeader, YRPacketHeaderEACKLength(ioSequencesCount));
    
    if (ioSequencesCount && *ioSequencesCount > 0) {
        YRPacketHeaderSetEACKs(eackHeader, sequences, *ioSequencesCount);
    }
    
    YRPacketHeaderSetPayloadLength(commonHeader, payloadLength);
    
    if (payloadLength > 0) {
        memcpy(YRPacketGetPayloadStart(packet), payload, payloadLength);
        YRPacketHeaderSetCHK(commonHeader);
        
        // Add ability to specify to copy bytes or to set as byref
        //        YRPacketSetFlags(packet, YRPacketFlagIsByRef);
    }
    
    YRPacketFinalize(packet);
    
    return packet;
}

YRPacketRef YRPacketCreateWithPayload(YRSequenceNumberType seqNumber, YRSequenceNumberType ackNumber, const void *payload, YRPayloadLengthType payloadLength) {
    YRPacketRef packet = calloc(1, YRPacketLengthForPayload(payloadLength));
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    YRPacketHeaderSetAckNumber(header, ackNumber);
    YRPacketHeaderSetHeaderLength(header, kYRPacketHeaderGenericLength);
    YRPacketHeaderSetPayloadLength(header, payloadLength);
    
    if (payloadLength > 0) {
        memcpy(YRPacketGetPayloadStart(packet), payload, payloadLength);
        YRPacketHeaderSetCHK(header);
        
        // Add ability to specify to copy bytes or to set as byref
//        YRPacketSetFlags(packet, YRPacketFlagIsByRef);
    }
    
    YRPacketFinalize(packet);
    
    return packet;
}

void YRPacketDestroy(YRPacketRef packet) {
    // TODO: Improve
    if (packet) {
        memset(packet, 0, YRPacketGetLength(packet));
        
        free(packet);
    }
}

#pragma mark - Introspection

YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet) {
    return (YRPacketHeaderRef)(&packet->header);
}

void *YRPacketGetPayload(YRPacketRef packet, YRPayloadLengthType *outPayloadSize) {
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    YRPayloadLengthType payloadLength = YRPacketHeaderGetPayloadLength(header);
    
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
    YRHeaderLengthType payloadLength = YRPacketHeaderGetPayloadLength(header);

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
        return !YRPacketHeaderIsRST(header) && !YRPacketHeaderIsNUL(header) && (YRPacketHeaderGetPayloadLength(header) == 0) && (!YRPacketHeaderHasEACK(header));
    }
    
    if (YRPacketHeaderIsRST(header)) {
        return !YRPacketHeaderIsSYN(header) && !YRPacketHeaderIsNUL(header) && (YRPacketHeaderGetPayloadLength(header) == 0);
    }
    
    if (YRPacketHeaderIsNUL(header)) {
        if (!YRPacketHeaderHasACKOrEACK(header)) {
            return false;
        } else {
            return !YRPacketHeaderIsSYN(header) && !YRPacketHeaderIsRST(header) && (YRPacketHeaderGetPayloadLength(header) == 0);
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
    YRPayloadLengthType payloadLength = YRLightweightInputStreamReadInt16(stream);
    
    // Must always return true, because we checked that header length is not greater than stream size.
    YRLightweightInputStreamSetIndexTo(stream, headerLength);
    
    YRPayloadLengthType realPayloadLength = 0;
    YRLightweightInputSteamMemalignCurrentPointer(stream, &realPayloadLength);
    
    if (payloadLength != realPayloadLength) {
        return false;
    }
    
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
    YRPayloadLengthType payloadLength = YRPacketHeaderGetPayloadLength(header);
    YRChecksumType checksum = YRPacketHeaderGetChecksum(header);

    YRLightweightOutputStreamWriteInt8(stream, packetDescription);
    YRLightweightOutputStreamWriteInt8(stream, headerLength);
    YRLightweightOutputStreamWriteInt8(stream, seqNumber);
    YRLightweightOutputStreamWriteInt8(stream, ackNumber);
    YRLightweightOutputStreamWriteInt16(stream, payloadLength);
    YRLightweightOutputStreamWriteInt32(stream, checksum);
    
    // TODO: Packet-specific serializing
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
            YRLightweightOutputStreamWriteInt8(stream, eacks[i]);
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
    return YRPacketDeserializeAt(stream, calloc(1, 1024));
}

YRPacketRef YRPacketDeserializeAt(YRLightweightInputStreamRef stream, void *packetBuffer) {
    YRPacketRef packet = (YRPacketRef)packetBuffer;
    YRPacketHeaderRef header = YRPacketGetHeader(packet);
    
    YRLightweightInputSteamReset(stream);

    YRPacketDescriptionType packetDescription = YRLightweightInputStreamReadInt8(stream);
    YRHeaderLengthType headerLength = YRLightweightInputStreamReadInt8(stream);
    YRSequenceNumberType seqNumber = YRLightweightInputStreamReadInt8(stream);
    YRSequenceNumberType ackNumber = YRLightweightInputStreamReadInt8(stream);
    YRPayloadLengthType payloadLength = YRLightweightInputStreamReadInt16(stream);
    YRChecksumType checksum = YRLightweightInputStreamReadInt32(stream);
    
    YRPacketHeaderSetPacketDescription(header, packetDescription);
    YRPacketHeaderSetHeaderLength(header, headerLength);
    YRPacketHeaderSetSequenceNumber(header, seqNumber);
    
    if (YRPacketHeaderHasACK(header)) {
        YRPacketHeaderSetAckNumber(header, ackNumber);
    }
    
    YRPacketHeaderSetPayloadLength(header, payloadLength);
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
        YRPacketHeaderSetEACKs(eackHeader, YRLightweightInputStreamCurrentPointer(stream, NULL), eacksCount);
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
            *((uintptr_t *)(YRPacketGetPayloadStart(packet))) = (uintptr_t)rawPayloadAddress;
            
            packet->flags |= YRPacketFlagIsByRef;
        } else {
            return NULL;
        }
    }

    return packet;
}

void YRPacketCopyPayloadInline(YRPacketRef packet) {
    if (packet->flags & YRPacketFlagIsByRef) {
        void *payloadStart = YRPacketGetPayloadStart(packet);
        
        memcpy(YRPacketGetPayloadPointer(packet), payloadStart, YRPacketHeaderGetPayloadLength(YRPacketGetHeader(packet)));
        
        packet->flags &= ~YRPacketFlagIsByRef;
    }
}

#pragma mark - Private

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
    YRPayloadLengthType payloadLength = YRPacketHeaderGetPayloadLength(header);
    
    if (YRPacketHeaderHasCHK(header)) {
        // Iterate through payload
        for (YRChecksumType *iterator = YRPacketGetPayloadStart(packet);
             iterator < (YRChecksumType *)((uint8_t *)payloadStart + payloadLength);
             iterator++) {
            sum += *iterator;
        }
    }
    
    // TODO: Make more independent of type size
    while (sum >> 32) {
        sum = (sum & 0xFFFFFFFF) + (sum >> 32);
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
    
    if (packet->flags & YRPacketFlagIsByRef) {
        return (void *)(*payloadPointer);
    } else {
        return payloadPointer;
    }
}
