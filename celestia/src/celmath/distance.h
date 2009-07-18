// distance.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Distance calculation for various geometric objects.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_DISTANCE_H_
#define _CELMATH_DISTANCE_H_

#include "mathlib.h"
#include "ray.h"
#include "sphere.h"
#include "ellipsoid.h"
#include <Eigen/Core>


template<class T> T distance(const Point3<T>& p, const Sphere<T>& s)
{
    return abs(s.center.distanceTo(p) - s.radius);
}

template<class T> T distance(const Point3<T>& p, const Ellipsoid<T>& e)
{
    return 0.0f;
}

template<class T> T distance(const Point3<T>& p, const Ray3<T>& r)
{
    Eigen::Matrix<T, 3, 1> p2(p.x, p.y, p.z);
    T t = ((p2 - r.origin).dot(r.direction)) / r.direction.squaredNorm();
    if (t <= 0)
        return (p2 - r.origin).norm();
    else
        return (p2 - r.point(t)).norm();
}

template<class T> T distance(const Eigen::Matrix<T, 3, 1>& p, const Ray3<T>& r)
{
    T t = ((p - r.origin).dot(r.direction)) / r.direction.squaredNorm();
    if (t <= 0)
        return (p - r.origin).norm();
    else
        return (p - r.point(t)).norm();
}

// Distance between a point and a segment defined by orig+dir*t, 0 <= t <= 1
template<class T> T distanceToSegment(const Point3<T>& p,
				      const Point3<T>& origin,
				      const Vector3<T>& direction)
{
    T t = ((p - origin) * direction) / (direction * direction);
    if (t <= 0)
        return p.distanceTo(origin);
    else if (t >= 1)
        return p.distanceTo(origin + direction);
    else
        return p.distanceTo(origin + direction * t);
}


#endif // _CELMATH_DISTANCE_H_
