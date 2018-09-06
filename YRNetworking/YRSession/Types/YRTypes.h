//
//  YRTypes.h
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/23/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#ifndef YRTypes_h
#define YRTypes_h

#import "YRPacketHeader.h"

//#define YRMakeMultipleTo(what, to) (((what) + ((to) - 1)) & (~((to) - 1)))
//
//typedef uint8_t YRProtocolVersionType;
//YRProtocolVersionType const kYRProtocolVersion = 0x01;
typedef uint8_t YRPacketDescriptionType;
typedef uint8_t YRProtocolVersionType;
typedef uint8_t YRHeaderLengthType;
typedef uint16_t YRSequenceNumberType;
typedef uint16_t YRPayloadLengthType;
typedef uint32_t YRChecksumType;

#endif /* YRTypes_h */
