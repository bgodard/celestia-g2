// rotation.h
//
// Copyright (C) 2004-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ROTATION_H_
#define _CELENGINE_ROTATION_H_

#include <celmath/quaternion.h>
#include <celengine/astro.h>


/*! A RotationModel object describes the orientation of an object
 *  over some time range.
 */
class RotationModel
{
 public:
    virtual ~RotationModel() {};

    /*! Return the orientation of an object in its reference frame at the
     * specified time (TDB). Some rotations can be decomposed into two parts:
     * a fixed or slowly varying part, and a much more rapidly varying part.
     * The rotation of a planet is such an example. The rapidly varying part
     * is referred to as spin; the slowly varying part determines the
     * equatorial plane. When the rotation of an object can be decomposed
     * in this way, the overall orientation = spin * equator. Otherwise,
     * orientation = spin.
     */
    Quatd orientationAtTime(double tjd) const
    {
        return spin(tjd) * equatorOrientationAtTime(tjd);
    }

    /*! Return the orientation of the equatorial plane (normal to the primary
     *  axis of rotation.) The overall orientation of the object is
     *  spin * equator. If there is no primary axis of rotation, equator = 1
     *  and orientation = spin.
     */
    virtual Quatd equatorOrientationAtTime(double /*tjd*/) const
    {
        return Quatd(1.0, 0.0, 0.0, 0.0);
    }

    /*! Return the rotation about primary axis of rotation (if there is one.)
     *  The overall orientation is spin * equator. For objects without a
     *  primary axis of rotation, spin *is* the orientation.
     */
    virtual Quatd spin(double tjd) const = 0;

    virtual double getPeriod() const
    {
        return 0.0;
    };

    virtual bool isPeriodic() const
    {
        return false;
    };

    // Return the time range over which the orientation model is valid;
    // if the model is always valid, begin and end should be equal.
    virtual void getValidRange(double& begin, double& end) const
    {
        begin = 0.0;
        end = 0.0;
    };
};


/*! The simplest rotation model is ConstantOrientation, which describes
 *  an orientation that is fixed within a reference frame.
 */
class ConstantOrientation : public RotationModel
{
 public:
    ConstantOrientation(const Quatd& q);
    virtual ~ConstantOrientation();

    virtual Quatd spin(double tjd) const;

 private:
    Quatd orientation;
};


/*! UniformRotationModel describes an object that rotates with a constant
 *  angular velocity.
 */
class UniformRotationModel : public RotationModel
{
 public:
    UniformRotationModel(float _period,
                         float _offset,
                         double _epoch,
                         float _inclination,
                         float _ascendingNode);
    virtual ~UniformRotationModel();

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual Quatd equatorOrientationAtTime(double tjd) const;
    virtual Quatd spin(double tjd) const;

 private:
    float period;        // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float inclination;   // tilt of rotation axis w.r.t. reference plane
    float ascendingNode; // longitude of ascending node of equator on the refernce plane
};


/*! PrecessingRotationModel describes an object with a spin axis that
 *  precesses at a constant rate about some axis.
 */
class PrecessingRotationModel : public RotationModel
{
 public:
    PrecessingRotationModel(float _period,
                            float _offset,
                            double _epoch,
                            float _inclination,
                            float _ascendingNode,
                            float _precPeriod);
    virtual ~PrecessingRotationModel();

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual Quatd equatorOrientationAtTime(double tjd) const;
    virtual Quatd spin(double tjd) const;

 private:
    float period;        // sidereal rotation period (in Julian days)
    float offset;        // rotation at epoch
    double epoch;
    float inclination;   // tilt of rotation axis w.r.t. reference plane
    float ascendingNode; // longitude of ascending node of equator on the refernce plane

    float precessionPeriod; // period of precession (in Julian days)
};

#endif // _CELENGINE_ROTATION_H_
