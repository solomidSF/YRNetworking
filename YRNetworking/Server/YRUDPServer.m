//
//  YRConnectionService.m
//  TestNetworkedPhysics
//
//  Created by Yuriy Romanchenko on 6/20/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRUDPServer.h"

// Common
#import "YRTypes.h"
#import "YRObjcSession.h"

#import "GCDAsyncUdpSocket.h"

@interface YRUDPServer () <GCDAsyncUdpSocketDelegate>
@end

@implementation YRUDPServer {
    uint32_t _desiredPort;
    GCDAsyncUdpSocket *_socket;
    
    dispatch_queue_t _serverQueue;
    
    NSMutableArray <YRObjcSession *> *_activeSessions;
    long _currentTag;
}

+ (instancetype)server {
    return [[self alloc] initWithPort:0];
}

+ (instancetype)serverWithPort:(uint32_t)port {
    return [[self alloc] initWithPort:port];
}

- (instancetype)initWithPort:(uint32)port {
    if (self = [super init]) {
        _desiredPort = port;
        
        _serverQueue = dispatch_queue_create("com.udpserver.yr", DISPATCH_QUEUE_SERIAL);
        _socket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:_serverQueue];
        
        _activeSessions = [NSMutableArray new];
        
        
    }

    return self;
}

#pragma mark - Public

- (BOOL)setup:(NSError *__autoreleasing *)error {
    if (_state == YRUDPServerReady) {
        return YES;
    }
    
    BOOL didBind = [_socket bindToPort:_desiredPort error:error];
    
    BOOL isReceiving = NO;
    
    if (didBind) {
        isReceiving = [_socket beginReceiving:error];
    }
    
    _state = didBind && isReceiving ? YRUDPServerReady : YRUDPServerNotReady;

    return didBind && isReceiving;
}

- (void)continueAcceptingNewConnections {
    _isAcceptingNewConnections = YES;
}

- (void)stopAcceptingNewConnections {
    _isAcceptingNewConnections = NO;
}

//- (void)disconnectClient:(YRUDPConnectedClient *)client {
//
//}
//
//#pragma mark - Sending
//
//- (void)sendReliably:(id)data to:(YRUDPConnectedClient *)client {
//
//}
//
//- (void)sendUnreliably:(id)data to:(YRUDPConnectedClient *)client {
//
//}

#pragma mark - Private

- (void)invalidate {
    // TODO:
    
    _state = YRUDPServerNotReady;
}

- (YRObjcSession *)sessionWithAddress:(NSData*)address {
    for (YRObjcSession *session in _activeSessions) {
        if ([session.peerAddress isEqual:address]) {
            return session;
        }
    }
    
    YRObjcSession *newSession = [self createSessionWithAddress:address];
    
    [_activeSessions addObject:newSession];
    
    return newSession;
}

- (YRObjcSession *)createSessionWithAddress:(NSData *)address {
    __typeof(self) __weak weakSelf = self;
    
    YRObjcSessionContext *context = [YRObjcSessionContext new];
    
    context.peerAddress = address;
    
    context.connectionStateCallout = ^(YRObjcSession *session, YRSessionState newState) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (!strongSelf) {
            return;
        }
        
        NSString *humanReadableState = @[@"Closed",
                                         @"Waiting",
                                         @"Initiating",
                                         @"Connecting",
                                         @"Connected",
                                         @"Disconnecting"][newState];
        
        NSLog(@"New state: %@", humanReadableState);
    };
    
    context.receiveCallout = ^(YRObjcSession *session, NSData *receivedData) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (!strongSelf) {
            return;
        }
        
        NSLog(@"Received: %@", [[NSString alloc] initWithData:receivedData encoding:NSUTF8StringEncoding]);
    };
    
    context.sendCallout = ^(YRObjcSession *session, NSData *dataToSend) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (!strongSelf) {
            return;
        }
        
        NSLog(@"[YRUDPServer]: Will send data: %@ with tag %ld", dataToSend, strongSelf->_currentTag);
        [strongSelf->_socket sendData:[NSData dataWithBytes:dataToSend.bytes length:dataToSend.length] toAddress:session.peerAddress withTimeout:0 tag:strongSelf->_currentTag];
        strongSelf->_currentTag++;
    };

    YRObjcSession *newSession = [[YRObjcSession alloc] initWithContext:context];
    
    [newSession wait];
    
    return newSession;
}

#pragma mark - <GCDAsyncUdpSocketDelegate>

/**
 * Called when the datagram with the given tag has been sent.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didSendDataWithTag:(long)tag {
    // We won't notify session about successful write, because it's not needed, as sent packet should be ack'ed by other side.
    NSLog(@"[YRUDPServer]: Did send data with tag: %ld", tag);
}

/**
 * Called if an error occurs while trying to send a datagram.
 * This could be due to a timeout, or something more serious such as the data being too large to fit in a sigle packet.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didNotSendDataWithTag:(long)tag dueToError:(NSError * _Nullable)error {
    // We won't notify failure as session will try to resend data after reaching transmission timeout.
    NSLog(@"[YRUDPServer]: Failed to send data with tag: %ld with error: %@", tag, error);
}

/**
 * Called when the socket has received the requested datagram.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
    fromAddress:(NSData *)address withFilterContext:(nullable id)filterContext {
    if (data.length == 0) {
        return;
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        YRObjcSession *session = [self sessionWithAddress:address];
        
        [session receive:data];
    });
}

/**
 * Called when the socket is closed.
 **/
- (void)udpSocketDidClose:(GCDAsyncUdpSocket *)sock withError:(NSError * _Nullable)error {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self invalidate];
        
        !self.onSocketDidClose ?: self.onSocketDidClose(self, error);
    });
}

@end
