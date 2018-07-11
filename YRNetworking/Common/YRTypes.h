//
//  YRTypes.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/23/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRTypes_h
#define YRTypes_h

typedef enum {
    // Synchronization segment.
    YRPacketDescriptionSYN = 1 << 0,
    // Indicates acknowledgment number in the header is valid.
    YRPacketDescriptionACK = 1 << 1,
    // Indicates extended acknowledge segment is present.
    YRPacketDescriptionEAK = 1 << 2,
    // Indicates the packet is a reset segment.
    YRPacketDescriptionRST = 1 << 3,
    // Indicates the packet is a null segment.
    YRPacketDescriptionNUL = 1 << 4,
    // Indicates whether the Checksum field contains the checksum of just the header or the header and the body (data).
    YRPacketDescriptionCHK = 1 << 5,
    // Version of protocol used to generate this packet.
    YRPacketDescriptionProtocolVersionMask = 0x11 << 6
} YRPacketDescription;

// SYN Segment:
// 0             7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| |A| | | | | | |               |
//|1|C|0|0|0|0|0|0|       28      |
//| |K| | | | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//+  Sequence #   +   Ack Number  |
//+---------------+---------------+
//| Vers  | Spare | Max # of Out  |
//|       |       | standing Segs |
//+---------------+---------------+
//| Option Flags  |     Spare     |
//+---------------+---------------+
//|      Maximum Segment Size     |
//+---------------+---------------+
//| Retransmission Timeout Value  |
//+---------------+---------------+
//| Cumulative Ack Timeout Value  |
//+---------------+---------------+
//|   Null Segment Timeout Value  |
//+---------------+---------------+
//| Transfer State Timeout Value  |
//+---------------+---------------+
//|  Max Retrans  | Max Cum Ack   |
//+---------------+---------------+
//| Max Out of Seq| Max Auto Reset|
//+---------------+---------------+
//|    Connection Identifier      |
//+                               +
//|      (32 bits in length)      |
//+---------------+---------------+
//|           Checksum            |
//+---------------+---------------+

// ACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|0|0|0|0|0|0|       6       |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|           Checksum            |
//+---------------+---------------+

// EACK segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|1|0|0|0|0|0|     N + 6     |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|1st out of seq |2nd out of seq |
//|  ack number   |   ack number  |
//+---------------+---------------+
//|  . . .        |Nth out of seq |
//|               |   ack number  |
//+---------------+---------------+
//|            Checksum           |
//+---------------+---------------+

// RST segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//| |A| | | | | | |               |
//|0|C|0|1|0|0|0|0|        6      |
//| |K| | | | | | |               |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |   Ack Number  |
//+---------------+---------------+
//|         Header Checksum       |
//+---------------+---------------+

// NUL segment:
// 0 1 2 3 4 5 6 7 8            15
//+-+-+-+-+-+-+-+-+---------------+
//|0|1|0|0|1|0|0|0|       6       |
//+-+-+-+-+-+-+-+-+---------------+
//| Sequence #    |  Ack Number   |
//+---------------+---------------+
//|            Checksum           |
//+---------------+---------------+

typedef struct  {
    YRPacketDescription packetDescription;
    uint8_t headerLength;
    uint8_t sequenceNumber;
    uint8_t ackNumber;
    uint16_t dataLength;
    uint32_t checksum;
    void *variableData;
} YRPacketHeader;

#define YRMinimumPacketHeaderSize (sizeof(YRPacketHeader) - sizeof(void *))

typedef struct  {
    YRPacketDescription packetDescription;
    uint8_t headerLength;
    uint8_t sequenceNumber;
    uint8_t ackNumber;
    uint8_t version;
    uint8_t maxNumberOfOutstangingSegments;
    uint16_t flags;
    uint16_t maximumSegmentSize;
    uint16_t retransmissionTimeoutValue; // ms
    uint16_t cumulativeAckTimeoutValue; // ms
    uint16_t nullSegmentTimeoutValue; // ms
    uint16_t tcsTimeoutValue; // ms
    uint8_t maxRetransmissions;
    uint8_t maxCumulativeAck;
    uint8_t maxOutOfSequence;
    uint8_t maxAutoReset;
    uint32_t connectionIdentifier;
    uint16_t checksum;
} YRSYNPacketHeader;

#endif /* YRTypes_h */
