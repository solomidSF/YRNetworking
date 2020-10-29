//
// YRPacketHeader.c
//
// The MIT License (MIT)
//
// Copyright (c) 2020 Yurii Romanchenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "YRInternal.h"

typedef struct YRPacketHeader {
	YRChecksumType checksum; // 4 bytes
	YRSequenceNumberType sequenceNumber; // 2 bytes
	YRSequenceNumberType ackNumber; // 2 bytes
	YRPacketDescriptionType packetDescription; // 1 byte
	YRHeaderLengthType headerLength; // 1 byte
	YRProtocolVersionType protocolVersion; // 1 byte
	uint8_t reserved;
} YRPacketHeader;

typedef struct YRPacketHeaderSYN {
	YRPacketHeader base; // 12 bytes
	// SYN-related data
	YRRUDPConnectionConfiguration connectionConfiguration; // 10 bytes
	// 2 bytes padding
} YRPacketHeaderSYN;

typedef struct YRPacketHeaderRST {
	YRPacketHeader base;
	// RST-related data
} YRPacketHeaderRST;

typedef struct YRPacketPayloadHeader {
	YRPacketHeader base;
	YRPayloadLengthType payloadLength;
} YRPacketPayloadHeader;

typedef struct YRPacketHeaderEACK {
	YRPacketPayloadHeader payloadHeader;
	// EACK-related data
	YRHeaderLengthType eacksCount;
	YRSequenceNumberType eacks[];
} YRPacketHeaderEACK;

#pragma mark - Constants

YRProtocolVersionType const kYRProtocolVersion = 0x01;

size_t const kYRPacketHeaderMaxDataStructureLength =
	sizeof(YRPacketHeaderEACK) + YRMaxPacketHeaderSize;

#pragma mark - Common

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
	header->protocolVersion = version;
}

void YRPacketHeaderSetReserved(YRPacketHeaderRef header, uint8_t reserved) {
	header->reserved = reserved;
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
	return header->protocolVersion;
}

uint8_t YRPacketHeaderGetReserved(YRPacketHeaderRef header) {
	return header->reserved;
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

YRPacketHeaderRef YRPacketHeaderSYNGetBaseHeader(YRPacketHeaderSYNRef synHeader) {
	return &synHeader->base;
}

void YRPacketHeaderSYNSetConfiguration(YRPacketHeaderSYNRef synHeader, YRRUDPConnectionConfiguration configuration) {
	YRPacketHeaderSetSYN(YRPacketHeaderSYNGetBaseHeader(synHeader));
    synHeader->connectionConfiguration = configuration;
}

YRRUDPConnectionConfiguration YRPacketHeaderSYNGetConfiguration(YRPacketHeaderSYNRef synHeader) {
    return synHeader->connectionConfiguration;
}

#pragma mark - RST Header

YRPacketHeaderRef YRPacketHeaderRSTGetBaseHeader(YRPacketHeaderRSTRef rstHeader) {
	return &rstHeader->base;
}

void YRPacketHeaderRSTSetErrorCode(YRPacketHeaderRSTRef rstHeader, YRRUDPError errCode) {
	YRPacketHeaderSetRST(YRPacketHeaderRSTGetBaseHeader(rstHeader));
	YRPacketHeaderSetReserved(&rstHeader->base, errCode);
}

YRRUDPError YRPacketHeaderRSTGetErrorCode(YRPacketHeaderRSTRef rstHeader) {
	return YRPacketHeaderGetReserved(&rstHeader->base);
}

#pragma mark - Payload Header

bool YRPacketHeaderHasPayloadLength(YRPacketHeaderRef header) {
    return (header->packetDescription & (~(YRPacketDescriptionACK | YRPacketDescriptionEACK | YRPacketDescriptionCHK))) == 0;
}

YRPacketHeaderRef YRPacketPayloadHeaderGetBaseHeader(YRPacketPayloadHeaderRef header) {
	return &header->base;
}

void YRPacketPayloadHeaderSetPayloadLength(YRPacketPayloadHeaderRef header, YRPayloadLengthType length) {
    header->payloadLength = length;
}

YRPayloadLengthType YRPacketPayloadHeaderGetPayloadLength(YRPacketPayloadHeaderRef header) {
    return header->payloadLength;
}

#pragma mark - EACK Header

YRPacketPayloadHeaderRef YRPacketHeaderEACKGetPayloadHeader(YRPacketHeaderEACKRef eackHeader) {
	return &eackHeader->payloadHeader;
}

YRPacketHeaderRef YRPacketHeaderEACKGetBaseHeader(YRPacketHeaderEACKRef eackHeader) {
	return YRPacketPayloadHeaderGetBaseHeader(YRPacketHeaderEACKGetPayloadHeader(eackHeader));
}

void YRPacketHeaderEACKSetEACKs(
	YRPacketHeaderEACKRef eackHeader,
	YRSequenceNumberType *eacks,
	YRHeaderLengthType eacksCount
) {
    if (eacksCount > 0) {
        eackHeader->payloadHeader.base.packetDescription |= YRPacketDescriptionEACK;
    }
    
    eackHeader->eacksCount = eacksCount;
    memcpy(eackHeader->eacks, eacks, eacksCount * sizeof(YRSequenceNumberType));
}

YRSequenceNumberType *YRPacketHeaderEACKGetEACKs(
	YRPacketHeaderEACKRef eackHeader,
	YRHeaderLengthType *eacksCount
) {
    if (eacksCount) {
        *eacksCount = eackHeader->eacksCount;
    }

    if (eackHeader->eacksCount > 0) {
        return eackHeader->eacks;
    } else {
        return NULL;
    }
}

#pragma mark - Utility

void YRPacketHeaderDebugInfo(char *buffer) {
	int offset = 0;

#define WRITE_BUF(s) \
	offset += sprintf(buffer + offset, s"\n")

#define WRITE_BUF_A(s, ...) \
	offset += sprintf(buffer + offset, s"\n", __VA_ARGS__)

	WRITE_BUF("<YRPacketHeader>");
	WRITE_BUF_A("Protocol version: %d", kYRProtocolVersion);
	WRITE_BUF("Data structure sizes:");
	WRITE_BUF_A("\tYRPacketHeader: %zu", sizeof(YRPacketHeader));
	WRITE_BUF_A("\tYRPacketHeaderSYN: %zu", sizeof(YRPacketHeaderSYN));
	WRITE_BUF_A("\tYRPacketHeaderRST: %zu", sizeof(YRPacketHeaderRST));
	WRITE_BUF_A("\tYRPacketHeaderNUL: %zu", sizeof(YRPacketHeader));
	WRITE_BUF_A("\tYRPacketPayloadHeader: %zu", sizeof(YRPacketPayloadHeader));
	WRITE_BUF_A("\tYRPacketHeaderEACK: %zu", sizeof(YRPacketHeaderEACK));
	WRITE_BUF_A("\tMax data structure length: %zu", kYRPacketHeaderMaxDataStructureLength);
}
