//
//  YRSession.c
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 8/17/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#include "YRSession.h"
#include "YRPacket.h"
#include "YRPacketsQueue.h"
#include "YRPacketsSendQueue.h"

#include <stdlib.h>

typedef struct YRSession {
    YRSessionState state; // TODO: Reduce size of this one
    uint32_t flags; // Some flags. We will need them, right?
    YRConnectionConfiguration localConnectionConfiguration;
    YRConnectionConfiguration remoteConnectionConfiguration;
    YRSessionCallbacks callbacks;
    
    // Determines if local peer should send NUL segments.
    bool isInitiator;
    
    YRSessionInfo sessionInfo;
    
    YRPacketsQueueRef sendQueue;
    YRPacketsQueueRef receiveQueue;
} YRSession;

static const YRSessionCallbacks kYRNullSessionCallbacks = {NULL, NULL, NULL};

#pragma mark - Prototypes

void YRSessionSetCallbacks(YRSessionRef session, YRSessionCallbacks callbacks);

void YRSessionTransiteToState(YRSessionRef session, YRSessionState state);
/**
 *  As session is created we cannot know in advance what configuration is for remote peer.
 */
// TODO: Let's go hardcore here, don't implement this, send queue will dynamically manage several regions of memory if it's needed.
size_t YRSessionGetFinalMinimumSize(YRConnectionConfiguration localConfiguration, YRConnectionConfiguration remoteConfiguration);

void YRSessionSchedulePacketSendReliably(YRSessionRef session, YRPacketRef packet);
void YRSessionSchedulePacketSendUnreliably(YRSessionRef session, YRPacketRef packet);

#pragma mark - Sizes

// TODO: Probably remove
size_t YRSessionGetMinimumSize(YRConnectionConfiguration configuration) {
    // TODO: Make extern.
//    const short alignmentInBytes = 8;
    const YRPayloadLengthType maximumSegmentSizeAllowed = (YRPayloadLengthType)(~0);
    // TODO: Think about this.
    const YRPayloadLengthType minimumSegmentSizeAllowed = (YRHeaderLengthType)(~0);

    if (configuration.maximumSegmentSize < minimumSegmentSizeAllowed) {
        configuration.maximumSegmentSize = minimumSegmentSizeAllowed;
    }

    if (configuration.maximumSegmentSize > maximumSegmentSizeAllowed) {
        configuration.maximumSegmentSize = maximumSegmentSizeAllowed;
    }

    // TODO: Ask yrpacket how much space is needed for segment with this maximum size.
    YRPayloadLengthType alignedMaximumSegmentSize = YRMakeMultipleTo(configuration.maximumSegmentSize, 8);
    YRSequenceNumberType maximumSequencesToReceive = configuration.maxNumberOfOutstandingSegments;
    
    size_t requiredSize = YRMakeMultipleTo(sizeof(YRSession) + alignedMaximumSegmentSize * maximumSequencesToReceive, 8);
    
    return requiredSize;
}

#pragma mark - Lifecycle

YRSessionRef YRSessionCreateWithConfiguration(YRConnectionConfiguration configuration, YRSessionCallbacks callbacks) {
    YRSessionRef session = calloc(1, YRSessionGetMinimumSize(configuration));
    
    if (session == NULL) {
        return NULL;
    }
    
    session->state = kYRSessionStateClosed;
    session->localConnectionConfiguration = configuration;
    
    YRSessionSetCallbacks(session, callbacks);
    
    return session;
}

void YRSessionDestroy(YRSessionRef session) {
    // TODO: Implement proper destruction
    YRSessionSetCallbacks(session, kYRNullSessionCallbacks);
}

#pragma mark - Configuration

void YRSessionConnect(YRSessionRef session) {
//    [_sessionLogger logInfo:@"[CONN_REQ] (%@)", [self humanReadableState:self.state]];

    if (YRSessionGetState(session) == kYRSessionStateClosed) {
        session->isInitiator = true;
        
        YRSessionTransiteToState(session, kYRSessionStateInitiating);
        
        YRPacketRef packet = YRPacketCreateSYN(session->localConnectionConfiguration, session->sessionInfo.sendNextSequenceNumber, 0, false, NULL);
        session->sessionInfo.sendNextSequenceNumber += 1;
        
        YRSessionSchedulePacketSendReliably(session, packet);
    } else {
//        [_sessionLogger logWarning:@"[CONN_REQ]: Trying to transite into '%@' from %@", [self humanReadableState:kYRSessionStateInitiating], [self humanReadableState:self.state]];
    }
}

void YRSessionWait(YRSessionRef session) {
    
}

void YRSessionClose(YRSessionRef session) {
    
}

void YRSessionInvalidate(YRSessionRef session) {
    
}

#pragma mark - Communication

void YRSessionReceive(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    
}
                                                                                        
void YRSessionSend(YRSessionRef session, void *payload, YRPayloadLengthType length) {
    
}

#pragma mark - State

YRSessionState YRSessionGetState(YRSessionRef session) {
    return session->state;
}

YRSessionInfo YRSessionGetSessionInfo(YRSessionRef session) {
    return session->sessionInfo;
}

YRConnectionConfiguration YRSessionGetLocalConnectionInfo(YRSessionRef session) {
    return session->localConnectionConfiguration;
}

YRConnectionConfiguration YRSessionGetRemoteConnectionInfo(YRSessionRef session) {
    return session->remoteConnectionConfiguration;
}

#pragma mark - Private

void YRSessionSetCallbacks(YRSessionRef session, YRSessionCallbacks callbacks) {
    if (callbacks.connectionStateCallout) {
        callbacks.connectionStateCallout = _Block_copy(callbacks.connectionStateCallout);
    }
    
    if (callbacks.sendCallout) {
        callbacks.sendCallout = _Block_copy(callbacks.sendCallout);
    }
    
    if (callbacks.receiveCallout) {
        callbacks.receiveCallout = _Block_copy(callbacks.receiveCallout);
    }
    
    if (session->callbacks.connectionStateCallout) {
        _Block_release(session->callbacks.connectionStateCallout);
    }

    if (session->callbacks.sendCallout) {
        _Block_release(session->callbacks.sendCallout);
    }

    if (session->callbacks.receiveCallout) {
        _Block_release(session->callbacks.receiveCallout);
    }
    
    session->callbacks = callbacks;
}

void YRSessionTransiteToState(YRSessionRef session, YRSessionState state) {
    if (session->state != state) {
        session->state = state;
    }
}

void YRSessionSchedulePacketSendReliably(YRSessionRef session, YRPacketRef packet) {
    YRPacketCopyPayloadInline(packet);
    
//    YRSequenceNumberType seqNumber = YRPacketHeaderGetSequenceNumber(YRPacketGetHeader(packet));
//    YRSendOperation *operation = [[YRSendOperation alloc] initWithPacket:packet sequenceNumber:seqNumber];
//
//    if ([_sendQueue[seqNumber] isKindOfClass:[YRSendOperation class]]) {
//        [_sendQueue[seqNumber] end];
//    }
//
//    _sendQueue[seqNumber] = operation;
//
//    [operation start];
//
//    __typeof(self) __weak weakSelf = self;
//    __typeof(YRLogger *) __weak weakLogger = _sessionLogger;
//
//    operation.onTransmissionTimeout = ^(YRSendOperation *operation) {
//        [weakLogger logInfo:@"[SEND_TIM]: Header: %@.", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(packet)]];
//
//        [operation start];
//        [weakSelf sendDataFromPacket:operation.packet];
//    };
//
//    [_sessionLogger logInfo:@"[SEND_REL]: Header: %@", [YRDebugUtils packetHeaderShortDescription:YRPacketGetHeader(operation.packet)]];
//
//    [self sendDataFromPacket:packet];
}

void YRSessionSchedulePacketSendUnreliably(YRSessionRef session, YRPacketRef packet) {
    
}

void YRSessionSendPacket(YRSessionRef session, YRPacketRef packet) {
    YRPayloadLengthType packetLength = YRPacketGetLength(packet);
    
    uint8_t outputStreamBuffer[kYRLightweightOutputStreamSize];
    uint8_t packetBuffer[packetLength];
    
    YRLightweightOutputStreamRef outputStream = YRLightweightOutputStreamCreateAt(packetBuffer, packetLength, outputStreamBuffer);
    
    YRPacketSerialize(packet, outputStream);
    
    !session->callbacks.sendCallout ?: session->callbacks.sendCallout(session, YRLightweightOutputStreamGetBytes(outputStream), packetLength);
}
