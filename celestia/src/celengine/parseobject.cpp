// parseobject.cpp
//
// Copyright (C) 2004 Chris Laurel <claurel@shatters.net>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/debug.h>
#include "parseobject.h"
#include "customorbit.h"
#include "trajmanager.h"


using namespace std;


bool ParseDate(Hash* hash, const string& name, double& jd)
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
    ParseDate(orbitData, "Epoch", epoch);

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

    return new EllipticalOrbit(pericenterDistance,
                               eccentricity,
                               degToRad(inclination),
                               degToRad(ascendingNode),
                               degToRad(argOfPericenter),
                               degToRad(anomalyAtEpoch),
                               period,
                               epoch);
}


Orbit* CreateOrbit(PlanetarySystem* system,
                   Hash* planetData,
                   const string& path,
                   bool usePlanetUnits)
{
    Orbit* orbit = NULL;

    string customOrbitName;
    if (planetData->getString("CustomOrbit", customOrbitName))
    {
        orbit = GetCustomOrbit(customOrbitName);
        if (orbit != NULL)
        {
            return orbit;
        }
        DPRINTF(0, "Could not find custom orbit named '%s'\n",
                customOrbitName.c_str());
    }

    string sampOrbitFile;
    if (planetData->getString("SampledOrbit", sampOrbitFile))
    {
        DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(sampOrbitFile, path));
        orbit = GetTrajectoryManager()->find(orbitHandle);
        if (orbit != NULL)
        {
            return orbit;
        }
        DPRINTF(0, "Could not load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
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
    if (planetData->getVector("LongLat", longlat) && system != NULL)
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
