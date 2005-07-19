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
#include <celengine/body.h>
#include <celutil/util.h>

using namespace std;

static map<string, uint32> FeatureNameToFlag;
static bool featureTableInitialized = false;

struct FeatureNameEntry
{
    const char* name;
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
    parent(NULL),
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


string Location::getName(bool i18n) const
{
    if (!i18n || i18nName == "") return name;
    return i18nName;
}


void Location::setName(const string& _name)
{
    name = _name;
    i18nName = _(_name.c_str()); 
    if (name == i18nName) i18nName = "";
}


Vec3f Location::getPosition() const
{
    return position;
}


void Location::setPosition(const Vec3f& _position)
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


void Location::setInfoURL(const string&)
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

    for (int i = 0; i < (int)(sizeof(FeatureNames) / sizeof(FeatureNames[0])); i++)
    {
        FeatureNameToFlag[string(FeatureNames[i].name)] = FeatureNames[i].flag;
    }
}


uint32 Location::parseFeatureType(const string& s)
{
    if (!featureTableInitialized)
        initFeatureTypeTable();

    int flag = FeatureNameToFlag[s];
    return flag != 0 ? flag : (uint32) Other;
}


Body* Location::getParentBody() const
{
    return parent;
}


void Location::setParentBody(Body* _parent)
{
    parent = _parent;
}


Point3d Location::getPlanetocentricPosition(double t) const
{
    if (parent == NULL)
        return Point3d(position.x, position.y, position.z);

    Quatd q = parent->getEclipticalToGeographic(t);
    return Point3d(position.x, position.y, position.z) * q.toMatrix3();
}


Point3d Location::getHeliocentricPosition(double t) const
{
    if (parent == NULL)
        return Point3d(position.x, position.y, position.z);

    return parent->getHeliocentricPosition(t) +
        (getPlanetocentricPosition(t) - Point3d(0.0, 0.0, 0.0));
}
