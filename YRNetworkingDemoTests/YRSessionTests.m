//
//  YRSessionTests.m
//  YRNetworkingDemoTests
//
//  Created by Yuriy Romanchenko on 8/1/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <XCTest/XCTest.h>

#import "YRSession.h"
#import "YRPacket.h"

@interface YRSessionTests : XCTestCase
@end

@implementation YRSessionTests {
    YRSession *_sessionOne;
    YRSession *_sessionTwo;
    
    BOOL _simulateLatency;
    uint32_t _currentPacketNumber;
    uint32_t _sessionOnePacketLossPercents;
    uint32_t _sessionTwoPacketLossPercents;
    
    void (^_connectionStateCallout) (YRSession *session, YRSessionState newState);
    void (^_sendCallout) (YRSession *session, NSData *data);
    void (^_receiveCallout) (YRSession *session, NSData *data);
    
    void (^_sessionTwoSendCallout) (YRSession *session, NSData *data);
    void (^_sessionTwoReceiveCallout) (YRSession *session, NSData *rcvedData);
}

- (void)setUp {
    [super setUp];
    
    YRSessionContext *contextOne = [self createSessionContext];
    YRSessionContext *contextTwo = [self createSessionContext];
    
    _sessionOne = [[YRSession alloc] initWithContext:contextOne];
    _sessionTwo = [[YRSession alloc] initWithContext:contextTwo];
}

- (void)tearDown {
    _sessionOne = nil;
    _sessionTwo = nil;
    
    _currentPacketNumber = 0;
    _sessionOnePacketLossPercents = 0;
    _sessionTwoPacketLossPercents = 0;
    _simulateLatency = NO;
    
    _connectionStateCallout = nil;
    _sendCallout = nil;
    _receiveCallout = nil;

    _sessionTwoSendCallout = nil;
    _sessionTwoReceiveCallout = nil;
    
    [super tearDown];
}

#pragma mark - Private

- (YRSessionContext *)createSessionContext {
    __typeof(self) __weak weakSelf = self;

    YRSessionContext *context = [YRSessionContext new];
    context.peerAddress = nil;
    context.connectionStateCallout = ^(YRSession *session, YRSessionState newState) {
        [weakSelf newConnectionState:newState forSession:session];
    };
    
    context.receiveCallout = ^(YRSession *session, NSData *receivedData) {
        [weakSelf receivedData:receivedData fromSession:session];
    };
    
    context.sendCallout = ^(YRSession *session, NSData *dataToSend) {
        [weakSelf sendData:dataToSend fromSession:session];
    };
    
    return context;
}

- (void)newConnectionState:(YRSessionState)state forSession:(YRSession *)session {
    !_connectionStateCallout ?: _connectionStateCallout(session, state);
}

- (void)receivedData:(NSData *)data fromSession:(YRSession *)session {
    NSLog(@"Received: '%@' from: %@", data, session);
    
    if (session == _sessionTwo) {
        !_sessionTwoReceiveCallout ?: _sessionTwoReceiveCallout(session, data);
    }
}

- (void)sendData:(NSData *)data fromSession:(YRSession *)session {
    if (session == _sessionTwo) {
        !_sessionTwoSendCallout ?: _sessionTwoSendCallout(session, data);
    }
    
    __block NSData *retainedData = data;
    
    dispatch_block_t receiveBlock = ^{
        YRSession *sessionToReceive = (session == self->_sessionOne) ? self->_sessionTwo : self->_sessionOne;
        
        [sessionToReceive receive:retainedData];
    };
    
    uint32_t packetLoss = (session == self->_sessionOne) ? _sessionOnePacketLossPercents : _sessionTwoPacketLossPercents;
    
    uint32_t numberOfPacketsThatWontGetThrough = (uint32_t)round(packetLoss / 10.0);
    BOOL shouldDropPacket = packetLoss == 0 ? NO : ((_currentPacketNumber % (numberOfPacketsThatWontGetThrough + 1)) != 0);
    
    _currentPacketNumber++;
    
    if (shouldDropPacket) {
        return;
    }
    
    if (_simulateLatency) {
        retainedData = [NSData dataWithBytes:data.bytes length:data.length];
        dispatch_async(dispatch_get_main_queue(), receiveBlock);
    } else {
        receiveBlock();
    }
}

#pragma mark - Tests

- (void)testWaitConnectConnectionEstablishmentOrder {
    XCTestExpectation *expectation = [self expectationWithDescription:@"Wait/Connect connection established."];

    _connectionStateCallout = ^(YRSession *session, YRSessionState newState) {
        if (self->_sessionOne.state == kYRSessionStateConnected && self->_sessionTwo.state == kYRSessionStateConnected) {
            [expectation fulfill];
        }
    };
    
    [_sessionOne wait];
    [_sessionTwo connect];
    
    [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testConnectWaitConnectionEstablishmentOrder {
    XCTestExpectation *expectation = [self expectationWithDescription:@"Connect/Wait connection established."];
    
    _connectionStateCallout = ^(YRSession *session, YRSessionState newState) {
        if (self->_sessionOne.state == kYRSessionStateConnected && self->_sessionTwo.state == kYRSessionStateConnected) {
            [expectation fulfill];
        }
    };
    
    _simulateLatency = YES;
    
    [_sessionTwo connect];
    [_sessionOne wait];
    
    [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testSimultaneousConnectionEstablishment {
    XCTestExpectation *expectation = [self expectationWithDescription:@"Simultaneous connection established."];
    
    _connectionStateCallout = ^(YRSession *session, YRSessionState newState) {
        if (self->_sessionOne.state == kYRSessionStateConnected && self->_sessionTwo.state == kYRSessionStateConnected) {
            [expectation fulfill];
        }
    };

    _simulateLatency = YES;
    
    [_sessionOne connect];
    [_sessionTwo connect];
    
    [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testEACKPacket {
    XCTestExpectation *expectation = [self expectationWithDescription:@"Simultaneous connection established."];
    
    _connectionStateCallout = ^(YRSession *session, YRSessionState newState) {
        if (self->_sessionOne.state == kYRSessionStateConnected && self->_sessionTwo.state == kYRSessionStateConnected) {
            [expectation fulfill];
        }
    };
    
    [_sessionTwo wait];
    [_sessionOne connect];

    [self waitForExpectationsWithTimeout:5 handler:nil];
    
    YRSequenceNumberType nextSeq = [[_sessionOne valueForKey:@"_sendNextSequenceNumber"] intValue];
    YRSequenceNumberType lastAckedSeq = [[_sessionOne valueForKey:@"_rcvLatestAckedSegment"] intValue];

    YRSequenceNumberType packetsToSend = 240;
    
    for (int i = 1; i < packetsToSend + 1; i++) {
        YRPacketRef outOfSeqPacket = YRPacketCreateWithPayload(nextSeq + i, lastAckedSeq, &i, sizeof(int));
        uint8_t buffer[YRPacketGetLength(outOfSeqPacket)];

        uint8 bufferForStream[kYRLightweightOutputStreamSize];
        YRLightweightOutputStreamRef stream = YRLightweightOutputStreamCreateAt(buffer, YRPacketGetLength(outOfSeqPacket), bufferForStream);
        
        YRPacketSerialize(outOfSeqPacket, stream);
        
        NSData *bytes = [NSData dataWithBytesNoCopy:YRLightweightOutputStreamGetBytes(stream) length:YRPacketGetLength(outOfSeqPacket) freeWhenDone:NO];

        void (^sendCallout) (YRSession *session, NSData *data) = ^(YRSession *session, NSData *dataToSend) {
            uint8_t bufferForStream[kYRLightweightInputStreamSize];
            
            YRLightweightInputStreamRef stream = YRLightweightInputStreamCreateAt(dataToSend.bytes, dataToSend.length, bufferForStream);
            
            XCTAssertTrue(YRPacketCanDeserializeFromStream(stream), @"Can't deserialize packet!");
            
            YRPacketRef outgoingPacket = YRPacketDeserialize(stream);
            
            XCTAssertTrue(outgoingPacket != NULL, @"Packet could be deserialized but deserialization function returned NULL");
            
            YRPacketHeaderRef header = YRPacketGetHeader(outgoingPacket);
            
            XCTAssertTrue(YRPacketHeaderHasEACK(header), @"Packet header MUST have EACK(s)");
            
            YRPacketHeaderEACKRef eackHeader = (YRPacketHeaderEACKRef)header;
            
            YRSequenceNumberType eacksCount = 0;
            
            YRSequenceNumberType *eacks = YRPacketHeaderGetEACKs(eackHeader, &eacksCount);
            
            XCTAssertTrue(eacksCount == i);
            
            for (YRSequenceNumberType iterator = 0; iterator < eacksCount; iterator++) {
                XCTAssertTrue(eacks[iterator] == nextSeq + (iterator + 1), @"Invalid EACK format in header!");
            }

            YRPacketDestroy(outgoingPacket);
        };
        
        _sessionTwoSendCallout = sendCallout;
        
        [_sessionTwo receive:bytes];
    }

    int payloadWithZero = 0;
    
    YRPacketRef correctSeqPacket = YRPacketCreateWithPayload(nextSeq, lastAckedSeq, &payloadWithZero, sizeof(int));
    uint8_t buffer[YRPacketGetLength(correctSeqPacket)];
    
    uint8 bufferForStream[kYRLightweightOutputStreamSize];
    YRLightweightOutputStreamRef stream = YRLightweightOutputStreamCreateAt(buffer, YRPacketGetLength(correctSeqPacket), bufferForStream);
    
    YRPacketSerialize(correctSeqPacket, stream);
    
    NSData *bytes = [NSData dataWithBytesNoCopy:YRLightweightOutputStreamGetBytes(stream) length:YRPacketGetLength(correctSeqPacket) freeWhenDone:NO];

    void (^sendCallout) (YRSession *session, NSData *data) = ^(YRSession *session, NSData *dataToSend) {
        uint8_t bufferForStream[kYRLightweightInputStreamSize];
        
        YRLightweightInputStreamRef stream = YRLightweightInputStreamCreateAt(dataToSend.bytes, dataToSend.length, bufferForStream);
        
        XCTAssertTrue(YRPacketCanDeserializeFromStream(stream), @"Can't deserialize packet!");
        
        YRPacketRef outgoingPacket = YRPacketDeserialize(stream);
        
        XCTAssertTrue(outgoingPacket != NULL, @"Packet could be deserialized but deserialization function returned NULL");
        
        YRPacketHeaderRef header = YRPacketGetHeader(outgoingPacket);
        
        XCTAssertTrue(!YRPacketHeaderHasEACK(header), @"Packet header MUST not have EACK");
        
        XCTAssertTrue(YRPacketHeaderHasACK(header), @"Packet header MUST have ACK");
        
        YRSequenceNumberType ackNumber = YRPacketHeaderGetAckNumber(header);
        
        XCTAssertTrue(ackNumber == nextSeq + packetsToSend, @"Unexpected ACK by session two.");
        
        YRPacketDestroy(outgoingPacket);
    };
    
    __block int expectedToReceive = 0;
    
    void (^rcvCallout) (YRSession *session, NSData *data) = ^(YRSession *session, NSData *rcvedData) {
        XCTAssertTrue(rcvedData.length == sizeof(expectedToReceive), @"Payload size mismatch!");
        
        int received = 0;
        
        [rcvedData getBytes:&received length:rcvedData.length];
        
        XCTAssertTrue(received == expectedToReceive, @"Incorrect callout sequence for receiving session!");
        
        expectedToReceive++;
    };
    
    _sessionTwoSendCallout = sendCallout;
    _sessionTwoReceiveCallout = rcvCallout;
    
    [_sessionTwo receive:bytes];
}

@end
