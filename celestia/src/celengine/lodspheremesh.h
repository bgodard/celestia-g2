// lodspheremesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _LODSPHEREMESH_H_
#define _LODSPHEREMESH_H_

#include <celmath/vecmath.h>
#include <celengine/mesh.h>

class LODSphereMesh
{
public:
    LODSphereMesh();
    ~LODSphereMesh();

    void render(unsigned int attributes, const Frustum&, float lod);
    void render(const Frustum&, float lod);

 private:
    int renderPatches(int phi0, int theta0, 
                      int extent,
                      int level,
                      int step,
                      unsigned int attributes,
                      Point3f* fp);

    void renderSection(int phi0, int theta0,
                       int extent,
                       int step,
                       unsigned int attributes);

    float* vertices;
    float* normals;
    float* texCoords;
    float* tangents;
    int nIndices;
    unsigned short* indices;
};

#endif // _LODSPHEREMESH_H_
