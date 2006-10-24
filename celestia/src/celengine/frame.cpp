// frame.cpp
// 
// Copyright (C) 2003-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/frame.h>


RigidTransform FrameOfReference::toUniversal(const RigidTransform& xform,
                                             double t) const
{
    // Handle the easy case . . .
    if (coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = refObject.getPosition(t);

    if (coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        switch (refObject.getType())
        {
        case Selection::Type_Body:
            rotation = refObject.body()->getEclipticalToGeographic(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationModel()->orientationAtTime(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToGeographic(t);
            break;
        default:
            break;
        }

        Point3d p = (Point3d) xform.translation * rotation.toMatrix4();
        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * rotation);
    }
    else if (coordSys == astro::PhaseLock)
    {
        Mat3d m;
        Vec3d lookDir = refObject.getPosition(t) - targetObject.getPosition(t);
        lookDir.normalize();

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
            break;
        case Selection::Type_Star:
            {
                Star* star = refObject.star();
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationModel()->equatorOrientationAtTime(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
        default:
            return xform;
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else if (coordSys == astro::Chase)
    {
        Mat3d m;

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                    body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                lookDir.normalize();
                Vec3d v = lookDir ^ axisDir;
                v.normalize();
                Vec3d u = v ^ lookDir;
                m = Mat3d(v, u, -lookDir);
            }
            break;
        default:
            return xform;
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else
    {
        return RigidTransform(origin + xform.translation, xform.rotation);
    }
}


RigidTransform FrameOfReference::fromUniversal(const RigidTransform& xform,
                                               double t) const
{
    // Handle the easy case . . .
    if (coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = refObject.getPosition(t);

    if (coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        switch (refObject.getType())
        {
        case Selection::Type_Body:
            rotation = refObject.body()->getEclipticalToGeographic(t);
            break;
        case Selection::Type_Star:
            rotation = refObject.star()->getRotationModel()->orientationAtTime(t);
            break;
        case Selection::Type_Location:
            if (refObject.location()->getParentBody() != NULL)
                rotation = refObject.location()->getParentBody()->getEclipticalToGeographic(t);
            break;
        default:
            break;
        }
        Vec3d v = (xform.translation - origin) * (~rotation).toMatrix4();
        
        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~rotation);
    }
    else if (coordSys == astro::PhaseLock)
    {
        Mat3d m;
        Vec3d lookDir = refObject.getPosition(t) - targetObject.getPosition(t);
        lookDir.normalize();

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }
            break;

        case Selection::Type_Star:
            {
                Star* star = refObject.star();
                Vec3d axisDir = Vec3d(0, 1, 0) * star->getRotationModel()->equatorOrientationAtTime(t).toMatrix3();
                Vec3d v = axisDir ^ lookDir;
                v.normalize();
                Vec3d u = lookDir ^ v;
                m = Mat3d(v, u, lookDir);
            }

        default:
            return xform;
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else if (coordSys == astro::Chase)
    {
        Mat3d m;

        switch (refObject.getType())
        {
        case Selection::Type_Body:
            {
                Body* body = refObject.body();
                Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                    body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
                Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial(t).toMatrix3();
                lookDir.normalize();
                Vec3d v = lookDir ^ axisDir;
                v.normalize();
                Vec3d u = v ^ lookDir;
                m = Mat3d(v, u, -lookDir);
            }
            break;

        default:
            return xform;
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else
    {
        return RigidTransform(xform.translation.difference(origin),
                              xform.rotation);
    }
}


/*** ReferenceFrame ***/

ReferenceFrame::ReferenceFrame(Selection center) :
    centerObject(center)
{
}


UniversalCoord
ReferenceFrame::convertFrom(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord center = centerObject.getPosition(tjd);
    Vec3d relative = uc - center;

    return center + getOrientation(tjd).toMatrix3() * relative;
}


Point3d
ReferenceFrame::convertFromAstrocentric(const Point3d& p, double tjd) const
{
    Point3d center;
    if (centerObject.getType() == Selection::Type_Body)
    {
        Point3d center = centerObject.body()->getHeliocentricPosition(tjd);
        Vec3d relative = p - center;
        return p + getOrientation(tjd).toMatrix3() * relative;
    }
    else if (centerObject.getType() == Selection::Type_Star)
    {
        return getOrientation(tjd).toMatrix3() * p;
    }
    else
    {
        // bad if the center object is a galaxy
        // what about locations?
        return Point3d(0.0, 0.0, 0.0);
    }
}


UniversalCoord
ReferenceFrame::convertTo(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord center = centerObject.getPosition(tjd);
    Vec3d relative = uc - center;

    return center + conjugate(getOrientation(tjd)).toMatrix3() * relative;
}



/*** J2000EclipticFrame ***/

J2000EclipticFrame::J2000EclipticFrame(Selection center) :
    ReferenceFrame(center)
{
}


/*** J2000EquatorFrame ***/

J2000EquatorFrame::J2000EquatorFrame(Selection center) :
    ReferenceFrame(center)
{
}


Quatd
J2000EquatorFrame::getOrientation(double /* tjd */) const
{
    return Quatd::xrotation(23.4392911);
}


/*** BodyFixedFrame ***/

BodyFixedFrame::BodyFixedFrame(Selection center, Selection obj) :
    ReferenceFrame(center),
    fixObject(obj)
{
}


Quatd
BodyFixedFrame::getOrientation(double tjd) const
{
    switch (fixObject.getType())
    {
    case Selection::Type_Body:
        return fixObject.body()->getRotationModel()->orientationAtTime(tjd);
    case Selection::Type_Star:
        return fixObject.star()->getRotationModel()->orientationAtTime(tjd);
    default:
        return Quatd(1.0);
    }
}


/*** BodyMeanEquatorFrame ***/

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center, Selection obj) :
    ReferenceFrame(center),
    equatorObject(obj)
{
}

Quatd
BodyMeanEquatorFrame::getOrientation(double tjd) const
{
    switch (equatorObject.getType())
    {
    case Selection::Type_Body:
        return equatorObject.body()->getRotationModel()->equatorOrientationAtTime(tjd);
    case Selection::Type_Star:
        return equatorObject.star()->getRotationModel()->equatorOrientationAtTime(tjd);
    default:
        return Quatd(1.0);
    }
}



