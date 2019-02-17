//
//  YRSharedLogger.h
//  YRNetworkingDemo
//
//  Created by Yuriy Romanchenko on 7/15/18.
//  Copyright © 2018 Yuriy Romanchenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "YRLogger.h"

@interface YRSharedLogger : NSObject

@property (nonatomic) NSString *fileName;

+ (instancetype)shared;

- (YRLogger *)loggerWithReporter:(NSString *)reporter;

- (void)log:(NSString *)text reporter:(NSString *)reporter type:(NSString *)type;

@end
