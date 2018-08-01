//
//  main.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <arpa/inet.h>
#import "YRPacketHeader.h"

int main(int argc, const char * argv[]) {
//    uint64_t a = 0x0BB000DC0AFF0000;
//
//    uint8_t *byte = (uint8_t *)&a;
//
//    NSLog(@"[0]: 0x%02x", byte[0]);
//    NSLog(@"[1]: 0x%02x", byte[1]);
//    NSLog(@"[2]: 0x%02x", byte[2]);
//    NSLog(@"[3]: 0x%02x", byte[3]);
//    NSLog(@"[4]: 0x%02x", byte[4]);
//    NSLog(@"[5]: 0x%02x", byte[5]);
//    NSLog(@"[6]: 0x%02x", byte[6]);
//    NSLog(@"[7]: 0x%02x", byte[7]);
//
//    int64_t b = htonll(a);
//
//    NSLog(@"======================");
//
//    byte = (uint8_t *)&b;
//
//    NSLog(@"[0]: 0x%02x", byte[0]);
//    NSLog(@"[1]: 0x%02x", byte[1]);
//    NSLog(@"[2]: 0x%02x", byte[2]);
//    NSLog(@"[3]: 0x%02x", byte[3]);
//    NSLog(@"[4]: 0x%02x", byte[4]);
//    NSLog(@"[5]: 0x%02x", byte[5]);
//    NSLog(@"[6]: 0x%02x", byte[6]);
//    NSLog(@"[7]: 0x%02x", byte[7]);
//
//    YRPacketHeader header = {0};
//
//    YRPacketHeaderSetSYN(&header);
//    YRPacketHeaderSetSequenceNumber(&header, 0xA0);
//    YRPacketHeaderSetAckNumber(&header, 0xB0);
//    YRPacketHeaderSetDataLength(&header, 0xFFFF);
//    YRPacketHeaderFinalize(&header);
//
//    YRPacketHeader copy = header;
//    uint32_t *buffer = (uint32_t *)&copy;
//    buffer[0] = htonl(buffer[0]);
//    buffer[1] = htonl(buffer[1]);
//    buffer[2] = htonl(buffer[2]);
//
    return NSApplicationMain(argc, argv);
}
