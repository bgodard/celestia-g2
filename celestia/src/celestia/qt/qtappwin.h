// qtappwin.cpp
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Main window for Celestia Qt front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTAPPWIN_H_
#define _QTAPPWIN_H_

#include <QMainWindow>


class QMenu;
class QDockWidget;
class CelestiaGlWidget;
class CelestialBrowser;
class CelestiaCore;

class CelestiaAppWindow : public QMainWindow
{
    Q_OBJECT

 public:
    CelestiaAppWindow();

    void writeSettings();
    void readSettings();

 public slots:
    void celestia_tick();

 private slots:
    void centerSelection();
    void gotoSelection();
    void selectSun();

    void slotClose();
    void slotPreferences();

 private:
    void createActions();
    void createMenus();

 private:
    CelestiaGlWidget* glWidget;
    QDockWidget *toolsDock;
    CelestialBrowser* celestialBrowser;

    CelestiaCore* appCore;

    QMenu* fileMenu;
    QMenu* navMenu;
    QMenu* timeMenu;
    QMenu* viewMenu;
};


#endif // _QTAPPWIN_H_
