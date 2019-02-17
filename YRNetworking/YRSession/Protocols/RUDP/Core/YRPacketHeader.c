//
//  YRPacketHeader.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/17/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRPacketHeader.h"
#include <stdlib.h>
#include <string.h> // for memcpy

#pragma mark - Data Structures

// TODO: Think about removing 'pack', but currently everything is ok-aligned
#pragma pack(push, 1)

typedef struct YRPacketHeader {
    YRChecksumType checksum; // 4 bytes
    YRSequenceNumberType sequenceNumber; // 2 bytes
    YRSequenceNumberType ackNumber; // 2 bytes
    YRPacketDescriptionType packetDescription; // 1 byte
    YRHeaderLengthType headerLength; // 1 byte
    // pad 2 bytes to 12 (if we remove 'pack')
    
    // void *variableData;
    
//     What if we create a union here?
//    union {
//        uint8_t errorCode; // For RST
//        YRConnectionConfiguration connectionConfiguration; // For SYN
//        YRSequenceNumberType eacks[];
//        YRPayloadLengthType payloadLength;
//    } packetSpecificData;
} YRPacketHeader;

typedef struct YRPacketHeaderSYN {
    YRPacketHeader commonHeader;
    // SYN-related data
    YRConnectionConfiguration connectionConfiguration;
} YRPacketHeaderSYN;

typedef struct YRPacketHeaderRST {
    YRPacketHeader commonHeader;
    // RST-related data
    uint8_t errorCode;
} YRPacketHeaderRST;

typedef struct YRPacketPayloadHeader {
    YRPacketHeader base;
    YRPayloadLengthType payloadLength;
} YRPacketPayloadHeader;

typedef struct YRPacketHeaderEACK {
    YRPacketPayloadHeader payloadHeader;
    // EACK-related data
    YRSequenceNumberType eacks[];
} YRPacketHeaderEACK;

typedef struct YRPacketHeaderACK {
    YRPacketPayloadHeader payloadHeader;
} YRPacketHeaderACK;

#pragma pack(pop)

#pragma mark - Constants

YRProtocolVersionType const kYRProtocolVersion = 0x01;

YRHeaderLengthType const kYRPacketHeaderGenericLength = sizeof(YRPacketHeader);
YRHeaderLengthType const kYRPacketHeaderSYNLength = sizeof(YRPacketHeaderSYN);
YRHeaderLengthType const kYRPacketHeaderRSTLength = sizeof(YRPacketHeaderRST);
YRHeaderLengthType const kYRPacketPayloadHeaderLength = sizeof(YRPacketPayloadHeader);

YRHeaderLengthType YRPacketHeaderEACKLength(YRSequenceNumberType *ioCount) {
    size_t eackTypeSize = sizeof(YRSequenceNumberType);
    
    YRSequenceNumberType eacksCount = ioCount ? *ioCount : 0;
    
    YRHeaderLengthType bytesTaken = sizeof(YRPacketHeaderEACK);
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

#pragma mark - RST Header

void YRPacketRSTHeaderSetErrorCode(YRPacketHeaderRSTRef rstHeader, uint8_t errCode) {
    rstHeader->errorCode = errCode;
}

uint8_t YRPacketRSTHeaderGetErrorCode(YRPacketHeaderRSTRef rstHeader) {
    return rstHeader->errorCode;
}

#pragma mark - Payload Header

bool YRPacketHeaderHasPayloadLength(YRPacketHeaderRef header) {
    YRPacketDescriptionType packetDescriptionWithoutProtocol = header->packetDescription & (~(0x11 << kYRProtocolVersionOffset));
    return (packetDescriptionWithoutProtocol & (~(YRPacketDescriptionACK | YRPacketDescriptionEACK | YRPacketDescriptionCHK))) == 0;
}

void YRPacketHeaderSetPayloadLength(YRPacketPayloadHeaderRef header, YRPayloadLengthType length) {
    header->payloadLength = length;
}

YRPayloadLengthType YRPacketHeaderGetPayloadLength(YRPacketPayloadHeaderRef header) {
    return header->payloadLength;
}

#pragma mark - EACK Header

void YRPacketHeaderSetEACKs(YRPacketHeaderEACKRef eackHeader, YRSequenceNumberType *eacks, YRSequenceNumberType eacksCount) {
    if (eacksCount > 0) {
        eackHeader->payloadHeader.base.packetDescription |= YRPacketDescriptionEACK;
    }
    
    memcpy(eackHeader->eacks, eacks, eacksCount * sizeof(YRSequenceNumberType));
}

YRSequenceNumberType YRPacketHeaderEACKsCount(YRPacketHeaderEACKRef eackHeader) {
    YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(&eackHeader->payloadHeader.base);
    
    return YRPacketHeaderEACKsCountThatFit(headerLength - kYRPacketPayloadHeaderLength);
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
