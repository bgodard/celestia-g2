// simulation.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// The core of Celestia--tracks an observer moving through a
// stars and their solar systems.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>

#include "mathlib.h"
#include "vecmath.h"
#include "perlin.h"
#include "astro.h"
#include "simulation.h"

using namespace std;

#define VELOCITY_CHANGE_TIME      1.0f


Simulation::Simulation() :
    realTime(0.0),
    simTime(0.0),
    timeScale(1.0),
    stardb(NULL),
    solarSystemCatalog(NULL),
    galaxies(NULL),
    closestSolarSystem(NULL),
    selection(),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    faintestVisible(5.0f),
    hudDetail(1)
{
}

Simulation::~Simulation()
{
    // TODO: Clean up!
}


void Simulation::render(Renderer& renderer)
{
    renderer.render(observer,
                    *stardb,
                    faintestVisible,
                    closestSolarSystem,
                    galaxies,
                    selection,
                    simTime);
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


SolarSystem* Simulation::getSolarSystem(Star* star)
{
    if (star == NULL)
        return NULL;

    uint32 starNum = star->getCatalogNumber();
    SolarSystemCatalog::iterator iter = solarSystemCatalog->find(starNum);
    if (iter == solarSystemCatalog->end())
        return NULL;
    else
        return iter->second;
}


Star* Simulation::getSun(Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == NULL)
        return NULL;
    else
        return stardb->find(system->getStarNumber());
}


UniversalCoord Simulation::getSelectionPosition(Selection& sel, double when)
{
    if (sel.body != NULL)
    {
        Point3f sunPos(0.0f, 0.0f, 0.0f);
        Star* sun = getSun(sel.body);
        if (sun != NULL)
            sunPos = sun->getPosition();
        return astro::universalPosition(sel.body->getHeliocentricPosition(when),
                                        sunPos);
    }
    else if (sel.star != NULL)
    {
        return astro::universalPosition(Point3d(0.0, 0.0, 0.0), sel.star->getPosition());
    }
    else if (sel.galaxy != NULL)
    {
        Point3d p = sel.galaxy->getPosition();
        return astro::universalPosition(Point3d(0.0, 0.0, 0.0),
                                        Point3f((float) p.x, (float) p.y, (float) p.z));
    }
    else
    {
        return UniversalCoord(Point3d(0.0, 0.0, 0.0));
    }
}


float getSelectionSize(Selection& sel)
{
    if (sel.body != NULL)
        return sel.body->getRadius();
    else if (sel.star != NULL)
        return sel.star->getRadius();
    else if (sel.galaxy != NULL)
        return astro::lightYearsToKilometers(sel.galaxy->getRadius());
    else
        return 0.0f;
}


StarDatabase* Simulation::getStarDatabase() const
{
    return stardb;
}

SolarSystemCatalog* Simulation::getSolarSystemCatalog() const
{
    return solarSystemCatalog;
}

void Simulation::setStarDatabase(StarDatabase* db,
                                 SolarSystemCatalog* catalog,
                                 GalaxyList* galaxyList)
{
    stardb = db;
    solarSystemCatalog = catalog;
    galaxies = galaxyList;
}


// Get the time (Julian date)
double Simulation::getTime() const
{
    return simTime;
}

// Set the time to the specified Julian date
void Simulation::setTime(double jd)
{
    simTime = jd;
}


class ClosestStarFinder : public StarHandler
{
public:
    ClosestStarFinder(float _maxDistance);
    ~ClosestStarFinder() {};
    void process(const Star& star, float distance, float appMag);

public:
    float maxDistance;
    float closestDistance;
    Star* closestStar;
};

ClosestStarFinder::ClosestStarFinder(float _maxDistance) :
    maxDistance(_maxDistance), closestDistance(_maxDistance), closestStar(NULL)
{
}

void ClosestStarFinder::process(const Star& star, float distance, float appMag)
{
    if (distance < closestDistance)
        closestStar = const_cast<Star*>(&star);
}


// Tick the simulation by dt seconds
void Simulation::update(double dt)
{
    realTime += dt;
    simTime += (dt / 86400.0) * timeScale;

    if (observerMode == Travelling)
    {
        float t = clamp((realTime - journey.startTime) / journey.duration);

        // Smooth out the linear interpolation of position
        float u = (float) sin(sin(t * PI / 2) * PI / 2);
        Vec3d jv = journey.to - journey.from;
        UniversalCoord p = journey.from + jv * (double) u;

        // Interpolate the orientation using lookAt()
        // We gradually change the focus point for lookAt() from the initial 
        // focus to the destination star over the first half of the journey
        Vec3d lookDir0 = journey.initialFocus - journey.from;
        Vec3d lookDir1 = journey.finalFocus - journey.to;
        lookDir0.normalize();
        lookDir1.normalize();
        Vec3d lookDir;
        Quatf orientation;
        if (t < 0.5f)
        {
            // Smooth out the interpolation of the focus point to avoid
            // jarring changes in orientation
            double v = sin(t * PI);

            double c = lookDir0 * lookDir1;
            double angle = acos(c);
            if (c >= 1.0 || angle < 1.0e-6)
            {
                lookDir = lookDir0;
                orientation = journey.initialOrientation;
            }
            else
            {
                double s = sin(angle);
                double is = 1.0 / s;

                // Spherically interpolate the look direction between the
                // intial and final directions.
                lookDir = lookDir0 * (sin((1 - v) * angle) * is) +
                          lookDir1 * (sin(v * angle) * is);
            }
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
            lookDir = lookDir1;
            orientation = journey.finalOrientation;
        }
#if 0
        observer.setOrientation(lookAt(Point3f(0, 0, 0),
                                       Point3f((float) lookDir.x,
                                               (float) lookDir.y,
                                               (float) lookDir.z),
                                       journey.up));
#endif
        observer.setOrientation(orientation);
        observer.setPosition(p);

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            observer.setPosition(journey.to);
            observerMode = Free;
            observer.setVelocity(Vec3d(0, 0, 0));
            targetVelocity = Vec3d(0, 0, 0);
        }
    }
    else if (observerMode == Following)
    {
        Point3d posRelToSun = followInfo.body->getHeliocentricPosition(simTime) + followInfo.offset;
        observer.setPosition(astro::universalPosition(posRelToSun,
                                                      followInfo.sun->getPosition()));
    }
    else if (observerMode == GeosynchronousFollowing)
    {
        Point3d posRelToSun = followInfo.body->getHeliocentricPosition(simTime) + followInfo.offset * followInfo.body->getEclipticalToGeographic(simTime).toMatrix4();
        observer.setPosition(astro::universalPosition(posRelToSun,
                                                      followInfo.sun->getPosition()));
        Quatd q = followInfo.offsetR * followInfo.body->getEclipticalToGeographic(simTime);
        observer.setOrientation(Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z));
    }
    else
    {
        if (observer.getVelocity() != targetVelocity)
        {
            double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
            observer.setVelocity(observer.getVelocity() * (1.0 - t) +
                                 targetVelocity * t);
        }

        observer.update(dt);
    }

    // Find the closest solar system
    Point3f observerPos = (Point3f) observer.getPosition();
    ClosestStarFinder closestFinder(1.0f);
    stardb->findCloseStars(closestFinder, observerPos, 1.0f);
    closestSolarSystem = getSolarSystem(closestFinder.closestStar);
}


Selection Simulation::getSelection() const
{
    return selection;
}


void Simulation::setSelection(const Selection& sel)
{
    selection = sel;
}


struct PlanetPickInfo
{
    float cosClosestAngle;
    double closestDistance;
    Body* closestBody;
    Vec3d direction;
    Point3d origin;
    double jd;
};

bool ApproxPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->origin;
    bodyDir.normalize();
    double cosAngle = bodyDir * pickInfo->direction;
    if (cosAngle > pickInfo->cosClosestAngle)
    {
        pickInfo->cosClosestAngle = cosAngle;
        pickInfo->closestBody = body;
    }

    return true;
}


// Perform an intersection test between the pick ray and a body
bool ExactPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->origin;

    // This intersection test naively assumes that the body is spherical.
    double v = bodyDir * pickInfo->direction;
    double disc = square(body->getRadius()) - ((bodyDir * bodyDir) - square(v));

    if (disc > 0.0)
    {
        double distance = v - sqrt(disc);
        if (distance > 0.0 && distance < pickInfo->closestDistance)
        {
            pickInfo->closestDistance = distance;
            pickInfo->closestBody = body;
        }
    }

    return true;
}


Selection Simulation::pickPlanet(Observer& observer,
                                 Star& sun,
                                 SolarSystem& solarSystem,
                                 Vec3f pickRay)
{
    PlanetPickInfo pickInfo;

    // Transform the pick direction
    pickRay = pickRay * observer.getOrientation().toMatrix4();
    pickInfo.direction = Vec3d((double) pickRay.x,
                               (double) pickRay.y,
                               (double) pickRay.z);
    pickInfo.origin    = astro::heliocentricPosition(observer.getPosition(),
                                                     sun.getPosition());
    pickInfo.cosClosestAngle = -1.0f;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestBody = NULL;
    pickInfo.jd = simTime;

    // First see if there's a planet that the pick ray intersects.
    // Select the closest planet intersected.
    solarSystem.getPlanets()->traverse(ExactPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.closestBody != NULL)
    {
        return Selection(pickInfo.closestBody);
    }

    // If no planet was intersected by the pick ray, choose the planet
    // with the smallest angular separation from the pick ray.  Very distant
    // planets are likley to fail the intersection test even if the user
    // clicks on a pixel where the planet's disc has been rendered--in order
    // to make distant planets visible on the screen at all, their apparent size
    // has to be greater than their actual disc size.
    solarSystem.getPlanets()->traverse(ApproxPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.cosClosestAngle > cos(degToRad(0.5f)))
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}


/*
 * StarPicker is a callback class for StarDatabase::findVisibleStars
 */
class StarPicker : public StarHandler
{
public:
    StarPicker(const Point3f&, const Vec3f&, float);
    ~StarPicker() {};

    void process(const Star&, float, float);

public:
    const Star* pickedStar;
    Point3f pickOrigin;
    Vec3f pickRay;
    float cosAngleClosest;
};

StarPicker::StarPicker(const Point3f& _pickOrigin, const Vec3f& _pickRay,
                       float angle) :
    pickedStar(NULL),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    cosAngleClosest((float) cos(angle))
{
}

void StarPicker::process(const Star& star, float distance, float appMag)
{
    Vec3f starDir = star.getPosition() - pickOrigin;
    starDir.normalize();

    float cosAngle = starDir * pickRay;
    if (cosAngle > cosAngleClosest)
    {
        cosAngleClosest = cosAngle;
        pickedStar = &star;
    }
}


Selection Simulation::pickStar(Vec3f pickRay)
{
    float angle = degToRad(0.5f);

    // Transform the pick direction
    pickRay = pickRay * observer.getOrientation().toMatrix4();

    StarPicker picker((Point3f) observer.getPosition(), pickRay, angle);
    stardb->findVisibleStars(picker,
                             (Point3f) observer.getPosition(),
                             observer.getOrientation(),
                             angle, 1.0f,
                             faintestVisible);
    if (picker.pickedStar != NULL)
        return Selection(const_cast<Star*>(picker.pickedStar));
    else
        return Selection();
}


Selection Simulation::pickObject(Vec3f pickRay)
{
    Point3f observerPos = (Point3f) observer.getPosition();
    Selection sel;

    if (closestSolarSystem != NULL)
    {
        Star* sun = stardb->find(closestSolarSystem->getPlanets()->getStarNumber());
        if (sun != NULL)
            sel = pickPlanet(observer, *sun, *closestSolarSystem, pickRay);
    }

    if (sel.empty())
        sel = pickStar(pickRay);

    return sel;
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


void Simulation::computeGotoParameters(Selection& destination,
                                       JourneyParams& jparams,
                                       double gotoTime,
                                       Vec3d offset,
                                       astro::CoordinateSystem offsetFrame,
                                       Vec3f up,
                                       astro::CoordinateSystem upFrame)
{
    UniversalCoord targetPosition = getSelectionPosition(selection, simTime);
    Vec3d v = targetPosition - observer.getPosition();
    v.normalize();

    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = observer.getPosition();

    // The destination position lies along the line between the current
    // position and the star
    offset = toUniversal(offset, observer, selection, simTime, offsetFrame);
    jparams.to = targetPosition + offset;
    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;

    Vec3d upd(up.x, up.y, up.z);
    upd = toUniversal(upd, observer, selection, simTime, upFrame);
    jparams.up = Vec3f((float) upd.x, (float) upd.y, (float) upd.z);

    jparams.initialOrientation = observer.getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f to((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), to, jparams.up);
}


void Simulation::computeCenterParameters(Selection& destination, JourneyParams& jparams,
                                         double centerTime)
{
    UniversalCoord targetPosition = getSelectionPosition(selection, simTime);

    jparams.duration = centerTime;
    jparams.startTime = realTime;

    // Don't move through space, just rotate the camera
    jparams.from = observer.getPosition();
    jparams.to = jparams.from;

    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;
    jparams.up = Vec3f(0, 1, 0) * observer.getOrientation().toMatrix4();

    jparams.initialOrientation = observer.getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f to((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), to, jparams.up);
}

Observer& Simulation::getObserver()
{
    return observer;
}

Simulation::ObserverMode Simulation::getObserverMode() const
{
    return observerMode;
}

void Simulation::setObserverMode(Simulation::ObserverMode mode)
{
    observerMode = mode;
}

// Rotate the observer about its center.
void Simulation::rotate(Quatf q)
{
    if (observerMode == GeosynchronousFollowing)
    {
        Quatd qd(q.w, q.x, q.y, q.z);
        followInfo.offsetR = qd * followInfo.offsetR;
    }
    else
    {
        observer.setOrientation(q * observer.getOrientation());
    }
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
void Simulation::orbit(Quatf q)
{
    if (!selection.empty())
    {
        UniversalCoord focusPosition = getSelectionPosition(selection, simTime);
        Vec3d v = observer.getPosition() - focusPosition;
        double distance = v.length();

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quatf o = observer.getOrientation();
        Quatf q2 = ~o * q * o;

        // Make sure the quaternion remains a rotation
        q2.normalize();

        // TODO: Make Follow mode behave more like GeosynchronousFollow
        // mode; these are both essentially the same mode, except in different
        // coordinate systems.  In fact, Free mode is really just the trivial
        // case of Follow in the universal coordinate system.
        if (observerMode == GeosynchronousFollowing)
        {
            // Get a double precision version of the rotation
            Quatd qd(q.w, q.x, q.y, q.z);
            Quatd qd2 = ~followInfo.offsetR * qd * followInfo.offsetR;
            qd2.normalize();

            followInfo.offsetR = followInfo.offsetR * qd2;
            followInfo.offset = followInfo.offset * qd2.toMatrix3();
        }
        else
        {
            Quatd qd2(q2.w, q2.x, q2.y, q2.z);

            // Roundoff errors will accumulate and cause the distance between
            // viewer and focus to change unless we take steps to keep the
            // length of v constant.
            v = v * qd2.toMatrix3();
            v.normalize();
            v *= distance;

            observer.setPosition(focusPosition + v);
            observer.setOrientation(observer.getOrientation() * q2);
            if (observerMode == Following)
                followInfo.offset = v * astro::lightYearsToKilometers(1.0);
        }
    }
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Simulation::changeOrbitDistance(float d)
{
    if (!selection.empty())
    {
        UniversalCoord focusPosition = getSelectionPosition(selection, simTime);
        double size = getSelectionSize(selection);

        // Somewhat arbitrary parameters to chosen to give the camera movement
        // a nice feel.  They should probably be function parameters.
        double minOrbitDistance = astro::kilometersToLightYears(1.05 * size);
        double naturalOrbitDistance = astro::kilometersToLightYears(4.0 * size);

        // Determine distance and direction to the selected object
        Vec3d v = observer.getPosition() - focusPosition;
        double currentDistance = v.length();

        // TODO: This is sketchy . . .
        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);

            if (observerMode == GeosynchronousFollowing ||
                     observerMode == Following)
            {
                followInfo.offset *= (newDistance / currentDistance);
            }
            else
            {
                v = v * (newDistance / currentDistance);
                observer.setPosition(focusPosition + v);
            }
        }
    }
}


void Simulation::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3f v = Vec3f(0, 0, -s) * observer.getOrientation().toMatrix4();
    targetVelocity = Vec3d(v.x, v.y, v.z);
    initialVelocity = observer.getVelocity();
    beginAccelTime = realTime;
}

float Simulation::getTargetSpeed()
{
    return targetSpeed;
}

void Simulation::gotoSelection(double gotoTime, 
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = getSelectionPosition(selection, simTime);
        Vec3d v = pos - observer.getPosition();
        double distance = v.length();
        double maxOrbitDistance;
        if (selection.body != NULL)
            maxOrbitDistance = astro::kilometersToLightYears(5.0f * selection.body->getRadius());
        else if (selection.galaxy != NULL)
            maxOrbitDistance = 5.0f * selection.galaxy->getRadius();
        else
            maxOrbitDistance = 0.5f;
        double orbitDistance = (distance > maxOrbitDistance * 10.0f) ? maxOrbitDistance : distance * 0.1f;

        computeGotoParameters(selection, journey, gotoTime,
                              v * -(orbitDistance / distance), astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}

void Simulation::gotoSelection(double gotoTime,
                               double distance,
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = getSelectionPosition(selection, simTime);
        Vec3d v = pos - observer.getPosition();
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime,
                              v * -distance, astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}

void Simulation::gotoSelectionLongLat(double gotoTime,
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
                              Vec3d(x, y, z) * distance, astro::Geographic,
                              up, astro::Geographic);
        observerMode = Travelling;
    }
}

void Simulation::cancelMotion()
{
    observerMode = Free;
}

void Simulation::centerSelection(double centerTime)
{
    if (!selection.empty())
    {
        computeCenterParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}

void Simulation::follow()
{
    if (observerMode == Following)
    {
        observerMode = Free;
    }
    else
    {
        if (selection.body != NULL)
        {
            Star* sun = getSun(selection.body);
            if (sun != NULL)
            {
                observerMode = Following;
                followInfo.sun = sun;
                followInfo.body = selection.body;
                Point3d planetPos = selection.body->getHeliocentricPosition(simTime);
                Point3d observerPos = astro::heliocentricPosition(observer.getPosition(),
                                                                  sun->getPosition());
                followInfo.offset = observerPos - planetPos;
                Quatf o = observer.getOrientation();
                followInfo.offsetR = Quatd(o.w, o.x, o.y, o.z);
            }
        }
    }
}

void Simulation::geosynchronousFollow()
{
    if (observerMode == GeosynchronousFollowing)
    {
        observerMode = Free;
    }
    else
    {
        if (selection.body != NULL)
        {
            Star* sun = getSun(selection.body);
            if (sun != NULL)
            {
                observerMode = GeosynchronousFollowing;
                followInfo.sun = sun;
                followInfo.body = selection.body;
                Point3d planetPos = selection.body->getHeliocentricPosition(simTime);
                Point3d observerPos = astro::heliocentricPosition(observer.getPosition(),
                                                                  sun->getPosition());
                Quatf o = observer.getOrientation();
                Quatd od(o.w, o.x, o.y, o.z);
                Quatd q = followInfo.body->getEclipticalToGeographic(simTime);
                followInfo.offsetR = od * ~q;
                followInfo.offset = (observerPos - planetPos) * (~q).toMatrix4();
            }
        }
    }
}


void Simulation::selectStar(uint32 catalogNo)
{
    selection = Selection(stardb->find(catalogNo));
}


void Simulation::selectPlanet(int index)
{
    if (index < 0)
    {
        if (selection.body != NULL)
        {
            PlanetarySystem* system = selection.body->getSystem();
            if (system != NULL)
                selectStar(system->getStarNumber());
        }
    }
    else
    {
        Star* star = NULL;
        if (selection.star != NULL)
        {
            star = selection.star;
        }
        else if (selection.body != NULL)
        {
            star = getSun(selection.body);
        }

        SolarSystem* solarSystem = NULL;
        if (star != NULL)
            solarSystem = getSolarSystem(star);
        else
            solarSystem = closestSolarSystem;

        if (solarSystem != NULL && index < solarSystem->getPlanets()->getSystemSize())
            selection = Selection(solarSystem->getPlanets()->getBody(index));
    }
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star database
//   2. Search the galaxy catalog for a matching name.
//   3. Search the planets and moons in the planetary system of the currently selected
//      star
//   4. Search the planets and moons in any 'nearby' (< 0.1 ly) planetary systems
Selection Simulation::findObject(string s)
{
    Star* star = stardb->find(s);
    if (star != NULL)
        return Selection(star);

    if (galaxies != NULL)
    {
        for (GalaxyList::const_iterator iter = galaxies->begin();
             iter != galaxies->end(); iter++)
        {
            if ((*iter)->getName() == s)
                return Selection(*iter);
        }
    }
        
    {
        const PlanetarySystem* solarSystem = NULL;

        if (selection.star != NULL)
        {
            SolarSystem* sys = getSolarSystem(selection.star);
            if (sys != NULL)
                solarSystem = sys->getPlanets();
        }
        else if (selection.body != NULL)
        {
            solarSystem = selection.body->getSystem();
            while (solarSystem != NULL && solarSystem->getPrimaryBody() != NULL)
                solarSystem = solarSystem->getPrimaryBody()->getSystem();
        }
        
        if (solarSystem != NULL)
        {
            Body* body = solarSystem->find(s, true);
            if (body != NULL)
                return Selection(body);
        }

        if (closestSolarSystem != NULL)
        {
            Body* body = closestSolarSystem->getPlanets()->find(s, true);
            if (body != NULL)
                return Selection(body);
        }
    }

    return Selection();
}


// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.
Selection Simulation::findObjectFromPath(string s)
{
    string::size_type pos = s.find('/', 0);

    if (pos == string::npos)
        return findObject(s);

    string base(s, 0, pos);
    Selection sel = findObject(base);
    if (sel.empty())
        return sel;

    // Don't support paths relative to a galaxy . . . for now.
    if (sel.galaxy != NULL)
        return Selection();

    const PlanetarySystem* worlds = NULL;
    if (sel.body != NULL)
    {
        worlds = sel.body->getSatellites();
    }
    else if (sel.star != NULL)
    {
        SolarSystem* ssys = getSolarSystem(sel.star);
        if (ssys != NULL)
            worlds = ssys->getPlanets();
    }

    while (worlds != NULL)
    {
        string::size_type nextPos = s.find('/', pos + 1);
        string::size_type len;
        if (nextPos == string::npos)
            len = s.size() - pos - 1;
        else
            len = nextPos - pos - 1;

        string name = string(s, pos + 1, len);
        
        Body* body = worlds->find(name);
        if (body == NULL)
        {
            return Selection();
        }
        else if (nextPos == string::npos)
        {
            return Selection(body);
        }
        else
        {
            worlds = body->getSatellites();
            pos = nextPos;
        }
    }

    return Selection();
}


double Simulation::getTimeScale()
{
    return timeScale;
}

void Simulation::setTimeScale(double _timeScale)
{
    timeScale = _timeScale;
}


float Simulation::getFaintestVisible() const
{
    return faintestVisible;
}

void Simulation::setFaintestVisible(float magnitude)
{
    faintestVisible = magnitude;
}


int Simulation::getHUDDetail() const
{
    return hudDetail;
}

void Simulation::setHUDDetail(int detail)
{
    hudDetail = detail;
}


SolarSystem* Simulation::getNearestSolarSystem() const
{
    return closestSolarSystem;
}
