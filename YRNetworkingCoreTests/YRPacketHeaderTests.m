//
// YRPacketHeaderTests.m
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

@interface YRPacketHeaderTests : XCTestCase
@end

@implementation YRPacketHeaderTests {
	uint8_t *buffer;
}

- (void)setUp {
	buffer = calloc(1, kYRPacketHeaderMaxDataStructureLength);
}

- (void)tearDown {
	free(buffer);
}

- (void)testPrologue {
	char buffer[UINT16_MAX];
	YRPacketHeaderDebugInfo(buffer);
	NSLog(@"%s", buffer);
}

#pragma mark - Base Header

- (void)testPacketDescription {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;
	
	XCTAssertEqual(false, YRPacketHeaderIsSYN(header));
	YRPacketHeaderSetSYN(header);
	XCTAssertEqual(true, YRPacketHeaderIsSYN(header));
	
	XCTAssertEqual(false, YRPacketHeaderIsRST(header));
	YRPacketHeaderSetRST(header);
	XCTAssertEqual(true, YRPacketHeaderIsRST(header));

	XCTAssertEqual(false, YRPacketHeaderIsNUL(header));
	YRPacketHeaderSetNUL(header);
	XCTAssertEqual(true, YRPacketHeaderIsNUL(header));

	XCTAssertEqual(false, YRPacketHeaderHasCHK(header));
	YRPacketHeaderSetCHK(header);
	XCTAssertEqual(true, YRPacketHeaderHasCHK(header));
	
	XCTAssertEqual(
		(YRPacketDescriptionSYN | YRPacketDescriptionRST | YRPacketDescriptionNUL | YRPacketDescriptionCHK),
		YRPacketHeaderGetPacketDescription(header)
	);
	
	enum YRPacketDescription expected = (YRPacketDescriptionRST | YRPacketDescriptionCHK);
	YRPacketHeaderSetPacketDescription(header, expected);
	
	XCTAssertEqual(
		expected,
		YRPacketHeaderGetPacketDescription(header)
	);
	
}

- (void)testProtocolVersion {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;

	YRProtocolVersionType expected = 255;
	YRPacketHeaderSetProtocolVersion(header, expected);
	XCTAssertEqual(expected, YRPacketHeaderGetProtocolVersion(header));
}

- (void)testReserved {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;

	uint8_t expected = 127;
	YRPacketHeaderSetReserved(header, expected);
	XCTAssertEqual(expected, YRPacketHeaderGetReserved(header));
}

- (void)testHeaderLength {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;
	
	YRHeaderLengthType expected = 100;
	YRPacketHeaderSetHeaderLength(header, expected);
	XCTAssertEqual(expected, YRPacketHeaderGetHeaderLength(header));
}

- (void)testFlowControl {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;

	YRSequenceNumberType ack = 9993;
	YRSequenceNumberType seq = 12333;
	
	YRPacketHeaderSetAckNumber(header, ack);
	XCTAssertEqual(true, YRPacketHeaderHasACK(header));
	XCTAssertEqual(true, YRPacketHeaderHasACKOrEACK(header));
	XCTAssertEqual(ack, YRPacketHeaderGetAckNumber(header));
	
	YRPacketHeaderSetSequenceNumber(header, seq);
	XCTAssertEqual(seq, YRPacketHeaderGetSequenceNumber(header));
}

- (void)testChecksum {
	YRPacketHeaderRef header = (YRPacketHeaderRef)buffer;
	YRChecksumType expected = 0xFF432;
	
	YRPacketHeaderSetChecksum(header, expected);
	XCTAssertEqual(expected, YRPacketHeaderGetChecksum(header));
}

#pragma mark - SYN Header

- (void)testSYNBaseHeader {
	YRPacketHeaderSYNRef header = (YRPacketHeaderSYNRef)buffer;
	XCTAssert(YRPacketHeaderSYNGetBaseHeader(header) != NULL);
}

- (void)testSYNConfiguration {
	YRPacketHeaderSYNRef header = (YRPacketHeaderSYNRef)buffer;
	YRRUDPConnectionConfiguration config = (YRRUDPConnectionConfiguration) {
		500,
		30,
		15,
		10,
		253,
		63
	};
	
	YRPacketHeaderSYNSetConfiguration(header, config);
	
	YRRUDPConnectionConfiguration actual = YRPacketHeaderSYNGetConfiguration(header);

	XCTAssertEqual(config.options, actual.options);
	XCTAssertEqual(config.retransmissionTimeoutValue, actual.retransmissionTimeoutValue);
	XCTAssertEqual(config.nullSegmentTimeoutValue, actual.nullSegmentTimeoutValue);
	XCTAssertEqual(config.maximumSegmentSize, actual.maximumSegmentSize);
	XCTAssertEqual(config.maxRetransmissions, actual.maxRetransmissions);
	XCTAssertEqual(config.maxNumberOfOutstandingSegments, actual.maxNumberOfOutstandingSegments);
}

#pragma mark - RST Header

- (void)testRSTBaseHeader {
	YRPacketHeaderRSTRef header = (YRPacketHeaderRSTRef)buffer;
	XCTAssert(YRPacketHeaderRSTGetBaseHeader(header) != NULL);
}

- (void)testRSTErrorCode {
	YRPacketHeaderRSTRef header = (YRPacketHeaderRSTRef)buffer;
	YRRUDPError expected = YRRUDPErrorNone;
	
	YRPacketHeaderRSTSetErrorCode(header, expected);
	XCTAssertEqual(expected, YRPacketHeaderRSTGetErrorCode(header));
}

#pragma mark - Payload Header

- (void)testPayloadBaseHeader {
	YRPacketPayloadHeaderRef header = (YRPacketPayloadHeaderRef)buffer;
	XCTAssert(YRPacketPayloadHeaderGetBaseHeader(header) != NULL);
}

- (void)testPayloadPayloadLength {
	YRPacketPayloadHeaderRef header = (YRPacketPayloadHeaderRef)buffer;
	
	YRPayloadLengthType expected = 45645;
	YRPacketPayloadHeaderSetPayloadLength(header, expected);
	XCTAssertEqual(expected, YRPacketPayloadHeaderGetPayloadLength(header));
}

#pragma mark - EACK Header

- (void)testEACKPayloadHeader {
	YRPacketHeaderEACKRef header = (YRPacketHeaderEACKRef)buffer;
	XCTAssert(YRPacketHeaderEACKGetPayloadHeader(header) != NULL);
}

- (void)testEACKBaseHeader {
	YRPacketHeaderEACKRef header = (YRPacketHeaderEACKRef)buffer;
	XCTAssert(YRPacketHeaderEACKGetBaseHeader(header) != NULL);
}

- (void)testEACKs {
	YRPacketHeaderEACKRef header = (YRPacketHeaderEACKRef)buffer;
	YRHeaderLengthType eacksCount = (YRHeaderLengthType)(~0) / sizeof(YRSequenceNumberType);
	YRSequenceNumberType *eacks = malloc(sizeof(YRSequenceNumberType) * eacksCount);
	
	for (YRHeaderLengthType i = 0; i < eacksCount; i++) {
		eacks[i] = arc4random() % (YRSequenceNumberType)(~0);
	}
	
	YRPacketHeaderSetEACKs(header, eacks, eacksCount);
	
	YRHeaderLengthType actualEacksCount = 0;
	YRSequenceNumberType *actualEacks = YRPacketHeaderGetEACKs(header, &actualEacksCount);
	
	XCTAssertEqual(eacksCount, actualEacksCount);
	
	for (YRHeaderLengthType i = 0; i < actualEacksCount; i++) {
		XCTAssertEqual(eacks[i], actualEacks[i]);
	}
}

@end
