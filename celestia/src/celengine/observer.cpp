// observer.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/solve.h>
#include "observer.h"
#include "simulation.h"

using namespace std;


#define LY 9466411842000.000
#define VELOCITY_CHANGE_TIME      0.25f



Observer::Observer(Simulation* _sim) :
    sim(_sim),
    velocity(0.0, 0.0, 0.0),
    angularVelocity(0.0f, 0.0f, 0.0f),
    realTime(0.0),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    trackingOrientation(1.0f, 0.0f, 0.0f, 0.0f)
{
}


double Observer::getSimTime() const
{
    return sim->getTime();
};


UniversalCoord Observer::getPosition() const
{
    // TODO: Optimize this!  Dirty bit should be set by Simulation::setTime and
    // by Observer::update(), Observer::setOrientation(), Observer::setPosition()
    return frame.toUniversal(situation, getSimTime()).translation;
}


Point3d Observer::getRelativePosition(const Point3d& p) const
{
    BigFix x(p.x);
    BigFix y(p.y);
    BigFix z(p.z);

    UniversalCoord position = getPosition();
    double dx = (double) (position.x - x);
    double dy = (double) (position.y - y);
    double dz = (double) (position.z - z);

    return Point3d(dx / LY, dy / LY, dz / LY);
}


Quatf Observer::getOrientation() const
{
    Quatd q = frame.toUniversal(situation, getSimTime()).rotation;
    return Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z);
}


void Observer::setOrientation(const Quatf& q)
{
    RigidTransform rt = frame.toUniversal(situation, getSimTime());
    rt.rotation = Quatd(q.w, q.x, q.y, q.z);
    situation = frame.fromUniversal(rt, getSimTime());
}


void Observer::setOrientation(const Quatd& q)
{
    setOrientation(Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z));
}


Vec3d Observer::getVelocity() const
{
    return velocity;
}


void Observer::setVelocity(const Vec3d& v)
{
    velocity = v;
}


Vec3f Observer::getAngularVelocity() const
{
    return angularVelocity;
}


void Observer::setAngularVelocity(const Vec3f& v)
{
    angularVelocity = v;
}


void Observer::setPosition(const Point3d& p)
{
    setPosition(UniversalCoord(p));
}


void Observer::setPosition(const UniversalCoord& p)
{
    RigidTransform rt = frame.toUniversal(situation, getSimTime());
    rt.translation = p;
    situation = frame.fromUniversal(rt, getSimTime());
}


RigidTransform Observer::getSituation() const
{
    return frame.toUniversal(situation, getSimTime());
}


void Observer::setSituation(const RigidTransform& xform)
{
    situation = frame.fromUniversal(xform, getSimTime());
}


Vec3d toUniversal(const Vec3d& v,
                  const Observer& observer,
                  const Selection& sel,
                  double t,
                  astro::CoordinateSystem frame)
{
    switch (frame)
    {
    case astro::ObserverLocal:
        {
            Quatf q = observer.getOrientation();
            Quatd qd(q.w, q.x, q.y, q.z);
            return v * qd.toMatrix3();
        }
        
    case astro::Geographic:
        if (sel.body == NULL)
            return v;
        else
            return v * sel.body->getGeographicToHeliocentric(t);
        
    case astro::Equatorial:
        if (sel.body == NULL)
            return v;
        else
            return v * sel.body->getLocalToHeliocentric(t);

    case astro::Ecliptical:
        // TODO: Multiply this by the planetary system's ecliptic orientation,
        // once this field is added.
        return v;

    case astro::Universal:
        return v;

    default:
        // assert(0);
        return v;
    }
}


static Quatf lookAt(Point3f from, Point3f to, Vec3f up)
{
    Vec3f n = to - from;
    n.normalize();
    Vec3f v = n ^ up;
    v.normalize();
    Vec3f u = v ^ n;
    return Quatf(Mat3f(v, u, -n));
}


double Observer::getArrivalTime() const
{
    if (observerMode != Travelling)
        return realTime;
    else
        return journey.startTime + journey.duration;
}


// Tick the simulation by dt seconds
void Observer::update(double dt)
{
    realTime += dt;
    printf("time: %lf\n", realTime);

    if (observerMode == Travelling)
    {
        float t = clamp((realTime - journey.startTime) / journey.duration);

        Vec3d jv = journey.to - journey.from;
        UniversalCoord p;

        // Another interpolation method . . . accelerate exponentially,
        // maintain a constant velocity for a period of time, then
        // decelerate.  The portion of the trip spent accelerating is
        // controlled by the parameter journey.accelTime; a value of 1 means
        // that the entire first half of the trip will be spent accelerating
        // and there will be no coasting at constant velocity.
        {
            double u = t < 0.5 ? t * 2 : (1 - t) * 2;
            double x;
            if (u < journey.accelTime)
            {
                x = exp(journey.expFactor * u) - 1.0;
            }
            else
            {
                x = exp(journey.expFactor * journey.accelTime) *
                    (journey.expFactor * (u - journey.accelTime) + 1) - 1;
            }

            Vec3d v = jv;
            if (v.length() == 0.0)
            {
                p = journey.from;
            }
            else
            {
                v.normalize();
                if (t < 0.5)
                    p = journey.from + v * astro::kilometersToMicroLightYears(x);
                else
                    p = journey.to - v * astro::kilometersToMicroLightYears(x);
            }
        }

        // Spherically interpolate the orientation over the first half
        // of the journey.
        Quatf orientation;
        if (t < 0.5f)
        {
            // Smooth out the interpolation to avoid jarring changes in
            // orientation
            double v = sin(t * PI);

            // Be careful to choose the shortest path when interpolating
            if (norm(journey.initialOrientation - journey.finalOrientation) <
                norm(journey.initialOrientation + journey.finalOrientation))
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           journey.finalOrientation, v);
            }
            else
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           -journey.finalOrientation, v);
            }
        }
        else
        {
            orientation = journey.finalOrientation;
        }

        situation = RigidTransform(p, orientation);

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            situation = RigidTransform(journey.to, journey.finalOrientation);
            observerMode = Free;
            setVelocity(Vec3d(0, 0, 0));
//            targetVelocity = Vec3d(0, 0, 0);
        }
    }

    if (getVelocity() != targetVelocity)
    {
        double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
        setVelocity(getVelocity() * (1.0 - t) + targetVelocity * t);
    }

    // Update the position
    situation.translation = situation.translation + getVelocity() * dt;

    if (observerMode == Free)
    {
        // Update the observer's orientation
        Vec3f fAV = getAngularVelocity();
        Vec3d AV(fAV.x, fAV.y, fAV.z);
        Quatd dr = 0.5 * (AV * situation.rotation);
        situation.rotation += dt * dr;
        situation.rotation.normalize();
    }

    if (!trackObject.empty())
    {
        Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix3();
        Vec3d vn = trackObject.getPosition(getSimTime()) - getPosition();
        Point3f to((float) vn.x, (float) vn.y, (float) vn.z);
        setOrientation(lookAt(Point3f(0, 0, 0), to, up));
    }
}


Selection Observer::getTrackedObject() const
{
    return trackObject;
}


void Observer::setTrackedObject(const Selection& sel)
{
    trackObject = sel;
}


void Observer::reverseOrientation()
{
    Quatf q = getOrientation();
    q.yrotate((float) PI);
    setOrientation(q);
}



struct TravelExpFunc : public unary_function<double, double>
{
    double dist, s;

    TravelExpFunc(double d, double _s) : dist(d), s(_s) {};

    double operator()(double x) const
    {
        // return (1.0 / x) * (exp(x / 2.0) - 1.0) - 0.5 - dist / 2.0;
        return exp(x * s) * (x * (1 - s) + 1) - 1 - dist;
    }
};


void Observer::computeGotoParameters(const Selection& destination,
                                     JourneyParams& jparams,
                                     double gotoTime,
                                     Vec3d offset,
                                     astro::CoordinateSystem offsetFrame,
                                     Vec3f up,
                                     astro::CoordinateSystem upFrame)
{
    UniversalCoord targetPosition = destination.getPosition(getSimTime());
    Vec3d v = targetPosition - getPosition();
    v.normalize();

    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = getPosition();

    // The destination position lies along the line between the current
    // position and the star
    offset = toUniversal(offset, *this, destination, getSimTime(), offsetFrame);
    jparams.to = targetPosition + offset;

    Vec3d upd(up.x, up.y, up.z);
    upd = toUniversal(upd, *this, destination, getSimTime(), upFrame);
    Vec3f upf = Vec3f((float) upd.x, (float) upd.y, (float) upd.z);

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, upf);

    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;

    setFrame(FrameOfReference(frame.coordSys, destination));

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getSimTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);
    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getSimTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


void Observer::computeCenterParameters(const Selection& destination,
                                       JourneyParams& jparams,
                                       double centerTime)
{
    UniversalCoord targetPosition = destination.getPosition(getSimTime());

    jparams.duration = centerTime;
    jparams.startTime = realTime;

    // Don't move through space, just rotate the camera
    jparams.from = getPosition();
    jparams.to = jparams.from;

    Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix4();

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, up);

    jparams.accelTime = 0.5;
    jparams.expFactor = 0;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getSimTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);

    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getSimTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


Observer::ObserverMode Observer::getMode() const
{
    return observerMode;
}

void Observer::setMode(Observer::ObserverMode mode)
{
    observerMode = mode;
}


void Observer::setFrame(const FrameOfReference& _frame)
{
    RigidTransform transform = frame.toUniversal(situation, getSimTime());
    frame = _frame;
    situation = frame.fromUniversal(transform, getSimTime());
}


FrameOfReference Observer::getFrame() const
{
    return frame;
}


// Rotate the observer about its center.
void Observer::rotate(Quatf q)
{
    Quatd qd(q.w, q.x, q.y, q.z);
    situation.rotation = qd * situation.rotation;
}


// Orbit around the reference object (if there is one.)  This involves changing
// both the observer's position and orientation.
void Observer::orbit(const Selection& selection, Quatf q)
{
    Selection center = frame.refObject;
    if (center.empty() && !selection.empty())
    {
        center = selection;
        setFrame(FrameOfReference(frame.coordSys, center));
    }

    if (!center.empty())
    {
        // Get the focus position (center of rotation) in frame
        // coordinates; in order to make this function work in all
        // frames of reference, it's important to work in frame
        // coordinates.
        UniversalCoord focusPosition = center.getPosition(getSimTime());
        focusPosition = frame.fromUniversal(RigidTransform(focusPosition), getSimTime()).translation;

        // v = the vector from the observer's position to the focus
        Vec3d v = situation.translation - focusPosition;

        // Get a double precision version of the rotation
        Quatd qd(q.w, q.x, q.y, q.z);

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quatd qd2 = ~situation.rotation * qd * situation.rotation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.length();
        v = v * qd2.toMatrix3();
        v.normalize();
        v *= distance;

        situation.rotation = situation.rotation * qd2;
        situation.translation = focusPosition + v;
    }
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Observer::changeOrbitDistance(const Selection& selection, float d)
{
    Selection center = frame.refObject;
    if (center.empty() && !selection.empty())
    {
        center = selection;
        setFrame(FrameOfReference(frame.coordSys, center));
    }

    if (!center.empty())
    {
        UniversalCoord focusPosition = center.getPosition(getSimTime());
        
        double size = center.radius();

        // Somewhat arbitrary parameters to chosen to give the camera movement
        // a nice feel.  They should probably be function parameters.
        double minOrbitDistance = astro::kilometersToMicroLightYears(size);
        double naturalOrbitDistance = astro::kilometersToMicroLightYears(4.0 * size);

        // Determine distance and direction to the selected object
        Vec3d v = getPosition() - focusPosition;
        double currentDistance = v.length();

        // TODO: This is sketchy . . .
        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);
            RigidTransform framePos = frame.fromUniversal(RigidTransform(focusPosition + v),
                                                          getSimTime());
            situation.translation = framePos.translation;
        }
    }
}


void Observer::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3f v;

    if (trackObject.empty())
    {
        trackingOrientation = getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = Vec3f(0, 0, -s) * getOrientation().toMatrix4();
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = Vec3f(0, 0, -s) * trackingOrientation.toMatrix4();
    }

    targetVelocity = Vec3d(v.x, v.y, v.z);
    initialVelocity = getVelocity();
    beginAccelTime = realTime;
}


float Observer::getTargetSpeed()
{
    return targetSpeed;
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime, 
                             Vec3f up,
                             astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getSimTime());
        Vec3d v = pos - getPosition();
        double distance = v.length();

        double maxOrbitDistance;
        if (selection.body != NULL)
            maxOrbitDistance = astro::kilometersToMicroLightYears(5.0f * selection.body->getRadius());
        else if (selection.deepsky != NULL)
            maxOrbitDistance = 5.0f * selection.deepsky->getRadius() * 1e6f;
        else if (selection.star != NULL)
            maxOrbitDistance = astro::kilometersToMicroLightYears(100.0f * selection.star->getRadius());
        else
            maxOrbitDistance = 0.5f;

        double radius = selection.radius();
        double minOrbitDistance = astro::kilometersToMicroLightYears(1.01 * radius);

        double orbitDistance = (distance > maxOrbitDistance * 10.0f) ? maxOrbitDistance : distance * 0.1f;
        if (orbitDistance < minOrbitDistance)
            orbitDistance = minOrbitDistance;

        computeGotoParameters(selection, journey, gotoTime,
                              v * -(orbitDistance / distance), astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             double distance,
                             Vec3f up,
                             astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getSimTime());
        Vec3d v = pos - getPosition();
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime,
                              v * -distance * 1e6, astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


void Observer::gotoSelectionLongLat(const Selection& selection,
                                    double gotoTime,
                                    double distance,
                                    float longitude,
                                    float latitude,
                                    Vec3f up)
{
    if (!selection.empty())
    {
        double phi = -latitude + PI / 2;
        double theta = longitude - PI;
        double x = cos(theta) * sin(phi);
        double y = cos(phi);
        double z = -sin(theta) * sin(phi);
        computeGotoParameters(selection, journey, gotoTime,
                              Vec3d(x, y, z) * distance * 1e6, astro::Geographic,
                              up, astro::Geographic);
        observerMode = Travelling;
    }
}


void Observer::gotoLocation(const RigidTransform& transform,
                            double duration)
{
    journey.startTime = realTime;
    journey.duration = duration;
    
    RigidTransform from(getPosition(), getOrientation());
    from = frame.fromUniversal(from, getSimTime());
    journey.from = from.translation;
    journey.initialOrientation= Quatf((float) from.rotation.w, (float) from.rotation.x,
                                      (float) from.rotation.y, (float) from.rotation.z);
    
    journey.to = transform.translation;
    journey.finalOrientation = Quatf((float) transform.rotation.w,
                                     (float) transform.rotation.x,
                                     (float) transform.rotation.y,
                                     (float) transform.rotation.z);

    journey.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(journey.from.distanceTo(journey.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    journey.expFactor = sol.first;

    observerMode = Travelling;
}


void Observer::getSelectionLongLat(const Selection& selection,
                                   double& distance,
                                   double& longitude,
                                   double& latitude)
{
    // Compute distance (km) and lat/long (degrees) of observer with
    // respect to currently selected object.
    if (!selection.empty())
    {
        FrameOfReference refFrame(astro::Geographic, selection.body);
        RigidTransform xform = refFrame.fromUniversal(RigidTransform(getPosition(), getOrientation()),
                                                      getSimTime());

        Point3d pos = (Point3d) xform.translation;

        distance = pos.distanceFromOrigin();
        longitude = -radToDeg(atan2(-pos.z, -pos.x));
        latitude = radToDeg(PI/2 - acos(pos.y / distance));

        // Convert distance from light years to kilometers.
        distance = astro::microLightYearsToKilometers(distance);
    }
}


void Observer::gotoSurface(const Selection& sel, double duration)
{
    Vec3d vd = getPosition() - sel.getPosition(getSimTime());
    Vec3f vf((float) vd.x, (float) vd.y, (float) vd.z);
    vf.normalize();
    Vec3f viewDir = Vec3f(0, 0, -1) * getOrientation().toMatrix3();
    Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix3();
    Quatf q = getOrientation();
    if (vf * viewDir < 0.0f)
    {
        q = lookAt(Point3f(0, 0, 0), Point3f(0.0f, 0.0f, 0.0f) + up, vf);
    }
    else
    {
    }
    
    FrameOfReference frame(astro::Geographic, sel);
    RigidTransform rt = frame.fromUniversal(RigidTransform(getPosition(), q),
                                            getSimTime());

    double height = 1.0001 * astro::kilometersToMicroLightYears(sel.radius());
    Vec3d dir = rt.translation - Point3d(0.0, 0.0, 0.0);
    dir.normalize();
    dir *= height;

    rt.translation = UniversalCoord(dir.x, dir.y, dir.z);
    gotoLocation(rt, duration);
};


void Observer::cancelMotion()
{
    observerMode = Free;
}


void Observer::centerSelection(const Selection& selection, double centerTime)
{
    if (!selection.empty())
    {
        computeCenterParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}


void Observer::follow(const Selection& selection)
{
    if (!selection.empty())
    {
        setFrame(FrameOfReference(astro::Ecliptical, selection));
    }
}


void Observer::geosynchronousFollow(const Selection& selection)
{
    if (selection.body != NULL)
    {
        setFrame(FrameOfReference(astro::Geographic, selection.body));
    }
}

void Observer::phaseLock(const Selection& selection)
{
    if (frame.refObject.body != NULL)
    {
        if (selection == frame.refObject)
        {
            setFrame(FrameOfReference(astro::PhaseLock, selection,
                                      Selection(selection.body->getSystem()->getStar())));
        }
        else
        {
            setFrame(FrameOfReference(astro::PhaseLock, frame.refObject, selection));
        }
    }
}

void Observer::chase(const Selection& selection)
{
    if (selection.body != NULL)
    {
        setFrame(FrameOfReference(astro::Chase, selection.body));
    }
}


