// observer.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "observer.h"

#define LY 9466411842000.000


Observer::Observer() : orientation(0, 0, 0, 1),
                       velocity(0, 0, 0)
{
}


UniversalCoord Observer::getPosition() const
{
    return position;
}


Point3d Observer::getRelativePosition(Point3d& p) const
{
    BigFix x(p.x);
    BigFix y(p.y);
    BigFix z(p.z);
    double dx = (double) (position.x - x);
    double dy = (double) (position.y - y);
    double dz = (double) (position.z - z);

    return Point3d(dx / LY, dy / LY, dz / LY);
}


Quatf Observer::getOrientation() const
{
    return orientation;
}


void Observer::setOrientation(Quatf q)
{
    orientation = q;
}


Vec3d Observer::getVelocity() const
{
    return velocity;
}


void Observer::setVelocity(Vec3d v)
{
    velocity = v;
}


void Observer::setPosition(Point3d p)
{
    position = UniversalCoord(p);
}


void Observer::setPosition(UniversalCoord p)
{
    position = p;
}


void Observer::update(double dt)
{
    BigFix x(velocity.x * dt);
    BigFix y(velocity.y * dt);
    BigFix z(velocity.z * dt);
    position.x = position.x + x;
    position.y = position.y + y;
    position.z = position.z + z;
}
