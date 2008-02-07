// parseobject.cpp
//
// Copyright (C) 2004-2007 Chris Laurel <claurel@gmail.com>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celutil/debug.h>
#include "parseobject.h"
#include "customorbit.h"
#include "customrotation.h"
#include "spiceorbit.h"
#include "scriptorbit.h"
#include "scriptrotation.h"
#include "frame.h"
#include "trajmanager.h"
#include "rotationmanager.h"
#include "universe.h"

using namespace std;


bool
ParseDate(Hash* hash, const string& name, double& jd)
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


/*!
 * Create a new Keplerian orbit from an ssc property table:
 *
 * EllipticalOrbit
 * {
 *     # One of the following is required to specify orbit size:
 *     SemiMajorAxis <number>
 *     PericenterDistance <number>
 *
 *     # Required
 *     Period <number>
 *     
 *     Eccentricity <number>   (default: 0.0)
 *     Inclination <degrees>   (default: 0.0)
 *     AscendingNode <degrees> (default: 0.0)
 *
 *     # One or none of the following:
 *     ArgOfPericenter <degrees>  (default: 0.0)
 *     LongOfPericenter <degrees> (default: 0.0)
 *
 *     Epoch <date> (default J2000.0)
 *
 *     # One or none of the following:
 *     MeanAnomaly <degrees>     (default: 0.0)
 *     MeanLongitude <degrees>   (default: 0.0)
 * }
 *
 * If usePlanetUnits is true:
 *     Period is in Julian years
 *     SemiMajorAxis or PericenterDistance is in AU
 * Otherwise:
 *     Period is in Julian days
 *     SemiMajorAxis or PericenterDistance is in kilometers.    
 */
static EllipticalOrbit*
CreateEllipticalOrbit(Hash* orbitData,
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
            clog << "SemiMajorAxis/PericenterDistance missing!  Skipping planet . . .\n";
            return NULL;
        }
    }

    double period = 0.0;
    if (!orbitData->getNumber("Period", period))
    {
        clog << "Period missing!  Skipping planet . . .\n";
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
        period = period * 365.25;
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


/*!
 * Create a new sampled orbit from an ssc property table:
 *
 * SampledTrajectory
 * {
 *     Source <string>
 *     Interpolation "Cubic" | "Linear"
 *     DoublePrecision <boolean>
 * }
 *
 * Source is the only required field. Interpolation defaults to cubic, and
 * DoublePrecision defaults to true.
 */
static Orbit*
CreateSampledTrajectory(Hash* trajData, const string& path)
{
    string sourceName;
    if (!trajData->getString("Source", sourceName))
    {
        clog << "SampledTrajectory is missing a source.\n";
        return NULL;
    }

    // Read interpolation type; string value must be either "Linear" or "Cubic"
    // Default interpolation type is cubic.
    string interpolationString;
    TrajectoryInterpolation interpolation = TrajectoryInterpolationCubic;
    if (trajData->getString("Interpolation", interpolationString))
    {
        if (!compareIgnoringCase(interpolationString, "linear"))
            interpolation = TrajectoryInterpolationLinear;
        else if (!compareIgnoringCase(interpolationString, "cubic"))
            interpolation = TrajectoryInterpolationCubic;
        else
            clog << "Unknown interpolation type " << interpolationString << endl; // non-fatal error
    }

    // Double precision is true by default
    bool useDoublePrecision = true;
    trajData->getBoolean("DoublePrecision", useDoublePrecision);
    TrajectoryPrecision precision = useDoublePrecision ? TrajectoryPrecisionDouble : TrajectoryPrecisionSingle;

    DPRINTF(1, "Attempting to load sampled trajectory from source '%s'\n", sourceName.c_str());
    ResourceHandle orbitHandle = GetTrajectoryManager()->getHandle(TrajectoryInfo(sourceName, path, interpolation, precision));
    Orbit* orbit = GetTrajectoryManager()->find(orbitHandle);
    if (orbit == NULL)
    {
        clog << "Could not load sampled trajectory from '" << sourceName << "'\n";
    }

    return orbit;
}


#ifdef USE_SPICE
static SpiceOrbit*
CreateSpiceOrbit(Hash* orbitData,
                 const string& path,
                 bool usePlanetUnits)
{
    string targetBodyName;
    string originName;
    string kernelFileName;
    if (!orbitData->getString("Kernel", kernelFileName))
    {
        clog << "Kernel filename missing from SPICE orbit\n";
        return NULL;
    }

    if (!orbitData->getString("Target", targetBodyName))
    {
        clog << "Target name missing from SPICE orbit\n";
        return NULL;
    }

    if (!orbitData->getString("Origin", originName))
    {
        clog << "Origin name missing from SPICE orbit\n";
        return NULL;
    }

    double beginningTDBJD = 0.0;
    if (!ParseDate(orbitData, "Beginning", beginningTDBJD))
    {
        clog << "Beginning date missing from SPICE orbit\n";
        return NULL;
    }

    double endingTDBJD = 0.0;
    if (!ParseDate(orbitData, "Ending", endingTDBJD))
    {
        clog << "Ending date missing from SPICE orbit\n";
        return NULL;
    }

    // A bounding radius for culling is required for SPICE orbits
    double boundingRadius = 0.0;
    if (!orbitData->getNumber("BoundingRadius", boundingRadius))
    {
        clog << "Bounding Radius missing from SPICE orbit\n";
        return NULL;
    }

    // The period of the orbit may be specified if appropriate; a value
    // of zero for the period (the default), means that the orbit will
    // be considered aperiodic.
    double period = 0.0;
    orbitData->getNumber("Period", period);

    if (usePlanetUnits)
    {
        boundingRadius = astro::AUtoKilometers(boundingRadius);
        period = period * 365.25;
    }

    SpiceOrbit* orbit = new SpiceOrbit(kernelFileName,
                                       targetBodyName,
                                       originName,
                                       period,
                                       boundingRadius,
                                       beginningTDBJD,
                                       endingTDBJD);
    if (!orbit->init(path))
    {
        // Error using SPICE library; destroy the orbit; hopefully a
        // fallback is defined in the SSC file.
        delete orbit;
        orbit = NULL;
    }

    return orbit;
}
#endif


static ScriptedOrbit*
CreateScriptedOrbit(Hash* orbitData,
                    const string& /*path*/)
{
#if !defined(CELX)
    clog << "ScriptedOrbit not usable without scripting support.\n";
    return NULL;
#else

    // Function name is required
    string funcName;
    if (!orbitData->getString("Function", funcName))
    {
        clog << "Function name missing from script orbit definition.\n";
        return NULL;
    }

    // Module name is optional
    string moduleName;
    orbitData->getString("Module", moduleName);

    ScriptedOrbit* scriptedOrbit = new ScriptedOrbit();
    if (scriptedOrbit != NULL)
    {
        if (!scriptedOrbit->initialize(moduleName, funcName, orbitData))
        {
            delete scriptedOrbit;
            scriptedOrbit = NULL;
        }
    }

    return scriptedOrbit;
#endif
}


Orbit*
CreateOrbit(PlanetarySystem* system,
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
        clog << "Could not find custom orbit named '" << customOrbitName <<
            "'\n";
    }

#ifdef USE_SPICE
    Value* spiceOrbitDataValue = planetData->getValue("SpiceOrbit");
    if (spiceOrbitDataValue != NULL)
    {
        if (spiceOrbitDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect spice orbit syntax.\n";
            return NULL;
        }
        else
        {
            orbit = CreateSpiceOrbit(spiceOrbitDataValue->getHash(), path, usePlanetUnits);
            if (orbit != NULL)
            {
                return orbit;
            }
            clog << "Bad spice orbit\n";
            DPRINTF(0, "Could not load SPICE orbit\n");
        }
    }
#endif

    // Trajectory calculated by Lua script
    Value* scriptedOrbitValue = planetData->getValue("ScriptedOrbit");
    if (scriptedOrbitValue != NULL)
    {
        if (scriptedOrbitValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect scripted orbit syntax.\n";
            return NULL;
        }
        else
        {
            orbit = CreateScriptedOrbit(scriptedOrbitValue->getHash(), path);
            if (orbit != NULL)
                return orbit;
        }
    }

    // New 1.5.0 style for sampled trajectories. Permits specification of
    // precision and interpolation type.
    Value* sampledTrajDataValue = planetData->getValue("SampledTrajectory");
    if (sampledTrajDataValue != NULL)
    {
        if (sampledTrajDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect syntax for SampledTrajectory.\n";
            return NULL;
        }
        else
        {
            return CreateSampledTrajectory(sampledTrajDataValue->getHash(), path);
        }
    }

    // Old style for sampled trajectories. Assumes cubic interpolation and
    // single precision.
    string sampOrbitFile;
    if (planetData->getString("SampledOrbit", sampOrbitFile))
    {
        DPRINTF(1, "Attempting to load sampled orbit file '%s'\n",
                sampOrbitFile.c_str());
        ResourceHandle orbitHandle =
            GetTrajectoryManager()->getHandle(TrajectoryInfo(sampOrbitFile,
                                                             path,
                                                             TrajectoryInterpolationCubic,
                                                             TrajectoryPrecisionSingle));
        orbit = GetTrajectoryManager()->find(orbitHandle);
        if (orbit != NULL)
        {
            return orbit;
        }
        clog << "Could not load sampled orbit file '" << sampOrbitFile << "'\n";
    }

    Value* orbitDataValue = planetData->getValue("EllipticalOrbit");
    if (orbitDataValue != NULL)
    {
        if (orbitDataValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect elliptical orbit syntax.\n";
            return NULL;
        }
        else
        {
            return CreateEllipticalOrbit(orbitDataValue->getHash(),
                                         usePlanetUnits);
        }
    }

    // Create an 'orbit' that places the object at a fixed point in its
    // reference frame.
    Vec3d fixedPosition(0.0, 0.0, 0.0);
    if (planetData->getVector("FixedPosition", fixedPosition))
    {
        // Convert to Celestia's coordinate system
        fixedPosition = Vec3d(fixedPosition.x,
                              fixedPosition.z,
                              -fixedPosition.y);

        if (usePlanetUnits)
            fixedPosition = fixedPosition * astro::AUtoKilometers(1.0);

        return new FixedOrbit(Point3d(0.0, 0.0, 0.0) + fixedPosition);
    }

    // LongLat will make an object fixed relative to the surface of its parent
    // object. This is done by creating an orbit with a period equal to the
    // rotation rate of the parent object. A body-fixed reference frame is a
    // much better way to accomplish this.
    Vec3d longlat(0.0, 0.0, 0.0);
    if (planetData->getVector("LongLat", longlat) && system != NULL)
    {
        Body* parent = system->getPrimaryBody();
        if (parent != NULL)
        {
            Vec3d pos = parent->planetocentricToCartesian(longlat.x, longlat.y, longlat.z);
            return new SynchronousOrbit(*parent, Point3d(pos.x, pos.y, pos.z));
        }
        else
        {
            // TODO: Allow fixing objects to the surface of stars.
        }
        return NULL;
    }

    return NULL;
}


static ConstantOrientation*
CreateFixedRotationModel(double offset,
                         double inclination,
                         double ascendingNode)
{
    Quatd q = Quatd::yrotation(-PI - offset) *
              Quatd::xrotation(-inclination) *
              Quatd::yrotation(-ascendingNode);

    return new ConstantOrientation(q);
}


static RotationModel*
CreateUniformRotationModel(Hash* rotationData,
                           double syncRotationPeriod)
{
    // Default to synchronous rotation
    double period = syncRotationPeriod;
    if (rotationData->getNumber("Period", period))
    {
        period = period / 24.0;
    }

    float offset = 0.0f;
    if (rotationData->getNumber("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getNumber("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getNumber("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
    {
        return CreateFixedRotationModel(offset, inclination, ascendingNode);
    }
    else
    {
        return new UniformRotationModel(period,
                                        offset,
                                        epoch,
                                        inclination,
                                        ascendingNode);
    }
}


static ConstantOrientation*
CreateFixedRotationModel(Hash* rotationData)
{
    double offset = 0.0;
    if (rotationData->getNumber("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double inclination = 0.0;
    if (rotationData->getNumber("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    double ascendingNode = 0.0;
    if (rotationData->getNumber("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    Quatd q = Quatd::yrotation(-PI - offset) *
              Quatd::xrotation(-inclination) *
              Quatd::yrotation(-ascendingNode);

    return new ConstantOrientation(q);
}


static RotationModel*
CreatePrecessingRotationModel(Hash* rotationData,
                              double syncRotationPeriod)
{
    // Default to synchronous rotation
    double period = syncRotationPeriod;
    if (rotationData->getNumber("Period", period))
    {
        period = period / 24.0;
    }

    float offset = 0.0f;
    if (rotationData->getNumber("MeridianAngle", offset))
    {
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    ParseDate(rotationData, "Epoch", epoch);

    float inclination = 0.0f;
    if (rotationData->getNumber("Inclination", inclination))
    {
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (rotationData->getNumber("AscendingNode", ascendingNode))
    {
        ascendingNode = degToRad(ascendingNode);
    }

    // The default value of 0 is handled specially, interpreted to indicate
    // that there's no precession.
    double precessionPeriod = 0.0;
    if (rotationData->getNumber("PrecessionPeriod", precessionPeriod))
    {
        // The precession period is specified in the ssc file in units
        // of years, but internally Celestia uses days.
        precessionPeriod = precessionPeriod * 365.25;
    }

    // No period was specified, and the default synchronous
    // rotation period is zero, indicating that the object
    // doesn't have a periodic orbit. Default to a constant
    // orientation instead.
    if (period == 0.0)
    {
        return CreateFixedRotationModel(offset, inclination, ascendingNode);
    }
    else
    {
        return new PrecessingRotationModel(period,
                                           offset,
                                           epoch,
                                           inclination,
                                           ascendingNode,
                                           precessionPeriod);
    }
}


static ScriptedRotation*
CreateScriptedRotation(Hash* rotationData,
                       const string& /*path*/)
{
#if !defined(CELX)
    clog << "ScriptedRotation not usable without scripting support.\n";
    return NULL;
#else

    // Function name is required
    string funcName;
    if (!rotationData->getString("Function", funcName))
    {
        clog << "Function name missing from scripted rotation definition.\n";
        return NULL;
    }

    // Module name is optional
    string moduleName;
    rotationData->getString("Module", moduleName);

    ScriptedRotation* scriptedRotation = new ScriptedRotation();
    if (scriptedRotation != NULL)
    {
        if (!scriptedRotation->initialize(moduleName, funcName, rotationData))
        {
            delete scriptedRotation;
            scriptedRotation = NULL;
        }
    }

    return scriptedRotation;
#endif
}


// Parse rotation information. Unfortunately, Celestia didn't originally have
// RotationModel objects, so information about the rotation of the object isn't
// grouped into a single subobject--the ssc fields relevant for rotation just
// appear in the top level structure.
RotationModel*
CreateRotationModel(Hash* planetData,
                    const string& path,
                    double syncRotationPeriod)
{
    RotationModel* rotationModel = NULL;

    // If more than one rotation model is specified, the following precedence
    // is used to determine which one should be used:
    //   CustomRotation
    //   SPICE C-Kernel
    //   SampledOrientation
    //   PrecessingRotation
    //   UniformRotation
    //   legacy rotation parameters

    string customRotationModelName;
    if (planetData->getString("CustomRotation", customRotationModelName))
    {
        rotationModel = GetCustomRotationModel(customRotationModelName);
        if (rotationModel != NULL)
        {
            return rotationModel;
        }
        clog << "Could not find custom rotation model named '" <<
            customRotationModelName << "'\n";
    }

#ifdef USE_SPICE
    // TODO: implement SPICE frame based rotations
#endif

    Value* scriptedRotationValue = planetData->getValue("ScriptedRotation");
    if (scriptedRotationValue != NULL)
    {
        if (scriptedRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect scripted rotation syntax.\n";
            return NULL;
        }
        else
        {
            rotationModel = CreateScriptedRotation(scriptedRotationValue->getHash(), path);
            if (rotationModel != NULL)
                return rotationModel;
        }
    }

    string sampOrientationFile;
    if (planetData->getString("SampledOrientation", sampOrientationFile))
    {
        DPRINTF(1, "Attempting to load orientation file '%s'\n",
                sampOrientationFile.c_str());
        ResourceHandle orientationHandle =
            GetRotationModelManager()->getHandle(RotationModelInfo(sampOrientationFile, path));
        rotationModel = GetRotationModelManager()->find(orientationHandle);
        if (rotationModel != NULL)
        {
            return rotationModel;
        }

        clog << "Could not load rotation model file '" <<
            sampOrientationFile << "'\n";
    }

    Value* precessingRotationValue = planetData->getValue("PrecessingRotation");
    if (precessingRotationValue != NULL)
    {
        if (precessingRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect syntax for precessing rotation.\n";
            return NULL;
        }
        else
        {
            return CreatePrecessingRotationModel(precessingRotationValue->getHash(),
                                                 syncRotationPeriod);
        }
    }

    Value* uniformRotationValue = planetData->getValue("UniformRotation");
    if (uniformRotationValue != NULL)
    {
        if (uniformRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect uniform rotation syntax.\n";
            return NULL;
        }
        else
        {
            return CreateUniformRotationModel(uniformRotationValue->getHash(),
                                              syncRotationPeriod);
        }
    }

    Value* fixedRotationValue = planetData->getValue("FixedRotation");
    if (fixedRotationValue != NULL)
    {
        if (fixedRotationValue->getType() != Value::HashType)
        {
            clog << "Object has incorrect fixed rotation syntax.\n";
            return NULL;
        }
        else
        {
            return CreateFixedRotationModel(fixedRotationValue->getHash());
        }
    }

    // For backward compatibility we need to support rotation parameters
    // that appear in the main block of the object definition.
    // Default to synchronous rotation
    bool specified = false;
    double period = syncRotationPeriod;
    if (planetData->getNumber("RotationPeriod", period))
    {
        specified = true;
        period = period / 24.0f;
    }

    float offset = 0.0f;
    if (planetData->getNumber("RotationOffset", offset))
    {
        specified = true;
        offset = degToRad(offset);
    }

    double epoch = astro::J2000;
    if (ParseDate(planetData, "RotationEpoch", epoch))
    {
        specified = true;
    }

    float inclination = 0.0f;
    if (planetData->getNumber("Obliquity", inclination))
    {
        specified = true;
        inclination = degToRad(inclination);
    }

    float ascendingNode = 0.0f;
    if (planetData->getNumber("EquatorAscendingNode", ascendingNode))
    {
        specified = true;
        ascendingNode = degToRad(ascendingNode);
    }

    double precessionRate = 0.0f;
    if (planetData->getNumber("PrecessionRate", precessionRate))
    {
        specified = true;
    }

    if (specified)
    {
        RotationModel* rm = NULL;
        if (period == 0.0)
        {
            // No period was specified, and the default synchronous
            // rotation period is zero, indicating that the object
            // doesn't have a periodic orbit. Default to a constant
            // orientation instead.
            rm = CreateFixedRotationModel(offset, inclination, ascendingNode);
        }
        else if (precessionRate == 0.0)
        {
            rm = new UniformRotationModel(period,
                                          offset,
                                          epoch,
                                          inclination,
                                          ascendingNode);
        }
        else
        {
            rm = new PrecessingRotationModel(period,
                                             offset,
                                             epoch,
                                             inclination,
                                             ascendingNode,
                                             -360.0 / precessionRate);
        }

        return rm;
    }
    else
    {
        // No rotation fields specified
        return NULL;
    }
}


RotationModel* CreateDefaultRotationModel(double syncRotationPeriod)
{
    if (syncRotationPeriod == 0.0)
    {
        // If syncRotationPeriod is 0, the orbit of the object is
        // aperiodic and we'll just return a FixedRotation.
        return new ConstantOrientation(Quatd(1.0));
    }
    else
    {
        return new UniformRotationModel(syncRotationPeriod,
                                        0.0f,
                                        astro::J2000,
                                        0.0f,
                                        0.0f);
    }
}


// Get the center object of a frame definition. Return an empty selection
// if it's missing or refers to an object that doesn't exist.
static Selection
getFrameCenter(const Universe& universe, Hash* frameData)
{
    string centerName;
    if (!frameData->getString("Center", centerName))
    {
        cerr << "No center specified for reference frame.\n";
        return Selection();
    }

    Selection centerObject = universe.findPath(centerName, NULL, 0);
    if (centerObject.empty())
    {
        cerr << "Center object '" << centerName << "' of reference frame not found.\n";
        return Selection();
    }

    // Should verify that center object is a star or planet, and
    // that it is a member of the same star system as the body in which
    // the frame will be used.

    return centerObject;
}


static BodyFixedFrame*
CreateBodyFixedFrame(const Universe& universe,
                     Hash* frameData)
{
    Selection center = getFrameCenter(universe, frameData);
    if (center.empty())
        return NULL;
    else
        return new BodyFixedFrame(center, center);
}


static BodyMeanEquatorFrame*
CreateMeanEquatorFrame(const Universe& universe,
                       Hash* frameData)
{
    Selection center = getFrameCenter(universe, frameData);
    if (center.empty())
        return NULL;

    Selection obj = center;
    string objName;
    if (frameData->getString("Object", objName))
    {
        obj = universe.findPath(objName, NULL, 0);
        if (obj.empty())
        {
            clog << "Object '" << objName << "' for mean equator frame not found.\n";
            return NULL;
        }
    }

    clog << "CreateMeanEquatorFrame " << center.getName() << ", " << obj.getName() << "\n";

    double freezeEpoch = 0.0;
    if (ParseDate(frameData, "Freeze", freezeEpoch))
    {
        return new BodyMeanEquatorFrame(center, obj, freezeEpoch);
    }
    else
    {
        return new BodyMeanEquatorFrame(center, obj);
    }
}


// Convert a string to an axis label. Permitted axis labels are
// x, y, z, -x, -y, and -z. +x, +y, and +z are allowed as synonyms for
// x, y, z. Case is ignored.
static int
parseAxisLabel(const std::string& label)
{
    if (compareIgnoringCase(label, "x") == 0 ||
        compareIgnoringCase(label, "+x") == 0)
    {
        return 1;
    }

    if (compareIgnoringCase(label, "y") == 0 ||
        compareIgnoringCase(label, "+y") == 0)
    {
        return 2;
    }

    if (compareIgnoringCase(label, "z") == 0 ||
        compareIgnoringCase(label, "+z") == 0)
    {
        return 3;
    }

    if (compareIgnoringCase(label, "-x") == 0)
    {
        return -1;
    }

    if (compareIgnoringCase(label, "-y") == 0)
    {
        return -2;
    }

    if (compareIgnoringCase(label, "-z") == 0)
    {
        return -3;
    }

    return 0;
}


static int
getAxis(Hash* vectorData)
{
    string axisLabel;
    if (!vectorData->getString("Axis", axisLabel))
    {
        DPRINTF(0, "Bad two-vector frame: missing axis label for vector.\n");
        return 0;
    }

    int axis = parseAxisLabel(axisLabel);
    if (axis == 0)
    {
        DPRINTF(0, "Bad two-vector frame: vector has invalid axis label.\n");
    }

    // Permute axis labels to match non-standard Celestia coordinate
    // conventions: y <- z, z <- -y
    switch (axis)
    {
    case 2:
        return -3;
    case -2:
        return 3;
    case 3:
        return 2;
    case -3:
        return -2;
    default:
        return axis;
    }

    return axis;
}


// Get the target object of a direction vector definition. Return an
// empty selection if it's missing or refers to an object that doesn't exist.
static Selection
getVectorTarget(const Universe& universe, Hash* vectorData)
{
    string targetName;
    if (!vectorData->getString("Target", targetName))
    {
        clog << "Bad two-vector frame: no target specified for vector.\n";
        return Selection();
    }

    Selection targetObject = universe.findPath(targetName, NULL, 0);
    if (targetObject.empty())
    {
        clog << "Bad two-vector frame: target object '" << targetName << "' of vector not found.\n";
        return Selection();
    }

    return targetObject;
}


// Get the observer object of a direction vector definition. Return an
// empty selection if it's missing or refers to an object that doesn't exist.
static Selection
getVectorObserver(const Universe& universe, Hash* vectorData)
{
    string obsName;
    if (!vectorData->getString("Observer", obsName))
    {
        // Omission of observer is permitted; it will default to the
        // frame center.
        return Selection();
    }

    Selection obsObject = universe.findPath(obsName, NULL, 0);
    if (obsObject.empty())
    {
        clog << "Bad two-vector frame: observer object '" << obsObject.getName() << "' of vector not found.\n";
        return Selection();
    }

    return obsObject;
}


static FrameVector*
CreateFrameVector(const Universe& universe,
                  const Selection& center,
                  Hash* vectorData)
{
    Value* value = NULL;

    value = vectorData->getValue("RelativePosition");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* relPosData = value->getHash();
        Selection observer = getVectorObserver(universe, relPosData);
        Selection target = getVectorTarget(universe, relPosData);
        // Default observer is the frame center
        if (observer.empty())
            observer = center;

        if (observer.empty() || target.empty())
            return NULL;
        else
            return new FrameVector(FrameVector::createRelativePositionVector(observer, target));
    }

    value = vectorData->getValue("RelativeVelocity");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* relVData = value->getHash();
        Selection observer = getVectorObserver(universe, relVData);
        Selection target = getVectorTarget(universe, relVData);
        // Default observer is the frame center
        if (observer.empty())
            observer = center;

        if (observer.empty() || target.empty())
            return NULL;
        else
            return new FrameVector(FrameVector::createRelativeVelocityVector(observer, target));
    }

    value = vectorData->getValue("ConstantVector");
    if (value != NULL && value->getHash() != NULL)
    {
        Hash* constVecData = value->getHash();
        Vec3d vec(0.0, 0.0, 1.0);
        constVecData->getVector("Vector", vec);
        if (vec.length() == 0.0)
        {
            clog << "Bad two-vector frame: constant vector has length zero\n";
            return NULL;
        }
        vec.normalize();
        vec = Vec3d(vec.x, vec.z, -vec.y);

        // The frame for the vector is optional; a NULL frame indicates
        // J2000 ecliptic.
        ReferenceFrame* f = NULL;
        Value* frameValue = constVecData->getValue("Frame");
        if (frameValue != NULL)
        {
            f = CreateReferenceFrame(universe, frameValue);
            if (f == NULL)
                return NULL;
        }

        return new FrameVector(FrameVector::createConstantVector(vec, f));
    }
    else
    {
        clog << "Bad two-vector frame: unknown vector type\n";
        return NULL;
    }
}


static TwoVectorFrame*
CreateTwoVectorFrame(const Universe& universe,
                     Hash* frameData)
{
    Selection center = getFrameCenter(universe, frameData);
    if (center.empty())
        return NULL;

    // Primary and secondary vector definitions are required
    Value* primaryValue = frameData->getValue("Primary");
    if (!primaryValue)
    {
        clog << "Primary axis missing from two-vector frame.\n";
        return NULL;
    }

    Hash* primaryData = primaryValue->getHash();
    if (!primaryData)
    {
        clog << "Bad syntax for primary axis of two-vector frame.\n";
        return NULL;
    }

    Value* secondaryValue = frameData->getValue("Secondary");
    if (!secondaryValue)
    {
        clog << "Secondary axis missing from two-vector frame.\n";
        return NULL;
    }

    Hash* secondaryData = secondaryValue->getHash();
    if (!secondaryData)
    {
        clog << "Bad syntax for secondary axis of two-vector frame.\n";
        return NULL;
    }

    // Get and validate the axes for the direction vectors
    int primaryAxis = getAxis(primaryData);
    int secondaryAxis = getAxis(secondaryData);

    assert(abs(primaryAxis) <= 3);
    assert(abs(secondaryAxis) <= 3);
    if (primaryAxis == 0 || secondaryAxis == 0)
    {
        return NULL;
    }

    if (abs(primaryAxis) == abs(secondaryAxis))
    {
        clog << "Bad two-vector frame: axes for vectors are collinear.\n";
        return NULL;
    }

    FrameVector* primaryVector = CreateFrameVector(universe,
                                                   center,
                                                   primaryData);
    FrameVector* secondaryVector = CreateFrameVector(universe,
                                                     center,
                                                     secondaryData);

    TwoVectorFrame* frame = NULL;
    if (primaryVector != NULL && secondaryVector != NULL)
    {
        frame = new TwoVectorFrame(center,
                                   *primaryVector, primaryAxis,
                                   *secondaryVector, secondaryAxis);
    }

    delete primaryVector;
    delete secondaryVector;

    return frame;
}


static J2000EclipticFrame*
CreateJ2000EclipticFrame(const Universe& universe,
                         Hash* frameData)
{
    Selection center = getFrameCenter(universe, frameData);
    if (center.empty())
        return NULL;
    else
        return new J2000EclipticFrame(center);
}


static J2000EquatorFrame*
CreateJ2000EquatorFrame(const Universe& universe,
                        Hash* frameData)
{
    Selection center = getFrameCenter(universe, frameData);
    if (center.empty())
        return NULL;
    else
        return new J2000EquatorFrame(center);
}


static ReferenceFrame*
CreateComplexFrame(const Universe& universe, Hash* frameData)
{
    Value* value = frameData->getValue("BodyFixed");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect body-fixed frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateBodyFixedFrame(universe, value->getHash());
        }
    }

    value = frameData->getValue("MeanEquator");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect mean equator frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateMeanEquatorFrame(universe, value->getHash());
        }
    }

    value = frameData->getValue("TwoVector");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect two-vector frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateTwoVectorFrame(universe, value->getHash());
        }
    }

    value = frameData->getValue("EclipticJ2000");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect J2000 ecliptic frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateJ2000EclipticFrame(universe, value->getHash());
        }
    }

    value = frameData->getValue("EquatorJ2000");
    if (value != NULL)
    {
        if (value->getType() != Value::HashType)
        {
            clog << "Object has incorrect J2000 equator frame syntax.\n";
            return NULL;
        }
        else
        {
            return CreateJ2000EquatorFrame(universe, value->getHash());
        }
    }

    clog << "Frame definition does not have a valid frame type.\n";

    return NULL;
}


ReferenceFrame* CreateReferenceFrame(const Universe& universe,
                                     Value* frameValue)
{
    if (frameValue->getType() == Value::StringType)
    {
        // TODO: handle named frames
        return NULL;
    }
    else if (frameValue->getType() == Value::HashType)
    {
        return CreateComplexFrame(universe, frameValue->getHash());
    }
    else
    {
        clog << "Invalid syntax for frame definition.\n";
        return NULL;
    }
}
