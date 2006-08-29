// lightenv.h
//
// Structures that describe the lighting environment for rendering objects
// in Celestia.
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_LIGHTENV_H_
#define _CELENGINE_LIGHTENV_H_

#include <celutil/color.h>
#include <celmath/vecmath.h>
#include <vector>

static const unsigned int MaxLights = 8;

class DirectionalLight
{
public:
    Color color;
    float irradiance;
    Vec3f direction_eye;
    Vec3f direction_obj;

    // Required for eclipse shadows only--may be able to use
    // distance instead of position.
    Point3d position;
    float apparentSize;
};

class EclipseShadow
{
public:
    Point3f origin;
    Vec3f direction;
    float penumbraRadius;
    float umbraRadius;
};

class LightingState
{
public:
    LightingState() : nLights(0),
                      eyeDir_obj(0.0f, 0.0f, -1.0f),
                      eyePos_obj(0.0f, 0.0f, -1.0f)
    { shadows[0] = NULL; };

    unsigned int nLights;
    DirectionalLight lights[MaxLights];
    std::vector<EclipseShadow>* shadows[MaxLights];

    Vec3f eyeDir_obj;
    Point3f eyePos_obj;
    
    Vec3f ambientColor;
};

#endif // _CELENGINE_LIGHTENV_H_
