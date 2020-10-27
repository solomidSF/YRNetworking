//
//  YRPacketHeaderTests.m
//  YRNetworkingDemoTests
//
//  Created by Yuriy Romanchenko on 8/6/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "YRNetworking.h"
#import "YRDebugUtils.h"

@interface YRPacketHeaderTests : XCTestCase

@end

@implementation YRPacketHeaderTests {
}

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testPacketSizes {
    XCTAssertTrue(kYRPacketHeaderGenericLength == 10);
    XCTAssertTrue(kYRPacketHeaderSYNLength == 20);
    XCTAssertTrue(kYRPacketHeaderRSTLength == 11);
    XCTAssertTrue(kYRPacketPayloadHeaderLength == 12);
    
    for (YRSequenceNumberType i = 0; i < (YRSequenceNumberType)(~0); i++) {
        YRSequenceNumberType eacks = i;
        YRHeaderLengthType length = YRPacketHeaderEACKLength(&eacks);
        
        XCTAssertTrue(eacks * sizeof(YRSequenceNumberType) + kYRPacketPayloadHeaderLength == length);
    }
}

- (void)testGenericPacketHeader {
    BOOL traverseAllSeqAndAcks = NO;
    BOOL traverseAllHeaderValues = NO;
    BOOL traverseAllPayloadValues = NO;
    BOOL traverseAllChecksumValues = NO;
    
    uint32_t seqAndAckIterations = traverseAllSeqAndAcks ? (YRSequenceNumberType)(~0) : 31;
    uint32_t headerIterations = traverseAllHeaderValues ? (YRHeaderLengthType)(~0) : 15;
    uint32_t payloadIterations = traverseAllPayloadValues ? (YRPayloadLengthType)(~0) : 15;
    uint32_t checksumIterations = traverseAllChecksumValues ? (YRChecksumType)(~0) : 15;
    
    uint64_t totalIterations = (uint64_t)(seqAndAckIterations + 1) * (seqAndAckIterations + 1) * 4 * headerIterations * (payloadIterations + 1) * (checksumIterations + 1) * 32;
    NSLog(@"Total: %llu iterations.", totalIterations);
    NSLog(@"Iterations per ack increment: %llu.", totalIterations / ((uint64_t)(seqAndAckIterations + 1) * (seqAndAckIterations + 1)));
    
    for (YRSequenceNumberType seqIterator = 0; seqIterator <= seqAndAckIterations; seqIterator++) {
        NSLog(@"SEQ Incremented: %d", seqIterator);
        for (YRSequenceNumberType ackIterator = 0; ackIterator <= seqAndAckIterations; ackIterator++) {
            for (YRProtocolVersionType protocolVersionIterator = 0; protocolVersionIterator < 0x4; protocolVersionIterator++) {
                for (YRHeaderLengthType headerLengthIterator = 0; headerLengthIterator <= headerIterations; headerLengthIterator++) {
                    for (YRPayloadLengthType payloadLengthIterator = 0; payloadLengthIterator <= payloadIterations; payloadLengthIterator++) {
                        for (YRChecksumType checksumIterator = 0; checksumIterator <= checksumIterations; checksumIterator++) {
                            for (int synIterator = 0; synIterator < 2; synIterator++) {
                                for (int rstIterator = 0; rstIterator < 2; rstIterator++) {
                                    for (int nulIterator = 0; nulIterator < 2; nulIterator++) {
                                        for (int chkIterator = 0; chkIterator < 2; chkIterator++) {
                                            for (int hasAckIterator = 0; hasAckIterator < 2; hasAckIterator++) {
                                                // 1. Given
                                                BOOL isSYN = synIterator > 0;
                                                BOOL isRST = rstIterator > 0;
                                                BOOL isNUL = nulIterator > 0;
                                                BOOL hasCHK = chkIterator > 0;
                                                BOOL hasACK = hasAckIterator > 0;

                                                YRSequenceNumberType seqNumber = seqIterator * (YRSequenceNumberType)(~0) / (seqAndAckIterations);
                                                YRSequenceNumberType ackNumber = ackIterator * (YRSequenceNumberType)(~0) / (seqAndAckIterations);
                                                YRProtocolVersionType protocolVersion = protocolVersionIterator;
                                                YRHeaderLengthType headerLength = headerLengthIterator * (YRHeaderLengthType)(~0) / (headerIterations);
                                                YRPayloadLengthType payloadLength = payloadLengthIterator * (YRPayloadLengthType)(~0) / (payloadIterations);
                                                YRChecksumType checksum = checksumIterator * (YRChecksumType)(~0) / (checksumIterations);
                                                
                                                // 2. When
                                                // Set header variables.
                                                
                                                uint8_t headerBuffer[(YRHeaderLengthType)(~0)];
                                                memset(headerBuffer, 0, (YRHeaderLengthType)(~0));
                                                
                                                YRPacketHeaderRef header = (YRPacketHeaderRef)headerBuffer;
                                                
                                                if (isSYN) {
                                                    YRPacketHeaderSetSYN(header);
                                                }
                                                
                                                if (isRST) {
                                                    YRPacketHeaderSetRST(header);
                                                }
                                                
                                                if (isNUL) {
                                                    YRPacketHeaderSetNUL(header);
                                                }
                                                
                                                YRPacketHeaderSetProtocolVersion(header, protocolVersion);
                                                YRPacketHeaderSetHeaderLength(header, headerLength);
                                                YRPacketHeaderSetSequenceNumber(header, seqNumber);
                                                
                                                if (hasACK) {
                                                    YRPacketHeaderSetAckNumber(header, ackNumber);
                                                }
                                                
                                                if (hasCHK) {
                                                    YRPacketHeaderSetCHK(header);
                                                }
                                                
                                                YRPacketHeaderSetChecksum(header, checksum);
                                                
                                                if (YRPacketHeaderHasPayloadLength(header)) {
                                                    YRPacketHeaderSetPayloadLength((YRPacketPayloadHeaderRef)header, payloadLength);
                                                }

                                                // 3. Then
                                                // Check header variables.
                                                XCTAssertTrue(isSYN == YRPacketHeaderIsSYN(header));
                                                XCTAssertTrue(isRST == YRPacketHeaderIsRST(header));
                                                XCTAssertTrue(isNUL == YRPacketHeaderIsNUL(header));
                                                
                                                XCTAssertTrue(protocolVersion == YRPacketHeaderGetProtocolVersion(header));
                                                XCTAssertTrue(headerLength == YRPacketHeaderGetHeaderLength(header));
                                                XCTAssertTrue(seqNumber == YRPacketHeaderGetSequenceNumber(header));
                                                
                                                XCTAssertTrue(hasACK == YRPacketHeaderHasACK(header));
                                                XCTAssertTrue(hasACK == YRPacketHeaderHasACKOrEACK(header));
                                                
                                                if (hasACK) {
                                                    XCTAssertTrue(ackNumber == YRPacketHeaderGetAckNumber(header));
                                                }
                                                
                                                XCTAssertTrue(hasCHK == YRPacketHeaderHasCHK(header));
                                                
                                                if (YRPacketHeaderHasPayloadLength(header)) {
                                                    XCTAssertTrue(payloadLength == YRPacketHeaderGetPayloadLength((YRPacketPayloadHeaderRef)header));
                                                }
                                                
                                                XCTAssertTrue(checksum == YRPacketHeaderGetChecksum(header));
                                                
                                                XCTAssertTrue(!YRPacketHeaderHasEACK(header));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

- (void)testSYNPacketHeader {
    for (uint16_t optionsIterator = 0; optionsIterator <= 15; optionsIterator++) {
        for (uint16_t retransmissionIterator = 0; retransmissionIterator <= 15; retransmissionIterator++) {
            for (uint16_t nullTimeoutIterator = 0; nullTimeoutIterator <= 15; nullTimeoutIterator++) {
                for (uint16_t maxSegmentSizeIterator = 0; maxSegmentSizeIterator <= 15; maxSegmentSizeIterator++) {
                    for (uint8_t maxNumberOfSegmentsIterator = 0; maxNumberOfSegmentsIterator <= 15; maxNumberOfSegmentsIterator++) {
                        for (uint8_t maxRetransmissionsIterator = 0; maxRetransmissionsIterator <= 15; maxRetransmissionsIterator++) {
                            YRHeaderLengthType headerLength = kYRPacketHeaderSYNLength;
                            
                            uint8_t headerBuffer[headerLength];
                            memset(headerBuffer, 0, headerLength);

                            YRConnectionConfiguration configuration = {
                                .options = optionsIterator * (uint16_t)(~0) / 15,
                                .retransmissionTimeoutValue = retransmissionIterator * (uint16_t)(~0) / 15,
                                .nullSegmentTimeoutValue = nullTimeoutIterator * (uint16_t)(~0) / 15,
                                .maximumSegmentSize = maxSegmentSizeIterator * (uint16_t)(~0) / 15,
                                .maxNumberOfOutstandingSegments = maxNumberOfSegmentsIterator * (uint8_t)(~0) / 15,
                                .maxRetransmissions = maxRetransmissionsIterator * (uint8_t)(~0) / 15,
                            };
                            
                            YRPacketHeaderRef header = (YRPacketHeaderRef)headerBuffer;
                            
                            YRPacketHeaderSetSYN(header);
                            YRPacketHeaderSetHeaderLength(header, headerLength);
                            
                            XCTAssertTrue(YRPacketHeaderIsSYN(header));
                            XCTAssertTrue(YRPacketHeaderGetHeaderLength(header) == headerLength);
                            
                            YRPacketHeaderSYNRef synHeader = (YRPacketHeaderSYNRef)header;
                            
                            YRPacketSYNHeaderSetConfiguration(synHeader, configuration);
                            
                            YRConnectionConfiguration returnedConfiguration = YRPacketSYNHeaderGetConfiguration(synHeader);
                            
                            XCTAssertTrue(returnedConfiguration.options == configuration.options);
                            XCTAssertTrue(returnedConfiguration.retransmissionTimeoutValue == configuration.retransmissionTimeoutValue);
                            XCTAssertTrue(returnedConfiguration.nullSegmentTimeoutValue == configuration.nullSegmentTimeoutValue);
                            XCTAssertTrue(returnedConfiguration.maximumSegmentSize == configuration.maximumSegmentSize);
                            XCTAssertTrue(returnedConfiguration.maxNumberOfOutstandingSegments == configuration.maxNumberOfOutstandingSegments);
                            XCTAssertTrue(returnedConfiguration.maxRetransmissions == configuration.maxRetransmissions);
                        }
                    }
                }
            }
        }
    }
}

- (void)testRSTPacketHeader {
    for (uint8_t errorCodeIterator = 0; errorCodeIterator < (uint8_t)(~0); errorCodeIterator++) {
        YRHeaderLengthType headerLength = kYRPacketHeaderRSTLength;
        
        uint8_t headerBuffer[headerLength];
        memset(headerBuffer, 0, headerLength);

        YRPacketHeaderRef header = (YRPacketHeaderRef)headerBuffer;
        
        YRPacketHeaderSetRST(header);
        YRPacketHeaderSetHeaderLength(header, headerLength);

        XCTAssertTrue(YRPacketHeaderIsRST(header));
        XCTAssertTrue(YRPacketHeaderGetHeaderLength(header) == headerLength);
        
        YRPacketHeaderRSTRef rstHeader = (YRPacketHeaderRSTRef)header;
        
        YRPacketRSTHeaderSetErrorCode(rstHeader, errorCodeIterator);
        
        XCTAssertTrue(YRPacketRSTHeaderGetErrorCode(rstHeader) == errorCodeIterator);
    }
}

- (void)testEACKPacketHeader {
    YRHeaderLengthType emptyLength = YRPacketHeaderEACKLength(NULL);
    XCTAssert(emptyLength == kYRPacketPayloadHeaderLength);
    
    YRSequenceNumberType sequencesArray[(YRSequenceNumberType)(~0)];
    memset(sequencesArray, 0, (YRSequenceNumberType)(~0));
    
    YRSequenceNumberType *sequences = sequencesArray;
    
    for (YRSequenceNumberType seqIterator = 0; seqIterator < (YRSequenceNumberType)(~0); seqIterator++) {
        sequences[seqIterator] = arc4random();
    }
    
    for (YRSequenceNumberType eackIterator = 0; eackIterator < (YRSequenceNumberType)(~0); eackIterator++) {
        YRSequenceNumberType sequencesCount = eackIterator;
        YRHeaderLengthType headerLength = YRPacketHeaderEACKLength(&sequencesCount);
        
        uint8_t headerBuffer[headerLength];
        memset(headerBuffer, 0, headerLength);
        
        YRPacketHeaderRef header = (YRPacketHeaderRef)headerBuffer;
        YRPacketHeaderSetHeaderLength(header, headerLength);
        
        XCTAssert(YRPacketHeaderGetHeaderLength(header) == headerLength);

        YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)headerBuffer;

        YRPacketHeaderEACKSetEACKs(eackHeader, sequences, sequencesCount);
        
        if (sequencesCount > 0) {
            XCTAssertTrue(YRPacketHeaderHasEACK(header));
            XCTAssertTrue(YRPacketHeaderHasACKOrEACK(header));
        }
        
        XCTAssertTrue(YRPacketHeaderEACKsCount(eackHeader) == sequencesCount);

        YRSequenceNumberType returnedEacksCount = 0;
        YRSequenceNumberType *returnedSequences = YRPacketHeaderGetEACKs(eackHeader, &returnedEacksCount);
        
        XCTAssertTrue(returnedEacksCount == sequencesCount);
        
        for (YRSequenceNumberType seqToTestIterator = 0; seqToTestIterator < returnedEacksCount; seqToTestIterator++) {
            XCTAssertTrue(returnedSequences[seqToTestIterator] == sequences[seqToTestIterator]);
        }
        
//        NSLog(@"%@", [YRDebugUtils packetHeaderFullDescription:header]);
    }
}

@end
