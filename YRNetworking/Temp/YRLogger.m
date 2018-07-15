//
//  YRLogger.m
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import "YRLogger.h"
#import "YRSharedLogger.h"

@implementation YRLogger {
    NSFileHandle *_outFile;
}

- (instancetype)initWithReporter:(NSString *)reporter {
    if (self = [super init]) {
        _reporter = reporter;
    }
    
    return self;
}

- (void)logDebug:(NSString *)format, ... {
    va_list args;
    va_start(args, format);
    
    NSString *resultingString = [[NSString alloc] initWithFormat:format arguments:args];
    
    va_end(args);
    
    [[YRSharedLogger shared] log:resultingString reporter:self.reporter type:@"DEBUG"];
}

- (void)logInfo:(NSString *)format, ... {
    va_list args;
    va_start(args, format);
    
    NSString *resultingString = [[NSString alloc] initWithFormat:format arguments:args];
    
    va_end(args);
    
    [[YRSharedLogger shared] log:resultingString reporter:self.reporter type:@"INFO"];
}
                                         
- (void)logWarning:(NSString *)format, ... {
    va_list args;
    va_start(args, format);
    
    NSString *resultingString = [[NSString alloc] initWithFormat:format arguments:args];
    
    va_end(args);
    
    [[YRSharedLogger shared] log:resultingString reporter:self.reporter type:@"WARN"];
}

- (void)logError:(NSString *)format, ... {
    va_list args;
    va_start(args, format);
    
    NSString *resultingString = [[NSString alloc] initWithFormat:format arguments:args];
    
    va_end(args);
    
    [[YRSharedLogger shared] log:resultingString reporter:self.reporter type:@"ERROR"];
}

@end
