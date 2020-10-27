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
		YRPayloadLengthType length = YROutputStreamBytesWritten(stream); \
		if (packetLength) { \
			*packetLength = length; \
		} \
		YRPacketHeaderSetHeaderLength(base, length); \
		YRPacketSerializeHeader(base, stream); \
		YRPacketHeaderSetChecksum(base, YRPacketCalculateChecksum( \
			YRPacketHeaderGetHeaderLength(base), \
			false, \
			YROutputStreamGetBytes(stream), \
			0) \
		); \
		YRPacketSerializeHeader(base, stream); \
		YROutputStreamSetIndexTo(stream, length); \
		return (YRPacketRef)YROutputStreamGetBytes(stream); \
	} while (0)

#define YRPacketDoSerializeWithPayload(code) \
	do { \
		YRPacketSerializeHeader(base, stream); \
		YROutputStreamWriteUInt16(stream, payloadLength); \
		code \
		YRPacketHeaderSetHeaderLength(base, YROutputStreamBytesWritten(stream)); \
		YRPacketSerializePayload(payload, payloadLength, stream); \
		YRPayloadLengthType length = YROutputStreamBytesWritten(stream); \
		if (packetLength) { \
			*packetLength = length; \
		}\
		YRPacketSerializeHeader(base, stream); \
		YRPacketHeaderSetChecksum(base, YRPacketCalculateChecksum( \
			YRPacketHeaderGetHeaderLength(base), \
			YRPacketHeaderHasCHK(base), \
			YROutputStreamGetBytes(stream), \
			payloadLength) \
		);\
		YRPacketSerializeHeader(base, stream); \
		YROutputStreamSetIndexTo(stream, length); \
		return (YRPacketRef)YROutputStreamGetBytes(stream); \
	} while (0)

#pragma mark - Prototypes

bool YRPacketValidatePacketDescription(YRPacketDescriptionType packetDescription);
void YRPacketSerializeHeader(YRPacketHeaderRef header, YROutputStreamRef stream);
void YRPacketSerializePayload(const void *payload, YRPayloadLengthType length, YROutputStreamRef stream);
inline
YRChecksumType YRPacketCalculateChecksum(
	YRHeaderLengthType headerLength,
	bool usePayload,
	const void *packetStart,
	YRPayloadLengthType payloadLength
);

#pragma mark - Serialization

YRPacketRef YRPacketSerializeSYN(
	YRPacketHeaderSYNRef synHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
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

YRPacketRef YRPacketSerializeRST(
	YRPacketHeaderRSTRef rstHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	YRPacketHeaderRef base = YRPacketHeaderRSTGetBaseHeader(rstHeader);

	YRPacketDoSerialize();
}

YRPacketRef YRPacketSerializeNUL(
	YRPacketHeaderRef nulHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	YRPacketHeaderRef base = nulHeader;
	
	YRPacketDoSerialize();
}

YRPacketRef YRPacketSerializeEACK(
	YRPacketHeaderEACKRef eackHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	return YRPacketSerializeEACKWithPayload(eackHeader, NULL, 0, stream, packetLength);
}

YRPacketRef YRPacketSerializeEACKWithPayload(
	YRPacketHeaderEACKRef eackHeader,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(eackHeader);
	
	YRPacketDoSerializeWithPayload(
		YRHeaderLengthType count = 0;
		YRSequenceNumberType *eacks = YRPacketHeaderEACKGetEACKs(eackHeader, &count);
		
		for (YRSequenceNumberType i = 0;
			i < count && (YROutputStreamBytesWritten(stream) + sizeof(uint16_t) < YRMaxPacketHeaderSize);
			i++) {
			YROutputStreamWriteUInt16(stream, eacks[i]);
		}
	);
}

YRPacketRef YRPacketSerializeWithPayload(
	YRPacketPayloadHeaderRef header,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	YRPacketDoSerializeWithPayload();
}

void YRPacketDeserialize(
	YRInputStreamRef iStream,
	YRPacketHandlers handlers,
	void *context
) {
#define YREnsureOperationSucceeded(err, op) \
	op\
	if (!successful) { \
		!handlers.invalid ?: handlers.invalid(context, err); \
		return;\
	}

	// 1. Read base header

#define YRInputStreamSafeRead(variable, size) \
	YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid, \
		variable = YRInputStreamRead##size(iStream, &successful); \
	)
		
	bool successful = true;
	
	YRInputStreamSafeRead(YRPacketDescriptionType packetDescription, UInt8)
	YRInputStreamSafeRead(YRProtocolVersionType protocol, UInt8)
	YRInputStreamSafeRead(uint8_t reserved, UInt8)
    YRInputStreamSafeRead(YRHeaderLengthType headerLength, UInt8)
    YRInputStreamSafeRead(YRSequenceNumberType seqNumber, UInt16)
    YRInputStreamSafeRead(YRSequenceNumberType ackNumber, UInt16)
    YRInputStreamSafeRead(YRChecksumType checksum, UInt32)
    
    // 2. Validate packet
    
    YREnsureOperationSucceeded(kYRRUDPErrorProtocolVersionMismatch,
		successful = (protocol == kYRProtocolVersion);
    )
    
	YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
		successful = YRPacketValidatePacketDescription(packetDescription);
	)
	
	YRPayloadLengthType payloadLength = 0;
	
	YRPacketDescriptionType pdWithoutCHK = (packetDescription & ~YRPacketDescriptionCHK);
	if (pdWithoutCHK == YRPacketDescriptionACK ||
		pdWithoutCHK == YRPacketDescriptionEACK ||
		pdWithoutCHK == (YRPacketDescriptionACK | YRPacketDescriptionEACK)) {
		YRInputStreamSafeRead(payloadLength, UInt16)
	}
	
	YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
		YRPayloadLengthType packetLength = headerLength + payloadLength;
		successful = (packetLength == YRInputStreamSize(iStream));
	)

	YREnsureOperationSucceeded(kYRRUDPErrorChecksumMismatch,
		YRChecksumType calculatedChecksum = YRPacketCalculateChecksum(
			headerLength,
			packetDescription & YRPacketDescriptionCHK,
			YRInputStreamBufferStart(iStream),
			payloadLength
		);
		
		successful = (calculatedChecksum == 0);
	)
	
	// 3. Validate packet based on it's type and do callout
	
#define YRFillBaseHeader(code) \
	do { \
		YRPacketHeaderRef base = code;\
		YRPacketHeaderSetPacketDescription(base, packetDescription); \
		YRPacketHeaderSetProtocolVersion(base, protocol); \
		YRPacketHeaderSetReserved(base, reserved); \
		YRPacketHeaderSetHeaderLength(base, headerLength); \
		YRPacketHeaderSetSequenceNumber(base, seqNumber); \
		if (YRPacketHeaderHasACK(base)) { \
			YRPacketHeaderSetAckNumber(base, ackNumber); \
		} \
		YRPacketHeaderSetChecksum(base, checksum); \
	} while (0)
	
	uint8_t buffer[kYRPacketHeaderMaxDataStructureLength];
	memset(buffer, 0, kYRPacketHeaderMaxDataStructureLength);
	
	if (packetDescription & YRPacketDescriptionSYN) {
		YRPacketHeaderSYNRef header = (YRPacketHeaderSYNRef)buffer;
		YRFillBaseHeader(YRPacketHeaderSYNGetBaseHeader(header));
		
		YRRUDPConnectionConfiguration config;
		YRInputStreamSafeRead(config.options, UInt16);
		YRInputStreamSafeRead(config.retransmissionTimeoutValue, UInt16);
		YRInputStreamSafeRead(config.nullSegmentTimeoutValue, UInt16);
		YRInputStreamSafeRead(config.maximumSegmentSize, UInt16);
		YRInputStreamSafeRead(config.maxRetransmissions, UInt8);
		YRInputStreamSafeRead(config.maxNumberOfOutstandingSegments, UInt8);
				
		YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
			successful = (headerLength == YRInputStreamBytesRead(iStream));
		)
		
		YRPacketHeaderSYNSetConfiguration(header, config);

		!handlers.syn ?: handlers.syn(context, header);
		return;
	}
	
	if (packetDescription & YRPacketDescriptionRST) {
		YRPacketHeaderRSTRef header = (YRPacketHeaderRSTRef)buffer;
		YRFillBaseHeader(YRPacketHeaderRSTGetBaseHeader(header));

		YRPacketHeaderRSTSetErrorCode(header, reserved);
		
		!handlers.rst ?: handlers.rst(context, header);
		return;
	}
	
	if (packetDescription & YRPacketDescriptionNUL) {
		YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;
		YRFillBaseHeader(header);
		
		!handlers.nul ?: handlers.nul(context, header);
		return;
	}
	
	if (packetDescription & YRPacketDescriptionEACK) {
		YRPacketHeaderEACKRef header = (YRPacketHeaderEACKRef)buffer;
		YRFillBaseHeader(YRPacketHeaderEACKGetBaseHeader(header));
		
		YRPacketPayloadHeaderSetPayloadLength(
			YRPacketHeaderEACKGetPayloadHeader(header),
			payloadLength
		);
		
		// iStream is pointing into EACK area, double check for correctness
		YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
			successful = headerLength > YRInputStreamBytesRead(iStream) &&
				(headerLength - YRInputStreamBytesRead(iStream)) % sizeof(YRSequenceNumberType) == 0;
		)

		YRHeaderLengthType eacksCount = (headerLength - YRInputStreamCurrentIndex(iStream)) / sizeof(YRSequenceNumberType);
		YRSequenceNumberType eacks[eacksCount];
		
		for (YRHeaderLengthType i = 0; i < eacksCount; i++) {
			YRInputStreamSafeRead(eacks[i], UInt16)
		}
		
		YRPacketHeaderEACKSetEACKs(header, eacks, eacksCount);

		// iStream points into payload start, double check for correctness
		YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
			successful = (payloadLength == YRInputStreamBytesLeft(iStream));
		)
		
		!handlers.eack ?: handlers.eack(
			context,
			header,
			payloadLength > 0 ? YRInputStreamCurrentPointer(iStream, NULL) : NULL,
			payloadLength
		);
		return;
	}
	
	if (packetDescription & YRPacketDescriptionACK) {
		YRPacketPayloadHeaderRef header = (YRPacketPayloadHeaderRef)buffer;
		YRFillBaseHeader(YRPacketPayloadHeaderGetBaseHeader(header));

		YRPacketPayloadHeaderSetPayloadLength(header, payloadLength);
		
		// Double check payload length
		YREnsureOperationSucceeded(kYRRUDPErrorPacketInvalid,
			successful = (payloadLength == YRInputStreamBytesLeft(iStream));
		)
		
		!handlers.regular ?: handlers.regular(
			context,
			header,
			YRInputStreamCurrentPointer(iStream, NULL),
			payloadLength
		);
		return;
	}
}

#pragma mark - Validation

bool YRPacketValidatePacketDescription(YRPacketDescriptionType packetDescription) {
	if (packetDescription & YRPacketDescriptionSYN) {
		return
			!(packetDescription & YRPacketDescriptionRST) &&
			!(packetDescription & YRPacketDescriptionNUL) &&
			!(packetDescription & YRPacketDescriptionEACK);
	}

	if (packetDescription & YRPacketDescriptionRST) {
		return
			!(packetDescription & YRPacketDescriptionSYN) &&
			!(packetDescription & YRPacketDescriptionNUL) &&
			!(packetDescription & YRPacketDescriptionEACK);
	}

	if (packetDescription & YRPacketDescriptionNUL) {
		return
			!(packetDescription & YRPacketDescriptionSYN) &&
			!(packetDescription & YRPacketDescriptionRST) &&
			!(packetDescription & YRPacketDescriptionEACK);
	}
    
    return packetDescription & (YRPacketDescriptionACK | YRPacketDescriptionEACK);
}

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

YRChecksumType YRPacketCalculateChecksum(
	YRHeaderLengthType headerLength,
	bool usePayload,
	const void *packetStart,
	YRPayloadLengthType payloadLength
) {
	register uint64_t sum = 0;
	uint32_t packetLength = headerLength + payloadLength;
	YRChecksumType length = usePayload ? packetLength : headerLength;
	const void *iterator = packetStart;
	
	while (length > sizeof(YRChecksumType) - 1) {
		sum += ntohl(*((YRChecksumType *)iterator));
		length -= sizeof(YRChecksumType);
		iterator = ((YRChecksumType *)iterator) + 1;
	}
	
	while (length > 1) {
		sum += ntohs(*((uint16_t *)iterator));
		length -= sizeof(uint16_t);
		iterator = ((uint16_t *)iterator) + 1;
	}
	
	if (length > 0) {
		sum += *((uint8_t *)iterator);
	}
	
	while (sum >> (sizeof(YRChecksumType) * 8)) {
		sum = (sum & (YRChecksumType)(~0)) + (sum >> (sizeof(YRChecksumType) * 8));
	}
	
	return (YRChecksumType)(~sum);
}
