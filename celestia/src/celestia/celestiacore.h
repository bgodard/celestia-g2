// celestiacore.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIACORE_H_
#define _CELESTIACORE_H_

// #include "gl.h"
#include <celengine/solarsys.h>
#include <celengine/overlay.h>
#include <celengine/command.h>
#include <celengine/execution.h>
#include <celengine/texture.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include "configfile.h"
#include "favorites.h"
#include "destination.h"


class CelestiaCore
{
 public:
    enum {
        LeftButton   = 0x01,
        MiddleButton = 0x02,
        RightButton  = 0x04,
        ShiftKey     = 0x08,
        ControlKey   = 0x10,
    };

    enum {
        Joy_XAxis           = 0,
        Joy_YAxis           = 1,
        Joy_ZAxis           = 2,
    };

    enum {
        JoyButton1          = 0,
        JoyButton2          = 1,
        JoyButton3          = 2,
        JoyButton4          = 3,
        JoyButton5          = 4,
        JoyButton6          = 5,
        JoyButton7          = 6,
        JoyButton8          = 7,
        JoyButtonCount      = 8,
    };

    enum {
        Key_Left            =  1,
        Key_Right           =  2,
        Key_Up              =  3,
        Key_Down            =  4,
        Key_Home            =  5,
        Key_End             =  6,
        Key_PageUp          =  7,
        Key_PageDown        =  8,
        Key_Insert          =  9,
        Key_Delete          = 10,
        Key_F1              = 11,
        Key_F2              = 12,
        Key_F3              = 13,
        Key_F4              = 14,
        Key_F5              = 15,
        Key_F6              = 16,
        Key_F7              = 17,
        Key_F8              = 18,
        Key_F9              = 19,
        Key_F10             = 20,
        Key_NumPadDecimal   = 21,
        Key_NumPad0         = 22,
        Key_NumPad1         = 23,
        Key_NumPad2         = 24,
        Key_NumPad3         = 25,
        Key_NumPad4         = 26,
        Key_NumPad5         = 27,
        Key_NumPad6         = 28,
        Key_NumPad7         = 29,
        Key_NumPad8         = 30,
        Key_NumPad9         = 31,
        KeyCount            = 128,
    };

    typedef void (*ContextMenuFunc)(float, float, Selection);

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation();
    bool initRenderer();
    void start(double t);

    // event processing methods
    void charEntered(char);
    void keyDown(int);
    void keyUp(int);
    void mouseWheel(float, int);
    void mouseButtonDown(float, float, int);
    void mouseButtonUp(float, float, int);
    void mouseMove(float, float, int);
    void joystickAxis(int axis, float amount);
    void joystickButton(int button, bool down);
    void resize(int w, int h);
    void draw();
    void tick(double dt);

    Simulation* getSimulation() const;
    Renderer* getRenderer() const;
    void showText(std::string s);

    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(std::string);
    const FavoritesList* getFavorites();

    const DestinationList* getDestinations();

    int getTimeZoneBias() const;
    void setTimeZoneBias(int);

    int getHudDetail();

    void setContextMenuCallback(ContextMenuFunc);

    class Alerter
    {
    public:
        virtual ~Alerter() {};
        virtual void fatalError(const std::string&) = 0;
    };

    void setAlerter(Alerter*);

 private:
    void cancelScript();
    bool readStars(const CelestiaConfig&);
    void setFaintest(float);
    void renderOverlay();
    void fatalError(const std::string&);

 private:
    CelestiaConfig* config;
    StarDatabase* starDB;
    SolarSystemCatalog* solarSystemCatalog;
    GalaxyList* galaxies;
    AsterismList* asterisms;

    FavoritesList* favorites;
    DestinationList* destinations;

    Simulation* sim;
    Renderer* renderer;
    Overlay* overlay;
    int width;
    int height;

    TextureFont* font;
    TextureFont* titleFont;
    std::string messageText;
    std::string typedText;
    bool textEnterMode;
    int hudDetail;
    bool wireframe;
    bool editMode;

    CommandSequence* currentScript;
    CommandSequence* initScript;
    CommandSequence* demoScript;
    Execution* runningScript;
    ExecutionEnvironment* execEnv;

    int timeZoneBias;      // diff in seconds between local time and GMT

    // Frame rate counter variables
    bool showFPSCounter;
    int nFrames;
    double fps;
    double fpsCounterStartTime;

    float mouseMotion;
    double dollyMotion;
    double dollyTime;
    double zoomMotion;
    double zoomTime;

    double currentTime;
    double timeScale;
    bool paused;

    Vec3f joystickRotation;
    bool joyButtonsPressed[JoyButtonCount];
    bool keysPressed[KeyCount];
    double KeyAccel;

    ContextMenuFunc contextMenuCallback;

    Texture* logoTexture;

    Alerter* alerter;
};

#endif // _CELESTIACORE_H_
