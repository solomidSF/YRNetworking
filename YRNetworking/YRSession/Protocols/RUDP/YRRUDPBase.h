//
// YRRUDPBase.h
//
// The MIT License (MIT)
//
// Copyright (c) 2019 Yuri R.
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

#ifndef __YRRUDPBase__
#define __YRRUDPBase__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

//#define YRMakeMultipleTo(what, to) (((what) + ((to) - 1)) & (~((to) - 1)))
//
//typedef uint8_t YRProtocolVersionType;
//YRProtocolVersionType const kYRProtocolVersion = 0x01;
typedef uint8_t YRPacketDescriptionType;
typedef uint8_t YRProtocolVersionType;
typedef uint8_t YRHeaderLengthType;
typedef uint16_t YRSequenceNumberType;
typedef uint32_t YRChecksumType;

typedef struct {
    uint16_t options; // zero for now
    uint16_t retransmissionTimeoutValue; // ms
    uint16_t nullSegmentTimeoutValue; // ms
    uint16_t maximumSegmentSize;
    uint8_t maxNumberOfOutstandingSegments;
    uint8_t maxRetransmissions;
    //    uint8_t maxCumulativeAck;
    //    uint16_t cumulativeAckTimeoutValue; // ms
} YRRUDPConnectionConfiguration;

typedef enum {
    // Session is closed.
    kYRRUDPSessionStateClosed,
    // Session is waiting for incoming connection request from it's peer.
    kYRRUDPSessionStateWaiting,
    // Session is initiating connection.
    kYRRUDPSessionStateInitiating,
    // Session is establishing connection.
    kYRRUDPSessionStateConnecting,
    // Session has established connection.
    kYRRUDPSessionStateConnected,
    // Session is disconnecting from it's peer.
    kYRRUDPSessionStateDisconnecting
} YRRUDPSessionState;

#endif /* YRRUDPBase_h */
