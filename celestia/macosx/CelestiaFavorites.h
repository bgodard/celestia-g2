//
//  CelestiaFavorites.h
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CelestiaFavorite.h"
#import "myTree.h"


@interface CelestiaFavorites : MyTree
-(void)setSynchronize:(NSInvocation*)synchronize;
-(void)synchronize;
+(CelestiaFavorites*)sharedFavorites;
-(MyTree*)addNewFavorite:(NSString*)name;
-(MyTree*)addNewFolder:(NSString*)name;
@end
