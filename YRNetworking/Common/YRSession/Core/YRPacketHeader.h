//
//  YRPacketHeader.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRPacketHeader_h
#define YRPacketHeader_h

#include <stdint.h>
#include <stdbool.h>
#include "YRConnectionConfiguration.h"

#pragma mark - Declarations

#define YRMakeMultipleTo(what, to) (((uintptr_t)(what) + ((to) - 1)) & (~((to) - 1)))

static uint8_t const kYRProtocolVersionOffset = 6;

typedef uint8_t YRPacketDescriptionType;
typedef uint8_t YRProtocolVersionType;
typedef uint8_t YRHeaderLengthType;
typedef uint16_t YRSequenceNumberType;
typedef uint16_t YRPayloadLengthType;
typedef uint32_t YRChecksumType;

// TODO: Move to another place?
extern YRProtocolVersionType const kYRProtocolVersion;

#define YRMaximumPacketHeaderSize ((YRHeaderLengthType)(~0))

extern YRHeaderLengthType const kYRPacketHeaderGenericLength;
extern YRHeaderLengthType const kYRPacketHeaderSYNLength;
extern YRHeaderLengthType const kYRPacketHeaderRSTLength;

enum YRPacketDescription {
    // Synchronization segment. Mutually exclusive with RST && NUL.
    YRPacketDescriptionSYN = 1 << 0,
    // Indicates the packet is a reset segment. Mutually exclusive with SYN && NUL.
    YRPacketDescriptionRST = 1 << 1,
    // Indicates the packet is a null segment. Mutually exclusive with SYN && RST.
    YRPacketDescriptionNUL = 1 << 2,
    // Indicates acknowledgment number in the header is valid.
    YRPacketDescriptionACK = 1 << 3,
    // Indicates extended acknowledge segment is present.
    YRPacketDescriptionEACK = 1 << 4,
    // Indicates whether the Checksum field contains the checksum of just the header or the header and the body (data).
    YRPacketDescriptionCHK = 1 << 5,
    // Version of protocol used to generate this packet.
    YRPacketDescriptionProtocolVersionMask = 0x3 << kYRProtocolVersionOffset
};

typedef struct YRPacketHeader *YRPacketHeaderRef;
typedef struct YRPacketHeaderSYN *YRPacketHeaderSYNRef;
typedef struct YRPacketHeaderRST *YRPacketHeaderRSTRef;
typedef struct YRPacketHeaderEACK *YRPacketHeaderEACKRef;

/**
 *  Determines how much bytes needed to fit given eacks. (Note: Maximum header size is 255, so on return ioCount will contain how much eacks can be set to header)
 */
YRHeaderLengthType YRPacketHeaderEACKLength(YRSequenceNumberType *ioCount);

/**
 *  Determines how much eack's can fit into given length.
 */
YRSequenceNumberType YRPacketHeaderEACKsCountThatFit(YRHeaderLengthType headerLength);

#pragma mark - Configuration

// Generic Header

void YRPacketHeaderSetPacketDescription(YRPacketHeaderRef header, YRPacketDescriptionType packetDescription);
void YRPacketHeaderSetSYN(YRPacketHeaderRef header);
void YRPacketHeaderSetRST(YRPacketHeaderRef header);
void YRPacketHeaderSetNUL(YRPacketHeaderRef header);
void YRPacketHeaderSetProtocolVersion(YRPacketHeaderRef header, YRProtocolVersionType version);
void YRPacketHeaderSetHeaderLength(YRPacketHeaderRef header, YRHeaderLengthType length);
void YRPacketHeaderSetSequenceNumber(YRPacketHeaderRef header, YRSequenceNumberType seqNumber);
void YRPacketHeaderSetAckNumber(YRPacketHeaderRef header, YRSequenceNumberType ackNumber);
void YRPacketHeaderSetCHK(YRPacketHeaderRef header);
void YRPacketHeaderSetPayloadLength(YRPacketHeaderRef header, YRPayloadLengthType length);
void YRPacketHeaderSetChecksum(YRPacketHeaderRef header, YRChecksumType checksum);

YRPacketDescriptionType YRPacketHeaderGetPacketDescription(YRPacketHeaderRef header);
bool YRPacketHeaderIsSYN(YRPacketHeaderRef header);
bool YRPacketHeaderIsRST(YRPacketHeaderRef header);
bool YRPacketHeaderIsNUL(YRPacketHeaderRef header);
bool YRPacketHeaderHasACK(YRPacketHeaderRef header);
bool YRPacketHeaderHasEACK(YRPacketHeaderRef header);
bool YRPacketHeaderHasACKOrEACK(YRPacketHeaderRef header);
bool YRPacketHeaderHasCHK(YRPacketHeaderRef header);
YRProtocolVersionType YRPacketHeaderGetProtocolVersion(YRPacketHeaderRef header);
YRHeaderLengthType YRPacketHeaderGetHeaderLength(YRPacketHeaderRef header);
YRSequenceNumberType YRPacketHeaderGetSequenceNumber(YRPacketHeaderRef header);
YRSequenceNumberType YRPacketHeaderGetAckNumber(YRPacketHeaderRef header);
YRPayloadLengthType YRPacketHeaderGetPayloadLength(YRPacketHeaderRef header);
YRChecksumType YRPacketHeaderGetChecksum(YRPacketHeaderRef header);

#pragma mark - SYN Header

void YRPacketSYNHeaderSetConfiguration(YRPacketHeaderSYNRef synHeader, YRConnectionConfiguration configuration);
YRConnectionConfiguration YRPacketSYNHeaderGetConfiguration(YRPacketHeaderSYNRef synHeader);

#pragma mark - RST Header

void YRPacketRSTHeaderSetErrorCode(YRPacketHeaderRSTRef rstHeader, uint8_t errCode);
uint8_t YRPacketRSTHeaderGetErrorCode(YRPacketHeaderRSTRef rstHeader);

#pragma mark - EACK Header

void YRPacketHeaderSetEACKs(YRPacketHeaderEACKRef eackHeader, YRSequenceNumberType *eacks, YRSequenceNumberType eacksCount);

YRSequenceNumberType YRPacketHeaderEACKsCount(YRPacketHeaderEACKRef eackHeader);
YRSequenceNumberType *YRPacketHeaderGetEACKs(YRPacketHeaderEACKRef eackHeader, YRSequenceNumberType *eacksCount);

#endif /* YRPacketHeader_h */
