//
// YRPacketBuilder.h
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

#ifndef __YRPacketBuilder__
#define __YRPacketBuilder__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

#pragma mark - Factory Methods

YRPacketRef YRPacketCreateSYN(
	YRRUDPConnectionConfiguration configuration,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateRST(
	uint8_t errorCode,
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	bool hasACK,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateNUL(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateACK(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateEACK(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YRSequenceNumberType *sequences,
	YRSequenceNumberType *ioSequencesCount,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateEACKWithPayload(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	YRSequenceNumberType *sequences,
	YRSequenceNumberType *ioSequencesCount,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
);

YRPacketRef YRPacketCreateWithPayload(
	YRSequenceNumberType seqNumber,
	YRSequenceNumberType ackNumber,
	const void *payload,
	YRPayloadLengthType payloadLength,
	YROutputStreamRef stream
);

#endif
