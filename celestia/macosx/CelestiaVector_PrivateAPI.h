/*
 *  CelestiaMath_PrivateAPI.h
 *  celestia
 *
 *  Created by Bob Ippolito on Sat Jun 08 2002.
 *  Copyright (c) 2002 Chris Laurel. All rights reserved.
 *
 */

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
@interface CelestiaVector(PrivateAPI)
+(CelestiaVector*)vectorWithVec2f:(Vec2f)v;
+(CelestiaVector*)vectorWithVec3f:(Vec3f)v;
+(CelestiaVector*)vectorWithVec4f:(Vec4f)v;
+(CelestiaVector*)vectorWithVec3d:(Vec3d)v;
+(CelestiaVector*)vectorWithVec4d:(Vec4d)v;
+(CelestiaVector*)vectorWithQuatf:(Quatf)v;
+(CelestiaVector*)vectorWithQuatd:(Quatd)v;
+(CelestiaVector*)vectorWithPoint2f:(Point2f)v;
+(CelestiaVector*)vectorWithPoint3f:(Point3f)v;
+(CelestiaVector*)vectorWithPoint3d:(Point3d)v;
-(CelestiaVector*)initWithVec2f:(Vec2f)v;
-(CelestiaVector*)initWithVec3f:(Vec3f)v;
-(CelestiaVector*)initWithVec4f:(Vec4f)v;
-(CelestiaVector*)initWithVec3d:(Vec3d)v;
-(CelestiaVector*)initWithVec4d:(Vec4d)v;
-(CelestiaVector*)initWithPoint2f:(Point2f)v;
-(CelestiaVector*)initWithPoint3f:(Point3f)v;
-(CelestiaVector*)initWithPoint3d:(Point3d)v;
-(Point2f)point2f;
-(Point3f)point3f;
-(Point3d)point3d;
-(Vec2f)vec2f;
-(Vec3f)vec3f;
-(Vec4f)vec4f;
-(Vec3d)vec3d;
-(Vec4d)vec4d;
-(Quatf)quatf;
-(Quatd)quatd;
@end