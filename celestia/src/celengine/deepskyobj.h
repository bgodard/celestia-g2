// deepskyobj.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_DEEPSKYOBJ_H_
#define _CELENGINE_DEEPSKYOBJ_H_

#include <vector>
#include <string>
#include <iostream>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/parser.h>


class Nebula;
class Galaxy;
class OpenCluster;

class DeepSkyObject
{
 public:
    DeepSkyObject();
    virtual ~DeepSkyObject();

    std::string getName() const;
    void setName(const std::string&);

    Point3d getPosition() const;
    void setPosition(Point3d);

    Quatf getOrientation() const;
    void setOrientation(Quatf);

    float getRadius() const;
    void setRadius(float);

    std::string getInfoURL() const;
    void setInfoURL(const std::string&);

    virtual bool load(AssociativeArray*, const std::string& resPath);

    virtual void render(const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize) = 0;

    virtual unsigned int getRenderMask() { return 0; };
    virtual unsigned int getLabelMask() { return 0; };

 private:
    std::string name;
    Point3d position;
    Quatf orientation;
    float radius;
    std::string* infoURL;
};

typedef std::vector<DeepSkyObject*> DeepSkyCatalog;
int LoadDeepSkyObjects(DeepSkyCatalog&, std::istream& in,
                       const std::string& path);


#endif // _CELENGINE_DEEPSKYOBJ_H_
