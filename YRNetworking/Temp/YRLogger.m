//
//  YRLogger.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRLogger.h"

@implementation YRLogger {
    NSFileHandle *_outFile;
}

- (instancetype)initWithReporter:(NSString *)reporter fileName:(NSString *)fileName {
    if (self = [super init]) {
        _reporter = reporter;
        _fileName = fileName;
        
        if (_fileName.length > 0) {
            [[NSFileManager defaultManager] createFileAtPath:fileName contents:nil attributes:nil];

            _outFile = [NSFileHandle fileHandleForWritingAtPath:_fileName];
        }
    }
    return self;
}

- (void)logDebug:(NSString *)text {
    [self log:text type:@"DEBUG"];
}

- (void)logInfo:(NSString *)text {
    [self log:text type:@"INFO"];
}

- (void)logWarning:(NSString *)text {
    [self log:text type:@"WARN"];
}

- (void)logError:(NSString *)text {
    [self log:text type:@"ERROR"];
}

- (void)log:(NSString *)text type:(NSString *)type {
    NSString *stringToLog = [NSString stringWithFormat:@"%@ [%@]: <%@> %@\n", [NSDate date], self.reporter, type, text];
    
    [[NSFileHandle fileHandleWithStandardError] writeData:[stringToLog dataUsingEncoding:NSUTF8StringEncoding]];
    
    [_outFile writeData:[stringToLog dataUsingEncoding:NSUTF8StringEncoding]];
}

@end
