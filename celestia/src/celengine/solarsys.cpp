// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
// #include <limits>
#include <cstdio>

#ifndef _WIN32
#ifndef MACOSX_PB
#include <config.h>
#endif /* ! MACOSX_PB */
#endif /* ! _WIN32 */

#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <cstdio>
#include "astro.h"
#include "parser.h"
#include "customorbit.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "trajmanager.h"
#include "universe.h"
#include "multitexture.h"

using namespace std;

enum Disposition
{
    AddObject,
    ReplaceObject,
    ModifyObject,
};


/*!
  Solar system catalog (.ssc) files contain items of three different types:
  bodies, locations, and alternate surfaces.  Bodies planets, moons, asteroids,
  comets, and spacecraft.  Locations are points on the surfaces of bodies which
  may be labelled but aren't rendered.  Alternate surfaces are additional
  surface definitions for bodies.

  An ssc file contains zero or more definitions of this form:

  \code
  [disposition] [item type] "name" "parent name"
  {
     ...object info fields...
  }
  \endcode

  The disposition of the object determines what happens if an item with the
  same parent and same name already exists.  It may be one of the following:
  - Add - Default if none is specified.  Add the item even if one of the
    same name already exists.
  - Replace - Replace an existing item with the new one
  - Modify - Modify the existing item, changing the fields that appear
    in the new definition.

  All dispositions are equivalent to add if no item of the same name
  already exists.

  The item type is one of Body, Location, or AltSurface, defaulting to
  Body when no type is given.

  The name and parent name are both mandatory.
*/

static void errorMessagePrelude(const Tokenizer& tok)
{
    cerr << "Error in .ssc file (line " << tok.getLineNumber() << "): ";
}

static void sscError(const Tokenizer& tok,
                     const string& msg)
{
    errorMessagePrelude(tok);
    cerr << msg << '\n';
}


bool getDate(Hash* hash, const string& name, double& jd)
{
    // Check first for a number value representing a Julian date
    if (hash->getNumber(name, jd))
        return true;

    string dateString;
    if (hash->getString(name, dateString))
    {
        astro::Date date(1, 1, 1);
        if (astro::parseDate(dateString, date))
        {
            jd = (double) date;
            return true;
        }
    }

    return false;
}


static Location* CreateLocation(Hash* locationData,
                                Body* body)
{
    Location* location = new Location();

    Vec3d longlat(0.0, 0.0, 0.0);
    locationData->getVector("LongLat", longlat);

    Vec3f position = body->planetocentricToCartesian((float) longlat.x,
                                                     (float) longlat.y,
                                                     (float) longlat.z);
    location->setPosition(position);

    double size = 1.0;
    locationData->getNumber("Size", size);
    location->setSize((float) size);

    double importance = -1.0;
    locationData->getNumber("Importance", importance);
    location->setImportance((float) importance);

    string featureTypeName;
    if (locationData->getString("Type", featureTypeName))
        location->setFeatureType(Location::parseFeatureType(featureTypeName));

    return location;
}


static void FillinSurface(Hash* surfaceData,
                          Surface* surface,
                          const std::string& path)
{
    surfaceData->getColor("Color", surface->color);

    Color hazeColor = surface->hazeColor;
    float hazeDensity = hazeColor.alpha();
    if (surfaceData->getColor("HazeColor", hazeColor) | surfaceData->getNumber("HazeDensity", hazeDensity))
    {
        surface->hazeColor = Color(hazeColor.red(), hazeColor.green(),
                                   hazeColor.blue(), hazeDensity);
    }

    surfaceData->getColor("SpecularColor", surface->specularColor);
    surfaceData->getNumber("SpecularPower", surface->specularPower);

    string baseTexture;
    string bumpTexture;
    string nightTexture;
    string specularTexture;
    string normalTexture;
    string overlayTexture;
    bool applyBaseTexture = surfaceData->getString("Texture", baseTexture);
    bool applyBumpMap = surfaceData->getString("BumpMap", bumpTexture);
    bool applyNightMap = surfaceData->getString("NightTexture", nightTexture);
    bool separateSpecular = surfaceData->getString("SpecularTexture",
                                                   specularTexture);
    bool applyNormalMap = surfaceData->getString("NormalMap", normalTexture);
    bool applyOverlay = surfaceData->getString("OverlayTexture",
                                               overlayTexture);

    unsigned int baseFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int bumpFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int nightFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int specularFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    
    float bumpHeight = 2.5f;
    surfaceData->getNumber("BumpHeight", bumpHeight);

    bool blendTexture = false;
    surfaceData->getBoolean("BlendTexture", blendTexture);

    bool emissive = false;
    surfaceData->getBoolean("Emissive", emissive);

    bool compressTexture = false;
    surfaceData->getBoolean("CompressTexture", compressTexture);
    if (compressTexture)
        baseFlags |= TextureInfo::CompressTexture;

    if (blendTexture)
        surface->appearanceFlags |= Surface::BlendTexture;
    if (emissive)
        surface->appearanceFlags |= Surface::Emissive;
    if (applyBaseTexture)
        surface->appearanceFlags |= Surface::ApplyBaseTexture;
    if (applyBumpMap || applyNormalMap)
        surface->appearanceFlags |= Surface::ApplyBumpMap;
    if (applyNightMap)
        surface->appearanceFlags |= Surface::ApplyNightMap;
    if (separateSpecular)
        surface->appearanceFlags |= Surface::SeparateSpecularMap;
    if (applyOverlay)
        surface->appearanceFlags |= Surface::ApplyOverlay;
    if (surface->specularColor != Color(0.0f, 0.0f, 0.0f))
        surface->appearanceFlags |= Surface::SpecularReflection;

    if (applyBaseTexture)
        surface->baseTexture.setTexture(baseTexture, path, baseFlags);
    if (applyNightMap)
        surface->nightTexture.setTexture(nightTexture, path, nightFlags);
    if (separateSpecular)
        surface->specularTexture.setTexture(specularTexture, path, specularFlags);

    // If both are present, NormalMap overrides BumpMap
    if (applyNormalMap)
        surface->bumpTexture.setTexture(normalTexture, path, bumpFlags);
    else if (applyBumpMap)
        surface->bumpTexture.setTexture(bumpTexture, path, bumpHeight, bumpFlags);

    if (applyOverlay)
        surface->overlayTexture.setTexture(overlayTexture, path, baseFlags);
}


static EllipticalOrbit* CreateEllipticalOrbit(Hash* orbitData,
                                              bool usePlanetUnits)
{
    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    double pericenterDistance = 0.0;
    double semiMajorAxis = 0.0;
    if (!orbitData->getNumber("SemiMajorAxis", semiMajorAxis))
    {
        if (!orbitData->getNumber("PericenterDistance", pericenterDistance))
        {
            DPRINTF(0, "SemiMajorAxis/PericenterDistance missing!  Skipping planet . . .\n");
            return NULL;
        }
    }

    double period = 0.0;
    if (!orbitData->getNumber("Period", period))
    {
        DPRINTF(0, "Period missing!  Skipping planet . . .\n");
        return NULL;
    }

    double eccentricity = 0.0;
    orbitData->getNumber("Eccentricity", eccentricity);

    double inclination = 0.0;
    orbitData->getNumber("Inclination", inclination);

    double ascendingNode = 0.0;
    orbitData->getNumber("AscendingNode", ascendingNode);

    double argOfPericenter = 0.0;
    if (!orbitData->getNumber("ArgOfPericenter", argOfPericenter))
    {
        double longOfPericenter = 0.0;
        if (orbitData->getNumber("LongOfPericenter", longOfPericenter))
            argOfPericenter = longOfPericenter - ascendingNode;
    }

    double epoch = astro::J2000;
    getDate(orbitData, "Epoch", epoch);

    // Accept either the mean anomaly or mean longitude--use mean anomaly
    // if both are specified.
    double anomalyAtEpoch = 0.0;
    if (!orbitData->getNumber("MeanAnomaly", anomalyAtEpoch))
    {
        double longAtEpoch = 0.0;
        if (orbitData->getNumber("MeanLongitude", longAtEpoch))
            anomalyAtEpoch = longAtEpoch - (argOfPericenter + ascendingNode);
    }

    if (usePlanetUnits)
    {
        semiMajorAxis = astro::AUtoKilometers(semiMajorAxis);
        pericenterDistance = astro::AUtoKilometers(pericenterDistance);
        period = period * 365.25f;
    }

    // If we read the semi-major axis, use it to compute the pericenter
    // distance.
    if (semiMajorAxis != 0.0)
        pericenterDistance = semiMajorAxis * (1.0 - eccentricity);

    // cout << " bounding radius: " << semiMajorAxis * (1.0 + eccentricity) << "km\n";

    return new EllipticalOrbit(pericenterDistance,
                               eccentricity,
                               degToRad(inclination),
                               degToRad(ascendingNode),
                               degToRad(argOfPericenter),
                               degToRad(anomalyAtEpoch),
                               period,
                               epoch);
}


static void FillinRotationElements(Hash* rotationData,
                                   RotationElements& re,
                                   float orbitalPeriod)
{
    // The default is synchronous rotation (rotation period == orbital period)
    float period = orbitalPeriod;
    if (rotationData->getNumber("RotationPeriod", period))
        re.period = period / 24.0f;

    float offset = 0.0f;
    if (rotationData->getNumber("RotationOffset", offset))
        re.offset = degToRad(offset);

    rotationData->getNumber("RotationEpoch", re.epoch);

    float obliquity = 0.0f;
    if (rotationData->getNumber("Obliquity", obliquity))
        re.obliquity = degToRad(obliquity);

    float ascendingNode = 0.0f;
    if (rotationData->getNumber("EquatorAscendingNode", ascendingNode))
        re.ascendingNode = degToRad(ascendingNode);

    float precessionRate = 0.0f;
    if (rotationData->getNumber("PrecessionRate", precessionRate))
        re.precessionRate = degToRad(precessionRate);
}


static Orbit* CreateOrbit(PlanetarySystem* system,
                          Hash* planetData,
                          const string& path,
                          bool usePlanetUnits)
{
    Orbit* orbit = NULL;

    string customOrbitName;
    if (planetData->getString("CustomOrbit", customOrbitName))
    {
        orbit = GetCustomOrbit(customOrbitName);
        if (orbit == NULL)
        {
            DPRINTF(0, "Could not find custom orbit named '%s'\n",
                    customOrbitName.c_str());
        }
        return orbit;
    }

    string sampOrbitFile;
    if (planetData->getString("SampledOrbit", sampOrbitFile))
    {
        DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(sampOrbitFile, path));
        orbit = GetTrajectoryManager()->find(orbitHandle);
        if (orbit == NULL)
        {
            DPRINTF(0, "Could not load sampled orbit file '%s'\n",
                    sampOrbitFile.c_str());
        }
        return orbit;
    }

    Value* orbitDataValue = planetData->getValue("EllipticalOrbit");
    if (orbitDataValue != NULL)
    {
        if (orbitDataValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Object has incorrect elliptical orbit syntax.\n");
            return NULL;
        }
        else
        {
            return CreateEllipticalOrbit(orbitDataValue->getHash(),
                                         usePlanetUnits);
        }
    }

    Vec3d longlat(0.0, 0.0, 0.0);
    if (planetData->getVector("LongLat", longlat))
    {
        Body* parent = system->getPrimaryBody();
        if (parent != NULL)
        {
            Vec3f pos = parent->planetocentricToCartesian((float) longlat.x, (float) longlat.y, (float) longlat.z);
            Point3d posd(pos.x, pos.y, pos.z);
            return new SynchronousOrbit(*parent, posd);
        }
        else
        {
            // TODO: Allow fixing objects to the surface of stars.
        }
        return NULL;
    }

    return NULL;
}

// Create a body (planet or moon) using the values from a hash
// The usePlanetsUnits flags specifies whether period and semi-major axis
// are in years and AU rather than days and kilometers
static Body* CreatePlanet(PlanetarySystem* system,
                          Body* existingBody,
                          Hash* planetData,
                          const string& path,
                          Disposition disposition,
                          bool usePlanetUnits = true)
{
    Body* body = NULL;
  
    if (disposition == ModifyObject)
    {
        body = existingBody;
    }

    if (body == NULL)
    {
        body = new Body(system);
    }

    Orbit* orbit = CreateOrbit(system, planetData, path, usePlanetUnits);

    if (orbit != NULL)
    {
        body->setOrbit(orbit);
    }

    if (body->getOrbit() == NULL)
    {
        DPRINTF(0, "No valid orbit specified for object '%s'; skipping . . .\n",
                body->getName().c_str());
        delete body;
        return NULL;
    }

    double radius = (double)body->getRadius();
    planetData->getNumber("Radius", radius);
    body->setRadius((float) radius);

    int classification = body->getClassification();
    string classificationName;
    if (planetData->getString("Class", classificationName))
    {
        if (compareIgnoringCase(classificationName, "planet") == 0)
            classification = Body::Planet;
        else if (compareIgnoringCase(classificationName, "moon") == 0)
            classification = Body::Moon;
        else if (compareIgnoringCase(classificationName, "comet") == 0)
            classification = Body::Comet;
        else if (compareIgnoringCase(classificationName, "asteroid") == 0)
            classification = Body::Asteroid;
        else if (compareIgnoringCase(classificationName, "spacecraft") == 0)
            classification = Body::Spacecraft;
        else if (compareIgnoringCase(classificationName, "invisible") == 0)
            classification = Body::Invisible;
    }

    if (classification == Body::Unknown)
    {
        //Try to guess the type
        if (system->getPrimaryBody() != NULL)
        {
            if(radius > 0.1)
                classification = Body::Moon;
            else
                classification = Body::Spacecraft;
        }
        else
        {
            if(radius < 1000.0)
                classification = Body::Asteroid;
            else
                classification = Body::Planet;
        }
    }
    body->setClassification(classification);

    // g++ is missing limits header, so we can use this
    // double beginning   = -numeric_limits<double>::infinity();
    // double ending      =  numeric_limits<double>::infinity();
    double beginning   = -1.0e+50;
    double ending      =  1.0e+50;
    body->getLifespan(beginning, ending);
    getDate(planetData, "Beginning", beginning);
    getDate(planetData, "Ending", ending);
    body->setLifespan(beginning, ending);

    string infoURL;
    if (planetData->getString("InfoURL", infoURL))
        body->setInfoURL(infoURL);
    
    double albedo = 0.5;
    if (planetData->getNumber("Albedo", albedo))
        body->setAlbedo((float) albedo);

    double oblateness = 0.0;
    if (planetData->getNumber("Oblateness", oblateness))
        body->setOblateness((float) oblateness);
    
    double mass = 0.0;
    if (planetData->getNumber("Mass", mass))
        body->setMass((float) mass);

    Quatf orientation;
    if (planetData->getRotation("Orientation", orientation))
        body->setOrientation(orientation);

    RotationElements re = body->getRotationElements();
    re.period = (float) body->getOrbit()->getPeriod();
    FillinRotationElements(planetData, re, (float) body->getOrbit()->getPeriod());
    body->setRotationElements(re);

    Surface surface;
    if (disposition == ModifyObject)
    {
        surface = body->getSurface();
    }
    else
    {
        surface.color = Color(1.0f, 1.0f, 1.0f);
        surface.hazeColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    FillinSurface(planetData, &surface, path);
    body->setSurface(surface);

    {
        string model("");
        if (planetData->getString("Mesh", model))
        {
            Vec3f modelCenter(0.0f, 0.0f, 0.0f);
            if (planetData->getVector("MeshCenter", modelCenter))
            {
                // TODO: Adjust bounding radius if model center isn't
                // (0.0f, 0.0f, 0.0f)
            }

            ResourceHandle modelHandle = GetModelManager()->getHandle(ModelInfo(model, path, modelCenter));
            body->setModel(modelHandle);

        }
    }

    // Read the atmosphere
    {
        Value* atmosDataValue = planetData->getValue("Atmosphere");
        if (atmosDataValue != NULL)
        {
            if (atmosDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Atmosphere must be an assoc array.\n";
            }
            else
            {
                Hash* atmosData = atmosDataValue->getHash();
                assert(atmosData != NULL);
                
                Atmosphere* atmosphere = NULL;
                if (disposition == ModifyObject)
                {
                    atmosphere = body->getAtmosphere();
                    if (atmosphere == NULL)
                    {
                        Atmosphere atm;
                        body->setAtmosphere(atm);
                        atmosphere = body->getAtmosphere();
                    }
                }
                else
                {
                    atmosphere = new Atmosphere();
                }
                atmosData->getNumber("Height", atmosphere->height);
                atmosData->getColor("Lower", atmosphere->lowerColor);
                atmosData->getColor("Upper", atmosphere->upperColor);
                atmosData->getColor("Sky", atmosphere->skyColor);
                atmosData->getColor("Sunset", atmosphere->sunsetColor);
                atmosData->getNumber("CloudHeight", atmosphere->cloudHeight);
                if (atmosData->getNumber("CloudSpeed", atmosphere->cloudSpeed))
                    atmosphere->cloudSpeed = degToRad(atmosphere->cloudSpeed);

                string cloudTexture;
                if (atmosData->getString("CloudMap", cloudTexture))
                {
                    atmosphere->cloudTexture.setTexture(cloudTexture,
                                                        path,
                                                        TextureInfo::WrapTexture);
                }

                body->setAtmosphere(*atmosphere);
                if (disposition != ModifyObject)
                    delete atmosphere;
            }

            delete atmosDataValue;
        }
    }

    // Read the ring system
    {
        Value* ringsDataValue = planetData->getValue("Rings");
        if (ringsDataValue != NULL)
        {
            if (ringsDataValue->getType() != Value::HashType)
            {
                cout << "ReadSolarSystem: Rings must be an assoc array.\n";
            }
            else
            {
                Hash* ringsData = ringsDataValue->getHash();
                // ASSERT(ringsData != NULL);

                double inner = 0.0, outer = 0.0;
                ringsData->getNumber("Inner", inner);
                ringsData->getNumber("Outer", outer);

                Color color(1.0f, 1.0f, 1.0f);
                ringsData->getColor("Color", color);

                string textureName;
                ringsData->getString("Texture", textureName);
                MultiResTexture ringTex(textureName, path);

                body->setRings(RingSystem((float) inner, (float) outer,
                                          color, ringTex));
            }

            delete ringsDataValue;
        }
    }

    return body;
}


bool LoadSolarSystemObjects(istream& in,
                            Universe& universe,
                            const std::string& directory)
{
    Tokenizer tokenizer(&in); 
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        // Read the disposition; if none is specified, the default is Add.
        Disposition disposition = AddObject;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Add")
            {
                disposition = AddObject;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Replace")
            {
                disposition = ReplaceObject;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Modify")
            {
                disposition = ModifyObject;
                tokenizer.nextToken();
            }
        }

        // Read the item type; if none is specified the default is Body
        string itemType("Body");
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            itemType = tokenizer.getNameValue();
            tokenizer.nextToken();
        }

        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            sscError(tokenizer, "object name expected");
            return false;
        }
        string name = tokenizer.getStringValue();

        if (tokenizer.nextToken() != Tokenizer::TokenString)
        {
            sscError(tokenizer, "bad parent object name");
            return false;
        }
        string parentName = tokenizer.getStringValue();

        Value* objectDataValue = parser.readValue();
        if (objectDataValue == NULL)
        {
            sscError(tokenizer, "bad object definition");
            return false;
        }

        if (objectDataValue->getType() != Value::HashType)
        {
            sscError(tokenizer, "{ expected");
            return false;
        }
        Hash* objectData = objectDataValue->getHash();

        Selection parent = universe.findPath(parentName, NULL, 0);
        PlanetarySystem* parentSystem = NULL;

        if (itemType == "Body")
        {
            bool orbitsPlanet = false;
            if (parent.star() != NULL)
            {
                SolarSystem* solarSystem = universe.getSolarSystem(parent.star());
                if (solarSystem == NULL)
                {
                    // No solar system defined for this star yet, so we need
                    // to create it.
                    solarSystem = universe.createSolarSystem(parent.star());
                }
                parentSystem = solarSystem->getPlanets();
            }
            else if (parent.body() != NULL)
            {
                // Parent is a planet or moon
                parentSystem = parent.body()->getSatellites();
                if (parentSystem == NULL)
                {
                    // If the planet doesn't already have any satellites, we
                    // have to create a new planetary system for it.
                    parentSystem = new PlanetarySystem(parent.body());
                    parent.body()->setSatellites(parentSystem);
                }
                orbitsPlanet = true;
            }
            else
            {
                errorMessagePrelude(tokenizer);
                cerr << "parent body '" << parentName << "' of '" << name << "' not found.\n";
            }

            if (parentSystem != NULL)
            {
                Body* existingBody = parentSystem->find(name);
                if (existingBody && disposition == AddObject)
                {
                    errorMessagePrelude(tokenizer);
                    cerr << "warning duplicate definition of " <<
                        parentName << " " <<  name << '\n';
                }
                
                Body* body = CreatePlanet(parentSystem, existingBody, objectData, directory, disposition, !orbitsPlanet);
                if (body != NULL)
                {
                    body->setName(name);
                    if (disposition == ReplaceObject)
                    {
                        parentSystem->replaceBody(existingBody, body);
                        delete existingBody;
                    } 
                    else if (disposition == AddObject)
                    {
                        parentSystem->addBody(body);
                    }
                }
            }
        }
        else if (itemType == "AltSurface")
        {
            Surface* surface = new Surface();
            FillinSurface(objectData, surface, directory);
            if (surface != NULL && parent.body() != NULL)
                parent.body()->addAlternateSurface(name, surface);
            else
                sscError(tokenizer, "bad alternate surface");
        }
        else if (itemType == "Location")
        {
            if (parent.body() != NULL)
            {
                Location* location = CreateLocation(objectData, parent.body());
                if (location != NULL)
                {
                    location->setName(name);
                    parent.body()->addLocation(location);
                }
                else
                {
                    sscError(tokenizer, "bad location");
                }
            }
            else
            {
                errorMessagePrelude(tokenizer);
                cerr << "parent body '" << parentName << "' of '" << name << "' not found.\n";
            }
        }
    }

    // TODO: Return some notification if there's an error parsing the file
    return true;
}


SolarSystem::SolarSystem(Star* _star) : star(_star)
{
    planets = new PlanetarySystem(_star);
}


Star* SolarSystem::getStar() const
{
    return star;
}

Point3f SolarSystem::getCenter() const
{
    // TODO: This is a very simple method at the moment, but it will get
    // more complex when planets around multistar systems are supported
    // where the planets may orbit the center of mass of two stars.
    return star->getPosition();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}
