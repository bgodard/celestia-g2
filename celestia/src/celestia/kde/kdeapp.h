/***************************************************************************
                          kdeapp.h  -  description
                             -------------------
    begin                : Tue Jul 16 22:28:19 CEST 2002
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDE_MAIN_WINDOW_H
#define KDE_MAIN_WINDOW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <qwidget.h>
#include <kmainwindow.h>
#include <kaction.h>
#include <qvaluestack.h>
#include <kurl.h>

#include <kbookmarkmanager.h>
#include "kcelbookmarkowner.h"
#include "celestiacore.h"
#include "kdeglwidget.h"
#include "celengine/render.h"


class KdeApp;

class KdeWatcher : public CelestiaWatcher {
public:
    KdeWatcher(CelestiaCore* core, KdeApp* app) : kdeapp(app),CelestiaWatcher(*core) {};
    virtual void notifyChange(CelestiaCore*, int);
protected:
    KdeApp *kdeapp;
};


class KdeApp : public KMainWindow, virtual public KCelBookmarkOwner
{
Q_OBJECT
friend void KdeWatcher::notifyChange(CelestiaCore*, int);

public:
    /** construtor */
    KdeApp(QWidget* parent=0, const char *name=0);
    /** destructor */
    ~KdeApp() {};

    QString getOpenGLInfo();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void setStartURL(KURL url);
    void goToURL(KURL url);

    void openBookmarkURL(const QString& _url);
    QString currentTitle() const ;
    QString currentURL() const ;
    QString currentIcon() const;

public slots:
    void celestia_tick() { appCore->tick(); glWidget->updateGL(); }
    void slotFileOpen();
    void slotFileOpenRecent(const KURL&);
    void slotNull();
    void slotClose();
    void slotHome();
    void slotZoomIn();
    void slotZoomOut();
    void slotToggleToolbar();
    void slotToggleMenubar();
    void slotConfigureToolbars();
    void slotNewToolbarConfig();
    void slotKeyBindings();
    void slotOpenGLInfo();
    void slotPreferences();
    void slotSetTimeNow();
    void slotAccelerateTime();
    void slotSlowDownTime();
    void slotPauseTime();
    void slotReverseTime();

    void slotShowStars();
    void slotShowPlanets();
    void slotShowGalaxies();
    void slotShowDiagrams();
    void slotShowCloudMaps();
    void slotShowOrbits();
    void slotShowCelestialSphere();
    void slotShowNightMaps();
    void slotShowAtmospheres();
    void slotShowSmoothLines();
    void slotShowEclipseShadows();
    void slotShowStarsAsPoints();
    void slotShowRingShadows();
    void slotShowStarLabels();
    void slotShowPlanetLabels();
    void slotShowMoonLabels();
    void slotShowConstellationLabels();
    void slotShowGalaxyLabels();
    void slotShowAsteroidLabels();
    void slotShowSpacecraftLabels();
    void slotAmbientLightLevel(int l);
    void slotFaintestVisible(int m);
    void slotHudDetail(int l);

    void slotDisplayLocalTime();
    void slotVertexShader();
    void slotPixelShader();
    void slotGrabImage();
    void slotFullScreen();

    void slotBack();
    void slotForward();
    void slotCopyUrl();
    void slotGoTo();

    void slotShowBookmarkBar();

    void slotOpenFileURL(const KURL& url);
    void slotBackAboutToShow();
    void slotBackActivated (int i);
    void slotForwardAboutToShow();
    void slotForwardActivated(int i);

    static void popupMenu(float x, float y, Selection sel);

protected:
    CelestiaCore* appCore;
    Renderer*   renderer;
    Simulation* sim;
    KdeGlWidget* glWidget;
    void initActions();
    KRecentFilesAction *openRecent;
    KToolBar *bookmarkBar;

    bool queryExit();
    bool queryClose();

    KToggleAction* toggleMenubar;
    KToggleAction* toggleToolbar;

    void resyncMenus();
    void resyncAmbient();
    void resyncFaintest();
    void resyncVerbosity();
    void resyncHistory();

    KdeWatcher *kdewatcher;

    KToolBarPopupAction *backAction, *forwardAction;
    static KdeApp* app;
};


#endif
