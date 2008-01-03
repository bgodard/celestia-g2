// universe.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _UNIVERSE_H_
#define _UNIVERSE_H_

#include <vector>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/univcoord.h>
#include <celengine/stardb.h>
#include <celengine/dsodb.h>
#include <celengine/solarsys.h>
#include <celengine/deepskyobj.h>
#include <celengine/asterism.h>
#include <celengine/boundaries.h>
#include <celengine/marker.h>
#include <celengine/selection.h>


class Universe
{
 public:
    Universe();
    ~Universe();

    StarDatabase* getStarCatalog() const;
    void setStarCatalog(StarDatabase*);

    SolarSystemCatalog* getSolarSystemCatalog() const;
    void setSolarSystemCatalog(SolarSystemCatalog*);

    DSODatabase* getDSOCatalog() const;
    void setDSOCatalog(DSODatabase*);

    AsterismList* getAsterisms() const;
    void setAsterisms(AsterismList*);

    ConstellationBoundaries* getBoundaries() const;
    void setBoundaries(ConstellationBoundaries*);

    Selection pick(const UniversalCoord& origin,
                   const Vec3f& direction,
                   double when,
                   int   renderFlags,
                   float faintestMag,
                   float tolerance = 0.0f);

    Selection pickStar(const UniversalCoord& origin,
                       const Vec3f& direction,
                       double when,
                       float faintest,
                       float tolerance = 0.0f);

    Selection pickDeepSkyObject(const UniversalCoord& origin,
                                const Vec3f& direction,
                                int   renderFlags,
                                float faintest,
                                float tolerance = 0.0f);

    Selection find(const std::string& s,
                   Selection* contexts = NULL,
                   int nContexts = 0,
                   bool i18n = false) const;
    Selection findPath(const std::string& s,
                       Selection* contexts = NULL,
                       int nContexts = 0,
                       bool i18n = false) const;
    Selection findChildObject(const Selection& sel,
                              const string& name,
                              bool i18n = false) const;
    Selection findObjectInContext(const Selection& sel,
                                  const string& name,
                                  bool i18n = false) const;

    std::vector<std::string> getCompletion(const std::string& s,
                                           Selection* contexts = NULL,
                                           int nContexts = 0,
                                           bool withLocations = false);
    std::vector<std::string> getCompletionPath(const std::string& s,
                                               Selection* contexts = NULL,
                                               int nContexts = 0,
                                               bool withLocations = false);


    SolarSystem* getNearestSolarSystem(const UniversalCoord& position) const;
    SolarSystem* getSolarSystem(const Star* star) const;
    SolarSystem* getSolarSystem(const Selection&) const;
    SolarSystem* createSolarSystem(Star* star) const;

    void getNearStars(const UniversalCoord& position,
                      float maxDistance,
                      std::vector<const Star*>& stars) const;

    void markObject(const Selection&,
                    float size,
                    Color color,
                    Marker::Symbol symbol,
                    int priority,
                    string label);
    void unmarkObject(const Selection&, int priority);
    void unmarkAll();
    bool isMarked(const Selection&, int priority) const;
    MarkerList* getMarkers() const;

 private:
    Selection pickPlanet(SolarSystem& solarSystem,
                         const UniversalCoord&,
                         const Vec3f&,
                         double when,
                         float faintestMag,
                         float tolerance);

 private:
    StarDatabase* starCatalog;
    DSODatabase*             dsoCatalog;
    SolarSystemCatalog* solarSystemCatalog;
    AsterismList* asterisms;
    ConstellationBoundaries* boundaries;
    MarkerList* markers;

    std::vector<const Star*> closeStars;
};

#endif // UNIVERSE_H_
