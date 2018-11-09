//
//  testClass.m
//  ClaneTestDemo
//
//  Created by Max on 2018/8/28.
//  Copyright © 2018年 Max. All rights reserved.
//

#import "testClass.h"

@protocol testProtocol <NSObject>

- (void)add:(int)a;

@end

@interface TestClass () <    testProtocol,NSCopying,NSCoding>
{
    NSString *_className;
}

@end

@implementation   TestClass    : NSObject{
}

+ (void)initialize
{
    NSLog(@"test");
    

    
    //    dispatch_async(dispatch_get_main_queue(), ^{
    //
    //
    //    });
    
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        
    });
    

    dispatch_async(dispatch_get_main_queue(), ^{
        
    });
}

- (void)test
{
    if ((@"".length) >= '1')
    {
        
    }
    
    {
        NSLog(@"--------------");
    }
    
    NSNumber *a = [[NSNumber alloc] initWithBool:YES];
    
    NSNumber *g = [NSNumber numberWithBool:YES];
    
    NSDictionary *b = [NSDictionary dictionaryWithObjectsAndKeys:@"a",@"b", nil];
    
    NSDictionary *f = [[NSDictionary alloc]initWithObjectsAndKeys:@"a",@"b", nil];
    
    NSArray *c = [NSArray arrayWithObjects:@1,@2, nil];
    
    NSArray *d = [[NSArray alloc] initWithObjects:@1,@2, nil];
    
    NSLog(@"%@",@[a,b,c,d,f,g]);
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        
    });
    
    
    dispatch_async(dispatch_get_main_queue(), ^{
        
    });
}

@end


@interface TestClass      (Privite) <NSCopying,NSCoding>

@end

@implementation TestClass (Privite)

@end
