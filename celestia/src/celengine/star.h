// star.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STAR_H_
#define _STAR_H_

#include <vector>
#include <celutil/basictypes.h>
#include <celutil/reshandle.h>
#include <celutil/color.h>
#include <celmath/vecmath.h>
#include <celengine/univcoord.h>
#include <celengine/celestia.h>
#include <celengine/stellarclass.h>
#include <celengine/rotation.h>
#include <celengine/multitexture.h>

class Orbit;
class Star;

class StarDetails
{
    friend class Star;

 public:
    StarDetails();
    StarDetails(const StarDetails&);

    ~StarDetails();
    
 private:
    // Prohibit assignment of StarDetails objects
    StarDetails& operator=(const StarDetails&);

 public:
    inline float getRadius() const;
    inline float getTemperature() const;
    inline ResourceHandle getModel() const;
    inline MultiResTexture getTexture() const;
    inline Orbit* getOrbit() const;
    inline float getOrbitalRadius() const;
    inline const char* getSpectralType() const;
    inline float getBolometricCorrection() const;
    inline Star* getOrbitBarycenter() const;
    inline bool getVisibility() const;
    inline const RotationModel* getRotationModel() const;
    inline Vec3f getEllipsoidSemiAxes() const;

    void setRadius(float);
    void setTemperature(float);
    void setSpectralType(const std::string&);
    void setBolometricCorrection(float);
    void setTexture(const MultiResTexture&);
    void setModel(ResourceHandle);
    void setOrbit(Orbit*);
    void setOrbitBarycenter(Star*);
    void setOrbitalRadius(float);
    void computeOrbitalRadius();
    void setVisibility(bool);
    void setRotationModel(const RotationModel*);
    void setEllipsoidSemiAxes(const Vec3f&);

    bool shared() const;
    
    enum
    {
        KnowRadius   = 0x1,
        KnowRotation = 0x2,
    };
    inline uint32 getKnowledge() const;
    inline bool getKnowledge(uint32) const;
    void setKnowledge(uint32);
    void addKnowledge(uint32);

 private:
    void addOrbitingStar(Star*);

 private:
    float radius;
    float temperature;
    float bolometricCorrection;

    uint32 knowledge;
    bool visible;
    char spectralType[8];

    MultiResTexture texture;
    ResourceHandle model;

    Orbit* orbit;
    float orbitalRadius;
    Star* barycenter;

    const RotationModel* rotationModel;

    Vec3f semiAxes;

    std::vector<Star*>* orbitingStars;
    bool isShared;

 public:
    struct StarTextureSet
    {
        MultiResTexture defaultTex;
        MultiResTexture neutronStarTex;
        MultiResTexture starTex[StellarClass::Spectral_Count];
    };
    
 public:
    static StarDetails* GetStarDetails(const StellarClass&);
    static StarDetails* CreateStandardStarType(const std::string& _specType,
                                               float _temperature,
                                               float _rotationPeriod);

    static StarDetails* GetNormalStarDetails(StellarClass::SpectralClass specClass,
                                             unsigned int subclass,
                                             StellarClass::LuminosityClass lumClass);
    static StarDetails* GetWhiteDwarfDetails(StellarClass::SpectralClass specClass,
                                             unsigned int subclass);
    static StarDetails* GetNeutronStarDetails();
    static StarDetails* GetBlackHoleDetails();
    static StarDetails* GetBarycenterDetails();
    
    static void SetStarTextures(const StarTextureSet&);
    
 private:
    static StarTextureSet starTextures;
};


float
StarDetails::getRadius() const
{
    return radius;
}

float
StarDetails::getTemperature() const
{
    return temperature;
}

ResourceHandle
StarDetails::getModel() const
{
    return model;
}

MultiResTexture
StarDetails::getTexture() const
{
    return texture;
}

Orbit*
StarDetails::getOrbit() const
{
    return orbit;
}

float
StarDetails::getOrbitalRadius() const
{
    return orbitalRadius;
}

uint32
StarDetails::getKnowledge() const
{
    return knowledge;
}

bool
StarDetails::getKnowledge(uint32 knowledgeFlags) const
{
    return ((knowledge & knowledgeFlags) == knowledgeFlags);
}

const char*
StarDetails::getSpectralType() const
{
    return spectralType;
}

float
StarDetails::getBolometricCorrection() const
{
    return bolometricCorrection;
}

Star*
StarDetails::getOrbitBarycenter() const
{
    return barycenter;
}

bool
StarDetails::getVisibility() const
{
    return visible;
}

const RotationModel*
StarDetails::getRotationModel() const
{
    return rotationModel;
}

Vec3f
StarDetails::getEllipsoidSemiAxes() const
{
    return semiAxes;
}


class Star
{
public:
    inline Star();
    ~Star();

    // Accessor methods for members of the star class
    inline uint32 getCatalogNumber() const;
    inline Point3f getPosition() const;
    inline float getAbsoluteMagnitude() const;
    float getApparentMagnitude(float) const;
    float getLuminosity() const;

    // Return the exact position of the star, accounting for its orbit
    UniversalCoord getPosition(double t) const;
    UniversalCoord getOrbitBarycenterPosition(double t) const;

	Vec3d getVelocity(double t) const;

    void setCatalogNumber(uint32);
    void setPosition(float, float, float);
    void setPosition(Point3f);
    void setAbsoluteMagnitude(float);
    void setLuminosity(float);

    void setDetails(StarDetails*);
    void setOrbitBarycenter(Star*);
    void computeOrbitalRadius();

    void setRotationModel(const RotationModel*);

    void addOrbitingStar(Star*);
    inline const std::vector<Star*>* getOrbitingStars() const;

    // Accessor methods that delegate to StarDetails
    float getRadius() const;
    inline float getTemperature() const;
    inline const char* getSpectralType() const;
    inline float getBolometricMagnitude() const;
    MultiResTexture getTexture() const;
    ResourceHandle getModel() const;
    inline Orbit* getOrbit() const;
    inline float getOrbitalRadius() const;
    inline Star* getOrbitBarycenter() const;
    inline bool getVisibility() const;
    inline uint32 getKnowledge() const;
    inline const RotationModel* getRotationModel() const;
    inline Vec3f getEllipsoidSemiAxes() const;

    enum {
        MaxTychoCatalogNumber = 0xf0000000,
        InvalidCatalogNumber = 0xffffffff,
    };

private:
    uint32 catalogNumber;
    Point3f position;
    float absMag;
    StarDetails* details;
};


Star::Star() :
    catalogNumber(InvalidCatalogNumber),
    position(0, 0, 0),
    absMag(4.83f),
    details(NULL)
{
}

uint32
Star::getCatalogNumber() const
{
    return catalogNumber;
}

float
Star::getAbsoluteMagnitude() const
{
    return absMag;
}


// This getPosition() method returns the approximate star position; that is,
// star position without any orbital motion taken into account.  For a
// star in an orbit, the position should be set to the 'root' barycenter
// of the system.
Point3f
Star::getPosition() const
{
    return position;
}

float
Star::getTemperature() const
{
    return details->getTemperature();
}

const char*
Star::getSpectralType() const
{
    return details->getSpectralType();
}

float
Star::getBolometricMagnitude() const
{
    return absMag + details->getBolometricCorrection();
}

Orbit*
Star::getOrbit() const
{
    return details->getOrbit();
}

float
Star::getOrbitalRadius() const
{
    return details->getOrbitalRadius();
}

Star*
Star::getOrbitBarycenter() const
{
    return details->getOrbitBarycenter();
}

bool
Star::getVisibility() const
{
    return details->getVisibility();
}

const RotationModel*
Star::getRotationModel() const
{
    return details->getRotationModel();
}

Vec3f
Star::getEllipsoidSemiAxes() const
{
    return details->getEllipsoidSemiAxes();
}

const std::vector<Star*>*
Star::getOrbitingStars() const
{
    return details->orbitingStars;
}

#endif // _STAR_H_
