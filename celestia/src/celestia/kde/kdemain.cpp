/***************************************************************************
                          kdemain.cpp  -  description
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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "kdeuniquecelestia.h"

static const char *description =
    I18N_NOOP("Celestia");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE


static KCmdLineOptions options[] =
{ { "+url", I18N_NOOP("Start and go to url"), 0},
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{    
    KAboutData aboutData( "celestia", I18N_NOOP("Celestia"),
      VERSION, description, KAboutData::License_GPL,
      "(c) 2002, Chris Laurel", 0, "http://www.shatters.net/celestia/", "chris@teyssier.org");
    aboutData.addAuthor("Chris Laurel",0, "claurel@shatters.net");
    aboutData.addAuthor("Clint Weisbrod",0, "cweisbrod@adelphia.net");
    aboutData.addAuthor("Fridger Schrempp",0, "t00fri@mail.desy.de");
    aboutData.addAuthor("Bob Ippolito", "Mac OS X version", "bob@redivi.com");
    aboutData.addAuthor("Hank Ramsey", "Mac OS X version");
    aboutData.addAuthor("Christophe Teyssier", "KDE interface", "chris@teyssier.org");

    aboutData.addCredit("Frank Gregorio", "Celestia User's Guide");
    aboutData.addCredit("Hitoshi Suzuki", "Japanese README translation");
    aboutData.addCredit("Christophe Teyssier", "DocBook and HTML conversion of User's Guide", "chris@teyssier.org");
    aboutData.addCredit("Diego Rodriguez", "Acrobat conversion of User's Guide");

    aboutData.addCredit("Deon Ramsey", "Unix installer, GTK interface");
    aboutData.addCredit("Christophe André", "Eclipse finder and rendering of orbits", "kendrix@wanadoo.fr");
    aboutData.addCredit("Colin Walters", "Endianness fixes");
    aboutData.addCredit("Peter Chapman", "Orbit path rendering changes");
    aboutData.addCredit("James Holmes");
    aboutData.addCredit("Harald Schmidt", "Lua scripting enhancements, bug fixes");
  
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KUniqueApplication::addCmdLineOptions();

    KdeUniqueCelestia a;

    return a.exec();
}
