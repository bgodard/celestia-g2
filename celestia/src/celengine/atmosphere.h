// atmosphere.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ATMOSPHERE_H_
#define _ATMOSPHERE_H_

#include <celutil/reshandle.h>
#include <celutil/color.h>


class Atmosphere
{
 public:
    Atmosphere() :
        height(0.0f),
        cloudHeight(0.0f),
        cloudSpeed(0.0f),
        cloudTexture() {};

 public:
    float height;
    Color lowerColor;
    Color upperColor;
    Color skyColor;
    float cloudHeight;
    float cloudSpeed;
    MultiResTexture cloudTexture;
};

#endif // _ATMOSPHERE_H_

