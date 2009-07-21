// selection.cpp
// 
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cassert>
#include "astro.h"
#include "selection.h"
#include "frametree.h"
#include "eigenport.h"

using namespace Eigen;
using namespace std;


// Some velocities are computed by differentiation; units
// are Julian days.
static const double VELOCITY_DIFF_DELTA = 1.0 / 1440.0;


double Selection::radius() const
{
    switch (type)
    {
    case Type_Star:
        return star()->getRadius();
    case Type_Body:
        return body()->getRadius();
    case Type_DeepSky:
        return astro::lightYearsToKilometers(deepsky()->getRadius());
    case Type_Location:
        // The size of a location is its diameter, so divide by 2.
        return location()->getSize() / 2.0f;
    default:
        return 0.0;
    }
}


UniversalCoord Selection::getPosition(double t) const
{
    switch (type)
    {
    case Type_Body:
        return body()->getPosition(t);
        
    case Type_Star:
        return star()->getPosition(t);

    case Type_DeepSky:
        {
            Vector3d p = deepsky()->getPosition();
            // NOTE: cast to single precision is only present to maintain compatibility with
            // Celestia 1.6.0.
            return UniversalCoord::CreateLy(p.cast<float>());
#if CELVEC
            return astro::universalPosition(Point3d(0.0, 0.0, 0.0),
                                            Point3f((float) p.x, (float) p.y, (float) p.z));
#endif
        }
        
    case Type_Location:
        {
            Body* body = location()->getParentBody();
            if (body != NULL)
            {
#if CELVEC
                Point3d planetocentricPos = location()->getPlanetocentricPosition(t) *
                    astro::kilometersToMicroLightYears(1.0);
                return body->getPosition(t) + planetocentricPos;
#endif
                return body->getPosition(t).offsetKm(toEigen(location()->getPlanetocentricPosition(t)));
            }
            else
            {
                // Bad location; all locations should have a parent.
                assert(0);
                return UniversalCoord::Zero();
            }
        }

    default:
        return UniversalCoord::Zero();
    }
}


Vector3d Selection::getVelocity(double t) const
{
    switch (type)
    {
    case Type_Body:
        return body()->getVelocity(t);
        
    case Type_Star:
        return star()->getVelocity(t);

    case Type_DeepSky:
        return Vector3d::Zero();

    case Type_Location:
		{
			// For now, just use differentiation for location velocities.
#if CELVEC
			Vec3d ulyPerJD = (getPosition(t) - getPosition(t - VELOCITY_DIFF_DELTA)) * (1.0 / VELOCITY_DIFF_DELTA);
			return ulyPerJD * astro::microLightYearsToKilometers(1.0);
#endif
            return getPosition(t).offsetFromKm(getPosition(t - VELOCITY_DIFF_DELTA)) / VELOCITY_DIFF_DELTA;
		}

    default:
        return Vector3d::Zero();
    }
}


string Selection::getName(bool i18n) const
{
    switch (type)
    {
    case Type_Star:
        {
            char buf[20];
            sprintf(buf, "#%d", star()->getCatalogNumber());
            return string(buf);
        }

    case Type_DeepSky:
        {
            char buf[20];
            sprintf(buf, "#%d", deepsky()->getCatalogNumber());
            return string(buf);
        }
        
    case Type_Body:
        {
            string name = body()->getName(i18n);
            PlanetarySystem* system = body()->getSystem();
            while (system != NULL)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != NULL)
                {
                    name = parent->getName(i18n) + '/' + name;
                    system = parent->getSystem();
                }
                else
                {
                    const Star* parentStar = system->getStar();
                    if (parentStar != NULL)
                    {
                        char buf[20];
                        sprintf(buf, "#%d", parentStar->getCatalogNumber());
                        name = string(buf) + '/' + name;
                    }
                    system = NULL;
                }
            }
            return name;
        }

    case Type_Location:
        if (location()->getParentBody() == NULL)
        {
            return location()->getName(i18n);
        }
        else
        {
            return Selection(location()->getParentBody()).getName(i18n) + '/' +
                location()->getName(i18n);
        }

    default:
        return "";
    }
}


Selection Selection::parent() const
{
    switch (type)
    {
    case Type_Location:
        return Selection(location()->getParentBody());

    case Type_Body:
        if (body()->getSystem())
        {
            if (body()->getSystem()->getPrimaryBody() != NULL)
                return Selection(body()->getSystem()->getPrimaryBody());
            else
                return Selection(body()->getSystem()->getStar());
        }
        else
        {
            return Selection();
        }
        break;

    case Type_Star:
        return Selection(star()->getOrbitBarycenter());
        
    case Type_DeepSky:
        // Currently no hierarchy for stars and deep sky objects.
        return Selection();

    default:
        return Selection();
    }
}


/*! Return true if the selection's visibility flag is set. */
bool Selection::isVisible() const
{
    switch (type)
    {
    case Type_Body:
        return body()->isVisible();
    case Type_Star:
        return true;
    case Type_DeepSky:
        return deepsky()->isVisible();
    default:
        return false;
    }
}
