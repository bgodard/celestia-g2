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
#include "simulation.h"

using namespace std;


Simulation::Simulation(Universe* _universe) :
    realTime(0.0),
    timeScale(1.0),
    syncTime(true),
    universe(_universe),
    closestSolarSystem(NULL),
    selection(),
    faintestVisible(5.0f)
{
    activeObserver = new Observer();
    observers.insert(observers.end(), activeObserver);
}


Simulation::~Simulation()
{
    for (vector<Observer*>::iterator iter = observers.begin();
         iter != observers.end(); iter++)
    {
        delete *iter;
    }
}


static const Star* getSun(Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == NULL)
        return NULL;
    else
        return system->getStar();
}


void Simulation::render(Renderer& renderer)
{
    renderer.render(*activeObserver,
                    *universe,
                    faintestVisible,
                    selection);
}


void Simulation::render(Renderer& renderer, Observer& observer)
{
    renderer.render(observer,
                    *universe,
                    faintestVisible,
                    selection);
}


Universe* Simulation::getUniverse() const
{
    return universe;
}


// Get the time (Julian date)
double Simulation::getTime() const
{
    return activeObserver->getTime();
}

// Set the time to the specified Julian date
void Simulation::setTime(double jd)
{
    if (syncTime)
    {
        for (vector<Observer*>::iterator iter = observers.begin();
             iter != observers.end(); iter++)
        {
            (*iter)->setTime(jd);
        }
    }
    else
    {
        activeObserver->setTime(jd);
    }
}


// Get the clock time elapsed since the object was created
double Simulation::getRealTime() const
{
    return realTime;
}


double Simulation::getArrivalTime() const
{
    return activeObserver->getArrivalTime();
}


// Tick the simulation by dt seconds
void Simulation::update(double dt)
{
    realTime += dt;

    for (vector<Observer*>::iterator iter = observers.begin();
         iter != observers.end(); iter++)
    {
        (*iter)->update(dt, timeScale);
    }

    // Find the closest solar system
    closestSolarSystem = universe->getNearestSolarSystem(activeObserver->getPosition());
}


Selection Simulation::getSelection() const
{
    return selection;
}


void Simulation::setSelection(const Selection& sel)
{
    if (sel != selection)
    {
        universe->unmarkObject(selection, 0);
        selection = sel;
        universe->markObject(selection,
                             10.0f,
                             Color(1.0f, 0.0f, 0.0f, 0.9f),
                             Marker::Diamond,
                             0);
    }
}


Selection Simulation::getTrackedObject() const
{
    return activeObserver->getTrackedObject();
}


void Simulation::setTrackedObject(const Selection& sel)
{
    activeObserver->setTrackedObject(sel);
}


Selection Simulation::pickObject(Vec3f pickRay, float tolerance)
{
    return universe->pick(activeObserver->getPosition(),
                          pickRay * activeObserver->getOrientation().toMatrix4(),
                          activeObserver->getTime(),
                          faintestVisible,
                          tolerance);
}

void Simulation::reverseObserverOrientation()
{
    activeObserver->reverseOrientation();
}


Observer& Simulation::getObserver()
{
    return *activeObserver;
}


Observer* Simulation::addObserver()
{
    Observer* o = new Observer();
    observers.insert(observers.end(), o);
    return o;
}


void Simulation::removeObserver(Observer* o)
{
    vector<Observer*>::iterator iter = find(observers.begin(), observers.end(), o);
    if (iter != observers.end())
        observers.erase(iter);
}


Observer* Simulation::getActiveObserver()
{
    return activeObserver;
}


void Simulation::setActiveObserver(Observer* o)
{
    vector<Observer*>::iterator iter= find(observers.begin(), observers.end(), o);
    if (iter != observers.end())
        activeObserver = o;
}


void Simulation::setObserverPosition(const UniversalCoord& pos)
{
    activeObserver->setPosition(pos);
}

void Simulation::setObserverOrientation(const Quatf& orientation)
{
    activeObserver->setOrientation(orientation);
}


Observer::ObserverMode Simulation::getObserverMode() const
{
    return activeObserver->getMode();
}

void Simulation::setObserverMode(Observer::ObserverMode mode)
{
    activeObserver->setMode(mode);
}

void Simulation::setFrame(astro::CoordinateSystem coordSys,
                          const Selection& sel)
{
    activeObserver->setFrame(FrameOfReference(coordSys, sel));
}

void Simulation::setFrame(const FrameOfReference& _frame)
{
    activeObserver->setFrame(_frame);
}

FrameOfReference Simulation::getFrame() const
{
    return activeObserver->getFrame();
}

// Rotate the observer about its center.
void Simulation::rotate(Quatf q)
{
    activeObserver->rotate(q);
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
void Simulation::orbit(Quatf q)
{
    activeObserver->orbit(selection, q);
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Simulation::changeOrbitDistance(float d)
{
    activeObserver->changeOrbitDistance(selection, d);
}


void Simulation::setTargetSpeed(float s)
{
    activeObserver->setTargetSpeed(s);
}

float Simulation::getTargetSpeed()
{
    return activeObserver->getTargetSpeed();
}

void Simulation::gotoSelection(double gotoTime, 
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (selection.getType() == Selection::Type_Location)
    {
        activeObserver->gotoSelectionGC(selection,
                                        gotoTime, 0.0, 0.5,
                                        up, upFrame);
    }
    else
    {
        activeObserver->gotoSelection(selection, gotoTime, up, upFrame);
    }
}

void Simulation::gotoSelection(double gotoTime,
                               double distance,
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    activeObserver->gotoSelection(selection, gotoTime, distance, up, upFrame);
}

void Simulation::gotoSelectionLongLat(double gotoTime,
                                      double distance,
                                      float longitude,
                                      float latitude,
                                      Vec3f up)
{
    activeObserver->gotoSelectionLongLat(selection, gotoTime, distance,
                                         longitude, latitude, up);
}


void Simulation::gotoLocation(const RigidTransform& transform,
                              double duration)
{
    activeObserver->gotoLocation(transform, duration);
}


void Simulation::getSelectionLongLat(double& distance,
                                     double& longitude,
                                     double& latitude)
{
    activeObserver->getSelectionLongLat(selection, distance, longitude, latitude);
}


void Simulation::gotoSurface(double duration)
{
    activeObserver->gotoSurface(selection, duration);
};


void Simulation::cancelMotion()
{
    activeObserver->cancelMotion();
}

void Simulation::centerSelection(double centerTime)
{
    activeObserver->centerSelection(selection, centerTime);
}

void Simulation::follow()
{
    activeObserver->follow(selection);
}

void Simulation::geosynchronousFollow()
{
    activeObserver->geosynchronousFollow(selection);
}

void Simulation::phaseLock()
{
    activeObserver->phaseLock(selection);
}

void Simulation::chase()
{
    activeObserver->chase(selection);
}


// Choose a planet around a star given it's index in the planetary system.
// The planetary system is either the system of the selected object, or the
// nearest planetary system if no object is selected.  If index is less than
// zero, pick the star.  This function should probably be in celestiacore.cpp.
void Simulation::selectPlanet(int index)
{
    if (index < 0)
    {
        if (selection.getType() == Selection::Type_Body)
        {
            PlanetarySystem* system = selection.body()->getSystem();
            if (system != NULL)
                setSelection(system->getStar());
        }
    }
    else
    {
        const Star* star = NULL;
        if (selection.getType() == Selection::Type_Star)
            star = selection.star();
        else if (selection.getType() == Selection::Type_Body)
            star = getSun(selection.body());

        SolarSystem* solarSystem = NULL;
        if (star != NULL)
            solarSystem = universe->getSolarSystem(star);
        else
            solarSystem = closestSolarSystem;

        if (solarSystem != NULL &&
            index < solarSystem->getPlanets()->getSystemSize())
        {
            setSelection(Selection(solarSystem->getPlanets()->getBody(index)));
        }
    }
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star database
//   2. Search the deep sky catalog for a matching name.
//   3. Search the planets and moons in the planetary system of the currently selected
//      star
//   4. Search the planets and moons in any 'nearby' (< 0.1 ly) planetary systems
Selection Simulation::findObject(string s)
{
    PlanetarySystem* path[2];
    int nPathEntries = 0;

    switch (selection.getType())
    {
    case Selection::Type_Star:
        {
            SolarSystem* sys = universe->getSolarSystem(selection.star());
            if (sys != NULL)
                path[nPathEntries++] = sys->getPlanets();
        }
        break;

    case Selection::Type_Body:
        {
            PlanetarySystem* sys = selection.body()->getSystem();
            while (sys != NULL && sys->getPrimaryBody() != NULL)
                sys = sys->getPrimaryBody()->getSystem();
            path[nPathEntries++] = sys;
        }
        break;
        
    default:
        break;
    }

    if (closestSolarSystem != NULL)
        path[nPathEntries++] = closestSolarSystem->getPlanets();

    return universe->find(s, path, nPathEntries);
}


// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.
Selection Simulation::findObjectFromPath(string s)
{
    PlanetarySystem* path[2];
    int nPathEntries = 0;

    switch (selection.getType())
    {
    case Selection::Type_Star:
        {
            SolarSystem* sys = universe->getSolarSystem(selection.star());
            if (sys != NULL)
                path[nPathEntries++] = sys->getPlanets();
        }
        break;

    case Selection::Type_Body:
        {
            PlanetarySystem* sys = selection.body()->getSystem();
            while (sys != NULL && sys->getPrimaryBody() != NULL)
                sys = sys->getPrimaryBody()->getSystem();
            path[nPathEntries++] = sys;
        }
        break;

    default:
        break;
    }

    if (closestSolarSystem != NULL)
        path[nPathEntries++] = closestSolarSystem->getPlanets();

    return universe->findPath(s, path, nPathEntries);
}

std::vector<std::string> Simulation::getObjectCompletion(string s)
{
    PlanetarySystem* path[2];
    int nPathEntries = 0;
    PlanetarySystem* sys = NULL;

    switch (selection.getType())
    {
    case Selection::Type_Star:
        {
            SolarSystem* solsys = universe->getSolarSystem(selection.star());
            if (solsys != NULL)
                sys = path[nPathEntries++] = solsys->getPlanets();
        }
        break;

    case Selection::Type_Body:
        {
            sys = selection.body()->getSystem();
            while (sys != NULL && sys->getPrimaryBody() != NULL)
                sys = sys->getPrimaryBody()->getSystem();
            path[nPathEntries++] = sys;
        }
        break;

    default:
        break;
    }

    if (closestSolarSystem != NULL && closestSolarSystem->getPlanets() != sys)
        path[nPathEntries++] = closestSolarSystem->getPlanets();

    return universe->getCompletionPath(s, path, nPathEntries);
}


double Simulation::getTimeScale() const
{
    return timeScale;
}

void Simulation::setTimeScale(double _timeScale)
{
    timeScale = _timeScale;
}

bool Simulation::getSyncTime() const
{
    return syncTime;
}

void Simulation::setSyncTime(bool sync)
{
    syncTime = sync;
}

// Synchronize all observers to active observer time
void Simulation::synchronizeTime()
{
    for (vector<Observer*>::iterator iter = observers.begin();
         iter != observers.end(); iter++)
    {
        (*iter)->setTime(activeObserver->getTime());
    }
}


float Simulation::getFaintestVisible() const
{
    return faintestVisible;
}


void Simulation::setFaintestVisible(float magnitude)
{
    faintestVisible = magnitude;
}


SolarSystem* Simulation::getNearestSolarSystem() const
{
    return closestSolarSystem;
}
