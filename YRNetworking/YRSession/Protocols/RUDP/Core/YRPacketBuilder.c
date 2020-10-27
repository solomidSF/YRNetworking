//
// YRPacketBuilder.c
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

#define YRPacketBuild(code) \
	do { \
		YRPacketBuildHeader(base, seqNumber, ackNumber, hasACK); \
		code \
	} while (0)

#define YRPacketBuildWithPayload(code) \
	do { \
		YRPacketBuildHeader(base, seqNumber, ackNumber, hasACK); \
		code \
	} while (0)
	
#pragma mark - Prototypes

void YRPacketBuildHeader(
	YRPacketHeaderRef header,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK
);

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(
	YRRUDPConnectionConfiguration configuration,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	uint8_t rawHeader[kYRPacketHeaderMaxDataStructureLength];
	YRPacketHeaderSYNRef header = (YRPacketHeaderSYNRef)rawHeader;
	YRPacketHeaderRef base = YRPacketHeaderSYNGetBaseHeader(header);

	YRPacketBuild(
		 YRPacketHeaderSYNSetConfiguration(header, configuration);
	);

	return YRPacketSerializeSYN(header, stream, packetLength);
}

YRPacketRef YRPacketCreateRST(
	uint8_t errorCode,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	uint8_t rawHeader[kYRPacketHeaderMaxDataStructureLength];
	YRPacketHeaderRSTRef header = (YRPacketHeaderRSTRef)rawHeader;
	YRPacketHeaderRef base = YRPacketHeaderRSTGetBaseHeader(header);

	YRPacketBuild(
		YRPacketHeaderRSTSetErrorCode(header, errorCode);
	);
	
	return YRPacketSerializeRST(header, stream, packetLength);
}

YRPacketRef YRPacketCreateNUL(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	bool hasACK = true;
	uint8_t rawHeader[kYRPacketHeaderMaxDataStructureLength];
	YRPacketHeaderRef base = (YRPacketHeaderRef)rawHeader;
	
	YRPacketBuild(
		YRPacketHeaderSetNUL(base);
	);

	return YRPacketSerializeNUL(base, stream, packetLength);
}

YRPacketRef YRPacketCreateACK(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	return YRPacketCreateWithPayload(
		seqNumber,
		ackNumber,
		NULL,
		0,
		stream,
		packetLength
	);
}

YRPacketRef YRPacketCreateEACK(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YRSequenceNumberType *sequences,
	YRSequenceNumberType *ioSequencesCount,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	return YRPacketCreateEACKWithPayload(
		seqNumber,
		ackNumber,
		sequences,
		ioSequencesCount,
		NULL,
		0,
		stream,
		packetLength
	);
}

YRPacketRef YRPacketCreateEACKWithPayload(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YRSequenceNumberType *sequences,
	YRSequenceNumberType *ioSequencesCount,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	bool hasACK = true;
	uint8_t rawHeader[kYRPacketHeaderMaxDataStructureLength];
	YRPacketHeaderEACKRef header = (YRPacketHeaderEACKRef)rawHeader;
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(header);
	
	YRPacketBuildWithPayload(
		YRPacketPayloadHeaderSetPayloadLength(YRPacketHeaderEACKGetPayloadHeader(header), payloadLength);
		YRPacketHeaderEACKSetEACKs(header, sequences, *ioSequencesCount);
	);
	
	return YRPacketSerializeEACK(header, stream, packetLength);
}

YRPacketRef YRPacketCreateWithPayload(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
) {
	bool hasACK = true;
	uint8_t rawHeader[kYRPacketHeaderMaxDataStructureLength];
	YRPacketPayloadHeaderRef header = (YRPacketPayloadHeaderRef)rawHeader;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);

	YRPacketBuildWithPayload(
		YRPacketPayloadHeaderSetPayloadLength(header, payloadLength);
	);
	
	return YRPacketSerializeWithPayload(header, payload, payloadLength, stream, packetLength);
}

#pragma mark - Private

void YRPacketBuildHeader(
	YRPacketHeaderRef header,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK
) {
	YRPacketHeaderSetSequenceNumber(header, seqNumber);
	
	if (hasACK) {
		YRPacketHeaderSetAckNumber(header, ackNumber);
	}
	
	YRPacketHeaderSetProtocolVersion(header, kYRProtocolVersion);
}
