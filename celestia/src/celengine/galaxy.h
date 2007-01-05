// galaxy.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _GALAXY_H_
#define _GALAXY_H_

#include <celengine/deepskyobj.h>


struct Blob
{
    Point3f        position;
    unsigned int   colorIndex;
    float          brightness;
};

struct GalacticForm
{
    std::vector<Blob>* blobs;
    Vec3f scale;
};

class Galaxy : public DeepSkyObject
{
 public:
    Galaxy();
    virtual const char* getType() const;
    virtual void setType(const std::string&);
    virtual size_t getDescription(char* buf, size_t bufLength) const;
    virtual std::string getCustomTmpName() const;
    virtual void setCustomTmpName(const std::string&);

    float getDetail() const;
    void setDetail(float);
    //    float getBrightness() const;
    //    void setBrightness();

    virtual bool pick(const Ray3d& ray,
                      double& distanceToPicker,
                      double& cosToBoundCenter) const;
    virtual bool load(AssociativeArray*, const std::string&);
    virtual void render(const GLContext& context,
                        const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize);
    virtual void renderGalaxyPointSprites(const GLContext& context,
                                          const Vec3f& offset,
                                          const Quatf& viewerOrientation,
                                          float brightness,
                                          float pixelSize);
    virtual void renderGalaxyEllipsoid(const GLContext& context,
                                       const Vec3f& offset,
                                       const Quatf& viewerOrientation,
                                       float brightness,
                                       float pixelSize);

    GalacticForm* getForm() const;

    static void  increaseLightGain();
    static void  decreaseLightGain();
    static float getLightGain();
    static void  setLightGain(float);
    static void hsv2rgb( float *r, float *g, float *b, float h, float s, float v );

    virtual unsigned int getRenderMask() const;
    virtual unsigned int getLabelMask() const;

 public:
    enum GalaxyType {
        S0   =  0,
        Sa   =  1,
        Sb   =  2,
        Sc   =  3,
        SBa  =  4,
        SBb  =  5,
        SBc  =  6,
        E0   =  7,
        E1   =  8,
        E2   =  9,
        E3   = 10,
        E4   = 11,
        E5   = 12,
        E6   = 13,
        E7   = 14,
        Irr  = 15
    };

 private:
    float detail;
    std::string* customTmpName;
    //    float brightness;
    GalaxyType type;
    GalacticForm* form;

    static float lightGain;
};

//std::ostream& operator<<(std::ostream& s, const Galaxy::GalaxyType& sc);

#endif // _GALAXY_H_
