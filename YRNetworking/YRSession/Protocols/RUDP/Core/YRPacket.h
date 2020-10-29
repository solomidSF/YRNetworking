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

#pragma mark - Declarations

// TODO: <RF> Drop this symbol?
typedef struct YRPacket *YRPacketRef;

typedef struct {
	void (*syn) (void *context, YRPacketHeaderSYNRef header);
	void (*rst) (void *context, YRPacketHeaderRSTRef header);
	void (*nul) (void *context, YRPacketHeaderRef header);
	void (*eack) (void *context, YRPacketHeaderEACKRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (*regular) (void *context, YRPacketPayloadHeaderRef header, const void *payload, YRPayloadLengthType payloadLength);
	void (*invalid) (void *context, YRRUDPError error);
} YRPacketHandlers;

#pragma mark - Serialization

YRPacketRef YRPacketSerializeSYN(
	YRPacketHeaderSYNRef synHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

YRPacketRef YRPacketSerializeRST(
	YRPacketHeaderRSTRef rstHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

YRPacketRef YRPacketSerializeNUL(
	YRPacketHeaderRef nulHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

YRPacketRef YRPacketSerializeEACK(
	YRPacketHeaderEACKRef eackHeader,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

YRPacketRef YRPacketSerializeEACKWithPayload(
	YRPacketHeaderEACKRef eackHeader,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

YRPacketRef YRPacketSerializeWithPayload(
	YRPacketPayloadHeaderRef header,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream,
	YRPayloadLengthType *packetLength
);

void YRPacketDeserialize(
	YRInputStreamRef stream,
	YRPacketHandlers handlers,
	void *context
);

#pragma mark - Introspection

//void *YRPacketGetPayload(YRPacketRef packet, YRPayloadLengthType *payloadLength);
//YRPacketHeaderRef YRPacketGetHeader(YRPacketRef packet);
//YRPayloadLengthType YRPacketGetLength(YRPacketRef packet);
//YRRUDPError YRPacketCanDeserialize(YRInputStreamRef stream);

#endif
