/*
 *  CelestiaRenderer_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Fri Jun 07 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */


@interface CelestiaRenderer(PrivateAPI)
-(CelestiaRenderer*)initWithRenderer:(Renderer*)ren;
-(Renderer*)renderer;
@end