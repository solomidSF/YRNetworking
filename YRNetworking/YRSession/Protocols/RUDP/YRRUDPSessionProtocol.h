//
// YRRUDPSessionProtocol.h
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

// TODO: <RF> Rename to YRSessionProtocol
#ifndef __YRRUDPSessionProtocol__
#define __YRRUDPSessionProtocol__

#ifndef __YRNETWORKING_INDIRECT__
#error "Please #include <YRNetworking/YRNetworking.h> instead of this file directly."
#endif

typedef struct YRRUDPSessionProtocol *YRRUDPSessionProtocolRef;

typedef struct {
    YRSessionProtocolClientCallbacks clientCallbacks;
	// TODO: Error callbacks
} YRRUDPSessionProtocolCallbacks;

typedef struct {
	YRRUDPConnectionConfiguration connectionConfig;
} YRSessionProtocolConfig;

typedef struct {
	YRRUDPConnectionConfiguration localConnectionConfiguration;
	YRRUDPConnectionConfiguration remoteConnectionConfiguration;

	// Send-related
	YRSequenceNumberType sendInitialSequenceNumber;
	YRSequenceNumberType sendNextSequenceNumber;
	YRSequenceNumberType sendLatestUnackSegment;
	
	// Receive-related
	YRSequenceNumberType rcvLatestAckedSegment;
	YRSequenceNumberType rcvInitialSequenceNumber;
	
	bool shouldKeepAlive;
} YRRUDPSessionInfo;

#pragma mark - Interface

YRSessionRef YRRUDPSessionCreate(YRRUDPSessionProtocolCallbacks callbacks, YRSessionProtocolConfig config);

#endif 
