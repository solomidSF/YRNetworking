//
//  YRPacketTests.m
//  YRNetworkingCoreTests
//
//  Created by Yurii Romanchenko on 10/22/20.
//  Copyright © 2020 Yuriy Romanchenko. All rights reserved.
//

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
	YROutputStreamRef oStream;
	YRInputStreamRef iStream;
	void *buffer;
	void *headerBuffer;
	YRPayloadLengthType packetLength;
	YRPacketHandlers defaultHandlers;
	void (^synHandler) (YRPacketHeaderSYNRef header);
	void (^rstHandler) (YRPacketHeaderRSTRef header);
	void (^nulHandler) (YRPacketHeaderRef header);
	void (^eackHandler) (YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (^regularHandler) (YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (^invalidHandler) (YRRUDPError error);
}

- (void)setUp {
	YRSequenceNumberType bufferSize = (YRSequenceNumberType)(~0);
	buffer = malloc(bufferSize);
	oStream = YROutputStreamGet(buffer, bufferSize);
	iStream = YRInputStreamGet(buffer, bufferSize);
	
	headerBuffer = calloc(1, kYRPacketHeaderMaxDataStructureLength);
	
	defaultHandlers = (YRPacketHandlers) {
		&syn,
		&rst,
		&nul,
		&eack,
		&regular,
		&invalid
	};
	
	synHandler = ^(YRPacketHeaderSYNRef h) { XCTFail("Unhandled syn packet"); };
	rstHandler = ^(YRPacketHeaderRSTRef h) { XCTFail("Unhandled rst packet"); };
	nulHandler = ^(YRPacketHeaderRef h) { XCTFail("Unhandled nul packet"); };
	eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) { XCTFail("Unhandled eack packet"); };
	regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) { XCTFail("Unhandled regular packet"); };
	invalidHandler = ^(YRRUDPError e) { XCTFail("Unhandled invalid packet"); };
}

- (void)tearDown {
	free(buffer);
	free(headerBuffer);
}

- (void)testSerializeSYN {
	YRPacketHeaderSYNRef header = headerBuffer;
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
	YRPacketRef packet = YRPacketSerializeSYN(header, oStream, &packetLength);
	
	__block YRPacketHeaderSYNRef actual = NULL;
	
	synHandler = ^(YRPacketHeaderSYNRef h) { actual = h; };
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("SYN parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderSYNGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketHeaderSYNRef header = headerBuffer;
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
	YRPacketRef packet = YRPacketSerializeSYN(header, oStream, &packetLength);
	
	__block YRPacketHeaderSYNRef actual = NULL;
	
	synHandler = ^(YRPacketHeaderSYNRef h) { actual = h; };
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("SYN parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderSYNGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketHeaderRSTRef header = headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderRSTGetBaseHeader(header);
	
	YRSequenceNumberType seq = 254;
	YRRUDPError expected = kYRRUDPErrorProtocolVersionMismatch;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderRSTSetErrorCode(header, expected);
	
	__unused
	YRPacketRef packet = YRPacketSerializeRST(header, oStream, &packetLength);
	
	__block YRPacketHeaderRSTRef actual = NULL;
	
	rstHandler = ^(YRPacketHeaderRSTRef h) { actual = h; };
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("RST parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderRSTGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketHeaderRef header = headerBuffer;
	YRPacketHeaderRef base = header;
	
	YRSequenceNumberType seq = 254;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetNUL(header);
	
	__unused
	YRPacketRef packet = YRPacketSerializeNUL(header, oStream, &packetLength);
	
	__block YRPacketHeaderRef actual = NULL;
	
	nulHandler = ^(YRPacketHeaderRef h) { actual = h; };
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("NUL parse handler wasn't called");
		return;
	}
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketHeaderEACKRef header = headerBuffer;
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
	YRPacketRef packet = YRPacketSerializeEACK(header, oStream, &packetLength);
	
	__block YRPacketHeaderEACKRef actual = NULL;
	
	eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) {
		XCTAssert(p == nil);
		XCTAssertEqual(0, pl);
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("EACK parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderEACKGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketHeaderEACKRef header = headerBuffer;
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
		oStream,
		&packetLength
	);
	
	__block YRPacketHeaderEACKRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	eackHandler = ^(YRPacketHeaderEACKRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("EACK parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketHeaderEACKGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketPayloadHeaderRef header = headerBuffer;
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
		oStream,
		&packetLength
	);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketPayloadHeaderRef header = headerBuffer;
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
		oStream,
		&packetLength
	);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketPayloadHeaderRef header = headerBuffer;
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
		oStream,
		&packetLength
	);
	
	YROutputStreamDestroy(poStream);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketPayloadHeaderRef header = headerBuffer;
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
		oStream,
		&packetLength
	);
	
	YROutputStreamDestroy(poStream);
	
	__block YRPacketPayloadHeaderRef actual = NULL;
	__block const void *actualP = NULL;
	__block YRPayloadLengthType actualPL = 0;
	regularHandler = ^(YRPacketPayloadHeaderRef h, const void *p, YRPayloadLengthType pl) {
		actualP = p;
		actualPL = pl;
		actual = h;
	};
	
	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	if (actual == NULL) {
		XCTFail("Regular parse handler wasn't called");
		return;
	}
	
	YRPacketHeaderRef actualBase = YRPacketPayloadHeaderGetBaseHeader(actual);
	
	XCTAssertEqual(
		YROutputStreamBytesWritten(oStream),
		packetLength
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
	YRPacketPayloadHeaderRef header = headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, 7);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectProtocol {
	YRPacketPayloadHeaderRef header = headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;
	
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion - 1);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorProtocolVersionMismatch;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectPacketDescription {
	YRPacketPayloadHeaderRef header = headerBuffer;
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
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializePacketWithIncorrectSize {
	YRPacketPayloadHeaderRef header = headerBuffer;
	YRPacketHeaderRef base = YRPacketPayloadHeaderGetBaseHeader(header);
	
	YRSequenceNumberType seq = 44334;
	YRSequenceNumberType ack = 2;

	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetAckNumber(base, ack);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	
	YRPayloadLengthType packetLength = 0;
	int32_t payload = 42;
	
	__unused
	YRPacketRef packet = YRPacketSerializeWithPayload(header, &payload, sizeof(int32_t), oStream, &packetLength);

	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, packetLength * 2);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));

	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeWithIncorrectChecksum {
	YRPacketHeaderRef header = headerBuffer;
	YRPacketHeaderRef base = header;
	
	YRSequenceNumberType seq = 254;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);
	YRPacketHeaderSetNUL(header);
	
	__unused
	YRPacketRef packet = YRPacketSerializeNUL(header, oStream, &packetLength);
	// Simulate malformed packet
	YROutputStreamSetIndexTo(oStream, packetLength - 1);
	YROutputStreamWriteUInt8(oStream, 0xde);

	YRRUDPError expected = kYRRUDPErrorChecksumMismatch;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));
	
	XCTAssertEqual(actual, expected);
}

- (void)testDeserializeEACKWithNoEACKS {
	YRPacketHeaderEACKRef header = headerBuffer;
	YRPacketHeaderRef base = YRPacketHeaderEACKGetBaseHeader(header);
	
	YRSequenceNumberType seq = 10233;
	YRPacketHeaderSetSequenceNumber(base, seq);
	YRPacketHeaderSetProtocolVersion(base, kYRProtocolVersion);

	YRPayloadLengthType eacksCount = 0;
	YRPacketHeaderEACKSetEACKs(header, NULL, eacksCount);
	
	__unused
	YRPacketRef packet = YRPacketSerializeEACK(header, oStream, &packetLength);
	
	YRRUDPError expected = kYRRUDPErrorPacketInvalid;
	__block YRRUDPError actual = kYRRUDPErrorUnknown;
	
	invalidHandler = ^(YRRUDPError e) { actual = e; };

	iStream = YRInputStreamGet(buffer, packetLength);
	YRPacketDeserialize(iStream, defaultHandlers, (__bridge void *)(self));

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
	test->synHandler(header);
}

void rst(void *context, YRPacketHeaderRSTRef header) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->rstHandler(header);
}

void nul(void *context, YRPacketHeaderRef header) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->nulHandler(header);
}

void eack(void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->eackHandler(header, payload, payloadLength);
}

void regular(void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->regularHandler(header, payload, payloadLength);
}

void invalid(void *context, YRRUDPError error) {
	YRPacketTests *test = (__bridge YRPacketTests *)(context);
	test->invalidHandler(error);
}

@end
