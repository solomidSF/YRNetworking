//
//  ServerViewController.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/14/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "ServerViewController.h"
#import "YRUDPServer.h"
#import "YRSharedLogger.h"

@interface ServerViewController ()

@end

@implementation ServerViewController {
    YRUDPServer *_server;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
    [[YRSharedLogger shared] setFileName:@"Server.log"];
    
    _server = [YRUDPServer serverWithPort:28016];
    
    NSError *setupError = nil;
    [_server setup:&setupError];
    
    if (setupError) {
        NSLog(@"Failed to setup server: %@", setupError);
    }
}

@end

