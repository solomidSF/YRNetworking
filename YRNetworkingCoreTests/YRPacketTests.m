//
// YRPacketTests.m
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

#import <XCTest/XCTest.h>
#import "YRNetworking.h"

typedef struct {
	uint8_t command;
	uint16_t var1;
	uint32_t var2;
	uint64_t var3;
	char var4;
	float var5;
	double var6;
	float x;
	float y;
	float z;
	float angularVelocity;
	float velocity;
} YRMockPayload;

@interface YRPacketTests : XCTestCase
@end

@implementation YRPacketTests {
	YROutputStreamRef _oStream;
	YRInputStreamRef _iStream;
	void *_buffer;
	void *_headerBuffer;
	YRPayloadLengthType _packetLength;
	YRPacketHandlers _defaultHandlers;
	void (^_synHandler) (YRPacketHeaderSYNRef header);
	void (^_rstHandler) (YRPacketHeaderRSTRef header);
	void (^_nulHandler) (YRPacketHeaderRef header);
	void (^_eackHandler) (YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (^_regularHandler) (YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (^_invalidHandler) (YRRUDPError error);
}

- (void)setUp {
	YRSequenceNumberType bufferSize = (YRSequenceNumberType)(~0);
	_buffer = malloc(bufferSize);
	_oStream = YROutputStreamGet(_buffer, bufferSize);
	_iStream = YRInputStreamGet(_buffer, bufferSize);
	
	_headerBuffer = calloc(1, kYRPacketHeaderMaxDataStructureLength);
	
	_defaultHandlers = (YRPacketHandlers) {
		&syn,
		&rst,
		&nul,
		&eack,
		&regular,
		&invalid
	};
	
	_synHandler = ^(YRPacketHeaderSYNRef h) { XCTFail("Unhandled syn packet"); };
	_rstHandler = ^(YRPacketHeaderRSTRef h) { XCTFail("Unhandled rst packet"); };
	_nulHandler = ^(YRPacketHeaderRef h) { XCTFail("Unhandled nul packet"); };
	_eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) { XCTFail("Unhandled eack packet"); };
	_regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) { XCTFail("Unhandled regular packet"); };
	_invalidHandler = ^(YRRUDPError e) { XCTFail("Unhandled invalid packet"); };
}

- (void)tearDown {
	free(_buffer);
	free(_headerBuffer);
}

- (void)testSerializeSYN {
	YRPacketHeaderSYNRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderSYNGetBaseHeader(header);
	YRSequenceNumberType seq = 65001;
	YRRUDPConnectionConfiguration expected;
    expected.options = 0xAB;
    expected.retransmissionTimeoutValue = 200;
    expected.nullSegmentTimeoutValue = 100;
    expected.maximumSegmentSize = 800;
	expected.maxRetransmissions = 5;
    expected.maxNumberOfOutstandingSegments = 16;

	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSYNSetConfiguration(header, expected);
	
	__unused
	YRPacketRef packet = YRPacketSerializeSYN(header, _oStream, &_packetLength);
	
	__block YRPacketHeaderSYNRef actual = NULL;
	
	_synHandler = ^(YRPacketHeaderSYNRef h) { actual = h; };
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("SYN parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderSYNGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionSYN,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
		
	YRRUDPConnectionConfiguration actualCC = YRPacketHeaderSYNGetConfiguration(actual);
	
    XCTAssertEqual(expected.options, actualCC.options);
	XCTAssertEqual(expected.retransmissionTimeoutValue, actualCC.retransmissionTimeoutValue);
	XCTAssertEqual(expected.nullSegmentTimeoutValue, actualCC.nullSegmentTimeoutValue);
	XCTAssertEqual(expected.maximumSegmentSize, actualCC.maximumSegmentSize);
	XCTAssertEqual(expected.maxRetransmissions, actualCC.maxRetransmissions);
	XCTAssertEqual(expected.maxNumberOfOutstandingSegments, actualCC.maxNumberOfOutstandingSegments);
}

- (void)testSerializeSYNACK {
	YRPacketHeaderSYNRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderSYNGetBaseHeader(header);
	YRSequenceNumberType seq = 65001;
	YRSequenceNumberType ack = 22433;
	YRRUDPConnectionConfiguration expected;
    expected.options = 0xAB;
    expected.retransmissionTimeoutValue = 200;
    expected.nullSegmentTimeoutValue = 100;
    expected.maximumSegmentSize = 800;
	expected.maxRetransmissions = 5;
    expected.maxNumberOfOutstandingSegments = 16;

	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSYNSetConfiguration(header, expected);
	
	__unused
	YRPacketRef packet = YRPacketSerializeSYN(header, _oStream, &_packetLength);
	
	__block YRPacketHeaderSYNRef actual = NULL;
	
	_synHandler = ^(YRPacketHeaderSYNRef h) { actual = h; };
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("SYN parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderSYNGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionSYN | YRPacketDescriptionACK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(actualBase));
		
	YRRUDPConnectionConfiguration actualCC = YRPacketHeaderSYNGetConfiguration(actual);
	
    XCTAssertEqual(expected.options, actualCC.options);
	XCTAssertEqual(expected.retransmissionTimeoutValue, actualCC.retransmissionTimeoutValue);
	XCTAssertEqual(expected.nullSegmentTimeoutValue, actualCC.nullSegmentTimeoutValue);
	XCTAssertEqual(expected.maximumSegmentSize, actualCC.maximumSegmentSize);
	XCTAssertEqual(expected.maxRetransmissions, actualCC.maxRetransmissions);
	XCTAssertEqual(expected.maxNumberOfOutstandingSegments, actualCC.maxNumberOfOutstandingSegments);
}

- (void)testSerializeRST {
	YRPacketHeaderRSTRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderRSTGetBaseHeader(header);
	
	YRSequenceNumberType seq = 254;
	YRRUDPError expected = kYRRUDPErrorProtocolVersionMismatch;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderRSTSetErrorCode(header, expected);
	
	__unused
	YRPacketRef packet = YRPacketSerializeRST(header, _oStream, &_packetLength);
	
	__block YRPacketHeaderRSTRef actual = NULL;
	
	_rstHandler = ^(YRPacketHeaderRSTRef h) { actual = h; };
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("RST parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderRSTGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionRST,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
		
	YRRUDPError actualError = YRPacketHeaderRSTGetErrorCode(actual);
	
    XCTAssertEqual(expected, actualError);
}

- (void)testSerializeNUL {
	YRPacketHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = header;
	
	YRSequenceNumberType seq = 254;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetNUL(header);
	
	__unused
	YRPacketRef packet = YRPacketSerializeNUL(header, _oStream, &_packetLength);
	
	__block YRPacketHeaderRef actual = NULL;
	
	_nulHandler = ^(YRPacketHeaderRef h) { actual = h; };
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("NUL parse handler wasn't called");
		return;
	}
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actual)
	);

	XCTAssertEqual(
		YRPacketDescriptionNUL,
		YRPacketHeaderGetPacketDescription(actual)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actual));
}

- (void)testSerializeEACK {
	YRPacketHeaderEACKRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(header);
	
	YRSequenceNumberType seq = 25333;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);

	YRPayloadLengthType eacksCount = (YRHeaderLengthType)(~0) / sizeof(YRSequenceNumberType);
	YRSequenceNumberType eacks[eacksCount];
	
	for (YRPayloadLengthType i = 0; i < eacksCount; i++) {
		eacks[i] = arc4random();
	}
	
	YRPacketHeaderEACKSetEACKs(header, eacks, eacksCount);

	__unused
	YRPacketRef packet = YRPacketSerializeEACK(header, _oStream, &_packetLength);
	
	__block YRPacketHeaderEACKRef actual = NULL;
	
	_eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) {
		XCTAssert(p == nil);
		XCTAssertEqual(0, pl);
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("EACK parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderEACKGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionEACK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
		
	YRHeaderLengthType actualEACKsCount = 0;
	YRSequenceNumberType *actualEACKs = YRPacketHeaderEACKGetEACKs(actual, &actualEACKsCount);
	
	XCTAssert(actualEACKsCount <= eacksCount);
	
	for (YRHeaderLengthType i = 0; i < actualEACKsCount; i++) {
		XCTAssertEqual(actualEACKs[i], eacks[i]);
	}
}

- (void)testSerializeEACKWithPayload {
	YRPacketHeaderEACKRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(header);
	
	YRSequenceNumberType seq = 553;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);

	YRPayloadLengthType eacksCount = (YRHeaderLengthType)(~0) / sizeof(YRSequenceNumberType);
	YRSequenceNumberType eacks[eacksCount];
	
	for (YRPayloadLengthType i = 0; i < eacksCount; i++) {
		eacks[i] = arc4random();
	}
	
	YRPacketHeaderEACKSetEACKs(header, eacks, eacksCount);

	YRMockPayload expected = defaultMockPayload();
	
	__unused
	YRPacketRef packet = YRPacketSerializeEACKWithPayload(
		header,
		&expected,
		sizeof(expected),
		_oStream,
		&_packetLength
	);
	
	__block YRPacketHeaderEACKRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	_eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("EACK parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderEACKGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionEACK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
		
	YRHeaderLengthType actualEACKsCount = 0;
	YRSequenceNumberType *actualEACKs = YRPacketHeaderEACKGetEACKs(actual, &actualEACKsCount);
	
	XCTAssert(actualEACKsCount <= eacksCount);
	
	for (YRHeaderLengthType i = 0; i < actualEACKsCount; i++) {
		XCTAssertEqual(actualEACKs[i], eacks[i]);
	}
	
	XCTAssertEqual(sizeof(YRMockPayload), actualPL);
	
	YRMockPayload actualPP = *(YRMockPayload *)actualP;
	
	XCTAssertEqual(expected.command, actualPP.command);
	XCTAssertEqual(expected.var1, actualPP.var1);
	XCTAssertEqual(expected.var2, actualPP.var2);
	XCTAssertEqual(expected.var3, actualPP.var3);
	XCTAssertEqual(expected.var4, actualPP.var4);
	XCTAssertEqual(expected.var5, actualPP.var5);
	XCTAssertEqual(expected.var6, actualPP.var6);
	XCTAssertEqual(expected.x, actualPP.x);
	XCTAssertEqual(expected.y, actualPP.y);
	XCTAssertEqual(expected.z, actualPP.z);
	XCTAssertEqual(expected.angularVelocity, actualPP.angularVelocity);
	XCTAssertEqual(expected.velocity, actualPP.velocity);
}

- (void)testSerializeWithPayload {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetAckNumber(base, ack);
	
	YRMockPayload expected = defaultMockPayload();
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(
		header,
		&expected,
		sizeof(expected),
		_oStream,
		&_packetLength
	);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	_regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionACK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(actualBase));
	XCTAssertEqual(sizeof(YRMockPayload), actualPL);
	
	YRMockPayload actualPP = *(YRMockPayload *)actualP;
	
	XCTAssertEqual(expected.command, actualPP.command);
	XCTAssertEqual(expected.var1, actualPP.var1);
	XCTAssertEqual(expected.var2, actualPP.var2);
	XCTAssertEqual(expected.var3, actualPP.var3);
	XCTAssertEqual(expected.var4, actualPP.var4);
	XCTAssertEqual(expected.var5, actualPP.var5);
	XCTAssertEqual(expected.var6, actualPP.var6);
	XCTAssertEqual(expected.x, actualPP.x);
	XCTAssertEqual(expected.y, actualPP.y);
	XCTAssertEqual(expected.z, actualPP.z);
	XCTAssertEqual(expected.angularVelocity, actualPP.angularVelocity);
	XCTAssertEqual(expected.velocity, actualPP.velocity);
}

- (void)testSerializeWithCheksummedPayload {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetCHK(base);
	
	YRMockPayload expected = defaultMockPayload();
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(
		header,
		&expected,
		sizeof(expected),
		_oStream,
		&_packetLength
	);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	_regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionACK | YRPacketDescriptionCHK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(actualBase));
	XCTAssertEqual(sizeof(YRMockPayload), actualPL);
	
	YRMockPayload actualPP = *(YRMockPayload *)actualP;
	
	XCTAssertEqual(expected.command, actualPP.command);
	XCTAssertEqual(expected.var1, actualPP.var1);
	XCTAssertEqual(expected.var2, actualPP.var2);
	XCTAssertEqual(expected.var3, actualPP.var3);
	XCTAssertEqual(expected.var4, actualPP.var4);
	XCTAssertEqual(expected.var5, actualPP.var5);
	XCTAssertEqual(expected.var6, actualPP.var6);
	XCTAssertEqual(expected.x, actualPP.x);
	XCTAssertEqual(expected.y, actualPP.y);
	XCTAssertEqual(expected.z, actualPP.z);
	XCTAssertEqual(expected.angularVelocity, actualPP.angularVelocity);
	XCTAssertEqual(expected.velocity, actualPP.velocity);
}

- (void)testSerializeWithByteOrderIndependentPayload {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetAckNumber(base, ack);
	
	YRMockPayload expected = defaultMockPayload();
	YRMockPayload beIndependent;
	YROutputStreamRef poStream = YROutputStreamCreate(&beIndependent, sizeof(beIndependent));

	YROutputStreamWriteUInt8(poStream, expected.command);
	YROutputStreamWriteUInt16(poStream, expected.var1);
	YROutputStreamWriteUInt32(poStream, expected.var2);
	YROutputStreamWriteUInt64(poStream, expected.var3);
	YROutputStreamWriteUInt8(poStream, expected.var4);
	YROutputStreamWriteFloat(poStream, expected.var5);
	YROutputStreamWriteDouble(poStream, expected.var6);
	YROutputStreamWriteFloat(poStream, expected.x);
	YROutputStreamWriteFloat(poStream, expected.y);
	YROutputStreamWriteFloat(poStream, expected.z);
	YROutputStreamWriteFloat(poStream, expected.angularVelocity);
	YROutputStreamWriteFloat(poStream, expected.velocity);
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(
		header,
		YROutputStreamGetBytes(poStream),
		YROutputStreamBytesWritten(poStream),
		_oStream,
		&_packetLength
	);
	
	YROutputStreamDestroy(poStream);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	_regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionACK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(actualBase));
		
	YRInputStreamRef piStream = YRInputStreamCreate(actualP, actualPL);
	
	XCTAssertEqual(expected.command, YRInputStreamReadUInt8(piStream, NULL));
	XCTAssertEqual(expected.var1, YRInputStreamReadUInt16(piStream, NULL));
	XCTAssertEqual(expected.var2, YRInputStreamReadUInt32(piStream, NULL));
	XCTAssertEqual(expected.var3, YRInputStreamReadUInt64(piStream, NULL));
	XCTAssertEqual(expected.var4, YRInputStreamReadUInt8(piStream, NULL));
	XCTAssertEqual(expected.var5, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.var6, YRInputStreamReadDouble(piStream, NULL));
	XCTAssertEqual(expected.x, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.y, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.z, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.angularVelocity, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.velocity, YRInputStreamReadFloat(piStream, NULL));
	
	YRInputStreamDestroy(piStream);
}

- (void)testSerializeWithChecksummedByteOrderIndependentPayload {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetCHK(base);
	
	YRMockPayload expected = defaultMockPayload();
	YRMockPayload beIndependent;
	YROutputStreamRef poStream = YROutputStreamCreate(&beIndependent, sizeof(beIndependent));

	YROutputStreamWriteUInt8(poStream, expected.command);
	YROutputStreamWriteUInt16(poStream, expected.var1);
	YROutputStreamWriteUInt32(poStream, expected.var2);
	YROutputStreamWriteUInt64(poStream, expected.var3);
	YROutputStreamWriteUInt8(poStream, expected.var4);
	YROutputStreamWriteFloat(poStream, expected.var5);
	YROutputStreamWriteDouble(poStream, expected.var6);
	YROutputStreamWriteFloat(poStream, expected.x);
	YROutputStreamWriteFloat(poStream, expected.y);
	YROutputStreamWriteFloat(poStream, expected.z);
	YROutputStreamWriteFloat(poStream, expected.angularVelocity);
	YROutputStreamWriteFloat(poStream, expected.velocity);
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(
		header,
		YROutputStreamGetBytes(poStream),
		YROutputStreamBytesWritten(poStream),
		_oStream,
		&_packetLength
	);
	
	YROutputStreamDestroy(poStream);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	_regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(_oStream),
		_packetLength
	);

	XCTAssertEqual(
		kYRProtocolVersion,
		YRPacketHeaderGetProtocolVersion(actualBase)
	);

	XCTAssertEqual(
		YRPacketDescriptionACK | YRPacketDescriptionCHK,
		YRPacketHeaderGetPacketDescription(actualBase)
	);
	
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(actualBase));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(actualBase));
		
	YRInputStreamRef piStream = YRInputStreamCreate(actualP, actualPL);
	
	XCTAssertEqual(expected.command, YRInputStreamReadUInt8(piStream, NULL));
	XCTAssertEqual(expected.var1, YRInputStreamReadUInt16(piStream, NULL));
	XCTAssertEqual(expected.var2, YRInputStreamReadUInt32(piStream, NULL));
	XCTAssertEqual(expected.var3, YRInputStreamReadUInt64(piStream, NULL));
	XCTAssertEqual(expected.var4, YRInputStreamReadUInt8(piStream, NULL));
	XCTAssertEqual(expected.var5, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.var6, YRInputStreamReadDouble(piStream, NULL));
	XCTAssertEqual(expected.x, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.y, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.z, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.angularVelocity, YRInputStreamReadFloat(piStream, NULL));
	XCTAssertEqual(expected.velocity, YRInputStreamReadFloat(piStream, NULL));
	
	YRInputStreamDestroy(piStream);
}

- (void)testDeserializeWithNonCompleteHeader {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), _oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, 7);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectProtocol {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion - 1);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), _oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorProtocolVersionMismatch;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectPacketDescription {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;

	YRPacketHeaderSetSYN(base);
	YRPacketHeaderSetRST(base);
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), _oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializePacketWithIncorrectSize {
	YRPacketPayloadHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;

	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), _oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, packetLength * 2);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectChecksum {
	YRPacketHeaderRef header = _headerBuffer;
	YRPacketHeaderRef base = header;
	
	YRSequenceNumberType seq = 254;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetNUL(header);
	
	__unused
	YRPacketRef packet = YRPacketSerializeNUL(header, _oStream, &_packetLength);
	// Simulate malformed packet
	YROutputStreamSetIndexTo(_oStream, _packetLength - 1);
	YROutputStreamWriteUInt8(_oStream, 0xde);

	YRRUDPError expected = kYRRUDPErrorChecksumMismatch;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));
	
	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeEACKWithNoEACKS {
	YRPacketHeaderEACKRef header = _headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(header);
	
	YRSequenceNumberType seq = 10233;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);

	YRPayloadLengthType eacksCount = 0;
	YRPacketHeaderEACKSetEACKs(header, NULL, eacksCount);
	
	__unused
	YRPacketRef packet = YRPacketSerializeEACK(header, _oStream, &_packetLength);
	
	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	_invalidHandler = ^(YRRUDPError e) { actual = e; };

	_iStream = YRInputStreamGet(_buffer, _packetLength);
	YRPacketDeserialize(_iStream, _defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

#pragma mark - Private

YRMockPayload defaultMockPayload() {
	return (YRMockPayload) {
		255,
		65535,
		(uint32_t)(~0),
		(uint64_t)(~0),
		'z',
		1.234567,
		1.234567891,
		50.223,
		60.134,
		70.223,
		181.04442,
		54.334322
	};
}

void syn(void *context, YRPacketHeaderSYNRef header) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_synHandler(header);
}

void rst(void *context, YRPacketHeaderRSTRef header) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_rstHandler(header);
}

void nul(void *context, YRPacketHeaderRef header) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_nulHandler(header);
}

void eack(void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_eackHandler(header, payload, payloadLength);
}

void regular(void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_regularHandler(header, payload, payloadLength);
}

void invalid(void *context, YRRUDPError error) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->_invalidHandler(error);
}

@end
