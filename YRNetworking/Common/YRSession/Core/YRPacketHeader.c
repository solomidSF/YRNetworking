//
//  YRPacketHeader.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/17/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRPacketHeader.h"
#include <stdlib.h>

// SYN Segment:
// 0             7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| |A| | | | | | |               |
//|1|C|0|0|0|0|0|0|       28      |
//| |K| | | | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//+  Sequence #   +   Ack Number  |
//+---------------+---------------+
//| Vers  | Spare | Max # of Out  |
//|       |       | standing Segs |
//+---------------+---------------+
//| Option Flags  |     Spare     |
//+---------------+---------------+
//|      Maximum Segment Size     |
//+---------------+---------------+
//| Retransmission Timeout Value  |
//+---------------+---------------+
//| Cumulative Ack Timeout Value  |
//+---------------+---------------+
//|   Null Segment Timeout Value  |
//+---------------+---------------+
//| Transfer State Timeout Value  |
//+---------------+---------------+
//|  Max Retrans  | Max Cum Ack   |
//+---------------+---------------+
//| Max Out of Seq| Max Auto Reset|
//+---------------+---------------+
//|    Connection Identifier      |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|           Checksum            |
//+---------------+---------------+

// ACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|0|0|0|0|0|0|       6       |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|           Checksum            |
//+---------------+---------------+

// EACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|1|0|0|0|0|0|     N + 6     |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|1st out of seq |2nd out of seq |
//|  ack number   |   ack number  |
//+---------------+---------------+
//|  . . .        |Nth out of seq |
//|               |   ack number  |
//+---------------+---------------+
//|            Checksum           |
//+---------------+---------------+

// RST segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| |A| | | | | | |               |
//|0|C|0|1|0|0|0|0|        6      |
//| |K| | | | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|         Header Checksum       |
//+---------------+---------------+

// NUL segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|0|0|1|0|0|0|       6       |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |  Ack Number   |
//+---------------+---------------+
//|            Checksum           |
//+---------------+---------------+

#pragma mark - Data Structures

#pragma pack(push, 1)

typedef struct YRPacketHeader {
    YRChecksumType checksum;
    YRPayloadLengthType payloadLength;
    YRSequenceNumberType sequenceNumber;
    YRSequenceNumberType ackNumber;
    YRPacketDescriptionType packetDescription;
    YRHeaderLengthType headerLength;
    // void *variableData;
} YRPacketHeader;

typedef struct YRPacketHeaderSYN {
    YRPacketHeader commonHeader;
    // SYN-related data
    YRConnectionConfiguration connectionConfiguration;
} YRPacketHeaderSYN;

typedef struct YRPacketHeaderEACK {
    YRPacketHeader commonHeader;
    // EACK-related data
    YRSequenceNumberType eacks[1];
} YRPacketHeaderEACK;

#pragma pack(pop)

#pragma mark - Constants

YRProtocolVersionType const kYRProtocolVersion = 0x01;

YRHeaderLengthType const kYRPacketHeaderSYNLength = sizeof(YRPacketHeaderSYN);
YRHeaderLengthType const kYRPacketHeaderGenericLength = sizeof(YRPacketHeader);

YRHeaderLengthType YRPacketHeaderEACKLength(YRSequenceNumberType *ioCount) {
    size_t eackTypeSize = sizeof(YRSequenceNumberType);
    
    YRSequenceNumberType eacksCount = ioCount ? *ioCount : 0;
    
    YRHeaderLengthType bytesTaken = kYRPacketHeaderGenericLength;
    YRHeaderLengthType bytesLeft = YRMaximumPacketHeaderSize - bytesTaken;
    
    if (eacksCount == 0) {
        return bytesTaken;
    }

    YRSequenceNumberType eacksThatFit = YRPacketHeaderEACKsCountThatFit(bytesLeft);

    if (eacksCount > eacksThatFit) {
        eacksCount = eacksThatFit;
    }
    
    if (ioCount) {
        *ioCount = eacksCount;
    }
    
    return bytesTaken + eacksCount * eackTypeSize;
}

YRSequenceNumberType YRPacketHeaderEACKsCountThatFit(YRHeaderLengthType headerLength) {
    size_t eackTypeSize = sizeof(YRSequenceNumberType);

    return headerLength / eackTypeSize;
}

#pragma mark - Configuration

void YRPacketHeaderSetPacketDescription(YRPacketHeaderRef header, YRPacketDescriptionType packetDescription) {
    header->packetDescription = packetDescription;
}

void YRPacketHeaderSetSYN(YRPacketHeaderRef header) {
    header->packetDescription |= YRPacketDescriptionSYN;
}

void YRPacketHeaderSetRST(YRPacketHeaderRef header) {
    header->packetDescription |= YRPacketDescriptionRST;
}

void YRPacketHeaderSetNUL(YRPacketHeaderRef header) {
    header->packetDescription |= YRPacketDescriptionNUL;
}

void YRPacketHeaderSetProtocolVersion(YRPacketHeaderRef header, YRProtocolVersionType version) {
    header->packetDescription |= (version << kYRProtocolVersionOffset);
}

void YRPacketHeaderSetHeaderLength(YRPacketHeaderRef header, YRHeaderLengthType length) {
    header->headerLength = length;
}

void YRPacketHeaderSetSequenceNumber(YRPacketHeaderRef header, YRSequenceNumberType seqNumber) {
    header->sequenceNumber = seqNumber;
}

void YRPacketHeaderSetAckNumber(YRPacketHeaderRef header, YRSequenceNumberType ackNumber) {
    header->packetDescription |= YRPacketDescriptionACK;
    header->ackNumber = ackNumber;
}

void YRPacketHeaderSetCHK(YRPacketHeaderRef header) {
    header->packetDescription |= YRPacketDescriptionCHK;
}

void YRPacketHeaderSetPayloadLength(YRPacketHeaderRef header, YRPayloadLengthType length) {
    header->payloadLength = length;
}

void YRPacketHeaderSetChecksum(YRPacketHeaderRef header, YRChecksumType checksum) {
    header->checksum = checksum;
}

YRPacketDescriptionType YRPacketHeaderGetPacketDescription(YRPacketHeaderRef header) {
    return header->packetDescription;
}

bool YRPacketHeaderIsSYN(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionSYN) > 0;
}

bool YRPacketHeaderIsRST(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionRST) > 0;
}

bool YRPacketHeaderIsNUL(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionNUL) > 0;
}

bool YRPacketHeaderHasACK(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionACK) > 0;
}

bool YRPacketHeaderHasEACK(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionEACK) > 0;
}

bool YRPacketHeaderHasACKOrEACK(YRPacketHeaderRef header) {
    return (header->packetDescription & (YRPacketDescriptionACK | YRPacketDescriptionEACK)) > 0;
}

bool YRPacketHeaderHasCHK(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionCHK) > 0;
}

YRProtocolVersionType YRPacketHeaderGetProtocolVersion(YRPacketHeaderRef header) {
    return (header->packetDescription & YRPacketDescriptionProtocolVersionMask) >> kYRProtocolVersionOffset;    
}

YRHeaderLengthType YRPacketHeaderGetHeaderLength(YRPacketHeaderRef header) {
    return header->headerLength;
}

YRSequenceNumberType YRPacketHeaderGetSequenceNumber(YRPacketHeaderRef header) {
    return header->sequenceNumber;
}

YRSequenceNumberType YRPacketHeaderGetAckNumber(YRPacketHeaderRef header) {
    return header->ackNumber;
}

YRPayloadLengthType YRPacketHeaderGetPayloadLength(YRPacketHeaderRef header) {
    return header->payloadLength;
}

YRChecksumType YRPacketHeaderGetChecksum(YRPacketHeaderRef header) {
    return header->checksum;
}

#pragma mark - SYN Header

void YRPacketSYNHeaderSetConfiguration(YRPacketHeaderSYNRef synHeader, YRConnectionConfiguration configuration) {
    synHeader->connectionConfiguration = configuration;
}

YRConnectionConfiguration YRPacketSYNHeaderGetConfiguration(YRPacketHeaderSYNRef synHeader) {
    return synHeader->connectionConfiguration;
}

//void YRPacketSYNHeaderSetOptions(YRPacketHeaderSYNRef synHeader, uint16_t options) {
//    synHeader->options = options;
//}
//
//void YRPacketSYNHeaderSetRetransmissionTimeout(YRPacketHeaderSYNRef synHeader, uint16_t ms) {
//    synHeader->retransmissionTimeoutValue = ms;
//}
//
//void YRPacketSYNHeaderSetNULSegmentTimeout(YRPacketHeaderSYNRef synHeader, uint16_t ms) {
//    synHeader->nullSegmentTimeoutValue = ms;
//}
//
//void YRPacketSYNHeaderSetMaximumSegmentSize(YRPacketHeaderSYNRef synHeader, uint16_t maximumSegmentSize) {
//    synHeader->maximumSegmentSize = maximumSegmentSize;
//}
//
//void YRPacketSYNHeaderSetMaximumNumberOfOutstandingSegments(YRPacketHeaderSYNRef synHeader, uint8_t maxNumberOfSegments) {
//    synHeader->maxNumberOfOutstandingSegments = maxNumberOfSegments;
//}
//
//void YRPacketSYNHeaderSetMaxRetransmissions(YRPacketHeaderSYNRef synHeader, uint8_t maxRetransmissions) {
//    synHeader->maxRetransmissions = maxRetransmissions;
//}
//
//uint16_t YRPacketSYNHeaderGetOptions(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->options;
//}
//
//uint16_t YRPacketSYNHeaderGetRetransmissionTimeout(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->retransmissionTimeoutValue;
//}
//
//uint16_t YRPacketSYNHeaderGetNULSegmentTimeout(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->nullSegmentTimeoutValue;
//}
//
//uint16_t YRPacketSYNHeaderGetMaximumSegmentSize(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->maximumSegmentSize;
//}
//
//uint8_t YRPacketSYNHeaderGetMaximumNumberOfOutstandingSegments(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->maxNumberOfOutstandingSegments;
//}
//
//uint8_t YRPacketSYNHeaderGetMaxRetransmissions(YRPacketHeaderSYNRef synHeader) {
//    return synHeader->maxRetransmissions;
//}

#pragma mark - EACK Header

void YRPacketHeaderSetEACKs(YRPacketHeaderEACKRef eackHeader, YRSequenceNumberType *eacks, YRSequenceNumberType eacksCount) {
    if (eacksCount > 0) {
        eackHeader->commonHeader.packetDescription |= YRPacketDescriptionEACK;
    }
    
    for (YRSequenceNumberType i = 0; i < eacksCount; i++) {
        eackHeader->eacks[i] = eacks[i];
    }
}

YRSequenceNumberType YRPacketHeaderEACKsCount(YRPacketHeaderEACKRef eackHeader) {
    YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(&eackHeader->commonHeader);
    
    return YRPacketHeaderEACKsCountThatFit(headerLength - kYRPacketHeaderGenericLength);
}

YRSequenceNumberType *YRPacketHeaderGetEACKs(YRPacketHeaderEACKRef eackHeader, YRSequenceNumberType *eacksCount) {
    YRSequenceNumberType resultingEACKsCount = YRPacketHeaderEACKsCount(eackHeader);
    
    if (eacksCount) {
        *eacksCount = resultingEACKsCount;
    }

    if (resultingEACKsCount > 0) {
        return eackHeader->eacks;
    } else {
        return NULL;
    }
}
