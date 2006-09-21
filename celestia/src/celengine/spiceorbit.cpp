// spiceorbit.cpp
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <cstdio>
#include "SpiceUsr.h"
#include "astro.h"
#include "spiceorbit.h"

using namespace std;


static bool getNAIFID(const string& name, int* id);

SpiceOrbit::SpiceOrbit(const std::string& _kernelFile,
                       const std::string& _targetBodyName,
                       const std::string& _originName,
                       double _period,
                       double _boundingRadius) :
    kernelFile(_kernelFile),
    targetBodyName(_targetBodyName),
    originName(_originName),
    period(_period),
    boundingRadius(_boundingRadius),
    spiceErr(false)
{
}


SpiceOrbit::~SpiceOrbit()
{
}


bool
SpiceOrbit::isPeriodic() const
{
    return period != 0.0;
};


double
SpiceOrbit::getPeriod() const
{
    if (isPeriodic())
    {
        return period;
    }
    else
    {
        return validIntervalEnd - validIntervalBegin;
    }
}


bool
SpiceOrbit::init(const std::string& path)
{
    string filepath = path + string("/data/") + kernelFile;
    furnsh_c(filepath.c_str());

    // If there was an error loading the kernel, flag an error
    // and don't bother calling SPICE to compute the orbit.
    if (failed_c())
    {
        spiceErr = true;
        char errMsg[1024];
        getmsg_c("long", sizeof(errMsg), errMsg);
        clog << errMsg << "\n";

        // Reset the SPICE error state so that future calls to
        // SPICE can still succeed.
        reset_c();
    }

    // Get the ID codes for the target
    if (!getNAIFID(targetBodyName, &targetID))
    {
        clog << "Couldn't find SPICE ID for " << targetBodyName << "\n";
        return false;
    }

    if (!getNAIFID(originName, &originID))
    {
        clog << "Couldn't find SPICE ID for " << originName << "\n";
        return false;
    }

    // Get coverage window
    const int MaxIntervals = 10;

    SPICEDOUBLE_CELL ( cover, MaxIntervals * 2 );
    spkcov_c(filepath.c_str(), targetID, &cover);
    
    // First check the coverage window of the target. Only consider the
    // first interval in the window.
    // TODO: consider supporting noncontiguous coverage
    SpiceInt nIntervals = card_c(&cover) / 2;
    SpiceDouble beginning = 0.0;
    SpiceDouble ending = 0.0;
    if (nIntervals > 0)
    {
        wnfetd_c(&cover, 0, &beginning, &ending);
        validIntervalBegin = astro::secsToDays(beginning + 0.001) + astro::J2000;
        validIntervalEnd = astro::secsToDays(ending - 0.001) + astro::J2000;
    }
    else
    {
        // No valid coverage window for target; use the coverage of the
        // origin.
        spkcov_c(filepath.c_str(), originID, &cover);
        nIntervals = card_c(&cover) / 2;
        if (nIntervals > 0)
        {
            wnfetd_c(&cover, 0, &beginning, &ending);
            validIntervalBegin = astro::secsToDays(beginning + 0.001) + astro::J2000;
            validIntervalEnd = astro::secsToDays(ending - 0.001) + astro::J2000;
        }
        else
        {
            // No coverage window defined
            validIntervalBegin = validIntervalEnd = astro::J2000;
        }
    }
    
    return !spiceErr;
}


Point3d
SpiceOrbit::computePosition(double jd) const
{
    if (jd < validIntervalBegin)
        jd = validIntervalBegin;
    else if (jd > validIntervalEnd)
        jd = validIntervalEnd;

    if (spiceErr)
    {
        return Point3d(0.0, 0.0, 0.0);
    }
    else
    {
        // Input time for SPICE is seconds after J2000
        double t = astro::daysToSecs(jd - astro::J2000);
        double state[6];    // State is position and velocity
        double lt;          // One way light travel time

        // TODO: use spkgps_c instead of spkezr as it should
        // be faster.
        spkezr_c(targetBodyName.c_str(),
                 t,
                 "eclipj2000",
                 "none",
                 originName.c_str(),
                 state,
                 &lt);

        // TODO: test calculating the orbit at initialization time
        // to try and catch errors earlier.
        if (failed_c())
        {
            // Print the error message
            char errMsg[1024];
            getmsg_c("long", sizeof(errMsg), errMsg);
            clog << errMsg << "\n";
            
            // Reset the error state
            reset_c();
        }

        // Transform into Celestia's coordinate system
        return Point3d(state[0], state[2], -state[1]);
    }
}


void SpiceOrbit::getValidRange(double& begin, double& end) const
{
    begin = validIntervalBegin;
    end = validIntervalEnd;
}


bool getNAIFID(const string& name, int* id)
{
    SpiceInt spiceID = 0;
    SpiceBoolean found = SPICEFALSE;
    
    // Don't call bodn2c on an empty string because SPICE generates
    // an error if we do.
    if (!name.empty())
    {
        bodn2c_c(name.c_str(), &spiceID, &found);
        if (found)
        {
            *id = (int) spiceID;
        }
        else
        {
            // Is it a numeric ID?
            if (sscanf(name.c_str(), " %d", &spiceID) == 1)
            {
                *id = (int) spiceID;
                found = SPICETRUE;
            }
        }
    }

    return found == SPICETRUE;
}
