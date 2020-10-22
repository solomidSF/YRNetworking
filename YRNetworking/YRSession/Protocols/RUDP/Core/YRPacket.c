//
// YRPacket.c
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

// Network Packets Layout:

// SYN Segment:
// 0             7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| | | |A| | | | |               |
//|1|0|0|C|0|0|0|0|       22      |
//| | | |K| | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//|   Protocol    |    Reserved   |
//+---------------+---------------+
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
//|0|1|0|C|0|0|0|0|       12      |
//| | | |K| | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//|   Protocol    |   Error Code  |
//+---------------+---------------+
//|           Sequence #          |
//+---------------+---------------+
//|           Ack Number          |
//+---------------+---------------+
//|         Header Checksum       |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+

// NUL segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|0|1|1|0|0|0|0|       12      |
//+-+-+-+-+-+-+-+-+---------------+
//|   Protocol    |    Reserved   |
//+---------------+---------------+
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
//|0|0|0|1|0|0|0|0|      14       |
//+-+-+-+-+-+-+-+-+---------------+
//|   Protocol    |    Reserved   |
//+---------------+---------------+
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
//|            Payload            |
//+---------------+---------------+

// EACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|0|0|1|1|0|0|0|   2 * N + 14  |
//+-+-+-+-+-+-+-+-+---------------+
//|   Protocol    |    Reserved   |
//+---------------+---------------+
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
//|            Payload            |
//+---------------+---------------+

#define YRPacketDoSerialize(code) \
	do { \
		YRPacketSerializeHeader(base, stream); \
		code \
		YRPacketHeaderSetHeaderLength(base, YROutputStreamGetWrittenSize(stream)); \
		YRPacketCalculateChecksum(base, YROutputStreamGetBytes(stream), 0); \
		YRPacketSerializeHeader(base, stream); \
		return (YRPacketRef)YROutputStreamGetBytes(stream); \
	} while (0)

#define YRPacketDoSerializeWithPayload(code) \
	do { \
		YRPacketSerializeHeader(base, stream); \
		YROutputStreamWriteUInt16(stream, payloadLength); \
		code \
		YRPacketHeaderSetHeaderLength(base, YROutputStreamGetWrittenSize(stream)); \
		YRPacketSerializePayload(payload, payloadLength, stream); \
		YRPacketCalculateChecksum(base, YROutputStreamGetBytes(stream), payloadLength); \
		YRPacketSerializeHeader(base, stream); \
		return (YRPacketRef)YROutputStreamGetBytes(stream); \
	} while (0)

#pragma mark - Prototypes

void YRPacketSerializeHeader(YRPacketHeaderRef header, YROutputStreamRef stream);
void YRPacketSerializePayload(const void *payload, YRPayloadLengthType length, YROutputStreamRef stream);
inline
void YRPacketCalculateChecksum(
	YRPacketHeaderRef header,
	const void *packetStart,
	YRPayloadLengthType payloadLength
);

#pragma mark - Serialization

YRPacketRef YRPacketSerializeSYN(YRPacketHeaderSYNRef synHeader, YROutputStreamRef stream) {
	YRPacketHeaderRef base = YRPacketHeaderSYNGetBaseHeader(synHeader);
	YRRUDPConnectionConfiguration config = YRPacketHeaderSYNGetConfiguration(synHeader);
	
	YRPacketDoSerialize(
		YROutputStreamWriteUInt16(stream, config.options);
		YROutputStreamWriteUInt16(stream, config.retransmissionTimeoutValue);
		YROutputStreamWriteUInt16(stream, config.nullSegmentTimeoutValue);
		YROutputStreamWriteUInt16(stream, config.maximumSegmentSize);
		YROutputStreamWriteUInt8(stream, config.maxRetransmissions);
		YROutputStreamWriteUInt8(stream, config.maxNumberOfOutstandingSegments);
	);
}

YRPacketRef YRPacketSerializeRST(YRPacketHeaderRSTRef rstHeader, YROutputStreamRef stream) {
	YRPacketHeaderRef base = YRPacketHeaderRSTGetBaseHeader(rstHeader);

	YRPacketDoSerialize();
}

YRPacketRef YRPacketSerializeNUL(YRPacketHeaderRef nulHeader, YROutputStreamRef stream) {
	YRPacketHeaderRef base = nulHeader;
	
	YRPacketDoSerialize();
}

YRPacketRef YRPacketSerializeEACK(YRPacketHeaderEACKRef eackHeader, YROutputStreamRef stream) {
	return YRPacketSerializeEACKWithPayload(eackHeader, NULL, 0, stream);
}

YRPacketRef YRPacketSerializeEACKWithPayload(
	YRPacketHeaderEACKRef eackHeader,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
) {
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(eackHeader);
	
	YRPacketDoSerializeWithPayload(
		YRSequenceNumberType count = 0;
		YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(eackHeader, &count);
		
		for (YRSequenceNumberType i = 0; i < count; i++) {
			YROutputStreamWriteUInt16(stream, eacks[i]);
		}
	);
}

YRPacketRef YRPacketSerializeWithPayload(
	YRPacketPayloadHeaderRef header,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
) {
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	YRPacketDoSerializeWithPayload();
}

#pragma mark - Introspection

//YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet) {
//	return (YRPacketHeaderRef)(packet);
//}

#pragma mark - Private

void YRPacketSerializeHeader(YRPacketHeaderRef header, YROutputStreamRef stream) {
	YROutputStreamReset(stream);
	
	YRPacketDescriptionType packetDescription = YRPacketHeaderGetPacketDescription(header);
	YRProtocolVersionType protocol = YRPacketHeaderGetProtocolVersion(header);
	uint8_t reserved = YRPacketHeaderGetReserved(header);
	YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
	YRSequenceNumberType seqNumber = YRPacketHeaderGetSequenceNumber(header);
	YRSequenceNumberType ackNumber = YRPacketHeaderGetAckNumber(header);
	YRChecksumType checksum = YRPacketHeaderGetChecksum(header);

	YROutputStreamWriteUInt8(stream, packetDescription);
	YROutputStreamWriteUInt8(stream, protocol);
	YROutputStreamWriteUInt8(stream, reserved);
	YROutputStreamWriteUInt8(stream, headerLength);
	YROutputStreamWriteUInt16(stream, seqNumber);
	YROutputStreamWriteUInt16(stream, ackNumber);
	YROutputStreamWriteUInt32(stream, checksum);
}

void YRPacketSerializePayload(const void *payload, YRPayloadLengthType length, YROutputStreamRef stream) {
	YROutputStreamAppend(stream, payload, length);
}

void YRPacketCalculateChecksum(
	YRPacketHeaderRef header,
	const void *packetStart,
	YRPayloadLengthType payloadLength
) {
	YRHeaderLengthType headerLength = YRPacketHeaderGetHeaderLength(header);
	register uint64_t sum = 0;
	uint32_t packetLength = headerLength + payloadLength;
	YRChecksumType length = YRPacketHeaderHasCHK(header) ? packetLength : headerLength;
	const void *iterator = packetStart;
	
	while (length > sizeof(YRChecksumType) - 1) {
		sum += *(YRChecksumType *)iterator++;
		length -= sizeof(YRChecksumType);
	}
	
	while (length > 1) {
		sum += *(uint16_t *)iterator++;
		length -= sizeof(uint16_t);
	}
	
	if (length > 0) {
		sum += *(uint8_t *)iterator;
	}
	
	while (sum >> sizeof(YRChecksumType) * 8) {
		sum = (sum & (YRChecksumType)(~0)) + (sum >> (sizeof(YRChecksumType) * 8));
	}
	
	YRPacketHeaderSetChecksum(header, (YRChecksumType)(~sum));
}
