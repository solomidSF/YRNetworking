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
@property (nonatomic, readonly) NSString *fileName;

- (instancetype)initWithReporter:(NSString *)reporter fileName:(NSString *)fileName;

- (void)logDebug:(NSString *)text;
- (void)logInfo:(NSString *)text;
- (void)logWarning:(NSString *)text;
- (void)logError:(NSString *)text;

@end
