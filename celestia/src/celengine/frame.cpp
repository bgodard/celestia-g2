// frame.cpp
// 
// Reference frame base class.
//
// Copyright (C) 2003-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celengine/frame.h>
#include "eigenport.h"

using namespace Eigen;
using namespace std;


// Velocity for two-vector frames is computed by differentiation; units
// are Julian days.
static const double ANGULAR_VELOCITY_DIFF_DELTA = 1.0 / 1440.0;


/*** ReferenceFrame ***/

ReferenceFrame::ReferenceFrame(Selection center) :
    centerObject(center),
    refCount(0)
{
}


int
ReferenceFrame::addRef() const
{
    return ++refCount;
}


int
ReferenceFrame::release() const
{
    --refCount;
    assert(refCount >= 0);
    if (refCount <= 0)
        delete this;

    return refCount;
}


// High-precision rotation using 64.64 fixed point path. Rotate uc by
// the rotation specified by unit quaternion q.
static UniversalCoord rotate(const UniversalCoord& uc, const Quatd& q)
{
    Mat3d r = q.toMatrix3();
    UniversalCoord uc1;
    
    uc1.x = uc.x * BigFix(r[0].x) + uc.y * BigFix(r[1].x) + uc.z * BigFix(r[2].x);
    uc1.y = uc.x * BigFix(r[0].y) + uc.y * BigFix(r[1].y) + uc.z * BigFix(r[2].y);
    uc1.z = uc.x * BigFix(r[0].z) + uc.y * BigFix(r[1].z) + uc.z * BigFix(r[2].z);
    
    return uc1;
}


/*! Convert from universal coordinates to frame coordinates. This method
 *  uses 64.64 fixed point arithmetic in conversion, and is thus /much/ slower
 *  than convertFromAstrocentric(), which works with double precision
 *  floating points values. For cases when the bodies are all in the same
 *  solar system, convertFromAstrocentric() should be used.
 */
UniversalCoord
ReferenceFrame::convertFromUniversal(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord uc1 = uc.difference(centerObject.getPosition(tjd));
    return rotate(uc1, conjugate(getOrientation(tjd)));
}


Quatd
ReferenceFrame::convertFromUniversal(const Quatd& q, double tjd) const
{
    return q * conjugate(getOrientation(tjd));
}


/*! Convert from local coordinates to universal coordinates. This method
 *  uses 64.64 fixed point arithmetic in conversion, and is thus /much/ slower
 *  than convertFromAstrocentric(), which works with double precision
 *  floating points values. For cases when the bodies are all in the same
 *  solar system, convertFromAstrocentric() should be used.
 *
 *  To get the position of a solar system object in universal coordinates,
 *  it usually suffices to get the astrocentric position and then add that
 *  to the position of the star in universal coordinates. This avoids any
 *  expensive high-precision multiplication.
 */
UniversalCoord
ReferenceFrame::convertToUniversal(const UniversalCoord& uc, double tjd) const
{
    return centerObject.getPosition(tjd) + rotate(uc, getOrientation(tjd));
}


Quatd
ReferenceFrame::convertToUniversal(const Quatd& q, double tjd) const
{
    return q * getOrientation(tjd);
}


Point3d
ReferenceFrame::convertFromAstrocentric(const Point3d& p, double tjd) const
{
    if (centerObject.getType() == Selection::Type_Body)
    {
        Point3d center = ptFromEigen(centerObject.body()->getAstrocentricPosition(tjd));
        return Point3d(0.0, 0.0, 0.0) + (p - center) * conjugate(getOrientation(tjd)).toMatrix3();
    }
    else if (centerObject.getType() == Selection::Type_Star)
    {
        return p * conjugate(getOrientation(tjd)).toMatrix3();
    }
    else
    {
        // TODO:
        // bad if the center object is a galaxy
        // what about locations?
        return Point3d(0.0, 0.0, 0.0);
    }
}


Point3d
ReferenceFrame::convertToAstrocentric(const Point3d& p, double tjd) const
{
    if (centerObject.getType() == Selection::Type_Body)
    {
        Point3d center = ptFromEigen(centerObject.body()->getAstrocentricPosition(tjd));
        return center + p * getOrientation(tjd).toMatrix3();
    }
    else if (centerObject.getType() == Selection::Type_Star)
    {
        return p * getOrientation(tjd).toMatrix3();
    }
    else
    {
        // TODO:
        // bad if the center object is a galaxy
        // what about locations?
        return Point3d(0.0, 0.0, 0.0);
    }    
}


/*! Return the object that is the defined origin of the reference frame.
 */
Selection
ReferenceFrame::getCenter() const
{
    return centerObject;
}


Vec3d
ReferenceFrame::getAngularVelocity(double tjd) const
{
	Quatd q0 = getOrientation(tjd);
	Quatd q1 = getOrientation(tjd + ANGULAR_VELOCITY_DIFF_DELTA);
	Quatd dq = ~q0 * q1;

	if (fabs(dq.w) > 0.99999999)
		return Vec3d(0.0, 0.0, 0.0);

	Vec3d v(dq.x, dq.y, dq.z);
	v.normalize();
	return v * (2.0 * acos(dq.w) / ANGULAR_VELOCITY_DIFF_DELTA);
}


unsigned int
ReferenceFrame::nestingDepth(unsigned int maxDepth, FrameType frameType) const
{
    return this->nestingDepth(0, maxDepth, frameType);
}


static unsigned int
getFrameDepth(const Selection& sel, unsigned int depth, unsigned int maxDepth,
              ReferenceFrame::FrameType frameType)
{
    if (depth > maxDepth)
        return depth;

    Body* body = sel.body();
    if (sel.location() != NULL)
        body = sel.location()->getParentBody();

    if (body == NULL)
    {
        return depth;
    }

    unsigned int orbitFrameDepth = depth;
    unsigned int bodyFrameDepth = depth;
    // TODO: need to check /all/ orbit frames of body
    if (body->getOrbitFrame(0.0) != NULL && frameType == ReferenceFrame::PositionFrame)
    {
        orbitFrameDepth = body->getOrbitFrame(0.0)->nestingDepth(depth + 1, maxDepth, frameType);
        if (orbitFrameDepth > maxDepth)
            return orbitFrameDepth;
    }
        
    if (body->getBodyFrame(0.0) != NULL && frameType == ReferenceFrame::OrientationFrame)
    {
        bodyFrameDepth = body->getBodyFrame(0.0)->nestingDepth(depth + 1, maxDepth, frameType);
    }

    return max(orbitFrameDepth, bodyFrameDepth);
}



/*** J2000EclipticFrame ***/

J2000EclipticFrame::J2000EclipticFrame(Selection center) :
    ReferenceFrame(center)
{
}


bool
J2000EclipticFrame::isInertial() const
{
	return true;
}


unsigned int
J2000EclipticFrame::nestingDepth(unsigned int depth,
                                 unsigned int maxDepth,
                                 FrameType) const
{
    return getFrameDepth(getCenter(), depth, maxDepth, PositionFrame);
}


/*** J2000EquatorFrame ***/

J2000EquatorFrame::J2000EquatorFrame(Selection center) :
    ReferenceFrame(center)
{
}


Quatd
J2000EquatorFrame::getOrientation(double /* tjd */) const
{
    return Quatd::xrotation(astro::J2000Obliquity);
}


bool
J2000EquatorFrame::isInertial() const
{
	return true;
}


unsigned int
J2000EquatorFrame::nestingDepth(unsigned int depth,
                                unsigned int maxDepth,
                                FrameType) const
{
    return getFrameDepth(getCenter(), depth, maxDepth, PositionFrame);
}


/*** BodyFixedFrame ***/

BodyFixedFrame::BodyFixedFrame(Selection center, Selection obj) :
    ReferenceFrame(center),
    fixObject(obj)
{
}


Quatd
BodyFixedFrame::getOrientation(double tjd) const
{
    // Rotation of 180 degrees about the y axis is required
    // TODO: this rotation could go in getEclipticalToBodyFixed()
    Quaterniond yrot180(0.0, 0.0, 1.0, 0.0);

    switch (fixObject.getType())
    {
    case Selection::Type_Body:
        return fromEigen(yrot180 * fixObject.body()->getEclipticToBodyFixed(tjd));
    case Selection::Type_Star:
        return fromEigen(yrot180 * toEigen(fixObject.star()->getRotationModel()->orientationAtTime(tjd)));
    default:
        return fromEigen(yrot180);
    }
}


Vec3d
BodyFixedFrame::getAngularVelocity(double tjd) const
{
	switch (fixObject.getType())
	{
	case Selection::Type_Body:
        return fromEigen(fixObject.body()->getAngularVelocity(tjd));
	case Selection::Type_Star:
		return fixObject.star()->getRotationModel()->angularVelocityAtTime(tjd);
	default:
		return Vec3d(0.0, 0.0, 0.0);
	}
}


bool
BodyFixedFrame::isInertial() const
{
	return false;
}


unsigned int
BodyFixedFrame::nestingDepth(unsigned int depth,
                             unsigned int maxDepth,
                             FrameType) const
{
    unsigned int n = getFrameDepth(getCenter(), depth, maxDepth, PositionFrame);
    if (n > maxDepth)
    {
        return n;
    }
    else
    {
        unsigned int m = getFrameDepth(fixObject, depth, maxDepth, OrientationFrame);
        return max(m, n);
    }
}


/*** BodyMeanEquatorFrame ***/

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(astro::J2000),
    isFrozen(false)
{
}


BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj,
                                           double freeze) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(freeze),
    isFrozen(true)
{
}


Quatd
BodyMeanEquatorFrame::getOrientation(double tjd) const
{
    double t = isFrozen ? freezeEpoch : tjd;

    switch (equatorObject.getType())
    {
    case Selection::Type_Body:
        return fromEigen(equatorObject.body()->getEclipticToEquatorial(t));
    case Selection::Type_Star:
        return equatorObject.star()->getRotationModel()->equatorOrientationAtTime(t);
    default:
        return Quatd(1.0);
    }
}


Vec3d
BodyMeanEquatorFrame::getAngularVelocity(double tjd) const
{
	if (isFrozen)
	{
		return Vec3d(0.0, 0.0, 0.0);
	}
	else
	{
        if (equatorObject.body() != NULL)
        {
            return equatorObject.body()->getBodyFrame(tjd)->getAngularVelocity(tjd);
        }
        else
        {
            return Vec3d(0.0, 0.0, 0.0);
        }
	}
}


bool
BodyMeanEquatorFrame::isInertial() const
{
	if (isFrozen)
	{
		return true;
	}
	else
	{
		// Although the mean equator of an object may vary slightly due to precession,
		// treat it as an inertial frame as long as the body frame of the object is
		// also inertial.
		if (equatorObject.body() != NULL)
		{
            // TIMELINE-TODO: isInertial must take a time argument.
			return equatorObject.body()->getBodyFrame(0.0)->isInertial();
		}
        else
        {
            return true;
        }
	}
}


unsigned int
BodyMeanEquatorFrame::nestingDepth(unsigned int depth,
                                   unsigned int maxDepth,
                                   FrameType) const
{
    // Test origin and equator object (typically the same) frames
    unsigned int n =  getFrameDepth(getCenter(), depth, maxDepth, PositionFrame);
    if (n > maxDepth)
    {
        return n;
    }
    else
    {
        unsigned int m = getFrameDepth(equatorObject, depth, maxDepth, OrientationFrame);
        return max(m, n);
    }
}


/*** CachingFrame ***/

CachingFrame::CachingFrame(Selection _center) :
    ReferenceFrame(_center),
    lastTime(-1.0e50),
    lastOrientation(1.0),
	lastAngularVelocity(0.0, 0.0, 0.0),
	orientationCacheValid(false),
	angularVelocityCacheValid(false)
{
}


Quatd
CachingFrame::getOrientation(double tjd) const
{
	if (tjd != lastTime)
	{
		lastTime = tjd;
		lastOrientation = computeOrientation(tjd);
		orientationCacheValid = true;
		angularVelocityCacheValid = false;
	}
	else if (!orientationCacheValid)
	{
		lastOrientation = computeOrientation(tjd);
		orientationCacheValid = true;
	}

	return lastOrientation;
}


Vec3d CachingFrame::getAngularVelocity(double tjd) const
{
	if (tjd != lastTime)
	{
		lastTime = tjd;
		lastAngularVelocity = computeAngularVelocity(tjd);
		orientationCacheValid = false;
		angularVelocityCacheValid = true;
	}
	else if (!angularVelocityCacheValid)
	{
		lastAngularVelocity = computeAngularVelocity(tjd);
		angularVelocityCacheValid = true;
	}

	return lastAngularVelocity;
}


/*! Calculate the angular velocity at the specified time (units are
 *  radians / Julian day.) The default implementation just
 *  differentiates the orientation.
 */
Vec3d CachingFrame::computeAngularVelocity(double tjd) const
{
	Quatd q0 = getOrientation(tjd);

	// Call computeOrientation() instead of getOrientation() so that we
	// don't affect the cached value. 
	// TODO: check the valid ranges of the frame to make sure that
	// jd+dt is still in range.
	Quatd q1 = computeOrientation(tjd + ANGULAR_VELOCITY_DIFF_DELTA);

	Quatd dq = ~q0 * q1;

	if (fabs(dq.w) > 0.99999999)
		return Vec3d(0.0, 0.0, 0.0);

	Vec3d v(dq.x, dq.y, dq.z);
	v.normalize();
	return v * (2.0 * acos(dq.w) / ANGULAR_VELOCITY_DIFF_DELTA);
}



/*** TwoVectorFrame ***/

// Minimum angle permitted between primary and secondary axes of
// a two-vector frame.
const double TwoVectorFrame::Tolerance = 1.0e-6;

TwoVectorFrame::TwoVectorFrame(Selection center,
                               const FrameVector& prim,
                               int primAxis,
                               const FrameVector& sec,
                               int secAxis) :
    CachingFrame(center),
    primaryVector(prim),
    primaryAxis(primAxis),
    secondaryVector(sec),
    secondaryAxis(secAxis)
{
    // Verify that primary and secondary axes are valid
    assert(primaryAxis != 0 && secondaryAxis != 0);
    assert(abs(primaryAxis) <= 3 && abs(secondaryAxis) <= 3);
    // Verify that the primary and secondary axes aren't collinear
    assert(abs(primaryAxis) != abs(secondaryAxis));

    if ((abs(primaryAxis) != 1 && abs(secondaryAxis) != 1))
    {
        tertiaryAxis = 1;
    }
    else if (abs(primaryAxis) != 2 && abs(secondaryAxis) != 2)
    {
        tertiaryAxis = 2;
    }
    else
    {
        tertiaryAxis = 3;
    }
}


Quatd
TwoVectorFrame::computeOrientation(double tjd) const
{
    Vec3d v0 = primaryVector.direction(tjd);
    Vec3d v1 = secondaryVector.direction(tjd);

    // TODO: verify that v0 and v1 aren't zero length
    v0.normalize();
    v1.normalize();

    if (primaryAxis < 0)
        v0 = -v0;
    if (secondaryAxis < 0)
        v1 = -v1;

    Vec3d v2 = v0 ^ v1;

    // Check for degenerate case when the primary and secondary vectors
    // are collinear. A well-chosen two vector frame should never have this
    // problem.
    double length = v2.length();
    if (length < Tolerance)
    {
        // Just return identity . . .
        return Quatd(1.0);
    }
    else
    {
        v2 = v2 / length;

        // Determine whether the primary and secondary axes are in
        // right hand order.
        int rhAxis = abs(primaryAxis) + 1;
        if (rhAxis > 3)
            rhAxis = 1;
        bool rhOrder = rhAxis == abs(secondaryAxis);

        // Set the rotation matrix axes
        Vec3d v[3];
        v[abs(primaryAxis) - 1] = v0;

        // Reverse the cross products if the axes are not in right
        // hand order.
        if (rhOrder)
        {
            v[abs(secondaryAxis) - 1] = v2 ^ v0;
            v[abs(tertiaryAxis) - 1] = v2;
        }
        else
        {
            v[abs(secondaryAxis) - 1] = v0 ^ -v2;
            v[abs(tertiaryAxis) - 1] = -v2;
        }

        // The axes are the rows of a rotation matrix. The getOrientation
        // method must return the quaternion representation of the 
        // orientation, so convert the rotation matrix to a quaternion now.
        Quatd q = Quatd::matrixToQuaternion(Mat3d(v[0], v[1], v[2]));

        // A rotation matrix will have a determinant of 1; if the matrix also
        // includes a reflection, the determinant will be -1, indicating that
        // there's a bug and there's a reversed cross-product or sign error
        // somewhere.
        // assert(Mat3d(v[0], v[1], v[2]).determinant() > 0);

        return q;
    }
}


bool
TwoVectorFrame::isInertial() const
{
	// Although it's possible to specify an inertial two-vector frame, we won't
	// bother trying to distinguish these cases: all two-vector frames will be
	// treated as non-inertial.
	return true;
}


unsigned int
TwoVectorFrame::nestingDepth(unsigned int depth,
                             unsigned int maxDepth,
                             FrameType) const
{
    // Check nesting of the origin object as well as frames references by
    // the primary and secondary axes.
    unsigned int n = getFrameDepth(getCenter(), depth, maxDepth, PositionFrame);
    if (n > maxDepth)
        return n;

    unsigned int m = primaryVector.nestingDepth(depth, maxDepth);
    n = max(m, n);
    if (n > maxDepth)
        return n;

    m = secondaryVector.nestingDepth(depth, maxDepth);
    return max(m, n);
}



// Copy constructor
FrameVector::FrameVector(const FrameVector& fv) :
    vecType(fv.vecType),
    observer(fv.observer),
    target(fv.target),
    vec(fv.vec),
    frame(fv.frame)
{
    if (frame != NULL)
        frame->addRef();
}


// Assignment operator (since we have a copy constructor)
FrameVector&
FrameVector::operator=(const FrameVector& fv)
{
    vecType = fv.vecType;
    observer = fv.observer;
    target = fv.target;
    vec = fv.vec;

    if (frame != NULL)
        frame->release();
    frame = fv.frame;
    if (frame != NULL)
        frame->addRef();
    
    return *this;
}



FrameVector::FrameVector(FrameVectorType t) :
    vecType(t),
    observer(),
    target(),
    vec(0.0, 0.0, 0.0),
    frame(NULL)
{
}


FrameVector::~FrameVector()
{
    if (frame != NULL)
        frame->release();
}


FrameVector
FrameVector::createRelativePositionVector(const Selection& _observer,
                                          const Selection& _target)
{
    FrameVector fv(RelativePosition);
    fv.observer = _observer;
    fv.target = _target;

    return fv;
}


FrameVector
FrameVector::createRelativeVelocityVector(const Selection& _observer,
                                          const Selection& _target)
{
    FrameVector fv(RelativeVelocity);
    fv.observer = _observer;
    fv.target = _target;

    return fv;
}


FrameVector
FrameVector::createConstantVector(const Vec3d& _vec,
                                  const ReferenceFrame* _frame)
{
    FrameVector fv(ConstantVector);
    fv.vec = _vec;
    fv.frame = _frame;
    if (fv.frame != NULL)
        fv.frame->addRef();
    return fv;
}


Vec3d
FrameVector::direction(double tjd) const
{
    Vector3d v;

    switch (vecType)
    {
    case RelativePosition:
        v = target.getPosition(tjd).offsetFromKm(observer.getPosition(tjd));
        break;

    case RelativeVelocity:
        {
            Vector3d v0 = observer.getVelocity(tjd);
            Vector3d v1 = target.getVelocity(tjd);
            v = v1 - v0;
        }
        break;

    case ConstantVector:
        if (frame == NULL)
            v = toEigen(vec);
        else
            v = toEigen(vec * frame->getOrientation(tjd).toMatrix3());
        break;

    default:
        // unhandled vector type
        v = Vector3d::Zero();
        break;
    }

    return fromEigen(v);
}


unsigned int
FrameVector::nestingDepth(unsigned int depth,
                          unsigned int maxDepth) const
{
    switch (vecType)
    {
    case RelativePosition:
    case RelativeVelocity:
        {
            unsigned int n = getFrameDepth(observer, depth, maxDepth, ReferenceFrame::PositionFrame);
            if (n > maxDepth)
            {
                return n;
            }
            else
            {
                unsigned int m = getFrameDepth(target, depth, maxDepth, ReferenceFrame::PositionFrame);
                return max(m, n);
            }
        }
        break;

    case ConstantVector:
        if (depth > maxDepth)
            return depth;
        else
            return frame->nestingDepth(depth + 1, maxDepth, ReferenceFrame::OrientationFrame);
        break;

    default:
        return depth;
    }
}
