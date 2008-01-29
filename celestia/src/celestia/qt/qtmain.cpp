/***************************************************************************
                          qtmain.cpp  -  description
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

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QBitmap>
#include <vector>
#include "qtappwin.h"

using namespace std;


static const char *description = "Celestia";

// Command line options
static bool startFullscreen = false;
static bool runOnce = false;
static QString startURL;
static QString startDirectory;
static QString startScript;
static QStringList extrasDirectories;
static QString configFileName;
static bool useAlternateConfigFile = false;
static bool skipSplashScreen = false;

static bool ParseCommandLine();


int main(int argc, char *argv[])
{  
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setApplicationName("Celestia");

    ParseCommandLine();

    QPixmap pixmap("splash.png");
    QSplashScreen splash(pixmap);
    splash.setMask(pixmap.mask());

    // Disabled for now until issues with pixmap alpha channel
    // are resolved
    //splash.show();

    CelestiaAppWindow window;

    // Connect the splash screen to the main window so that it
    // can receive progress notifications as Celestia files required
    // for startup are loaded.
    QObject::connect(&window, SIGNAL(progressUpdate(const QString&, int, const QColor&)),
                     &splash, SLOT(showMessage(const QString&, int, const QColor&)));

    window.init(configFileName, extrasDirectories);
    window.show();

    splash.finish(&window);

    return app.exec();
}



static void CommandLineError(const char*)
{
}



bool ParseCommandLine()
{
    QStringList args = QCoreApplication::arguments();

    int i = 1;

    while (i < args.size())
    {
        bool isLastArg = (i == args.size() - 1);
#if 0
        if (strcmp(argv[i], "--verbose") == 0)
        {
            SetDebugVerbosity(1);
        }
        else
#endif
        if (args.at(i) == "--fullscreen")
        {
            startFullscreen = true;
        }
        else if (args.at(i) == "--once")
        {
            runOnce = true;
        }
        else if (args.at(i) == "--dir")
        {
            if (isLastArg)
            {
                CommandLineError("Directory expected after --dir");
                return false;
            }
            i++;
            startDirectory = args.at(i);
        }
        else if (args.at(i) == "--conf")
        {
            if (isLastArg)
            {
                CommandLineError("Configuration file name expected after --conf");
                return false;
            }
            i++;
            configFileName = args.at(i);
            useAlternateConfigFile = true;
        }
        else if (args.at(i) == "--extrasdir")
        {
            if (isLastArg)
            {
                CommandLineError("Directory expected after --extrasdir");
                return false;
            }
            i++;
            extrasDirectories.push_back(args.at(i));
        }
        else if (args.at(i) == "-u" || args.at(i) == "--url")
        {
            if (isLastArg)
            {
                CommandLineError("URL expected after --url");
                return false;
            }
            i++;
            startURL = args.at(i);
        }
        else if (args.at(i) == "-s" || args.at(i) == "--nosplash")
        {
            skipSplashScreen = true;
        }
        else
        {
            char* buf = new char[args.at(i).length() + 256];
            sprintf(buf, "Invalid command line option '%s'", args.at(i).toUtf8().data());
            CommandLineError(buf);
            delete[] buf;
            return false;
        }

        i++;
    }

    return true;
}
