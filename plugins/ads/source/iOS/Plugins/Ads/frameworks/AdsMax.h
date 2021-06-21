//
//  AdsAdmob.h
//  Ads
//
//  Created by Arturs Sosins on 6/25/13.
//  Copyright (c) 2013 Gideros Mobile. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AdsProtocol.h"
#import <AppLovinSDK/AppLovinSDK.h>
#import "AdsManager.h"

@interface AdsMax : NSObject <AdsProtocol>
@property(nonatomic, retain) MAAdView *view_;
@property(nonatomic, retain) NSString *appKey;
@property(nonatomic, retain) NSString *curType;
@property (nonatomic, retain) AdsManager *mngr;
@property (nonatomic, retain) NSMutableDictionary *units;
@property (nonatomic, retain) NSMutableDictionary *listeners;
@end

@interface AdsMaxListener : NSObject <MAAdDelegate,MARewardedAdDelegate,MAAdViewAdDelegate>
@property (nonatomic, retain) AdsState *state;
@property (nonatomic, retain) AdsMax *instance;
-(id)init:(AdsState*)state with:(AdsMax*)instance;
-(void)setType:(AdsState*)state with:(AdsMax*)instance;
@end
