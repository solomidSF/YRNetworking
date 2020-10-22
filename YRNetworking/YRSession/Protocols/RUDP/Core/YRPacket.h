//
// YRPacket.h
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

#ifndef __YRPacket__
#define __YRPacket__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

typedef struct YRPacket *YRPacketRef;

#pragma mark - Serialization

YRPacketRef YRPacketSerializeSYN(YRPacketHeaderSYNRef synHeader, YROutputStreamRef stream);
YRPacketRef YRPacketSerializeRST(YRPacketHeaderRSTRef rstHeader, YROutputStreamRef stream);
YRPacketRef YRPacketSerializeNUL(YRPacketHeaderRef nulHeader, YROutputStreamRef stream);
YRPacketRef YRPacketSerializeEACK(YRPacketHeaderEACKRef eackHeader, YROutputStreamRef stream);
YRPacketRef YRPacketSerializeEACKWithPayload(
	YRPacketHeaderEACKRef eackHeader,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
);
YRPacketRef YRPacketSerializeWithPayload(
	YRPacketPayloadHeaderRef header,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
);

//YRPacketRef YRPacketDeserialize(YRInputStreamRef stream);

#pragma mark - Introspection

//YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet);
//void *YRPacketGetPayload(YRPacketRef packet, YRPayloadLengthType *outPayloadSize);
//YRPayloadLengthType YRPacketGetLength(YRPacketRef packet);
//YRRUDPError YRPacketCanDeserialize(YRInputStreamRef stream);

#endif
