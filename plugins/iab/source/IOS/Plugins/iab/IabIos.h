//
//  IabIos.h
//  IAB
//
//  Created by Arturs Sosins on 10/15/13.
//  Copyright (c) 2013 Gideros Mobile. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <StoreKit/StoreKit.h>
#include "iab.h"

@interface IabIos : NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (nonatomic) NSString *iap;
@property (nonatomic) NSMutableDictionary *prods;
@property (nonatomic) NSMutableDictionary *prodKeys;

-(id)init;
-(void)deinit;
-(void)check;
-(void)setProducts:(NSMutableDictionary*) prods;
-(void)setConsumables;
-(void)request;
-(void)purchase:(NSString*)prod;
-(void)restore;
@end
