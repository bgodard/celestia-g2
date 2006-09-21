// spiceorbit.h
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SPICEORBIT_H_
#define _CELENGINE_SPICEORBIT_H_

#include <string>
#include <celengine/orbit.h>

class SpiceOrbit : public CachingOrbit
{
 public:
    SpiceOrbit(const std::string& _kernelFile,
               const std::string& _targetBodyName,
               const std::string& _originName,
               double _period,
               double _boundingRadius);
    virtual ~SpiceOrbit();

    bool init(const std::string& path);

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;

    virtual double getBoundingRadius() const
    {
        return boundingRadius;
    }

    Point3d computePosition(double jd) const;

    virtual void getValidRange(double& begin, double& end) const;

 private:
    const std::string kernelFile;
    const std::string targetBodyName;
    const std::string originName;
    double period;
    double boundingRadius;
    bool spiceErr;

    // NAIF ID codes for the target body and origin body
    int targetID;
    int originID;

    double validIntervalBegin;
    double validIntervalEnd;
};

#endif // _CELENGINE_SPICEORBIT_H_
