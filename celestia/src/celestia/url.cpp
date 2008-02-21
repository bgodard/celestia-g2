/***************************************************************************
                          url.cpp  -  description
                             -------------------
    begin                : Wed Aug 7 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string>
#include <cstdio>
#include <cassert>
#include "celestiacore.h"
#include "celengine/astro.h"
#include "url.h"

static const unsigned int CurrentCelestiaURLVersion = 2;

Url::Url()
{};

Url::Url(const std::string& str, CelestiaCore *core):
    urlStr(str),
    appCore(core),
    pauseState(false)
{
    std::string::size_type pos, endPrevious;
    std::vector<Selection> bodies;
    Simulation *sim = appCore->getSimulation();
    std::map<std::string, std::string> params = parseUrlParams(urlStr);

    
    if (urlStr.substr(0, 6) != "cel://")
    {
        urlStr = "";
        return;
    }
    
    pos = urlStr.find("/", 6);
    if (pos == std::string::npos) pos = urlStr.find("?", 6);

    if (pos == std::string::npos) modeStr = urlStr.substr(6);
    else modeStr = decode_string(urlStr.substr(6, pos - 6));


    if (!compareIgnoringCase(modeStr, std::string("Freeflight")))
    {
        mode = ObserverFrame::Universal;
        nbBodies = 0;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Follow")))
    {
        mode = ObserverFrame::Ecliptical;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("SyncOrbit")))
    {
        mode = ObserverFrame::BodyFixed;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Chase")))
    {
        mode = ObserverFrame::Chase;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("PhaseLock")))
    {
        mode = ObserverFrame::PhaseLock;
        nbBodies = 2;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Settings")))
    {
        type = Settings;
        nbBodies = 0;
    }

    if (nbBodies == -1)
    {
        urlStr = "";
        return; // Mode not recognized
    }

    // Version labelling of cel URLs was only added in Celestia 1.5, cel URL
    // version 2. Assume any URL without a version is version 1.
    unsigned int version = 1;
    if (params["ver"] != "")
    {
        sscanf(params["ver"].c_str(), "%u", &version);
    }

    endPrevious = pos;
    int nb = nbBodies, i=1;
    while (nb != 0 && endPrevious != std::string::npos) {
        std::string bodyName="";
        pos = urlStr.find("/", endPrevious + 1);
        if (pos == std::string::npos) pos = urlStr.find("?", endPrevious + 1);
        if (pos == std::string::npos) bodyName = urlStr.substr(endPrevious + 1);
        else bodyName = urlStr.substr(endPrevious + 1, pos - endPrevious - 1);
        endPrevious = pos;

        bodyName = decode_string(bodyName);
        pos = 0;
        if (i==1) body1 = bodyName;
        if (i==2) body2 = bodyName;
        while(pos != std::string::npos) {
            pos = bodyName.find(":", pos + 1);
            if (pos != std::string::npos) bodyName[pos]='/';
        }

        bodies.push_back(sim->findObjectFromPath(bodyName));

        nb--;
        i++;
    }

    if (nb != 0) {
        urlStr = "";
        return; // Number of bodies in Url doesn't match Mode
    }

    if (nbBodies == 0) ref = ObserverFrame();
    if (nbBodies == 1) ref = ObserverFrame(mode, bodies[0]);
    if (nbBodies == 2) ref = ObserverFrame(mode, bodies[0], bodies[1]);
    fromString = true;

    std::string time="";
    pos = urlStr.find("?", endPrevious + 1);
    if (pos == std::string::npos) time = urlStr.substr(endPrevious + 1);
    else time = urlStr.substr(endPrevious + 1, pos - endPrevious -1);
    time = decode_string(time);
    
    if (type != Settings)
    {
        if (params["dist"] != "")
            type = Relative;
        else
            type = Absolute;
    }

    switch (type) {
    case Absolute:
        date = astro::Date(0.0);
        sscanf(time.c_str(), "%d-%d-%dT%d:%d:%lf",
               &date.year, &date.month, &date.day,
               &date.hour, &date.minute, &date.seconds);

        coord = UniversalCoord(BigFix(params["x"]),
                               BigFix(params["y"]),
                               BigFix(params["z"]));

        float ow, ox, oy, oz;
        sscanf(params["ow"].c_str(), "%f", &ow);
        sscanf(params["ox"].c_str(), "%f", &ox);
        sscanf(params["oy"].c_str(), "%f", &oy);
        sscanf(params["oz"].c_str(), "%f", &oz);

        orientation = Quatf(ow, ox, oy, oz);

        // Intentional Fall-Through
    case Relative:
        if (params["dist"] != "") {
            sscanf(params["dist"].c_str(), "%lf", &distance);
        }
        if (params["long"] != "") {
            sscanf(params["long"].c_str(), "%lf", &longitude);
        }
        if (params["lat"] != "") {
            sscanf(params["lat"].c_str(), "%lf", &latitude);
        }
        if (params["select"] != "") {
                selectedStr = params["select"];
        }
        if (params["track"] != "") {
            trackedStr = params["track"];
        }
        if (params["ltd"] != "") {
            lightTimeDelay = (strcmp(params["ltd"].c_str(), "1") == 0);
        } else {
            lightTimeDelay = false;
        }
        if (params["fov"] != "") {
            sscanf(params["fov"].c_str(), "%f", &fieldOfView);
        }
        if (params["ts"] != "") {
            sscanf(params["ts"].c_str(), "%f", &timeScale);
        }
        if (params["p"] != "") {
            sscanf(params["p"].c_str(), "%d", &pauseState);
        }
        break;
    case Settings:
        break;
    }

    if (params["rf"] != "") {
        sscanf(params["rf"].c_str(), "%d", &renderFlags);
    }
    if (params["lm"] != "") {
        sscanf(params["lm"].c_str(), "%d", &labelMode);
    }

    evalName();
}


Url::Url(CelestiaCore* core, UrlType type)
{
    appCore = core;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();

    this->type = type;

    modeStr = getCoordSysName(sim->getFrame()->getCoordinateSystem());
    if (type == Settings) modeStr = "Settings";
    ref = *sim->getFrame();
    urlStr += "cel://" + modeStr;
    if (type != Settings && sim->getFrame()->getCoordinateSystem() != ObserverFrame::Universal) {
        body1 = getSelectionName(sim->getFrame()->getRefObject());
        urlStr += "/" + body1;
        if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::PhaseLock) {
            body2 = getSelectionName(sim->getFrame()->getTargetObject());
            urlStr += "/" + body2;
        }
    }

    char date_str[50];
    date = astro::Date(sim->getTime());
    char buff[255];

    switch (type) {
    case Absolute:
        snprintf(date_str, sizeof(date_str), "%04d-%02d-%02dT%02d:%02d:%08.5f",
            date.year, date.month, date.day, date.hour, date.minute, date.seconds);

        coord = sim->getObserver().getPosition();
        urlStr += std::string("/") + date_str + "?x=" + coord.x.toString();
        urlStr += "&y=" +  coord.y.toString();
        urlStr += "&z=" +  coord.z.toString();

        orientation = sim->getObserver().getOrientationf();
        sprintf(buff, "&ow=%f&ox=%f&oy=%f&oz=%f", orientation.w, orientation.x, orientation.y, orientation.z);
        urlStr += buff;
        break;
    case Relative:
        sim->getSelectionLongLat(distance, longitude, latitude);
        sprintf(buff, "dist=%f&long=%f&lat=%f", distance, longitude, latitude);
        urlStr += std::string("/?") + buff;
        break;
    case Settings:
        urlStr += std::string("/?");
        break;
    }

    switch (type) {
    case Absolute: // Intentional Fall-Through
    case Relative:
        tracked = sim->getTrackedObject();
        trackedStr = getSelectionName(tracked);
        if (trackedStr != "") urlStr += "&track=" + trackedStr;

        selected = sim->getSelection();
        selectedStr = getSelectionName(selected);
        if (selectedStr != "") urlStr += "&select=" + selectedStr;

        fieldOfView = radToDeg(sim->getActiveObserver()->getFOV());
        timeScale = (float) sim->getTimeScale();
        pauseState = sim->getPauseState();
        lightTimeDelay = appCore->getLightDelayActive();
        sprintf(buff, "&fov=%f&ts=%f&ltd=%c&p=%c&", fieldOfView,
            timeScale, lightTimeDelay?'1':'0', pauseState?'1':'0');
        urlStr += buff;
    case Settings: // Intentional Fall-Through
        renderFlags = renderer->getRenderFlags();
        labelMode = renderer->getLabelMode();
        sprintf(buff, "rf=%d&lm=%d", renderFlags, labelMode);
        urlStr += buff;
        break;
    }

    // Append the Celestia URL version
    {
        char buf[32];
        sprintf(buf, "&ver=%u", CurrentCelestiaURLVersion);
        urlStr += buf;
    }

    evalName();
}


std::string Url::getAsString() const
{
    return urlStr;
}


std::string Url::getName() const
{
    return name;
}


void Url::evalName()
{
    char buff[50];
    double lo = longitude, la = latitude;
    char los = 'E';
    char las = 'N';
    switch(type) {
    case Absolute:
        name = _(modeStr.c_str());
        if (body1 != "") name += " " + std::string(_(getBodyShortName(body1).c_str()));
        if (body2 != "") name += " " + std::string(_(getBodyShortName(body2).c_str()));
        if (trackedStr != "") name += " -> " + std::string(_(getBodyShortName(trackedStr).c_str()));
        if (selectedStr != "") name += " [" + std::string(_(getBodyShortName(selectedStr).c_str())) + "]";
        break;
    case Relative:
        if (selectedStr != "") name = std::string(_(getBodyShortName(selectedStr).c_str())) + " ";
        if (lo < 0) { lo = -lo; los = 'W'; }
        if (la < 0) { la = -la; las = 'S'; }
        sprintf(buff, "(%.1lf%c, %.1lf%c)", lo, los, la, las);
        name += buff;
        break;
    case Settings:
        name = _("Settings");
        break;
    }
}


std::string Url::getBodyShortName(const std::string& body) const
{
    std::string::size_type pos;
    if (body != "") {
        pos = body.rfind(":");
        if (pos != std::string::npos) return body.substr(pos+1);
        else return body;
    }
    return "";
}


std::map<std::string, std::string> Url::parseUrlParams(const std::string& url) const
{
    std::string::size_type pos, startName, startValue;
    std::map<std::string, std::string> params;

    pos = url.find("?");
    while (pos != std::string::npos) {
        startName = pos + 1;
        startValue = url.find("=", startName);
        pos = url.find("&", pos + 1);
        if (startValue != std::string::npos) {
             startValue++;
             if (pos != std::string::npos)
                 params[url.substr(startName, startValue - startName -1)] = decode_string(url.substr(startValue, pos - startValue));
             else
                 params[url.substr(startName, startValue - startName -1)] = decode_string(url.substr(startValue));
        }
    }

    return params;
}


std::string Url::getCoordSysName(ObserverFrame::CoordinateSystem mode) const
{
    switch (mode)
    {
    case ObserverFrame::Universal:
        return "Freeflight";
    case ObserverFrame::Ecliptical:
        return "Follow";
    case ObserverFrame::BodyFixed:
        return "SyncOrbit";
    case ObserverFrame::Chase:
        return "Chase";
    case ObserverFrame::PhaseLock:
        return "PhaseLock";
    case ObserverFrame::Equatorial:
        return "Unknown";
    case ObserverFrame::ObserverLocal:
        return "Unknown";
    }
    return "Unknown";
}


static std::string getBodyName(Universe* universe, Body* body)
{
    std::string name = body->getName();
    PlanetarySystem* parentSystem = body->getSystem();
    const Body* parentBody = NULL;

    if (parentSystem != NULL)
        parentBody = parentSystem->getPrimaryBody();
    else
        assert(0);
        // TODO: Figure out why the line below was added.
        //parentBody = body->getOrbitBarycenter();  

    while (parentBody != NULL)
    {
        name = parentBody->getName() + ":" + name;
        parentSystem = parentBody->getSystem();
        if (parentSystem == NULL)
            parentBody = NULL;
        else
            parentBody = parentSystem->getPrimaryBody();
    }

    if (body->getSystem()->getStar() != NULL)
    {
        name = universe->getStarCatalog()->getStarName(*(body->getSystem()->getStar())) + ":" + name;
    }

    return name;
}


std::string Url::getSelectionName(const Selection& selection) const
{
    Universe *universe = appCore->getSimulation()->getUniverse();

    switch (selection.getType())
    {
    case Selection::Type_Body:
        return getBodyName(universe, selection.body());

    case Selection::Type_Star:
        return universe->getStarCatalog()->getStarName(*selection.star());

    case Selection::Type_DeepSky:
        return universe->getDSOCatalog()->getDSOName(selection.deepsky());

    case Selection::Type_Location:
        {
            std::string name = selection.location()->getName();
            Body* parentBody = selection.location()->getParentBody();
            if (parentBody != NULL)
                name = getBodyName(universe, parentBody) + ":" + name;
            return name;
        }

    default:
        return "";
    }
}


void Url::goTo()
{
    Selection sel;

    if (urlStr == "")
        return;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();
    std::string::size_type pos;

    sim->update(0.0);

    switch(type) {
    case Absolute:// Intentional Fall-Through
    case Relative:
        sim->setFrame(ref.getCoordinateSystem(), ref.getRefObject(), ref.getTargetObject());
        sim->getActiveObserver()->setFOV(degToRad(fieldOfView));
        appCore->setZoomFromFOV();
        sim->setTimeScale(timeScale);
        sim->setPauseState(pauseState);
        appCore->setLightDelayActive(lightTimeDelay);

        if (selectedStr != "")
        {
            pos = 0;
            while(pos != std::string::npos)
            {
                pos = selectedStr.find(":", pos + 1);
                if (pos != std::string::npos) selectedStr[pos]='/';
            }
            sel = sim->findObjectFromPath(selectedStr);
            sim->setSelection(sel);
        }
        else
        {
            sim->setSelection(Selection());
        }

        if (trackedStr != "")
        {
            pos = 0;
            while(pos != std::string::npos)
            {
                pos = trackedStr.find(":", pos + 1);
                if (pos != std::string::npos) trackedStr[pos]='/';
            }
            sel = sim->findObjectFromPath(trackedStr);
            sim->setTrackedObject(sel);
        }
        else
        {
            if (!sim->getTrackedObject().empty())
                sim->setTrackedObject(Selection());
        }
        // Intentional Fall-Through
    case Settings:
        renderer->setRenderFlags(renderFlags);
        renderer->setLabelMode(labelMode);
        break;
    }

    switch(type) {
    case Absolute:
        sim->setTime((double) date);
        sim->setObserverPosition(coord);
        sim->setObserverOrientation(orientation);
        break;
    case Relative:
        sim->gotoSelectionLongLat(0, astro::kilometersToLightYears(distance), (float) (longitude * PI / 180), (float) (latitude * PI / 180), Vec3f(0, 1, 0));
        break;
    case Settings:
        break;
    }
}


Url::~Url()
{
}


std::string Url::decode_string(const std::string& str)
{
    std::string::size_type a=0, b;
    std::string out = "";

    b = str.find("%");
    while (b != std::string::npos)
    {
        unsigned int c;
        out += str.substr(a, b-a);
        std::string c_code = str.substr(b+1, 2);
        sscanf(c_code.c_str(), "%02x", &c);
        out += c;
        a = b + 3;
        b = str.find("%", a);
    }
    out += str.substr(a);

    return out;
}





