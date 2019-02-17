//
//  YRLogger.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface YRLogger : NSObject

@property (nonatomic, readonly) NSString *reporter;

- (instancetype)initWithReporter:(NSString *)reporter;

- (void)logDebug:(NSString *)format, ...;
- (void)logInfo:(NSString *)format, ...;
- (void)logWarning:(NSString *)format, ...;
- (void)logError:(NSString *)format, ...;

@end
