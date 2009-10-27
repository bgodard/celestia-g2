// samporient.cpp
//
// Copyright (C) 2006-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// The SampledOrientation class models orientation of a body by interpolating
// a sequence of key frames.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <Eigen/NewStdVector>
#include <celengine/samporient.h>
#include <celmath/mathlib.h>
#include <celmath/geomutil.h>
#include <cmath>
#include <cassert>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

using namespace Eigen;
using namespace std;

struct OrientationSample
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Eigen::Quaternionf q;
    double t;
};

typedef vector<OrientationSample, aligned_allocator<OrientationSample> > OrientationSampleVector;

/*!
 * Sampled orientation files are ASCII text files containing a sequence of
 * time stamped quaternion keys. Each record in the file has the form:
 *
 *   <time> <qw> <qx> <qy> <qz>
 *  
 * Where (qw qx qy qz) is a unit quaternion representing a rotation of
 *   theta = acos(qw)*2 radians about the axis (qx, qy, qz)*sin(theta/2).
 * The time values are Julian days in Barycentric Dynamical Time. The records
 * in the orientation file should be ordered so that their times are
 * monotonically increasing.
 *
 * A very simple example file:
 *
 *   2454025 1     0     0     0
 *   2454026 0.707 0.707 0     0
 *   2454027 0     0     1     0
 *
 * Note that while each record of this example file is on a separate line,
 * all whitespace is treated identically, so the entire file could be one
 * a single line.
 */

// 90 degree rotation about x-axis to convert orientation to Celestia's
// coordinate system.
static Quaternionf coordSysCorrection = XRotation((float) (PI / 2.0));


bool operator<(const OrientationSample& a, const OrientationSample& b)
{
    return a.t < b.t;
}

/*! SampledOrientation is a rotation model that interpolates a sequence
 *  of quaternion keyframes. Typically, an instance of SampledRotation will
 *  be created from a file with LoadSampledOrientation().
 */
class SampledOrientation : public RotationModel
{
public:
    SampledOrientation();
    virtual ~SampledOrientation();

    /*! Add another quaternion key to the sampled orientation. The keys
     *  should have monotonically increasing time values.
     */
    void addSample(double tjd, const Quaternionf& q);

    /*! The orientation of a sampled rotation model is entirely due
     *  to spin (i.e. there's no notion of an equatorial frame.)
     */
    virtual Eigen::Quaterniond spin(double tjd) const;

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;

    virtual void getValidRange(double& begin, double& end) const;

private:
    Quaternionf getOrientation(double tjd) const;

private:
    OrientationSampleVector samples;
    mutable int lastSample;

    enum InterpolationType
    {
        Linear = 0,
        Cubic = 1,
    };

    InterpolationType interpolation;
};


SampledOrientation::SampledOrientation() :
    lastSample(0),
    interpolation(Linear)
{
}


SampledOrientation::~SampledOrientation()
{
}


void
SampledOrientation::addSample(double t, const Eigen::Quaternionf& q)
{
    // TODO: add a check for out of sequence samples
    OrientationSample samp;
    samp.t = t;
    samp.q = q * coordSysCorrection;
    samples.push_back(samp);
}


Eigen::Quaterniond
SampledOrientation::spin(double tjd) const
{
    // TODO: cache the last value returned
    return getOrientation(tjd).cast<double>();
}


double SampledOrientation::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


bool SampledOrientation::isPeriodic() const
{
    return false;
}


void SampledOrientation::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


Quaternionf
SampledOrientation::getOrientation(double tjd) const
{
    Quaternionf orientation;
    if (samples.size() == 0)
    {
        orientation = Quaternionf::Identity();
    }
    else if (samples.size() == 1)
    {
        orientation = samples[0].q;
    }
    else
    {
        OrientationSample samp;
        samp.t = tjd;
        int n = lastSample;

        // Do a binary search to find the samples that define the orientation
        // at the current time. Cache the previous sample used and avoid
        // the search if the covers the requested time.
        if (n < 1 || n >= (int) samples.size() || tjd < samples[n - 1].t || tjd > samples[n].t)
        {
            OrientationSampleVector::const_iterator iter = lower_bound(samples.begin(),
                                                                       samples.end(),
                                                                       samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n == 0)
        {
            orientation = samples[0].q;
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == Linear)
            {
                OrientationSample s0 = samples[n - 1];
                OrientationSample s1 = samples[n];

                float t = (float) ((tjd - s0.t) / (s1.t - s0.t));
                orientation = s0.q.slerp(t, s1.q);
            }
            else if (interpolation == Cubic)
            {
                // TODO: add support for cubic interpolation of quaternions
                assert(0);
            }
            else
            {
                // Unknown interpolation type
                orientation = Quaternionf::Identity();
            }
        }
        else
        {
            orientation = samples[samples.size() - 1].q;
        }
    }

    return orientation;
}


RotationModel* LoadSampledOrientation(const string& filename)
{
    ifstream in(filename.c_str());
    if (!in.good())
        return NULL;

    SampledOrientation* sampOrientation = new SampledOrientation();
    int nSamples = 0;
    while (in.good())
    {
        double tjd;
        Quaternionf q;
        in >> tjd;
        in >> q.w();
        in >> q.x();
        in >> q.y();
        in >> q.z();
        q.normalize();

        if (in.good())
        {
            sampOrientation->addSample(tjd, q);
            nSamples++;
        }
    }

    return sampOrientation;
}
