//
// YRPacketHeader.h
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

#ifndef __YRPacketHeader__
#define __YRPacketHeader__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

typedef struct YRPacketHeader *YRPacketHeaderRef;
typedef struct YRPacketHeaderSYN *YRPacketHeaderSYNRef;
typedef struct YRPacketHeaderRST *YRPacketHeaderRSTRef;
typedef struct YRPacketPayloadHeader *YRPacketPayloadHeaderRef;
typedef struct YRPacketHeaderEACK *YRPacketHeaderEACKRef;

extern YRProtocolVersionType const kYRProtocolVersion;

#define YRMaxPacketHeaderSize ((YRHeaderLengthType)(~0) + 1)

// Max possible length for packet header internal data structure.
extern size_t const kYRPacketHeaderMaxDataStructureLength;

#pragma mark - Base Header

void YRPacketHeaderSetPacketDescription(YRPacketHeaderRef header, YRPacketDescriptionType packetDescription);
void YRPacketHeaderSetSYN(YRPacketHeaderRef header);
void YRPacketHeaderSetRST(YRPacketHeaderRef header);
void YRPacketHeaderSetNUL(YRPacketHeaderRef header);
void YRPacketHeaderSetCHK(YRPacketHeaderRef header);
void YRPacketHeaderSetProtocolVersion(YRPacketHeaderRef header, YRProtocolVersionType version);
void YRPacketHeaderSetReserved(YRPacketHeaderRef header, uint8_t reserved);
void YRPacketHeaderSetHeaderLength(YRPacketHeaderRef header, YRHeaderLengthType length);
void YRPacketHeaderSetSequenceNumber(YRPacketHeaderRef header, YRSequenceNumberType seqNumber);
void YRPacketHeaderSetAckNumber(YRPacketHeaderRef header, YRSequenceNumberType ackNumber);
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
uint8_t YRPacketHeaderGetReserved(YRPacketHeaderRef header);
YRHeaderLengthType YRPacketHeaderGetHeaderLength(YRPacketHeaderRef header);
YRSequenceNumberType YRPacketHeaderGetSequenceNumber(YRPacketHeaderRef header);
YRSequenceNumberType YRPacketHeaderGetAckNumber(YRPacketHeaderRef header);
YRChecksumType YRPacketHeaderGetChecksum(YRPacketHeaderRef header);

#pragma mark - SYN Header

YRPacketHeaderRef YRPacketHeaderSYNGetBaseHeader(YRPacketHeaderSYNRef synHeader);

void YRPacketHeaderSYNSetConfiguration(YRPacketHeaderSYNRef synHeader, YRRUDPConnectionConfiguration configuration);
YRRUDPConnectionConfiguration YRPacketHeaderSYNGetConfiguration(YRPacketHeaderSYNRef synHeader);

#pragma mark - RST Header

YRPacketHeaderRef YRPacketHeaderRSTGetBaseHeader(YRPacketHeaderRSTRef rstHeader);

void YRPacketHeaderRSTSetErrorCode(YRPacketHeaderRSTRef rstHeader, YRRUDPError errCode);
YRRUDPError YRPacketHeaderRSTGetErrorCode(YRPacketHeaderRSTRef rstHeader);

#pragma mark - Payload Header

YRPacketHeaderRef YRPacketPayloadHeaderGetBaseHeader(YRPacketPayloadHeaderRef header);

bool YRPacketHeaderHasPayloadLength(YRPacketHeaderRef header);
void YRPacketPayloadHeaderSetPayloadLength(YRPacketPayloadHeaderRef header, YRPayloadLengthType length);
YRPayloadLengthType YRPacketPayloadHeaderGetPayloadLength(YRPacketPayloadHeaderRef header);

#pragma mark - EACK Header

YRPacketPayloadHeaderRef YRPacketHeaderEACKGetPayloadHeader(YRPacketHeaderEACKRef eackHeader);
YRPacketHeaderRef YRPacketHeaderEACKGetBaseHeader(YRPacketHeaderEACKRef eackHeader);

void YRPacketHeaderSetEACKs(
	YRPacketHeaderEACKRef eackHeader,
	YRSequenceNumberType *eacks,
	YRHeaderLengthType eacksCount
);

YRSequenceNumberType *YRPacketHeaderGetEACKs(
	YRPacketHeaderEACKRef eackHeader,
	YRHeaderLengthType *eacksCount
);

#pragma mark - Utility

void YRPacketHeaderDebugInfo(char *buffer);

// TODO: Remove
//
///**
// *  Determines how much bytes needed to fit given eacks. (Note: Maximum header size is 256, so on return ioCount will contain how much eacks can be set to header)
// */
//YRHeaderLengthType YRPacketHeaderEACKLength(YRSequenceNumberType *ioCount);
//
///**
// *  Determines how much eack's can fit into given length.
// */
//YRSequenceNumberType YRPacketHeaderEACKsCountThatFit(YRHeaderLengthType headerLength);
//YRSequenceNumberType YRPacketHeaderEACKsCount(YRPacketHeaderEACKRef eackHeader);

#endif // __YRPacketHeader__
