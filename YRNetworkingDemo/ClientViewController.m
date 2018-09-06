//
//  ClientViewController.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/14/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "ClientViewController.h"
#import "YRObjcSession.h"
#import "GCDAsyncUdpSocket.h"

#import <arpa/inet.h>
#import <fcntl.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <net/if.h>
#import <sys/socket.h>
#import <sys/types.h>

#import "YRSharedLogger.h"

@interface ClientViewController ()
<GCDAsyncUdpSocketDelegate>
@end

@implementation ClientViewController {
    YRObjcSession *_client;
    YRLogger *_uiLogger;
    
    __unsafe_unretained IBOutlet NSTextView *_logTextView;
    
    GCDAsyncUdpSocket *_sock;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
    
    [YRSharedLogger shared].fileName = @"Client.log";
    _uiLogger = [[YRSharedLogger shared] loggerWithReporter:@"ClientVC"];
    
    NSError *err = nil;
    _sock = [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    [_sock bindToPort:0 error:&err];
    
    if (!err) {
        [_uiLogger logInfo:@"Did bind to '%d' port", _sock.localPort];
    } else {
        [_uiLogger logError:@"Couldn't bind to port with error: %@", err];
        return;
    }
    
    [_sock beginReceiving:&err];

    if (!err) {
        [_uiLogger logInfo:@"Enabled receiving on port!"];
    } else {
        [_uiLogger logError:@"Couldn't begin receiving on port with error: %@", err];
        return;
    }
    
    YRObjcSessionContext *ctx = [YRObjcSessionContext new];
    
    struct sockaddr_in nativeAddr4;
    nativeAddr4.sin_len         = sizeof(struct sockaddr_in);
    nativeAddr4.sin_family      = AF_INET;
    nativeAddr4.sin_port        = htons(28016);
    nativeAddr4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    memset(&(nativeAddr4.sin_zero), 0, sizeof(nativeAddr4.sin_zero));

    __typeof(self) __weak weakSelf = self;
    
    NSData *address = [NSData dataWithBytes:&nativeAddr4 length:sizeof(nativeAddr4)];
    
    ctx.peerAddress = address;
    
    ctx.connectionStateCallout = ^(YRObjcSession *session, YRSessionState newState) {
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
        
        strongSelf->_logTextView.string = [strongSelf->_logTextView.string stringByAppendingFormat:@"\nNew state: %@", humanReadableState];
    };
    
    ctx.receiveCallout = ^(YRObjcSession *session, NSData *data) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (!strongSelf) {
            return;
        }
        
        NSLog(@"Received: %@", [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]);

        strongSelf->_logTextView.string = [strongSelf->_logTextView.string stringByAppendingFormat:@"\nReceived: %@", [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]];
    };
    
    static long tag = 0;
    
    ctx.sendCallout = ^(YRObjcSession *session, NSData *data) {
        __typeof(weakSelf) __strong strongSelf = weakSelf;
        
        if (!strongSelf) {
            return;
        }
        
        NSLog(@"Asked to send: %@", data);
        
        strongSelf->_logTextView.string = [strongSelf->_logTextView.string stringByAppendingFormat:@"\nAsked to send: %@", data];
        
        // GCD goes async on it's own queue and data is created on stack!
        [strongSelf->_sock sendData:[NSData dataWithBytes:data.bytes length:data.length] toAddress:address withTimeout:-1 tag:tag++];
    };
    
    _client = [[YRObjcSession alloc] initWithContext:ctx];
    
    [_client connect];
}

#pragma mark - <GCDAsyncUdpSocketDelegate>

/**
 * By design, UDP is a connectionless protocol, and connecting is not needed.
 * However, you may optionally choose to connect to a particular host for reasons
 * outlined in the documentation for the various connect methods listed above.
 *
 * This method is called if one of the connect methods are invoked, and the connection is successful.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didConnectToAddress:(NSData *)address {
    NSLog(@"%s", __FUNCTION__);
}

/**
 * By design, UDP is a connectionless protocol, and connecting is not needed.
 * However, you may optionally choose to connect to a particular host for reasons
 * outlined in the documentation for the various connect methods listed above.
 *
 * This method is called if one of the connect methods are invoked, and the connection fails.
 * This may happen, for example, if a domain name is given for the host and the domain name is unable to be resolved.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didNotConnect:(NSError * _Nullable)error {
    NSLog(@"%s", __FUNCTION__);
}

/**
 * Called when the datagram with the given tag has been sent.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didSendDataWithTag:(long)tag {
    NSLog(@"%s", __FUNCTION__);
}

/**
 * Called if an error occurs while trying to send a datagram.
 * This could be due to a timeout, or something more serious such as the data being too large to fit in a sigle packet.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didNotSendDataWithTag:(long)tag dueToError:(NSError * _Nullable)error {
    NSLog(@"%s", __FUNCTION__);
}

/**
 * Called when the socket has received the requested datagram.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(nullable id)filterContext {
    _logTextView.string = [_logTextView.string stringByAppendingFormat:@"\nReceived RAW: %@", data];

    [_client receive:data];
}

/**
 * Called when the socket is closed.
 **/
- (void)udpSocketDidClose:(GCDAsyncUdpSocket *)sock withError:(NSError  * _Nullable)error {
    NSLog(@"%s", __FUNCTION__);
}

@end
