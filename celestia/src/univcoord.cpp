// univcoord.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "univcoord.h"


UniversalCoord::UniversalCoord()
{
}

UniversalCoord::UniversalCoord(BigFix _x, BigFix _y, BigFix _z) :
    x(_x), y(_y), z(_z)
{
}

UniversalCoord::UniversalCoord(double _x, double _y, double _z) :
    x(_x), y(_y), z(_z)
{
}

UniversalCoord::UniversalCoord(Point3d& p) :
    x(p.x), y(p.y), z(p.z)
{
}

UniversalCoord::UniversalCoord(Point3f& p) :
    x(p.x), y(p.y), z(p.z)
{
}

UniversalCoord::operator Point3d() const
{
    return Point3d((double) x, (double) y, (double) z);
}

UniversalCoord::operator Point3f() const
{
    return Point3f((float) x, (float) y, (float) z);
}

Vec3d operator-(const UniversalCoord uc0, const UniversalCoord uc1)
{
    return Vec3d((double) (uc0.x - uc1.x),
                 (double) (uc0.y - uc1.y),
                 (double) (uc0.z - uc1.z));
}

Vec3d operator-(const UniversalCoord uc, const Point3d p)
{
    return Vec3d((double) (uc.x - (BigFix) p.x),
                 (double) (uc.y - (BigFix) p.y),
                 (double) (uc.z - (BigFix) p.z));
}

Vec3d operator-(const Point3d p, const UniversalCoord uc)
{
    return Vec3d((double) ((BigFix) p.x - uc.x),
                 (double) ((BigFix) p.y - uc.y),
                 (double) ((BigFix) p.z - uc.z));
}

Vec3f operator-(const UniversalCoord uc, const Point3f p)
{
    return Vec3f((float) (uc.x - (BigFix) p.x),
                 (float) (uc.y - (BigFix) p.y),
                 (float) (uc.z - (BigFix) p.z));
}

Vec3f operator-(const Point3f p, const UniversalCoord uc)
{
    return Vec3f((float) ((BigFix) p.x - uc.x),
                 (float) ((BigFix) p.y - uc.y),
                 (float) ((BigFix) p.z - uc.z));
}

UniversalCoord operator+(const UniversalCoord uc, const Vec3d v)
{
    return UniversalCoord(uc.x + BigFix(v.x),
                          uc.y + BigFix(v.y),
                          uc.z + BigFix(v.z));
}

UniversalCoord operator+(const UniversalCoord uc, const Vec3f v)
{
    return UniversalCoord(uc.x + BigFix((double) v.x),
                          uc.y + BigFix((double) v.y),
                          uc.z + BigFix((double) v.z));
}

UniversalCoord operator-(const UniversalCoord uc, const Vec3d v)
{
    return UniversalCoord(uc.x - BigFix(v.x),
                          uc.y - BigFix(v.y),
                          uc.z - BigFix(v.z));
}

UniversalCoord operator-(const UniversalCoord uc, const Vec3f v)
{
    return UniversalCoord(uc.x - BigFix((double) v.x),
                          uc.y - BigFix((double) v.y),
                          uc.z - BigFix((double) v.z));
}
