// astro.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ASTRO_H_
#define _ASTRO_H_

#include <iostream>
#include <string>
#include <celmath/vecmath.h>
#include <celengine/univcoord.h>

#define SOLAR_ABSMAG  4.83f
#define LN_MAG        1.085736
#define LY_PER_PARSEC 3.26167
#define KM_PER_LY     9466411842000.000
#define KM_PER_AU     149597870.7
#define AU_PER_LY     (KM_PER_LY / KM_PER_AU)

namespace astro
{
    class Date
    {
    public:
        Date();
        Date(int Y, int M, int D);
        Date(double);

        operator double() const;

    public:
        int year;
        int month;
        int day;
        int hour;
        int minute;
        double seconds;
    };

    bool parseDate(const std::string&, Date&);

    enum CoordinateSystem
    {
        Universal       = 0,
        Ecliptical      = 1,
        Equatorial      = 2,
        Geographic      = 3,
        ObserverLocal   = 4,
        PhaseLock       = 5,
        Chase           = 6,
    };

    enum ReferencePlane
    {
        BodyEquator,        // planet equator if moon, ecliptic if planet
        Ecliptic_J2000,
        Equator_J2000,
    };

    float lumToAbsMag(float lum);
    float lumToAppMag(float lum, float lyrs);
    float absMagToLum(float mag);
    float appMagToLum(float mag, float lyrs);
    float absToAppMag(float absMag, float lyrs);
    float appToAbsMag(float appMag, float lyrs);
    float lightYearsToParsecs(float);
    float parsecsToLightYears(float);
    float lightYearsToKilometers(float);
    double lightYearsToKilometers(double);
    float kilometersToLightYears(float);
    double kilometersToLightYears(double);
    float lightYearsToAU(float);
    double lightYearsToAU(double);
    float AUtoLightYears(float);
    float AUtoKilometers(float);
    double AUtoKilometers(double);
    float kilometersToAU(float);
    double kilometersToAU(double);

    float microLightYearsToKilometers(float);
    double microLightYearsToKilometers(double);
    float kilometersToMicroLightYears(float);
    double kilometersToMicroLightYears(double);
    float microLightYearsToAU(float);
    double microLightYearsToAU(double);
    float AUtoMicroLightYears(float);
    double AUtoMicroLightYears(double);

    double secondsToJulianDate(double);
    double julianDateToSeconds(double);

    void decimalToDegMinSec(double angle, int& hours, int& minutes, double& seconds);
    double degMinSecToDecimal(int hours, int minutes, double seconds);

    float sphereIlluminationFraction(Point3d spherePos,
                                     Point3d viewerPos);

    Point3d heliocentricPosition(const UniversalCoord& universal,
                                 const Point3f& starPosition);
    UniversalCoord universalPosition(const Point3d& heliocentric,
                                     const Point3f& starPosition);
    UniversalCoord universalPosition(const Point3d& heliocentric,
                                     const UniversalCoord& starPosition);

    Point3f equatorialToCelestialCart(float ra, float dec, float distance);
    Point3d equatorialToCelestialCart(double ra, double dec, double distance);

    void Anomaly(double meanAnomaly, double eccentricity,
                 double& trueAnomaly, double& eccentricAnomaly);
    double meanEclipticObliquity(double jd);

    extern const double J2000;
    extern const double speedOfLight; // km/s
    extern const double G; // gravitational constant
    extern const double SolarMass;
    extern const double EarthMass;
    extern const double LunarMass;
};

// Convert a date structure to a Julian date

std::ostream& operator<<(std::ostream& s, const astro::Date);

#endif // _ASTRO_H_

