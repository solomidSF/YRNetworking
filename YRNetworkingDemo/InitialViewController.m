//
//  ViewController.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/5/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "InitialViewController.h"
#import "GCDAsyncUdpSocket.h"

#import <arpa/inet.h>
#import <fcntl.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <net/if.h>
#import <sys/socket.h>
#import <sys/types.h>

@implementation InitialViewController {
    __weak IBOutlet NSTextField *_addressTextField;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
    
    
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

#pragma mark - Callbacks

- (IBAction)clientClicked:(id)sender {
//    if (_addressTextField.stringValue.length > 0) {
//        NSString *address = _addressTextField.stringValue;
//        struct sockaddr addr = {0};
//
//        addr.sa_family = AF_INET;
//
//
//        NSData *addressData = [address dataUsingEncoding:NSUTF8StringEncoding];
//    } else {
//        NSAlert *alert = [NSAlert alertWithMessageText:@"Error" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@"Please enter correct address."];
//        [alert runModal];
//    }
}

- (IBAction)serverClicked:(id)sender {
    
}

@end
