// body.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
// Missing from g++ . . . why???
// #include <limits>
#include <cassert>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/utf8.h>
#include "mesh.h"
#include "meshmanager.h"
#include "body.h"

using namespace std;


Body::Body(PlanetarySystem* _system) :
    orbit(NULL),
    orbitRefPlane(astro::BodyEquator),
    radius(10000.0f),
    mass(0.0f),
    oblateness(0),
    albedo(0.5),
    orientation(1.0f),
    // Ugh.  Numeric_limits class is missing from g++
    // protos(-numeric_limits<double>::infinity()),
    // eschatos(numeric_limits<double>::infinity()),
    // Do it the ugly way instead:
    protos(-1.0e+50),
    eschatos(1.0e+50),
    model(InvalidResource),
    surface(Color(1.0f, 1.0f, 1.0f)),
    atmosphere(NULL),
    rings(NULL),
    satellites(NULL),
    classification(Unknown),
    altSurfaces(NULL),
    locations(NULL),
    locationsComputed(false)
{
    system = _system;
}


Body::~Body()
{
    // clean up orbit, atmosphere, etc.
    if (system != NULL)
        system->removeBody(this);
}


PlanetarySystem* Body::getSystem() const
{
    return system;
}


string Body::getName() const
{
    return name;
}


void Body::setName(const string _name)
{
    name = _name;
}


Orbit* Body::getOrbit() const
{
    return orbit;
}


void Body::setOrbit(Orbit* _orbit)
{
    if (orbit == NULL)
        delete orbit;
    orbit = _orbit;
}


astro::ReferencePlane Body::getOrbitReferencePlane() const
{
    return orbitRefPlane;
}


void Body::setOrbitReferencePlane(astro::ReferencePlane refPlane)
{
    orbitRefPlane = refPlane;
}


float Body::getRadius() const
{
    return radius;
}


void Body::setRadius(float _radius)
{
    radius = _radius;
}


// For an irregular object, the radius is defined to be the largest semi-axis
// of the axis-aligned bounding box.  The radius of the smallest sphere containing
// the object is potentially larger by a factor of sqrt(3)
float Body::getBoundingRadius() const
{
    if (model == InvalidResource)
        return radius;
    else
        return radius * 1.7320508f; // sqrt(3)
}


float Body::getMass() const
{
    return mass;
}


void Body::setMass(float _mass)
{
    mass = _mass;
}


float Body::getOblateness() const
{
    return oblateness;
}


void Body::setOblateness(float _oblateness)
{
    oblateness = _oblateness;
}


float Body::getAlbedo() const
{
    return albedo;
}


void Body::setAlbedo(float _albedo)
{
    albedo = _albedo;
}


Quatf Body::getOrientation() const
{
    return orientation;
}


void Body::setOrientation(const Quatf& q)
{
    orientation = q;
}


RotationElements Body::getRotationElements() const
{
    return rotationElements;
}


void Body::setRotationElements(const RotationElements& re)
{
    rotationElements = re;
}


const Surface& Body::getSurface() const
{
    return surface;
}


Surface& Body::getSurface()
{
    return surface;
}


void Body::setSurface(const Surface& surf)
{
    surface = surf;
}


ResourceHandle Body::getModel() const
{
    return model;
}

void Body::setModel(ResourceHandle _model)
{
    model = _model;
}


PlanetarySystem* Body::getSatellites() const
{
    return satellites;
}

void Body::setSatellites(PlanetarySystem* ssys)
{
    satellites = ssys;
}


RingSystem* Body::getRings() const
{
    return rings;
}

void Body::setRings(const RingSystem& _rings)
{
    if (rings == NULL)
        rings = new RingSystem(_rings);
    else
        *rings = _rings;
}


const Atmosphere* Body::getAtmosphere() const
{
    return atmosphere;
}

Atmosphere* Body::getAtmosphere()
{
    return atmosphere;
}

void Body::setAtmosphere(const Atmosphere& _atmosphere)
{
    if (atmosphere == NULL)
        atmosphere = new Atmosphere();
    *atmosphere = _atmosphere;
}


// Get a matrix which converts from local to heliocentric coordinates
Mat4d Body::getLocalToHeliocentric(double when) const
{
    double ascendingNode = (double) rotationElements.ascendingNode +
        rotationElements.precessionRate * (when - astro::J2000);

    Point3d pos = orbit->positionAtTime(when);
    Mat4d frame;

    switch (orbitRefPlane)
    {
    case astro::BodyEquator:
        frame = (Mat4d::xrotation(-rotationElements.obliquity) *
                 Mat4d::yrotation(-ascendingNode) *
                 Mat4d::translation(pos));
        break;
    case astro::Ecliptic_J2000:
        frame = Mat4d::translation(pos);
        break;
    case astro::Equator_J2000:
        frame = Mat4d::translation(pos);
        break;
    default:
        assert(0);
    }
 
    // Recurse up the hierarchy . . .
    if (system != NULL && system->getPrimaryBody() != NULL)
        frame = frame * system->getPrimaryBody()->getLocalToHeliocentric(when);

    return frame;
}


// Return the position of the center of the body in heliocentric coordinates
Point3d Body::getHeliocentricPosition(double when) const
{
    return Point3d(0.0, 0.0, 0.0) * getLocalToHeliocentric(when);
}


Quatd Body::getEclipticalToEquatorial(double when) const
{
    double ascendingNode = (double) rotationElements.ascendingNode +
        rotationElements.precessionRate * (when - astro::J2000);

    Quatd q =
        Quatd::xrotation(-rotationElements.obliquity) *
        Quatd::yrotation(-ascendingNode);

    // Recurse up the hierarchy . . .
    if (system != NULL && system->getPrimaryBody() != NULL)
        q = q * system->getPrimaryBody()->getEclipticalToEquatorial(when);

    return q;
}


Quatd Body::getEclipticalToGeographic(double when) const
{
    return getEquatorialToGeographic(when) * getEclipticalToEquatorial(when);
}


// The geographic coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.  An object with
// constant geographic coordinates will thus remain fixed with respect
// to a point on the surface of the body.
Quatd Body::getEquatorialToGeographic(double when) const
{
    double t = when - rotationElements.epoch;
    double rotations = t / (double) rotationElements.period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

    // Add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of
    // the texture.
    remainder += 0.5;
    
    Quatd q(1);
    q.yrotate(-remainder * 2 * PI - rotationElements.offset);
    return q;
}


Mat4d Body::getGeographicToHeliocentric(double when) const
{
    return getEquatorialToGeographic(when).toMatrix4() *
        getLocalToHeliocentric(when);
}


Vec3f Body::planetocentricToCartesian(float lon, float lat, float alt) const
{
    float phi = -degToRad(lat) + (float) PI / 2;
    float theta = degToRad(lon) - (float) PI;

    Vec3f pos((float) (cos(theta) * sin(phi)),
              (float) (cos(phi)),
              (float) (-sin(theta) * sin(phi)));

    return pos * (getRadius() + alt);
}


Vec3f Body::planetocentricToCartesian(const Vec3f& lonLatAlt) const
{
    return planetocentricToCartesian(lonLatAlt.x, lonLatAlt.y, lonLatAlt.z);
}


Vec3f Body::cartesianToPlanetocentric(const Vec3f& v) const
{
    Vec3f w = v;
    w.normalize();

    double lat = (float) PI / 2.0f - acos(w.y);
    double lon = atan2(w.z, -w.x);

    return Vec3f((float) lon, (float) lat, v.length() - getRadius());
}


bool Body::extant(double t) const
{
    return t >= protos && t < eschatos;
}


void Body::setLifespan(double begin, double end)
{
    protos = begin;
    eschatos = end;
}


void Body::getLifespan(double& begin, double& end) const
{
    begin = protos;
    end = eschatos;
}


#define SOLAR_IRRADIANCE   1367.6
#define SOLAR_POWER           3.8462e26

float Body::getLuminosity(const Star& sun,
                          float distanceFromSun) const
{
    // Compute the total power of the star in Watts
    double power = SOLAR_POWER * sun.getLuminosity();

    // Compute the irradiance at a distance of 1au from the star in W/m^2
    // double irradiance = power / sphereArea(astro::AUtoKilometers(1.0) * 1000);

    // Compute the irradiance at the body's distance from the star
    double satIrradiance = power / sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet
    double incidentEnergy = satIrradiance * circleArea(radius * 1000);

    double reflectedEnergy = incidentEnergy * albedo;
    
    // Compute the luminosity (i.e. power relative to solar power)
    return (float) (reflectedEnergy / SOLAR_POWER);
}


float Body::getApparentMagnitude(const Star& sun,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sun, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}


// Return the apparent magnitude of the body, corrected for the phase.
float Body::getApparentMagnitude(const Star& sun,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.length();
    double distanceToSun = sunPosition.length();
    float illuminatedFraction = (float) (1.0 + (viewerPosition / distanceToViewer) *
                                         (sunPosition / distanceToSun)) / 2.0f;

    return astro::lumToAppMag(getLuminosity(sun, (float) distanceToSun) * illuminatedFraction, (float) astro::kilometersToLightYears(distanceToViewer));
}


int Body::getClassification() const
{
    return classification;
}

void Body::setClassification(int _classification)
{
    classification = _classification;
}


string Body::getInfoURL() const
{
    return infoURL;
}

void Body::setInfoURL(const string& _infoURL)
{
    infoURL = _infoURL;
}


Surface* Body::getAlternateSurface(const string& name) const
{
    if (altSurfaces == NULL)
        return NULL;

    AltSurfaceTable::iterator iter = altSurfaces->find(name);
    if (iter == altSurfaces->end())
        return NULL;
    else
        return iter->second;
}


void Body::addAlternateSurface(const string& name, Surface* surface)
{
    if (altSurfaces == NULL)
        altSurfaces = new AltSurfaceTable();

    //altSurfaces->insert(AltSurfaceTable::value_type(name, surface));
    (*altSurfaces)[name] = surface;
}


vector<string>* Body::getAlternateSurfaceNames() const
{
    vector<string>* names = new vector<string>();
    if (altSurfaces != NULL)
    {
        for (AltSurfaceTable::const_iterator iter = altSurfaces->begin();
             iter != altSurfaces->end(); iter++)
        {
            names->insert(names->end(), iter->first);
        }
    }

    return names;
}


void Body::addLocation(Location* loc)
{
    assert(loc != NULL);
    if (loc == NULL)
        return;

    if (locations == NULL)
        locations = new vector<Location*>();
    locations->insert(locations->end(), loc);
    loc->setParentBody(this);
}


vector<Location*>* Body::getLocations() const
{
    return locations;
}


Location* Body::findLocation(const string& name) const
{
    if (locations == NULL)
        return NULL;

    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        if (!UTF8StringCompare(name, (*iter)->getName()))
            return *iter;
    }

    return NULL;
}


// Compute the positions of locations on an irregular object using ray-mesh
// intersections.  This is not automatically done when a location is added
// because it would force the loading of all meshes for objects with 
// defined locations; on-demand (i.e. when the object becomes visible to
// a user) loading of meshes is preferred.
void Body::computeLocations()
{
    if (locationsComputed)
        return;

    locationsComputed = true;

    // No work to do if there's no mesh, or if the mesh cannot be loaded
    if (model == InvalidResource)
        return;
    Model* m = GetModelManager()->find(model);
    if (m == NULL)
        return;

    // TODO: Implement separate radius and bounding radius so that this hack is
    // not necessary.
    double boundingRadius = 2.0;

    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        Vec3f v = (*iter)->getPosition();
        float alt = v.length() - radius;
        if (alt != -radius)
            v.normalize();
        v *= (float) boundingRadius;

        Ray3d ray(Point3d(v.x, v.y, v.z), Vec3d(-v.x, -v.y, -v.z));
        double t = 0.0;
        if (m->pick(ray, t))
        {
            v *= (float) ((1.0 - t) * radius + alt);
            (*iter)->setPosition(v);
        }
    }
}


/**** Implementation of PlanetarySystem ****/

PlanetarySystem::PlanetarySystem(Body* _primary) : primary(_primary)
{
    if (primary != NULL && primary->getSystem() != NULL)
        star = primary->getSystem()->getStar();
    else
        star = NULL;
}

PlanetarySystem::PlanetarySystem(Star* _star) :
    star(_star), primary(NULL)
{
}


void PlanetarySystem::addBody(Body* body)
{
    satellites.insert(satellites.end(), body);
}


void PlanetarySystem::removeBody(Body* body)
{
    for (vector<Body*>::iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (*iter == body)
        {
            satellites.erase(iter);
            break;
        }
    }
}


void PlanetarySystem::replaceBody(Body* oldBody, Body* newBody)
{
    for (vector<Body*>::iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (*iter == oldBody)
        {
            *iter = newBody;
            break;
        }
    }
}


Body* PlanetarySystem::find(string _name, bool deepSearch) const
{
    for (vector<Body*>::const_iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (UTF8StringCompare((*iter)->getName(), _name) == 0)
        {
            return *iter;
        }
        else if (deepSearch && (*iter)->getSatellites() != NULL)
        {
            Body* body = (*iter)->getSatellites()->find(_name, deepSearch);
            if (body != NULL)
                return body;
        }
    }

    return NULL;
}


bool PlanetarySystem::traverse(TraversalFunc func, void* info) const
{
    for (int i = 0; i < getSystemSize(); i++)
    {
        Body* body = getBody(i);
        // assert(body != NULL);
        if (!func(body, info))
            return false;
        if (body->getSatellites() != NULL)
        {
            if (!body->getSatellites()->traverse(func, info))
                return false;
        }
    }

    return true;
}

std::vector<std::string> PlanetarySystem::getCompletion(const std::string& _name, bool rec) const
{
    std::vector<std::string> completion;
      
    for (vector<Body*>::const_iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (UTF8StringCompare((*iter)->getName(), _name, _name.length()) == 0)
        {
            completion.push_back((*iter)->getName());
        }
        if (rec && (*iter)->getSatellites() != NULL)
        {
            std::vector<std::string> bodies = (*iter)->getSatellites()->getCompletion(_name);
            completion.insert(completion.end(), bodies.begin(), bodies.end());
        }
    }

    return completion;
}
