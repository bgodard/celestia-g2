// render.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _RENDER_H_
#define _RENDER_H_

#include <vector>
#include <string>
#include "stardb.h"
#include "visstars.h"
#include "observer.h"
#include "solarsys.h"
#include "galaxy.h"
#include "asterism.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "console.h"
#include "selection.h"


class Renderer
{
 public:
    Renderer();
    ~Renderer();
    
    bool init(int, int);
    void shutdown() {};
    void resize(int, int);

    float getFieldOfView();
    void setFieldOfView(float);

    void setRenderMode(int);

    void render(const Observer&,
                const StarDatabase&,
                const VisibleStarSet&,
                SolarSystem*,
                GalaxyList*,
                const Selection& sel,
                double now);
    
    // Convert window coordinates to a ray for picking
    Vec3f getPickRay(int winX, int winY);

    Console* getConsole() const;

    enum {
        NoLabels = 0,
        StarLabels = 1,
        PlanetLabels = 2,
        PlanetOrbits = 4,
    };
    enum {
        ShowNothing     =  0,
        ShowStars       =  1,
        ShowPlanets     =  2,
        ShowGalaxies    =  4,
        ShowDiagrams    =  8,
        ShowCloudMaps   = 16,
    };
    int getRenderFlags() const;
    void setRenderFlags(int);
    int getLabelMode() const;
    void setLabelMode(int);
    void addLabelledStar(Star*);
    void clearLabelledStars();
    float getAmbientLightLevel() const;
    void setAmbientLightLevel(float);
    bool getPerPixelLighting() const;
    void setPerPixelLighting(bool);
    bool perPixelLightingSupported() const;

    float getBrightnessScale() const;
    void setBrightnessScale(float);
    float getBrightnessBias() const;
    void setBrightnessBias(float);

    void showAsterisms(AsterismList*);

    typedef struct {
        string text;
        Color color;
        Point3f position;
    } Label;

    void addLabel(string, Color, Point3f);
    void clearLabels();

 public:
    // Internal types
    // TODO: Figure out how to make these private.  Even with a friend
    // 
    struct Particle
    {
        Point3f center;
        float size;
        Color color;
        float pad0, pad1, pad2;
    };

    typedef struct _RenderListEntry
    {
        Star* star;
        Body* body;
        Point3f position;
        Vec3f sun;
        float distance;
        float discSizeInPixels;
        float appMag;

        bool operator<(const _RenderListEntry& r)
        {
            return distance < r.distance;
        }
    } RenderListEntry;

 private:
    void renderStars(const StarDatabase& starDB,
                     const VisibleStarSet& visset,
                     const Observer& observer);
    void renderGalaxies(const GalaxyList& galaxies,
                        const Observer& observer);
    void renderPlanetarySystem(const Star& sun,
                               const PlanetarySystem& solSystem,
                               const Observer& observer,
                               Mat4d& frame,
                               double now,
                               bool showLabels = false);
    void renderPlanet(const Body& body,
                      Point3f pos,
                      Vec3f sunDirection,
                      float distance,
                      float appMag,
                      double now,
                      Quatf orientation);
    void renderStar(const Star& star,
                    Point3f pos,
                    float distance,
                    float appMag,
                    Quatf orientation,
                    double now);
    void renderBodyAsParticle(Point3f center,
                              float appMag,
                              float discSizeInPixels,
                              Color color,
                              const Quatf& orientation,
                              bool useHaloes);
    void labelStars(const vector<Star*>& stars,
                    const StarDatabase& starDB,
                    const Observer& observer);
    void renderParticles(const vector<Particle>& particles,
                         Quatf orientation);
    void renderLabels();

    
 private:
    int windowWidth;
    int windowHeight;
    float fov;
    float pixelSize;

    TextureManager* textureManager;
    MeshManager* meshManager;
    Console* console;

    int renderMode;
    int labelMode;
    int renderFlags;
    float ambientLightLevel;
    bool perPixelLightingEnabled;
    float brightnessBias;
    float brightnessScale;

    vector<RenderListEntry> renderList;
    vector<Particle> starParticles;
    vector<Particle> glareParticles;
    vector<Particle> planetParticles;
    vector<Label> labels;

    vector<Star*> labelledStars;

    AsterismList* asterisms;

    double modelMatrix[16];
    double projMatrix[16];

    int nSimultaneousTextures;
    bool useRegisterCombiners;
    bool useCubeMaps;
    bool useCompressedTextures;

 public:
    friend bool operator<(const Renderer::RenderListEntry&,
                          const Renderer::RenderListEntry&);
};


#if 0
class Renderer::ParticleList
{
private:
    vector<Particle> particles;

public:
    ParticleList() {};
    void clear() { particles.clear(); };
    void addParticle(Particle& p) { particles.insert(particles.end(), p); };
    void render(Quatf orientation);
};


class Renderer::RenderList
{
private:
    vector<RenderListEntry> renderables;

public:
    RenderList() {};
    void clear() { renderables.clear(); };
    void add(Body*, Point3f, Vec3f, float);
    void render(double, Quatf, float, TextureManager*);

    friend bool operator<(RenderListEntry&, RenderListEntry&);
};
#endif


#endif // _RENDER_H_
