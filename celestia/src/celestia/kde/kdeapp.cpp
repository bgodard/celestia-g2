/***************************************************************************
                          kdeapp.cpp  -  description
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


#include <fstream.h>

#include <qpushbutton.h>
#include <qslider.h>
#include <qlayout.h>
#include <qframe.h>
#include <qgrid.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <qclipboard.h>
#include <qregexp.h>

#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include <qkeycode.h>
#include <qtimer.h>
#include <qimage.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmenubar.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <qtextedit.h>
#include <klineeditdlg.h>

#include <qdatetime.h>
#include <kshortcut.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kstdaccel.h>
#include <kpopupmenu.h>
#include <kcelbookmarkmenu.h>
#include <kbookmarkbar.h>
#include <kcelbookmarkmanager.h>

#include "kdeglwidget.h"
#include "kdeapp.h"
#include "kdepreferencesdialog.h"
#include "celengine/cmdparser.h"
#include "url.h"

extern "C" bool ExtensionSupported(char *ext);

KdeApp* KdeApp::app=0;

KBookmarkManager* KCelBookmarkManager::s_bookmarkManager;

KdeApp::KdeApp(QWidget *parent, const char *name) : KMainWindow(parent, name)
{
    appCore=new CelestiaCore();
    if (appCore == NULL)
    {
        cerr << "Out of memory.\n";
        exit(1);
    }
    
    kdewatcher = new KdeWatcher(appCore, this);

    renderer = appCore->getRenderer();
    sim = appCore->getSimulation();

    app=this;
    appCore->setContextMenuCallback(KdeApp::popupMenu);

    setAcceptDrops(true);

    // Create our OpenGL widget
    glWidget = new KdeGlWidget( this, "kdeglwidget", appCore);

    setCentralWidget(glWidget);
    initActions();

    glWidget->setFocus();

    resize(640,480);
    setAutoSaveSettings("MainWindow");
    KConfig* conf = kapp->config();
    applyMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    restoreWindowSize(conf);
    conf->setGroup(0);

    if (conf->hasGroup("Shortcuts"))
        actionCollection()->readShortcutSettings("Shortcuts", conf);

    if (toolBar()->isHidden()) toggleToolbar->setChecked(false);
    if (menuBar()->isHidden()) toggleMenubar->setChecked(false);

    // We use a timer with a null timeout value
    // to add appCore->tick to Qt's event loop
    QTimer *t = new QTimer( dynamic_cast<QObject *>(this) );
    QObject::connect( t, SIGNAL(timeout()), SLOT(celestia_tick()) );
    t->start( 0, FALSE );

}

void KdeApp::setStartURL(KURL url) {
    if (url.protocol() == "cel") { 
            appCore->setStartURL(url.url().latin1());
    }
}

void KdeApp::goToURL(KURL url) {
    if (url.protocol() == "cel")  {
        appCore->addToHistory();
        appCore->goToUrl(url.prettyURL().latin1());
    }
}

void KdeApp::openBookmarkURL(const QString& _url) {
    KURL url(_url);
    appCore->addToHistory();
    appCore->goToUrl(url.prettyURL().latin1());
}

QString KdeApp::currentTitle() const {
    Url url(appCore);
    return QString(url.getName().c_str());
}

QString KdeApp::currentURL() const {
    Url url(appCore);
    return QString(url.getAsString().c_str());
}

QString KdeApp::currentIcon() const {
    QDateTime now = QDateTime::currentDateTime();
    QString iconName = QString::fromLatin1("favicons/celestia_") + now.toString("yyyyMMddhhmmsszzz");
    QString iconFilename = locateLocal("cache", iconName) + ".png";

    QImage grabedImage = glWidget->grabFrameBuffer();
    int width=grabedImage.width(), height=grabedImage.height();
    if (width > height) {
        grabedImage.copy((width - height)/2, 0, height, height).smoothScale(64,64).save(iconFilename, "PNG");
    } else {
        grabedImage.copy(0, (height - width)/2, width, width).smoothScale(64,64).save(iconFilename, "PNG");
    }

    return iconName;
}

void KdeWatcher::notifyChange(CelestiaCore * core, int property)
{
    if ((property & (CelestiaCore::RenderFlagsChanged|
                     CelestiaCore::LabelFlagsChanged|
                     CelestiaCore::TimeZoneChanged)))
        kdeapp->resyncMenus();
    else if (property & CelestiaCore::AmbientLightChanged)
        kdeapp->resyncAmbient();
    else if (property & CelestiaCore::FaintestChanged)
        kdeapp->resyncFaintest();
    else if (property & CelestiaCore::VerbosityLevelChanged)
        kdeapp->resyncVerbosity();
    else if (property & CelestiaCore::HistoryChanged)
        kdeapp->resyncHistory();
}

void KdeApp::resyncHistory() {
    std::vector<Url> history=appCore->getHistory();
    std::vector<Url>::size_type i=appCore->getHistoryCurrent();

    if (i >= history.size()-1) {
        action("go_forward")->setEnabled(false);
    } else {
        action("go_forward")->setEnabled(true);
    }
    if (i == 0) {
        action("go_back")->setEnabled(false);
    } else {
        action("go_back")->setEnabled(true);
    } 
}

void KdeApp::resyncMenus() {
    int rFlags = renderer->getRenderFlags();
    ((KToggleAction*)action("showStars"))->setChecked(rFlags & Renderer::ShowStars);
    ((KToggleAction*)action("showPlanets"))->setChecked(rFlags & Renderer::ShowPlanets);
    ((KToggleAction*)action("showGalaxies"))->setChecked(rFlags & Renderer::ShowGalaxies);
    ((KToggleAction*)action("showDiagrams"))->setChecked(rFlags & Renderer::ShowDiagrams);
    ((KToggleAction*)action("showCloudMaps"))->setChecked(rFlags & Renderer::ShowCloudMaps);
    ((KToggleAction*)action("showOrbits"))->setChecked(rFlags & Renderer::ShowOrbits);
    ((KToggleAction*)action("showCelestialSphere"))->setChecked(rFlags & Renderer::ShowCelestialSphere);
    ((KToggleAction*)action("showNightMaps"))->setChecked(rFlags & Renderer::ShowNightMaps);
    ((KToggleAction*)action("showAtmospheres"))->setChecked(rFlags & Renderer::ShowAtmospheres);
    ((KToggleAction*)action("showSmoothLines"))->setChecked(rFlags & Renderer::ShowSmoothLines);
    ((KToggleAction*)action("showEclipseShadows"))->setChecked(rFlags & Renderer::ShowEclipseShadows);
    ((KToggleAction*)action("showStarsAsPoints"))->setChecked(rFlags & Renderer::ShowStarsAsPoints);
    ((KToggleAction*)action("showRingShadows"))->setChecked(rFlags & Renderer::ShowRingShadows);
    ((KToggleAction*)action("showBoundaries"))->setChecked(rFlags & Renderer::ShowBoundaries);
    ((KToggleAction*)action("showAutoMag"))->setChecked(rFlags & Renderer::ShowAutoMag);
    ((KToggleAction*)action("showCometTails"))->setChecked(rFlags & Renderer::ShowCometTails);

    int lMode = renderer->getLabelMode();
    ((KToggleAction*)action("showStarLabels"))->setChecked(lMode & Renderer::StarLabels);
    ((KToggleAction*)action("showPlanetLabels"))->setChecked(lMode & Renderer::PlanetLabels);
    ((KToggleAction*)action("showMoonLabels"))->setChecked(lMode & Renderer::MoonLabels);
    ((KToggleAction*)action("showConstellationLabels"))->setChecked(lMode & Renderer::ConstellationLabels);
    ((KToggleAction*)action("showGalaxyLabels"))->setChecked(lMode & Renderer::GalaxyLabels);
    ((KToggleAction*)action("showAsteroidLabels"))->setChecked(lMode & Renderer::AsteroidLabels);
    ((KToggleAction*)action("showSpacecraftLabels"))->setChecked(lMode & Renderer::SpacecraftLabels);
}

void KdeApp::resyncAmbient() {
}
void KdeApp::resyncFaintest() {
}
void KdeApp::resyncVerbosity() {
}

void KdeApp::initActions()
{
    KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
    openRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
    openRecent->loadEntries(KGlobal::config());
    connect(openRecent, SIGNAL(urlSelected(const KURL&)), SLOT(slotOpenFileURL(const KURL&)));

    KStdAction::close(this, SLOT(slotClose()), actionCollection());

    new KAction(i18n("Go to..."), 0, 0, this, SLOT(slotGoTo()), actionCollection(), "go_to");

    backAction = new KToolBarPopupAction( i18n("&Back"), "back",
                                           KStdAccel::key(KStdAccel::Back), this, SLOT( slotBack() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Back ) );
    forwardAction = new KToolBarPopupAction( i18n("&Forward"), "forward",
                                           KStdAccel::key(KStdAccel::Forward), this, SLOT( slotForward() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Forward ) );
    connect( backAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotBackAboutToShow() ) );
    connect( backAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotBackActivated( int ) ) );
    connect( forwardAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotForwardAboutToShow() ) );
    connect( forwardAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotForwardActivated( int ) ) );
    KStdAction::home(this, SLOT(slotHome()), actionCollection());
    action("go_forward")->setEnabled(false);
    action("go_back")->setEnabled(false);

    KStdAction::zoomIn(this, SLOT(slotZoomIn()), actionCollection());
    KStdAction::zoomOut(this, SLOT(slotZoomOut()), actionCollection());

    KActionMenu *bookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark", actionCollection(), "bookmarks" );
    new KCelBookmarkMenu( KCelBookmarkManager::self(), this,
                                     bookmarks->popupMenu(), actionCollection(), true, true );


    KStdAction::preferences(this, SLOT(slotPreferences()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());
    KStdAction::keyBindings(this, SLOT(slotKeyBindings()), actionCollection());

    new KAction(i18n("Full Screen"), "window_fullscreen", 0, this, SLOT(slotFullScreen()), actionCollection(), "fullScreen");
    new KAction(i18n("Copy URL"), "edit_copy", 0, this, SLOT(slotCopyUrl()), actionCollection(), "copyUrl");


    new KAction(i18n("Set Time to Now"), "player_eject", 0, this, SLOT(slotSetTimeNow()), actionCollection(), "setTimeNow");
    new KAction(i18n("Accelerate Time"), "1uparrow", 0, this, SLOT(slotAccelerateTime()), actionCollection(), "accelerateTime");
    new KAction(i18n("Slow Down Time"), "1downarrow", 0, this, SLOT(slotSlowDownTime()), actionCollection(), "slowDownTime");
    new KAction(i18n("Pause Time"), "player_pause", 0, this, SLOT(slotPauseTime()), actionCollection(), "pauseTime");
    new KAction(i18n("Reverse Time"), "flipv", 0, this, SLOT(slotReverseTime()), actionCollection(), "reverseTime");

    int rFlags, lMode;
    bool isLocal = true;                   
    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("RendererFlags"))
        rFlags = KGlobal::config()->readNumEntry("RendererFlags");
    else rFlags = appCore->getRenderer()->getRenderFlags();

    if (KGlobal::config()->hasKey("LabelMode"))
        lMode = KGlobal::config()->readNumEntry("LabelMode");
    else lMode = appCore->getRenderer()->getLabelMode();

    if (KGlobal::config()->hasKey("TimeZoneBias"))
        isLocal = (KGlobal::config()->readNumEntry("TimeZoneBias") != 0);

    KToggleAction* showStars = new KToggleAction(i18n("Show Stars"), 0, this, SLOT(slotShowStars()), actionCollection(), "showStars");
    showStars->setChecked(rFlags & Renderer::ShowStars);

    KToggleAction* showPlanets = new KToggleAction(i18n("Show Planets"), 0, this, SLOT(slotShowPlanets()), actionCollection(), "showPlanets");
    showPlanets->setChecked(rFlags & Renderer::ShowPlanets);

    KToggleAction* showGalaxies = new KToggleAction(i18n("Show Galaxies"), 0, this, SLOT(slotShowGalaxies()), actionCollection(), "showGalaxies");
    showGalaxies->setChecked(rFlags & Renderer::ShowGalaxies);

    KToggleAction* showDiagrams = new KToggleAction(i18n("Show Constellations"), 0, this, SLOT(slotShowDiagrams()), actionCollection(), "showDiagrams");
    showDiagrams->setChecked(rFlags & Renderer::ShowDiagrams);

    KToggleAction* showCloudMaps = new KToggleAction(i18n("Show CloudMaps"), 0, this, SLOT(slotShowCloudMaps()), actionCollection(), "showCloudMaps");
    showCloudMaps->setChecked(rFlags & Renderer::ShowCloudMaps);

    KToggleAction* showOrbits = new KToggleAction(i18n("Show Orbits"), 0, this, SLOT(slotShowOrbits()), actionCollection(), "showOrbits");
    showOrbits->setChecked(rFlags & Renderer::ShowOrbits);

    KToggleAction* showCelestialSphere = new KToggleAction(i18n("Show Celestial Grid"), 0, this, SLOT(slotShowCelestialSphere()), actionCollection(), "showCelestialSphere");
    showCelestialSphere->setChecked(rFlags & Renderer::ShowCelestialSphere);

    KToggleAction* showNightMaps = new KToggleAction(i18n("Show Night Side Lights"), 0, this, SLOT(slotShowNightMaps()), actionCollection(), "showNightMaps");
    showNightMaps->setChecked(rFlags & Renderer::ShowNightMaps);

    KToggleAction* showAtmospheres = new KToggleAction(i18n("Show Atmospheres"), 0, this, SLOT(slotShowAtmospheres()), actionCollection(), "showAtmospheres");
    showAtmospheres->setChecked(rFlags & Renderer::ShowAtmospheres);

    KToggleAction* showSmoothLines = new KToggleAction(i18n("Show Smooth Orbit Lines"), 0, this, SLOT(slotShowSmoothLines()), actionCollection(), "showSmoothLines");
    showSmoothLines->setChecked(rFlags & Renderer::ShowSmoothLines);

    KToggleAction* showEclipseShadows = new KToggleAction(i18n("Show Eclipse Shadows"), 0, this, SLOT(slotShowEclipseShadows()), actionCollection(), "showEclipseShadows");
    showEclipseShadows->setChecked(rFlags & Renderer::ShowEclipseShadows);

    KToggleAction* showStarsAsPoints = new KToggleAction(i18n("Show Stars as Points"), 0, this, SLOT(slotShowStarsAsPoints()), actionCollection(), "showStarsAsPoints");
    showStarsAsPoints->setChecked(rFlags & Renderer::ShowStarsAsPoints);

    KToggleAction* showRingShadows = new KToggleAction(i18n("Show Ring Shadows"), 0, this, SLOT(slotShowRingShadows()), actionCollection(), "showRingShadows");
    showRingShadows->setChecked(rFlags & Renderer::ShowRingShadows);

    KToggleAction* showBoundaries = new KToggleAction(i18n("Show Boundaries"), 0, this, SLOT(slotShowBoundaries()), actionCollection(), "showBoundaries");
    showBoundaries->setChecked(rFlags & Renderer::ShowBoundaries);

    KToggleAction* showAutoMag = new KToggleAction(i18n("Auto Magnitudes"), 0, this, SLOT(slotShowAutoMag()), actionCollection(), "showAutoMag");
    showBoundaries->setChecked(rFlags & Renderer::ShowAutoMag);

    KToggleAction* showCometTails = new KToggleAction(i18n("Show Comet Tails"), 0, this, SLOT(slotShowCometTails()), actionCollection(), "showCometTails");
    showCometTails->setChecked(rFlags & Renderer::ShowCometTails);

    KToggleAction* showStarLabels = new KToggleAction(i18n("Show Star Labels"), 0, this, SLOT(slotShowStarLabels()), actionCollection(), "showStarLabels");
    showStarLabels->setChecked(lMode & Renderer::StarLabels);

    KToggleAction* showPlanetLabels = new KToggleAction(i18n("Show Planet Labels"), 0, this, SLOT(slotShowPlanetLabels()), actionCollection(), "showPlanetLabels");
    showPlanetLabels->setChecked(lMode & Renderer::PlanetLabels);

    KToggleAction* showMoonLabels = new KToggleAction(i18n("Show Moon Labels"), 0, this, SLOT(slotShowMoonLabels()), actionCollection(), "showMoonLabels");
    showMoonLabels->setChecked(lMode & Renderer::MoonLabels);

    KToggleAction* showConstellationLabels = new KToggleAction(i18n("Show Constellation Labels"), 0, this, SLOT(slotShowConstellationLabels()), actionCollection(), "showConstellationLabels");
    showConstellationLabels->setChecked(lMode & Renderer::ConstellationLabels);

    KToggleAction* showGalaxyLabels = new KToggleAction(i18n("Show Galaxy Labels"), 0, this, SLOT(slotShowGalaxyLabels()), actionCollection(), "showGalaxyLabels");
    showGalaxyLabels->setChecked(lMode & Renderer::GalaxyLabels);

    KToggleAction* showAsteroidLabels = new KToggleAction(i18n("Show Asteroid Labels"), 0, this, SLOT(slotShowAsteroidLabels()), actionCollection(), "showAsteroidLabels");
    showAsteroidLabels->setChecked(lMode & Renderer::AsteroidLabels);

    KToggleAction* showSpacecraftLabels = new KToggleAction(i18n("Show Spacecraft Labels"), 0, this, SLOT(slotShowSpacecraftLabels()), actionCollection(), "showSpacecraftLabels");
    showSpacecraftLabels->setChecked(lMode & Renderer::SpacecraftLabels);

    KToggleAction* displayLocalTime = new KToggleAction(i18n("Display Local Time"), 0, this, SLOT(slotDisplayLocalTime()), actionCollection(), "displayLocalTime");
    displayLocalTime->setChecked(isLocal);

    new KToggleAction(i18n("Enable Vertex Shader"), 0, this, SLOT(slotVertexShader()), actionCollection(), "vertexShader");
    new KToggleAction(i18n("Enable Pixel Shader"), 0, this, SLOT(slotPixelShader()), actionCollection(), "pixelShader");

    new KAction(i18n("Grab Image"), "filesave", 0, this, SLOT(slotGrabImage()), actionCollection(), "grabImage");

    new KAction(i18n("OpenGL info"), 0, this, SLOT(slotOpenGLInfo()),
                      actionCollection(), "opengl_info");

    toggleMenubar=KStdAction::showMenubar(this, SLOT(slotToggleMenubar()), actionCollection());
    toggleToolbar=KStdAction::showToolbar(this, SLOT(slotToggleToolbar()), actionCollection());

    KToggleAction *toggleBookmarkBar = new KToggleAction(i18n("Show Bookmark Toolbar"), 0, this,
        SLOT(slotShowBookmarkBar()), actionCollection(), "showBookmarkBar");

    createGUI();

    bookmarkBar = new KToolBar(this, QMainWindow::Top, true, "bookmarkBar");
    new KBookmarkBar( KCelBookmarkManager::self(), this, bookmarkBar, actionCollection(), this, "bookmarkBar");

    if (bookmarkBar->isHidden()) toggleBookmarkBar->setChecked(false);
    else toggleBookmarkBar->setChecked(true);
}

bool KdeApp::queryExit() { 
    KConfig* conf = kapp->config();
    saveMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    saveWindowSize(conf);
    conf->setGroup("Preferences");
    conf->writeEntry("RendererFlags", appCore->getRenderer()->getRenderFlags());
    conf->writeEntry("LabelMode", appCore->getRenderer()->getLabelMode());
    conf->writeEntry("AmbientLightLevel", appCore->getRenderer()->getAmbientLightLevel());
    conf->writeEntry("FaintestVisible", appCore->getSimulation()->getFaintestVisible());
    conf->writeEntry("HudDetail", appCore->getHudDetail());
    conf->writeEntry("TimeZoneBias", appCore->getTimeZoneBias());
    conf->writeEntry("VertexShader", appCore->getRenderer()->getVertexShaderEnabled());
    conf->writeEntry("PixelShader", appCore->getRenderer()->getFragmentShaderEnabled());
    conf->setGroup(0);
    actionCollection()->writeShortcutSettings("Shortcuts", conf);
    openRecent->saveEntries(KGlobal::config());
    return true;
}

bool KdeApp::queryClose() {
    KConfig* conf = kapp->config();
    saveMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    saveWindowSize(conf);
    conf->setGroup("Preferences");
    conf->writeEntry("RendererFlags", appCore->getRenderer()->getRenderFlags());
    conf->setGroup(0);
    return true;
}


void KdeApp::slotNull() {
    // dev only
}

void KdeApp::slotFullScreen() {
    static bool isFullScreen=false;

    if (isFullScreen) {
        showNormal();
        action("fullScreen")->setIcon("window_fullscreen");
    } else {
        showFullScreen();
        action("fullScreen")->setIcon("window_nofullscreen");
    }             
    isFullScreen = !isFullScreen;
}

void KdeApp::slotHome() {
    appCore->charEntered('h');
    appCore->charEntered('g');
}

void KdeApp::slotClose() {
    close();
}

void KdeApp::slotZoomIn() {
    if (renderer->getFieldOfView() > 0.01f)
        renderer->setFieldOfView(renderer->getFieldOfView() / 1.1f);
}

void KdeApp::slotZoomOut() {
    if (renderer->getFieldOfView() < 120.0f)
        renderer->setFieldOfView(renderer->getFieldOfView() * 1.1f);
}

void KdeApp::slotToggleToolbar() {
    if (toolBar()->isVisible()) toolBar()->hide();
    else toolBar()->show();
}

void KdeApp::slotToggleMenubar() {
    if (menuBar()->isVisible()) menuBar()->hide();
    else menuBar()->show();
}

void KdeApp::slotConfigureToolbars()
{
    saveMainWindowSettings( KGlobal::config(), "MainWindow" );
    KEditToolbar dlg(actionCollection());
    connect( &dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
    if (dlg.exec())
    {
        createGUI();
    }
}

void KdeApp::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
//    ...if you use any action list, use plugActionList on each here...
    applyMainWindowSettings( KGlobal::config(), "MainWindow" );
}

void KdeApp::slotKeyBindings()
{
    KKeyDialog dlg(false, this);
    dlg.insert(actionCollection());
    if (dlg.exec()) {
        dlg.commitChanges();
    }
}

void KdeApp::slotFileOpen()
{
    QString fileOpen = KFileDialog::getOpenFileName(0, "*.cel");
    if (fileOpen == "") return;

    slotOpenFileURL(KURL(fileOpen));
}

void KdeApp::slotOpenFileURL(const KURL& url) {
    QString file = url.directory(false) + url.fileName();

    ifstream scriptfile(file.latin1());
    if (!scriptfile.good()) {
        KMessageBox::error(this, "Error opening script file " + file);
        return;
    } else {
        CommandParser parser(scriptfile);
        CommandSequence* script = parser.parse();
        if (script == NULL) {
            const vector<string>* errors = parser.getErrors();
            const char* errorMsg = "";
            if (errors->size() > 0)
                errorMsg = (*errors)[0].c_str();
            KMessageBox::error(this, "Errors in script file " + file + "\n" + errorMsg);
            return;
        } else {
            openRecent->addURL(url);
            appCore->cancelScript();
            appCore->runScript(script);
        }
    }
}


QString KdeApp::getOpenGLInfo() {
    // Code grabbed from gtkmain.cpp
    char* vendor = (char*) glGetString(GL_VENDOR);
    char* render = (char*) glGetString(GL_RENDERER);
    char* version = (char*) glGetString(GL_VERSION);
    char* ext = (char*) glGetString(GL_EXTENSIONS);
    QString s;
    s = "Vendor : ";
    if (vendor != NULL)
        s += vendor;
    s += "\n";

    s += "Renderer : ";
    if (render != NULL)
        s += render;
    s += "\n";

    s += "Version : ";
    if (version != NULL)
        s += version;
    s += "\n";

    char buf[100];
    GLint simTextures = 1;
    if (ExtensionSupported("GL_ARB_multitexture"))
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
    sprintf(buf, "Max simultaneous textures: %d\n", simTextures);
    s += buf;

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    sprintf(buf, "Max texture size: %d\n\n", maxTextureSize);
    s += buf;

    s += "Supported Extensions:\n    ";
    if (ext != NULL)
    {
        QString extString(ext);
        unsigned int pos = extString.find(' ', 0);
        while (pos != string::npos)
        {
            extString.replace(pos, 1, "\n    ");
            pos = extString.find(' ', pos+5);
        }
        s += extString;
    }

    return s;
}

void KdeApp::slotOpenGLInfo() {
    KDialogBase dlg(this, "openglinfo", true, i18n("OpenGL Info"), KDialogBase::Ok);
    QTextEdit edit(&dlg);
    edit.append(getOpenGLInfo());
    edit.setFocusPolicy(QWidget::NoFocus);
    dlg.setMainWidget(&edit);
    dlg.resize(400,430);
    dlg.exec();
}

void KdeApp::slotPreferences() {
    KdePreferencesDialog dlg(this, appCore);

    dlg.exec();
}

void KdeApp::slotFileOpenRecent(const KURL&)
{

}

void KdeApp::slotReverseTime() {
    appCore->charEntered('j');
}

void KdeApp::slotAccelerateTime() {
    appCore->charEntered('l');
}

void KdeApp::slotPauseTime() {
    appCore->charEntered(' ');
}

void KdeApp::slotSlowDownTime() {
    appCore->charEntered('k');
}

void KdeApp::slotSetTimeNow() {
    QDateTime qdatetime=QDateTime::currentDateTime();
    qdatetime=qdatetime.addSecs(timezone-3600*daylight);

    QDate qdate=qdatetime.date();
    QTime qtime=qdatetime.time();

    astro::Date date(0.0);
    date.year=qdate.year();
    date.month=qdate.month();
    date.day=qdate.day();
    date.hour=qtime.hour();
    date.minute=qtime.minute();
    date.seconds=qtime.second();
    appCore->getSimulation()->setTime((double) date );
	appCore->getSimulation()->update(0.0);

}

void KdeApp::slotShowStars() {
    appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowStars);
}

void KdeApp::slotShowPlanets() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowPlanets);
}

void KdeApp::slotShowGalaxies() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowGalaxies);
}

void KdeApp::slotShowDiagrams() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowDiagrams);
}

void KdeApp::slotShowCloudMaps() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCloudMaps);
}

void KdeApp::slotShowOrbits() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowOrbits);
}

void KdeApp::slotShowCelestialSphere() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCelestialSphere);
}

void KdeApp::slotShowNightMaps() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowNightMaps);
}

void KdeApp::slotShowAtmospheres() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowAtmospheres);
}

void KdeApp::slotShowSmoothLines() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowSmoothLines);
}

void KdeApp::slotShowEclipseShadows() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowEclipseShadows);
}

void KdeApp::slotShowStarsAsPoints() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowStarsAsPoints);
}

void KdeApp::slotShowRingShadows() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowRingShadows);
}

void KdeApp::slotShowBoundaries() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowBoundaries);
}

void KdeApp::slotShowAutoMag() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowAutoMag);
}

void KdeApp::slotShowCometTails() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCometTails);
}

void KdeApp::slotShowStarLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::StarLabels);
}

void KdeApp::slotShowPlanetLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::PlanetLabels);
}

void KdeApp::slotShowMoonLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::MoonLabels);
}

void KdeApp::slotShowConstellationLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::ConstellationLabels);
}

void KdeApp::slotShowGalaxyLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::GalaxyLabels);
}

void KdeApp::slotShowAsteroidLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::AsteroidLabels);
}

void KdeApp::slotShowSpacecraftLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::SpacecraftLabels);
}

void KdeApp::slotAmbientLightLevel(int l) {
    appCore->getRenderer()->setAmbientLightLevel(l / 100.);
}

void KdeApp::slotFaintestVisible(int m) {
    appCore->getSimulation()->setFaintestVisible(m);
}

void KdeApp::slotHudDetail(int l) {
    appCore->setHudDetail(l);
}

void KdeApp::slotDisplayLocalTime() {
    if (appCore->getTimeZoneBias()) {
        appCore->setTimeZoneBias(0);
        appCore->setTimeZoneName(i18n("UTC").latin1());
    } else {
        appCore->setTimeZoneBias(-timezone+3600*daylight);
        appCore->setTimeZoneName(tzname[daylight?0:1]);
    }
}

void KdeApp::slotVertexShader() {
    if (!appCore->getRenderer()->vertexShaderSupported()) return;
    appCore->getRenderer()->setVertexShaderEnabled(!appCore->getRenderer()->getVertexShaderEnabled());
}

void KdeApp::slotPixelShader() {
    if (!appCore->getRenderer()->fragmentShaderSupported()) return;
    appCore->getRenderer()->setFragmentShaderEnabled(!appCore->getRenderer()->getFragmentShaderEnabled());
}

void KdeApp::slotGrabImage() {
    QString saveAsName = KFileDialog::getSaveFileName(0, "*.png");
    if (saveAsName != "") {
        QImage grabedImage = glWidget->grabFrameBuffer();
        grabedImage.save(saveAsName, "PNG");
    }
}

void KdeApp::slotShowBookmarkBar() {
    if (bookmarkBar->isVisible()) bookmarkBar->hide();
    else bookmarkBar->show();
}

void KdeApp::slotBack() {
    appCore->back();
}

void KdeApp::slotForward() {
    appCore->forward();
}

void KdeApp::slotCopyUrl() {
    Url url(appCore);
    static QClipboard *cb = QApplication::clipboard();
    cb->setText(url.getAsString().c_str());
}

void KdeApp::slotGoTo() {
    KLineEditDlg dlg(i18n("Enter URL"), "", this);

    if (dlg.exec()) {
        appCore->addToHistory();
        appCore->goToUrl(dlg.text().latin1());
    }
}

void KdeApp::dragEnterEvent(QDragEnterEvent* event) {
    KURL::List urls;
    event->accept(KURLDrag::canDecode(event) && KURLDrag::decode(event, urls) && urls.first().protocol() == "cel");
}

void KdeApp::dropEvent(QDropEvent* event) {
    KURL::List urls;
    if (KURLDrag::decode(event, urls) && urls.first().protocol() == "cel") {
        appCore->addToHistory();
        appCore->goToUrl(urls.first().url().latin1());
    }
}

void KdeApp::slotBackAboutToShow() {
    int i; 
    KPopupMenu* menu = backAction->popupMenu(); 
    std::vector<Url>::size_type current = appCore->getHistoryCurrent();
    int pos;
    std::vector<Url> history = appCore->getHistory(); 

    menu->clear(); 
    for (i=0, pos = current - 1 ; pos >= 0 && i < 15 ; pos--, i++) {
        menu->insertItem(QString(history[pos].getName().c_str()), pos);
    }
}

void KdeApp::slotBackActivated(int i) {
    appCore->setHistoryCurrent(i);
}

void KdeApp::slotForwardAboutToShow() {
    int i;
    KPopupMenu* menu = forwardAction->popupMenu();
    std::vector<Url>::size_type current = appCore->getHistoryCurrent();
    int pos;
    std::vector<Url> history = appCore->getHistory();

    menu->clear();
    for (i=0, pos = current + 1 ; pos < history.size() && i < 15 ; pos++, i++) {
        menu->insertItem(QString(history[pos].getName().c_str()), pos);
    }
}

void KdeApp::slotForwardActivated(int i) {
    appCore->setHistoryCurrent(i);
}

void KdeApp::popupMenu(float x, float y, Selection sel) {
    KPopupMenu popup(app);
    const PlanetarySystem* planets = 0;
    if (sel.body != NULL)
    {
        popup.insertTitle(sel.body->getName().c_str(), 0, 0);
        popup.insertItem(i18n("&Goto"), 1, 1);
        popup.insertItem(i18n("&Fallow"), 2, 2);
        popup.insertItem(i18n("S&ynch Orbit"), 3, 3);

        const PlanetarySystem* satellites = sel.body->getSatellites();
        planets = satellites;
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            popup.insertSeparator(4);
            KPopupMenu *planetaryMenu = new KPopupMenu(app);
            for (int i = 0; i < satellites->getSystemSize(); i++)
            {
                Body* body = satellites->getBody(i);
                planetaryMenu->insertItem(body->getName().c_str(), 10+i);
            }
            popup.insertItem(i18n("Satellites"), planetaryMenu, 5, 5);
        }
    }
    else if (sel.star != NULL)
    {
        Simulation *sim = app->appCore->getSimulation();
        std::string name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star);
        popup.insertTitle(name.c_str(), 0, 0);
        popup.insertItem(i18n("&Goto"), 1, 1);
                               
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            popup.insertSeparator(4);
            SolarSystem* solarSys = iter->second;
            KPopupMenu* planetsMenu = new KPopupMenu(app);
            planets = solarSys->getPlanets();
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                planetsMenu->insertItem(body->getName().c_str(), 10+i);
            }
            popup.insertItem(i18n("Planets"), planetsMenu, 5, 5);
        }               
    }
    else if (sel.galaxy != NULL)
    {
        popup.insertTitle(sel.galaxy->getName().c_str(), 0, 0);
        popup.insertItem(i18n("&Goto"), 1, 1);
    }

    int id=popup.exec(app->glWidget->mapToGlobal(QPoint(int(x),int(y))));

    if (id == 1) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('g');
        return;
    }
    if (id == 2) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('f');
        return;
    }
    if (id == 3) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('y');
        return;
    }

    if (id >= 10 && planets != 0) { // Planet or Satellite.
        Selection new_sel(planets->getBody(id - 10));
        app->appCore->getSimulation()->setSelection(new_sel);
        app->appCore->charEntered('g');
        return;
    }
}

