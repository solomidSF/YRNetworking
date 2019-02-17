//
//  YRSharedLogger.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRSharedLogger.h"

@implementation YRSharedLogger {
    NSFileHandle *_outFile;
}

+ (instancetype)shared {
    static id manager = nil;
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        manager = [self new];
    });
    
    return manager;
}

- (void)setFileName:(NSString *)fileName {
    if (_fileName != fileName) {
        _fileName = fileName;
        [_outFile closeFile];

        if (_fileName.length > 0) {
            [[NSFileManager defaultManager] createFileAtPath:_fileName contents:nil attributes:nil];
            
            _outFile = [NSFileHandle fileHandleForWritingAtPath:_fileName];
        }
        
        NSLog(@"[YRSharedLogger]: Out filename: %@", [[[NSFileManager defaultManager] currentDirectoryPath] stringByAppendingPathComponent:fileName]);
    }
}

- (YRLogger *)loggerWithReporter:(NSString *)reporter {
    YRLogger *logger = [[YRLogger alloc] initWithReporter:reporter];
    
    return logger;
}

- (void)log:(NSString *)text reporter:(NSString *)reporter type:(NSString *)type {
    NSString *stringToLog = [NSString stringWithFormat:@"%@ [%@]: <%@> %@\n", [NSDate date], reporter, type, text];
    
    [[NSFileHandle fileHandleWithStandardError] writeData:[stringToLog dataUsingEncoding:NSUTF8StringEncoding]];
    
    [_outFile writeData:[stringToLog dataUsingEncoding:NSUTF8StringEncoding]];
}

@end
