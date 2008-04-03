// body.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/utf8.h>
#include "mesh.h"
#include "meshmanager.h"
#include "body.h"
#include "frame.h"
#include "timeline.h"
#include "timelinephase.h"
#include "frametree.h"
#include "referencemark.h"

using namespace std;

Body::Body(PlanetarySystem* _system, const string& _name) :
    system(_system),
    satellites(NULL),
    timeline(NULL),
    frameTree(NULL),
    radius(1.0f),
    semiAxes(1.0f, 1.0f, 1.0f),
    mass(0.0f),
    albedo(0.5f),
    orientation(1.0f),
    model(InvalidResource),
    surface(Color(1.0f, 1.0f, 1.0f)),
    atmosphere(NULL),
    rings(NULL),
    classification(Unknown),
    altSurfaces(NULL),
    locations(NULL),
    locationsComputed(false),
    refMarks(0),
    referenceMarks(NULL),
    visible(1),
    clickable(1),
    visibleAsPoint(1),
    overrideOrbitColor(0),
    orbitVisibility(UseClassVisibility),
    secondaryIlluminator(true)
{
    setName(_name);
    recomputeCullingRadius();
    system->addBody(this);
}


Body::~Body()
{
    if (system != NULL)
        system->removeBody(this);
    // Remove from frame hierarchy

    // Clean up the reference mark list
    if (referenceMarks != NULL)
    {
        for (list<ReferenceMark*>::iterator iter = referenceMarks->begin(); iter != referenceMarks->end(); ++iter)
            delete *iter;
        delete referenceMarks;
    }

    delete timeline;

    delete satellites;
    delete frameTree;
}


/*! Reset body attributes to their default values. The object hierarchy is left untouched,
 *  i.e. child objects are not removed. Alternate surfaces and locations are not removed
 *  either.
 */
void Body::setDefaultProperties()
{
    radius = 1.0f;
    semiAxes = Vec3f(1.0f, 1.0f, 1.0f);
    mass = 0.0f;
    albedo = 0.5f;
    orientation = Quatf(1.0f);
    model = InvalidResource;
    surface = Surface(Color::White);
    delete atmosphere;
    atmosphere = NULL;
    delete rings;
    rings = NULL;
    classification = Unknown;
    visible = 1;
    clickable = 1;
    visibleAsPoint = 1;
    overrideOrbitColor = 0;
    orbitVisibility = UseClassVisibility;
    recomputeCullingRadius();
}


string Body::getName(bool i18n) const
{
    if (!i18n || i18nName == "") return name;
    return i18nName;
}


void Body::setName(const string _name)
{
    name = _name;
    i18nName = _(_name.c_str());
    if (name == i18nName) i18nName = "";
}


PlanetarySystem* Body::getSystem() const
{
    return system;
}


FrameTree* Body::getFrameTree() const
{
    return frameTree;
}


FrameTree* Body::getOrCreateFrameTree()
{
    if (frameTree == NULL)
        frameTree = new FrameTree(this);
    return frameTree;
}


const Timeline* Body::getTimeline() const
{
    return timeline;
}


void Body::setTimeline(Timeline* newTimeline)
{
    if (timeline != newTimeline)
    {
        delete timeline;
        timeline = newTimeline;
        markChanged();
    }
}


void Body::markChanged()
{
    if (timeline != NULL)
        timeline->markChanged();
}


void Body::markUpdated()
{
    if (frameTree != NULL)
        frameTree->markUpdated();
}


const ReferenceFrame* Body::getOrbitFrame(double tdb) const
{
    return timeline->findPhase(tdb)->orbitFrame();
}


const Orbit* Body::getOrbit(double tdb) const
{
    return timeline->findPhase(tdb)->orbit();
}


const ReferenceFrame* Body::getBodyFrame(double tdb) const
{
    return timeline->findPhase(tdb)->bodyFrame();
}


const RotationModel* Body::getRotationModel(double tdb) const
{
    return timeline->findPhase(tdb)->rotationModel();
}


/*! Get the radius of a sphere large enough to contain the primary
 *  geometry of the object: either a mesh or an ellipsoid.
 *  For an irregular (mesh) object, the radius is defined to be
 *  the largest semi-axis of the axis-aligned bounding box.  The
 *  radius of the smallest sphere containing the object is potentially
 *  larger by a factor of sqrt(3).
 *
 *  This method does not consider additional object features
 *  such as rings, atmospheres, or reference marks; use
 *  getCullingRadius() for that.
 */
float Body::getBoundingRadius() const
{
    if (model == InvalidResource)
        return radius;
    else
        return radius * 1.7320508f; // sqrt(3)
}


/*! Return the radius of sphere large enough to contain any geometry
 *  associated with this object: the primary geometry, comet tail,
 *  rings, atmosphere shell, cloud layers, or reference marks.
 */
float Body::getCullingRadius() const
{
    return cullingRadius;
}


float Body::getMass() const
{
    return mass;
}


void Body::setMass(float _mass)
{
    mass = _mass;
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


/*! Set the semiaxes of a body.
 */
void Body::setSemiAxes(const Vec3f& _semiAxes)
{
    semiAxes = _semiAxes;

    // Radius will always be the largest of the three semi axes
    radius = max(semiAxes.x, max(semiAxes.y, semiAxes.z));
    recomputeCullingRadius();
}


/*! Retrieve the body's semiaxes
 */
Vec3f Body::getSemiAxes() const
{
    return semiAxes;
}


/*! Get the radius of the body. For a spherical body, this is simply
 *  the sphere's radius. For an ellipsoidal body, the radius is the
 *  largest of the three semiaxes. For irregular bodies (with a shape
 *  represented by a mesh), the radius is the largest semiaxis of the
 *  mesh's axis aligned bounding axis. Note that this means some portions
 *  of the mesh may extend outside the sphere of the retrieved radius.
 *  To obtain the radius of a sphere that will definitely enclose the
 *  body, call getBoundingRadius() instead.
 */
float Body::getRadius() const
{
    return radius;
}


/*! Return true if the body is a perfect sphere.
*/
bool Body::isSphere() const
{
    return (model == InvalidResource) &&
           (semiAxes.x == semiAxes.y) && 
           (semiAxes.x == semiAxes.z);
}


/*! Return true if the body is ellipsoidal, with geometry determined
 *  completely by its semiaxes rather than a triangle based model.
 */
bool Body::isEllipsoid() const
{
    return model == InvalidResource;
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
    recomputeCullingRadius();
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
    recomputeCullingRadius();
}


// The following four functions are used to get the state of the body
// in universal coordinates:
//    * getPosition
//    * getOrientation
//    * getVelocity
//    * getAngularVelocity

/*! Get the position of the body in the universal coordinate system.
 *  This method uses high-precision coordinates and is thus
 *  slower relative to getAstrocentricPosition(), which works strictly
 *  with standard double precision. For most purposes,
 *  getAstrocentricPosition() should be used instead of the more
 *  general getPosition().
 */
UniversalCoord Body::getPosition(double tdb) const
{
    Point3d position(0.0, 0.0, 0.0);

    const TimelinePhase* phase = timeline->findPhase(tdb);
    Point3d p = phase->orbit()->positionAtTime(tdb);
    ReferenceFrame* frame = phase->orbitFrame();

    while (frame->getCenter().getType() == Selection::Type_Body)
    {
        phase = frame->getCenter().body()->timeline->findPhase(tdb);
        position += Vec3d(p.x, p.y, p.z) * frame->getOrientation(tdb).toMatrix3();
        p = phase->orbit()->positionAtTime(tdb);
        frame = phase->orbitFrame();
    }

    position += Vec3d(p.x, p.y, p.z) * frame->getOrientation(tdb).toMatrix3();

    if (frame->getCenter().star())
        return astro::universalPosition(position, frame->getCenter().star()->getPosition(tdb));
    else
        return astro::universalPosition(position, frame->getCenter().getPosition(tdb));
}


/*! Get the orientation of the body in the universal coordinate system.
 */
Quatd Body::getOrientation(double tdb) const
{
    // TODO: This method overloads getOrientation(), but the two do very
    // different things. The other method should be renamed to
    // getModelOrientation().
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->rotationModel()->orientationAtTime(tdb) *
           phase->bodyFrame()->getOrientation(tdb);
}


/*! Get the velocity of the body in the universal frame.
 */
Vec3d Body::getVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);

    ReferenceFrame* orbitFrame = phase->orbitFrame();

    Vec3d v = phase->orbit()->velocityAtTime(tdb);
    v = v * orbitFrame->getOrientation(tdb).toMatrix3() + orbitFrame->getCenter().getVelocity(tdb);

	if (!orbitFrame->isInertial())
	{
		Vec3d r = Selection(const_cast<Body*>(this)).getPosition(tdb) - orbitFrame->getCenter().getPosition(tdb);
		r = r * astro::microLightYearsToKilometers(1.0);
		v += cross(orbitFrame->getAngularVelocity(tdb), r);
	}

	return v;
}


/*! Get the angular velocity of the body in the universal frame.
 */
Vec3d Body::getAngularVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);

    Vec3d v = phase->rotationModel()->angularVelocityAtTime(tdb);

    ReferenceFrame* bodyFrame = phase->bodyFrame();
	v = v * bodyFrame->getOrientation(tdb).toMatrix3();
	if (!bodyFrame->isInertial())
	{
		v += bodyFrame->getAngularVelocity(tdb);
	}

    return v;
}


/*! Get the transformation which converts body coordinates into
 *  astrocentric coordinates. Some clarification on the meaning
 *  of 'astrocentric': the position of every solar system body
 *  is ultimately defined with respect to some star or star
 *  system barycenter.
 */
Mat4d Body::getLocalToAstrocentric(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);
    Point3d p = phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
    return Mat4d::translation(p);
}


/*! Get the position of the center of the body in astrocentric ecliptic coordinates.
 */
Point3d Body::getAstrocentricPosition(double tdb) const
{
    // TODO: Switch the iterative method used in getPosition
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
}


/*! Get a rotation that converts from the ecliptic frame to the body frame.
 */
Quatd Body::getEclipticToFrame(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->bodyFrame()->getOrientation(tdb);
}


/*! Get a rotation that converts from the ecliptic frame to the body's 
 *  mean equatorial frame.
 */
Quatd Body::getEclipticToEquatorial(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->rotationModel()->equatorOrientationAtTime(tdb) *
           phase->bodyFrame()->getOrientation(tdb); 
}


/*! Get a rotation that converts from the ecliptic frame to this
 *  objects's body fixed frame.
 */
Quatd Body::getEclipticToBodyFixed(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->rotationModel()->orientationAtTime(tdb) *
           phase->bodyFrame()->getOrientation(tdb);
}


// The body-fixed coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.
Quatd Body::getEquatorialToBodyFixed(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb);
    return phase->rotationModel()->spin(tdb);
}


/*! Get a transformation to convert from the object's body fixed frame
 *  to the astrocentric ecliptic frame.
 */
Mat4d Body::getBodyFixedToAstrocentric(double tdb) const
{
    return getEquatorialToBodyFixed(tdb).toMatrix4() * getLocalToAstrocentric(tdb);
}


Vec3d Body::planetocentricToCartesian(double lon, double lat, double alt) const
{
    double phi = -degToRad(lat) + PI / 2;
    double theta = degToRad(lon) - PI;

    Vec3d pos(cos(theta) * sin(phi),
              cos(phi),
              -sin(theta) * sin(phi));

    return pos * (getRadius() + alt);
}


Vec3d Body::planetocentricToCartesian(const Vec3d& lonLatAlt) const
{
    return planetocentricToCartesian(lonLatAlt.x, lonLatAlt.y, lonLatAlt.z);
}


/*! Convert cartesian body-fixed coordinates to spherical planetocentric
 *  coordinates.
 */
Vec3d Body::cartesianToPlanetocentric(const Vec3d& v) const
{
    Vec3d w = v;
    w.normalize();

    double lat = PI / 2.0 - acos(w.y);
    double lon = atan2(w.z, -w.x);

    return Vec3d(lon, lat, v.length() - getRadius());
}


/*! Convert body-centered ecliptic coordinates to spherical planetocentric
 *  coordinates.
 */
Vec3d Body::eclipticToPlanetocentric(const Vec3d& ecl, double tdb) const
{
    Quatd q = getEclipticToBodyFixed(tdb);
    Vec3d bf = ecl * (~q).toMatrix3();

    return cartesianToPlanetocentric(bf);
}


bool Body::extant(double t) const
{
    return timeline->includes(t);
}


void Body::getLifespan(double& begin, double& end) const
{
    begin = timeline->startTime();
    end = timeline->endTime();
}


float Body::getLuminosity(const Star& sun,
                          float distanceFromSun) const
{
    return getLuminosity(sun.getLuminosity(), distanceFromSun);
}


float Body::getLuminosity(float sunLuminosity,
                          float distanceFromSun) const
{
    // Compute the total power of the star in Watts
    double power = astro::SOLAR_POWER * sunLuminosity;

    // Compute the irradiance at a distance of 1au from the star in W/m^2
    // double irradiance = power / sphereArea(astro::AUtoKilometers(1.0) * 1000);

    // Compute the irradiance at the body's distance from the star
    double satIrradiance = power / sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet
    double incidentEnergy = satIrradiance * circleArea(radius * 1000);

    double reflectedEnergy = incidentEnergy * albedo;
    
    // Compute the luminosity (i.e. power relative to solar power)
    return (float) (reflectedEnergy / astro::SOLAR_POWER);
}


/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float Body::getApparentMagnitude(const Star& sun,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sun, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}


/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float Body::getApparentMagnitude(float sunLuminosity,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sunLuminosity, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}

/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float Body::getApparentMagnitude(const Star& sun,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    return getApparentMagnitude(sun.getLuminosity(),
                                sunPosition,
                                viewerPosition);
}


/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float Body::getApparentMagnitude(float sunLuminosity,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.length();
    double distanceToSun = sunPosition.length();
    float illuminatedFraction = (float) (1.0 + (viewerPosition / distanceToViewer) *
                                         (sunPosition / distanceToSun)) / 2.0f;

    return astro::lumToAppMag(getLuminosity(sunLuminosity, (float) distanceToSun) * illuminatedFraction, (float) astro::kilometersToLightYears(distanceToViewer));
}


int Body::getClassification() const
{
    return classification;
}

void Body::setClassification(int _classification)
{
    classification = _classification;
    recomputeCullingRadius();
    markChanged();
}


/*! Return the effective classification of this body used when rendering
 *  orbits. Normally, this is just the classification of the object, but
 *  invisible objects are treated specially: they behave as if they have
 *  the classification of their child objects. This fixes annoyances when
 *  planets are defined with orbits relative to their system barycenters.
 *  For example, Pluto's orbit can seen in a solar system scale view, even
 *  though its orbit is defined relative to the Pluto-Charon barycenter
 *  and is this just a few hundred kilometers in size.
 */
int Body::getOrbitClassification() const
{
    if (classification != Invisible || frameTree == NULL)
    {
        return classification;
    }
    else
    {
        int orbitClass = frameTree->childClassMask();
        if (orbitClass & Planet)
            return Planet;
        else if (orbitClass & DwarfPlanet)
            return DwarfPlanet;
        else if (orbitClass & Moon)
            return Moon;
        else if (orbitClass & Asteroid)
            return Asteroid;
        else if (orbitClass & Spacecraft)
            return Spacecraft;
        else
            return Invisible;
    }
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


Location* Body::findLocation(const string& name, bool i18n) const
{
    if (locations == NULL)
        return NULL;

    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        if (!UTF8StringCompare(name, (*iter)->getName(i18n)))
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


bool
Body::referenceMarkVisible(uint32 refmark) const
{
    return (refMarks & refmark) != 0;
}


uint32
Body::getVisibleReferenceMarks() const
{
    return refMarks;
}


void
Body::setVisibleReferenceMarks(uint32 refmarks)
{
	refMarks = refmarks;
}


/*! Add a new reference mark.
 */
void
Body::addReferenceMark(ReferenceMark* refMark)
{
    if (referenceMarks == NULL)
        referenceMarks = new list<ReferenceMark*>();
    referenceMarks->push_back(refMark);
    recomputeCullingRadius();
}


/*! Remove the first reference mark with the specified tag.
 */
void
Body::removeReferenceMark(const string& tag)
{
    if (referenceMarks != NULL)
    {
        ReferenceMark* refMark = findReferenceMark(tag);
        if (refMark != NULL)
        {
            referenceMarks->remove(refMark);
            delete refMark;
            recomputeCullingRadius();
        }
    }
}


/*! Find the first reference mark with the specified tag. If the body has
 *  no reference marks with the specified tag, this method will return
 *  NULL.
 */
ReferenceMark*
Body::findReferenceMark(const string& tag) const
{
    if (referenceMarks != NULL)
    {
        for (list<ReferenceMark*>::iterator iter = referenceMarks->begin(); iter != referenceMarks->end(); ++iter)
        {
            if ((*iter)->getTag() == tag)
                return *iter;
        }
    }

    return NULL;
}


/*! Get the list of reference marks associated with this body. May return
 *  NULL if there are no reference marks.
 */
const list<ReferenceMark*>*
Body::getReferenceMarks() const
{
    return referenceMarks;
}


/*! Sets whether or not the object is visible.
 */
void Body::setVisible(bool _visible)
{
    visible = _visible ? 1 : 0;
}


/*! Sets whether or not the object can be selected by clicking on
 *  it. If set to false, the object is completely ignored when the
 *  user clicks it, making it possible to select background objects.
 */
void Body::setClickable(bool _clickable)
{
    clickable = _clickable ? 1 : 0;
}


/*! Set whether or not the object is visible as a starlike point
 *  when it occupies less than a pixel onscreen. This is appropriate
 *  for planets and moons, but generally not desireable for buildings
 *  or spacecraft components.
 */
void Body::setVisibleAsPoint(bool _visibleAsPoint)
{
    visibleAsPoint = _visibleAsPoint ? 1 : 0;
}


/*! The orbitColorOverride flag is set to true if an alternate orbit
 *  color should be used (specified via setOrbitColor) instead of the
 *  default class orbit color.
 */
void Body::setOrbitColorOverridden(bool override)
{
    overrideOrbitColor = override ? 1 : 0;
}


/*! Set the visibility policy for the orbit of this object:
 *  - NeverVisible: Never show the orbit of this object.
 *  - UseClassVisibility: (Default) Show the orbit of this object
 *    its class is enabled in the orbit mask.
 *  - AlwaysVisible: Always show the orbit of this object whenever
 *    orbit paths are enabled.
 */
void Body::setOrbitVisibility(VisibilityPolicy _orbitVisibility)
{
    orbitVisibility = _orbitVisibility;
}


/*! Set the color used when rendering the orbit. This is only used
 *  when the orbitColorOverride flag is set to true; otherwise, the
 *  standard orbit color for all objects of the class is used.
 */
void Body::setOrbitColor(const Color& c)
{
    orbitColor = c;
}


/*! Set whether or not the object should be considered when calculating
 *  secondary illumination (e.g. planetshine.)
 */
void Body::setSecondaryIlluminator(bool enable)
{
    if (enable != secondaryIlluminator)
    {
        markChanged();
        secondaryIlluminator = enable;
    }
}


void Body::recomputeCullingRadius()
{
    float r = getBoundingRadius();

    if (rings != NULL)
        r = max(r, rings->outerRadius);

    if (atmosphere != NULL)
    {
        r = max(r, atmosphere->height);
        r = max(r, atmosphere->cloudHeight);
    }

    if (referenceMarks != NULL)
    {
        for (std::list<ReferenceMark*>::const_iterator iter = referenceMarks->begin();
             iter != referenceMarks->end(); iter++)
        {
            r = max(r, (*iter)->boundingSphereRadius());
        }
    }

    if (classification == Body::Comet)
        r = max(r, astro::AUtoKilometers(1.0f));

    if (r != cullingRadius)
    {
        cullingRadius = r;
        markChanged();
    }
}


/**** Implementation of PlanetarySystem ****/

/*! Return the equatorial frame for this object. This frame is used as
 *  the default frame for objects in SSC files that orbit non-stellar bodies.
 *  In order to avoid wasting memory, it is created until the first time it
 *  is requested.
 */

PlanetarySystem::PlanetarySystem(Body* _primary) :
    star(NULL),
    primary(_primary)
{
    if (primary != NULL && primary->getSystem() != NULL)
        star = primary->getSystem()->getStar();
}


PlanetarySystem::PlanetarySystem(Star* _star) :
    star(_star),
    primary(NULL)
{
}


PlanetarySystem::~PlanetarySystem()
{
}


void PlanetarySystem::addBody(Body* body)
{
    satellites.insert(satellites.end(), body);
    objectIndex.insert(make_pair(body->getName(), body));
    i18nObjectIndex.insert(make_pair(body->getName(true), body));
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

    // Erase the object from the object indices
    for (ObjectIndex::iterator iter = objectIndex.begin();
         iter != objectIndex.end(); iter++)
    {
        if (iter->second == body)
        {
            objectIndex.erase(iter);
            break;
        }
    }

    for (ObjectIndex::iterator iter = i18nObjectIndex.begin();
         iter != i18nObjectIndex.end(); iter++)
    {
        if (iter->second == body)
        {
            i18nObjectIndex.erase(iter);
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

    // Erase the object from the object indices
    for (ObjectIndex::iterator iter = objectIndex.begin();
         iter != objectIndex.end(); iter++)
    {
        if (iter->second == oldBody)
        {
            objectIndex.erase(iter);
            break;
        }
    }

    for (ObjectIndex::iterator iter = i18nObjectIndex.begin();
         iter != i18nObjectIndex.end(); iter++)
    {
        if (iter->second == oldBody)
        {
            i18nObjectIndex.erase(iter);
            break;
        }
    }

    // Add the replacement to the object indices
    objectIndex.insert(make_pair(newBody->getName(), newBody));
    i18nObjectIndex.insert(make_pair(newBody->getName(true), newBody));
}


Body* PlanetarySystem::find(const string& _name, bool deepSearch, bool i18n) const
{
    ObjectIndex index = i18n?i18nObjectIndex:objectIndex;
    ObjectIndex::const_iterator firstMatch = index.find(_name);
    if (firstMatch != index.end())
    {
        return firstMatch->second;
    }

    if (deepSearch)
    {
        for (vector<Body*>::const_iterator iter = satellites.begin();
             iter != satellites.end(); iter++)
        {
            if (UTF8StringCompare((*iter)->getName(i18n), _name) == 0)
            {
                return *iter;
            }
            else if (deepSearch && (*iter)->getSatellites() != NULL)
            {
                Body* body = (*iter)->getSatellites()->find(_name, deepSearch, i18n);
                if (body != NULL)
                    return body;
            }
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
    int _name_length = UTF8Length(_name);

    for (vector<Body*>::const_iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (UTF8StringCompare((*iter)->getName(true), _name, _name_length) == 0)
        {
            completion.push_back((*iter)->getName(true));
        }
        if (rec && (*iter)->getSatellites() != NULL)
        {
            std::vector<std::string> bodies = (*iter)->getSatellites()->getCompletion(_name);
            completion.insert(completion.end(), bodies.begin(), bodies.end());
        }
    }

    return completion;
}


/*! Get the order of the object in the list of children. Returns -1 if the
 *  specified body is not a child object.
 */
int PlanetarySystem::getOrder(const Body* body) const
{
    vector<Body*>::const_iterator iter = std::find(satellites.begin(), satellites.end(), body);
    if (iter == satellites.end())
        return -1;
    else
        return iter - satellites.begin();
}
