//
//  YRDebugUtils.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/18/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRDebugUtils.h"

@implementation YRDebugUtils

+ (NSString *)packetHeaderTypeDescription:(YRPacketHeaderRef)header {
    NSMutableArray *typeComponents = [NSMutableArray new];
    
    if (YRPacketHeaderIsSYN(header)) {
        [typeComponents addObject:@"SYN"];
    }
    
    if (YRPacketHeaderIsRST(header)) {
        [typeComponents addObject:@"RST"];
    }
    
    if (YRPacketHeaderIsNUL(header)) {
        [typeComponents addObject:@"NUL"];
    }
    
    if (YRPacketHeaderHasACK(header)) {
        [typeComponents addObject:@"ACK"];
    }
    
    if (YRPacketHeaderHasEACK(header)) {
        [typeComponents addObject:@"EACK"];
    }
    
    if (typeComponents.count == 0) {
        [typeComponents addObject:@"N/A"];
    }
    
    return [NSString stringWithFormat:@"Packet Type: [%@]", [typeComponents componentsJoinedByString:@"|"]];
}

+ (NSString *)packetHeaderShortDescription:(YRPacketHeaderRef)header {
    NSMutableArray *shortDescriptionComponents = [NSMutableArray new];
    
    [shortDescriptionComponents addObject:[NSString stringWithFormat:@"Seq#: %d", YRPacketHeaderGetSequenceNumber(header)]];
    [shortDescriptionComponents addObject:[self packetHeaderTypeDescription:header]];
//    [shortDescriptionComponents addObject:[NSString stringWithFormat:@"Is Logically Valid: %@", YRPacketHeaderIsLogicallyValid(header) ? @"YES" : @"NO"]];
    
    return [shortDescriptionComponents componentsJoinedByString:@" "];
}

+ (NSString *)packetHeaderFullDescription:(YRPacketHeaderRef)header {
    NSMutableArray *packetComponents = [NSMutableArray new];
    
    [packetComponents addObject:@"Packet Description:"];
    
    // 1. Type
    [packetComponents addObject:[self packetHeaderTypeDescription:header]];
    
    // 2. Protocol
    [packetComponents addObject:[NSString stringWithFormat:@"Protocol: %d", YRPacketHeaderGetProtocolVersion(header)]];
    
    // 3. Is Logically Valid
//    [packetComponents addObject:[NSString stringWithFormat:@"Is Logically Valid: %@", YRPacketHeaderIsLogicallyValid(header) ? @"YES" : @"NO"]];
    
    // 4. Header Length
    [packetComponents addObject:[NSString stringWithFormat:@"Header Length: %d bytes", YRPacketHeaderGetHeaderLength(header)]];
    
    // 5. Sequence Number
    [packetComponents addObject:[NSString stringWithFormat:@"Sequence #: %d", YRPacketHeaderGetSequenceNumber(header)]];
    
    // 6. Ack number
    [packetComponents addObject:[NSString stringWithFormat:@"Latest Ack'ed Seq #: %d", YRPacketHeaderGetAckNumber(header)]];
    
    // 5. Data Length
    [packetComponents addObject:[NSString stringWithFormat:@"Payload Length: %d bytes", YRPacketHeaderGetPayloadLength(header)]];
    
    // 7. Checksum
    [packetComponents addObject:[NSString stringWithFormat:@"Checksum: %d", YRPacketHeaderGetChecksum(header)]];
    
    return [packetComponents componentsJoinedByString:@"\n"];
}

@end
