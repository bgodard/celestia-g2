// location.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <map>
#include <celengine/location.h>
#include <celutil/util.h>

using namespace std;

static map<string, uint32> FeatureNameToFlag;
static bool featureTableInitialized = false;

struct FeatureNameEntry
{
    char* name;
    uint32 flag;
};

FeatureNameEntry FeatureNames[] =
{
    { "AA", Location::Crater },
    { "VA", Location::Vallis },
    { "MO", Location::Mons },
    { "PM", Location::Planum },
    { "CM", Location::Chasma },
    { "PE", Location::Patera },
    { "ME", Location::Mare },
    { "RU", Location::Rupes },
    { "TE", Location::Tessera },
    { "RE", Location::Regio },
    { "CH", Location::Chaos },
    { "TA", Location::Terra },
    { "AS", Location::Astrum },
    { "CR", Location::Corona },
    { "DO", Location::Dorsum },
    { "FO", Location::Fossa },
    { "CA", Location::Catena },
    { "MN", Location::Mensa },
    { "RI", Location::Rima },
    { "UN", Location::Undae },
    { "RT", Location::Reticulum },
    { "PL", Location::Planitia },
    { "LI", Location::Linea },
    { "FL", Location::Fluctus },
    { "FR", Location::Farrum },
    { "LF", Location::LandingSite },
    { "XX", Location::Other },
    { "City", Location::City },
    { "Observatory", Location::Observatory },
    { "Landing Site", Location::LandingSite },
    { "Crater", Location::Crater },
};


Location::Location() :
    position(0.0f, 0.0f, 0.0f),
    size(0.0f),
    importance(-1.0f),
    featureType(Other),
    infoURL(NULL)
{
}

Location::~Location()
{
    if (infoURL != NULL)
        delete infoURL;
}


string Location::getName() const
{
    return name;
}


void Location::setName(const string& _name)
{
    name = _name;
}


Point3f Location::getPosition() const
{
    return position;
}


void Location::setPosition(const Point3f& _position)
{
    position = _position;
}


float Location::getSize() const
{
    return size;
}


void Location::setSize(float _size)
{
    size = _size;
}


float Location::getImportance() const
{
    return importance;
}


void Location::setImportance(float _importance)
{
    importance = _importance;
}


string Location::getInfoURL() const
{
    return "";
}


void Location::setInfoURL(const string& url)
{
}


uint32 Location::getFeatureType() const
{
    return featureType;
}


void Location::setFeatureType(uint32 _featureType)
{
    featureType = _featureType;
}


static void initFeatureTypeTable()
{
    featureTableInitialized = true;

    for (int i = 0; i < sizeof(FeatureNames) / sizeof(FeatureNames[0]); i++)
    {
        FeatureNameToFlag[FeatureNames[i].name] = FeatureNames[i].flag;
    }
}


uint32 Location::parseFeatureType(const string& s)
{
    if (!featureTableInitialized)
        initFeatureTypeTable();

    int flag = FeatureNameToFlag[s];
    return flag != 0 ? flag : Other;
}


