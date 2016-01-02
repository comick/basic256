/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/



#include <iostream>
#include <stdio.h>

#include <QString>
#include <QMutex>
#include <QWaitCondition>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QDesktopWidget>

using namespace std;

#include "PauseButton.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"

// global mymutexes and timers
QMutex* mymutex;
QMutex* mydebugmutex;
QWaitCondition* waitCond;
QWaitCondition* waitDebugCond;


// the three main components of the UI (define globally)
MainWindow * mainwin;
BasicEdit * editwin;
BasicOutput * outwin;
BasicGraph * graphwin;
VariableWin * varwin;

// other variables that objects use "extern"
int currentKey;	// last non input key press.


MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f)
    :	QMainWindow(parent, f) {
    SETTINGS;
    bool v;

    mainwin = this;

    // create the global mymutexes and waits
    mymutex = new QMutex(QMutex::NonRecursive);
    mydebugmutex = new QMutex(QMutex::NonRecursive);
    waitCond = new QWaitCondition();
    waitDebugCond = new QWaitCondition();

    setWindowIcon(QIcon(":/images/basic256.png"));

    editwin = new BasicEdit();
    editwin->setObjectName( "editor" );
    editwinwgt = new BasicWidget();
    editwinwgt->setViewWidget(editwin);
    connect(editwin, SIGNAL(changeStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
    connect(editwin, SIGNAL(changeWindowTitle(QString)), this, SLOT(updateWindowTitle(QString)));

    outwin = new BasicOutput();
    outwin->setObjectName( "output" );
    outwin->setReadOnly(true);
    outwinwgt = new BasicWidget(QObject::tr("Text Output"));
    outwinwgt->setViewWidget(outwin);
    outdock = new DockWidget();
    outdock->setObjectName( "tdock" );
    outdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    outdock->setWidget(outwinwgt);
    outdock->setWindowTitle(QObject::tr("Text Output"));
    connect(editwin, SIGNAL(changeWindowTitle(QString)), this, SLOT(updateWindowTitle(QString)));

    graphwin = new BasicGraph();
    graphwin->setObjectName( "goutput" );
    graphwinwgt = new BasicWidget(QObject::tr("Graphics Output"));
    graphwinwgt->setViewWidget(graphwin);
    graphscroll = new QScrollArea();
	graphscroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
 	graphscroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	graphscroll->setWidget(graphwinwgt);
    graphdock = new DockWidget();
    graphdock->setObjectName( "graphdock" );
    graphdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    graphdock->setWidget(graphscroll);
    graphdock->setWindowTitle(QObject::tr("Graphics Output"));

    varwin = new VariableWin();
    varwinwgt = new BasicWidget(QObject::tr("Variable Watch"));
    varwinwgt->setViewWidget(varwin);
    vardock = new DockWidget();
    vardock->setObjectName( "vardock" );
    vardock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    vardock->setWidget(varwinwgt);
    vardock->setWindowTitle(QObject::tr("Variable Watch"));

    rc = new RunController();
    editsyntax = new EditSyntaxHighlighter(editwin->document());

    // Main window toolbar
    maintbar = new QToolBar();
    addToolBar(maintbar);

    // File menu
    filemenu = menuBar()->addMenu(QObject::tr("&File"));
    newact = filemenu->addAction(QIcon(":images/new.png"), QObject::tr("&New"));
    newact->setShortcut(Qt::Key_N + Qt::CTRL);
    openact = filemenu->addAction(QIcon(":images/open.png"), QObject::tr("&Open"));
    openact->setShortcut(Qt::Key_O + Qt::CTRL);
    saveact = filemenu->addAction(QIcon(":images/save.png"), QObject::tr("&Save"));
    saveact->setShortcut(Qt::Key_S + Qt::CTRL);
    saveasact = filemenu->addAction(QIcon(":images/saveas.png"), QObject::tr("Save &As"));
    saveasact->setShortcut(Qt::Key_S + Qt::CTRL + Qt::SHIFT);
    filemenu->addSeparator();
    printact = filemenu->addAction(QIcon(":images/print.png"), QObject::tr("&Print"));
    printact->setShortcut(Qt::Key_P + Qt::CTRL);
    filemenu->addSeparator();
    recentact[0] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[0]->setShortcut(Qt::Key_1 + Qt::CTRL);
    recentact[1] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[1]->setShortcut(Qt::Key_2 + Qt::CTRL);
    recentact[2] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[2]->setShortcut(Qt::Key_3 + Qt::CTRL);
    recentact[3] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[3]->setShortcut(Qt::Key_4 + Qt::CTRL);
    recentact[4] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[4]->setShortcut(Qt::Key_5 + Qt::CTRL);
    recentact[5] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[5]->setShortcut(Qt::Key_6 + Qt::CTRL);
    recentact[6] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[6]->setShortcut(Qt::Key_7 + Qt::CTRL);
    recentact[7] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[7]->setShortcut(Qt::Key_8 + Qt::CTRL);
    recentact[8] = filemenu->addAction(QIcon(":images/open.png"), QObject::tr(""));
    recentact[8]->setShortcut(Qt::Key_9 + Qt::CTRL);
    filemenu->addSeparator();
    exitact = filemenu->addAction(QIcon(":images/exit.png"), QObject::tr("&Exit"));
    exitact->setShortcut(Qt::Key_Q + Qt::CTRL);
    //
    showRecentList = true;
    QObject::connect(filemenu, SIGNAL(aboutToShow()), this, SLOT(updateRecent()));
    QObject::connect(newact, SIGNAL(triggered()), editwin, SLOT(newProgram()));
    QObject::connect(openact, SIGNAL(triggered()), editwin, SLOT(loadProgram()));
    QObject::connect(saveact, SIGNAL(triggered()), editwin, SLOT(saveProgram()));
    //QObject::connect(editwin, SIGNAL(textChanged()), saveact, SLOT(setEnabled()));
    //saveact->setEnabled(false);
    QObject::connect(saveasact, SIGNAL(triggered()), editwin, SLOT(saveAsProgram()));
    QObject::connect(printact, SIGNAL(triggered()), editwin, SLOT(slotPrint()));
    QObject::connect(recentact[0], SIGNAL(triggered()), editwin, SLOT(loadRecent0()));
    QObject::connect(recentact[1], SIGNAL(triggered()), editwin, SLOT(loadRecent1()));
    QObject::connect(recentact[2], SIGNAL(triggered()), editwin, SLOT(loadRecent2()));
    QObject::connect(recentact[3], SIGNAL(triggered()), editwin, SLOT(loadRecent3()));
    QObject::connect(recentact[4], SIGNAL(triggered()), editwin, SLOT(loadRecent4()));
    QObject::connect(recentact[5], SIGNAL(triggered()), editwin, SLOT(loadRecent5()));
    QObject::connect(recentact[6], SIGNAL(triggered()), editwin, SLOT(loadRecent6()));
    QObject::connect(recentact[7], SIGNAL(triggered()), editwin, SLOT(loadRecent7()));
    QObject::connect(recentact[8], SIGNAL(triggered()), editwin, SLOT(loadRecent8()));
    QObject::connect(exitact, SIGNAL(triggered()), this, SLOT(close()));

    // Edit menu
    editmenu = menuBar()->addMenu(QObject::tr("&Edit"));
    undoact = editmenu->addAction(QIcon(":images/undo.png"), QObject::tr("&Undo"));
    QObject::connect(editwin, SIGNAL(undoAvailable(bool)), undoact, SLOT(setEnabled(bool)));
    QObject::connect(undoact, SIGNAL(triggered()), editwin, SLOT(undo()));
    undoact->setShortcut(Qt::Key_U + Qt::CTRL);
    undoact->setEnabled(false);
    redoact = editmenu->addAction(QIcon(":images/redo.png"), QObject::tr("&Redo"));
    QObject::connect(editwin, SIGNAL(redoAvailable(bool)), redoact, SLOT(setEnabled(bool)));
    QObject::connect(redoact, SIGNAL(triggered()), editwin, SLOT(redo()));
    redoact->setShortcut(Qt::Key_R + Qt::CTRL);
    redoact->setEnabled(false);
    editmenu->addSeparator();
    cutact = editmenu->addAction(QIcon(":images/cut.png"), QObject::tr("Cu&t"));
    cutact->setShortcut(Qt::Key_X + Qt::CTRL);
    cutact->setEnabled(false);
    copyact = editmenu->addAction(QIcon(":images/copy.png"), QObject::tr("&Copy"));
    copyact->setShortcut(Qt::Key_C + Qt::CTRL);
    copyact->setEnabled(false);
    pasteact = editmenu->addAction(QIcon(":images/paste.png"), QObject::tr("&Paste"));
    pasteact->setShortcut(Qt::Key_P + Qt::CTRL);
    editmenu->addSeparator();
    selectallact = editmenu->addAction(QObject::tr("Select &All"));
    selectallact->setShortcut(Qt::Key_A + Qt::CTRL);
    editmenu->addSeparator();
    findact = editmenu->addAction(QObject::tr("&Find"));
    findact->setShortcut(Qt::Key_F + Qt::CTRL);
    findagain1 = new QShortcut(Qt::Key_F3, this);
    findagain2 = new QShortcut(Qt::Key_G + Qt::CTRL, this);
    replaceact = editmenu->addAction(QObject::tr("&Replace"));
    editmenu->addSeparator();
    beautifyact = editmenu->addAction(QObject::tr("&Beautify"));
    editmenu->addSeparator();
    prefact = editmenu->addAction(QIcon(":images/preferences.png"), QObject::tr("Preferences"));
    //
    QObject::connect(cutact, SIGNAL(triggered()), editwin, SLOT(cut()));
    QObject::connect(copyact, SIGNAL(triggered()), editwin, SLOT(copy()));
    QObject::connect(editwin, SIGNAL(copyAvailable(bool)), rc, SLOT(mainWindowEnableCopy(bool)));
    QObject::connect(pasteact, SIGNAL(triggered()), editwin, SLOT(paste()));
    QObject::connect(selectallact, SIGNAL(triggered()), editwin, SLOT(selectAll()));
    QObject::connect(findact, SIGNAL(triggered()), rc, SLOT(showFind()));
    QObject::connect(findagain1, SIGNAL(activated()), rc, SLOT(findAgain()));
    QObject::connect(findagain2, SIGNAL(activated()), rc, SLOT(findAgain()));
    QObject::connect(replaceact, SIGNAL(triggered()), rc, SLOT(showReplace()));
    QObject::connect(beautifyact, SIGNAL(triggered()), editwin, SLOT(beautifyProgram()));
    QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));

    bool extraSepAdded = false;
    if (outwinwgt->usesMenu()) {
        editmenu->addSeparator();
        extraSepAdded = true;
        editmenu->addMenu(outwinwgt->getMenu());
    }
    if (graphwinwgt->usesMenu()) {
        if (!extraSepAdded) {
            editmenu->addSeparator();
        }
        editmenu->addMenu(graphwinwgt->getMenu());
    }

    // View menuBar
    viewmenu = menuBar()->addMenu(QObject::tr("&View"));
    editWinVisibleAct = viewmenu->addAction(QObject::tr("&Edit Window"));
    textWinVisibleAct = viewmenu->addAction(QObject::tr("&Text Window"));
    graphWinVisibleAct = viewmenu->addAction(QObject::tr("&Graphics Window"));
    variableWinVisibleAct = viewmenu->addAction(QObject::tr("&Variable Watch Window"));
    editWinVisibleAct->setCheckable(true);
    textWinVisibleAct->setCheckable(true);
    graphWinVisibleAct->setCheckable(true);
    variableWinVisibleAct->setCheckable(true);
    v = settings.value(SETTINGSVISIBLE, true).toBool();
    editWinVisibleAct->setChecked(v);
    editwinwgt->setVisible(v);
    v = settings.value(SETTINGSOUTVISIBLE, true).toBool();
    textWinVisibleAct->setChecked(v);
    outdock->setVisible(v);
    v = settings.value(SETTINGSGRAPHVISIBLE, true).toBool();
    graphWinVisibleAct->setChecked(v);
    graphdock->setVisible(v);
    v = settings.value(SETTINGSVARVISIBLE, false).toBool();
    variableWinVisibleAct->setChecked(v);
    vardock->setVisible(v);

    QObject::connect(editWinVisibleAct, SIGNAL(toggled(bool)), editwinwgt, SLOT(setVisible(bool)));
    QObject::connect(textWinVisibleAct, SIGNAL(toggled(bool)), outdock, SLOT(setVisible(bool)));
    QObject::connect(graphWinVisibleAct, SIGNAL(toggled(bool)), graphdock, SLOT(setVisible(bool)));
    QObject::connect(variableWinVisibleAct, SIGNAL(toggled(bool)), vardock, SLOT(setVisible(bool)));

    // Editor and Output font and Editor settings
    viewmenu->addSeparator();
    fontact = viewmenu->addAction(QObject::tr("&Font"));
    QObject::connect(fontact, SIGNAL(triggered()), rc, SLOT(dialogFontSelect()));
    editWhitespaceAct = viewmenu->addAction(QObject::tr("Show &Whitespace Characters"));
    editWhitespaceAct->setCheckable(true);
    v = settings.value(SETTINGSEDITWHITESPACE, SETTINGSEDITWHITESPACEDEFAULT).toBool();
    editWhitespaceAct->setChecked(v);
    editwin->slotWhitespace(v);
    QObject::connect(editWhitespaceAct, SIGNAL(toggled(bool)), editwin, SLOT(slotWhitespace(bool)));

    // Graphics Grid Lines
    viewmenu->addSeparator();
    graphGridVisibleAct = viewmenu->addAction(QObject::tr("Graphics Window Grid &Lines"));
    graphGridVisibleAct->setCheckable(true);
    v = settings.value(SETTINGSGRAPHGRIDLINES, false).toBool();
    graphGridVisibleAct->setChecked(v);
    graphwin->slotGridLines(v);
    QObject::connect(graphGridVisibleAct, SIGNAL(toggled(bool)), graphwin, SLOT(slotGridLines(bool)));

    // Toolbars
    viewmenu->addSeparator();
    QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
    QAction *maintbaract = viewtbars->addAction(QObject::tr("&Main"));
    maintbaract->setCheckable(true);
    v = settings.value(SETTINGSTOOLBAR, true).toBool();
    maintbaract->setChecked(v);
    maintbar->setVisible(v);
    QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
    if (outwinwgt->usesToolBar()) {
        QAction *texttbaract = viewtbars->addAction(QObject::tr("&Text Output"));
        texttbaract->setCheckable(true);
        v = settings.value(SETTINGSOUTTOOLBAR, false).toBool();
        texttbaract->setChecked(v);
        outwinwgt->slotShowToolBar(v);
        QObject::connect(texttbaract, SIGNAL(toggled(bool)), outwinwgt, SLOT(slotShowToolBar(const bool)));
    }
    if (graphwinwgt->usesToolBar()) {
        QAction *graphtbaract = viewtbars->addAction(QObject::tr("&Graphics Output"));
        graphtbaract->setCheckable(true);
        v = settings.value(SETTINGSGRAPHTOOLBAR, false).toBool();
        graphtbaract->setChecked(v);
        graphwinwgt->slotShowToolBar(v);
        QObject::connect(graphtbaract, SIGNAL(toggled(bool)), graphwinwgt, SLOT(slotShowToolBar(const bool)));
    }

    // Run menu
    runmenu = menuBar()->addMenu(QObject::tr("&Run"));
    runact = runmenu->addAction(QIcon(":images/run.png"), QObject::tr("&Run"));
    runact->setShortcut(Qt::Key_F5);
    editmenu->addSeparator();
    debugact = runmenu->addAction(QIcon(":images/debug.png"), QObject::tr("&Debug"));
    debugact->setShortcut(Qt::Key_F5 + Qt::CTRL);
    stepact = runmenu->addAction(QIcon(":images/step.png"), QObject::tr("S&tep"));
    stepact->setShortcut(Qt::Key_F11);
    stepact->setEnabled(false);
    bpact = runmenu->addAction(QIcon(":images/break.png"), QObject::tr("Run &to"));
    bpact->setShortcut(Qt::Key_F11 + Qt::CTRL);
    bpact->setEnabled(false);
    stopact = runmenu->addAction(QIcon(":images/stop.png"), QObject::tr("&Stop"));
    stopact->setShortcut(Qt::Key_F5 + Qt::SHIFT);
    stopact->setEnabled(false);
    //runmenu->addSeparator();
    //QAction *saveByteCode = runmenu->addAction(QObject::tr("Save Compiled &Byte Code"));
    QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
    QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
    QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
    QObject::connect(bpact, SIGNAL(triggered()), rc, SLOT(stepBreakPoint()));
    QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
    //QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

    // Help menu
    QMenu *helpmenu = menuBar()->addMenu(QObject::tr("&Help"));
#if defined(WIN32PORTABLE) || defined(ANDROID)
    // in portable or android make doc online and context help online
    QAction *onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help"));
    onlinehact->setShortcut(Qt::Key_F1);
    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
    QShortcut* helpthis = new QShortcut(Qt::Key_F1 + Qt::SHIFT, this);
    QObject::connect(helpthis, SIGNAL(activated()), rc, SLOT(showOnlineContextDocumentation()));
#else
    // in installed mode make doc offline and online and context help offline
    QAction *docact = helpmenu->addAction(QIcon(":images/help.png"), QObject::tr("&Help"));
    docact->setShortcut(Qt::Key_F1);
    QObject::connect(docact, SIGNAL(triggered()), rc, SLOT(showDocumentation()));
    QShortcut* helpthis = new QShortcut(Qt::Key_F1 + Qt::SHIFT, this);
    QObject::connect(helpthis, SIGNAL(activated()), rc, SLOT(showContextDocumentation()));
    QAction *onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help"));
    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
#endif
    helpmenu->addSeparator();
    QAction *aboutact = helpmenu->addAction(QObject::tr("&About BASIC-256"));
    QObject::connect(aboutact, SIGNAL(triggered()), this, SLOT(about()));

    // Add actions to main window toolbar
    maintbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    maintbar->addAction(newact);
    maintbar->addAction(openact);
    maintbar->addAction(saveact);
    maintbar->addSeparator();
    maintbar->addAction(runact);
    maintbar->addAction(debugact);
    maintbar->addAction(stepact);
    maintbar->addAction(bpact);
    maintbar->addAction(stopact);
    maintbar->addSeparator();
    maintbar->addAction(undoact);
    maintbar->addAction(redoact);
    maintbar->addAction(cutact);
    maintbar->addAction(copyact);
    maintbar->addAction(pasteact);

    setCentralWidget(editwinwgt);
    addDockWidget(Qt::RightDockWidgetArea, outdock);
    addDockWidget(Qt::RightDockWidgetArea, graphdock);
    addDockWidget(Qt::LeftDockWidgetArea, vardock);
    setContextMenuPolicy(Qt::NoContextMenu);

    // position where the docks and main window were last on screen
    // unless the position is off the screen
    // NOT android - Android - FULL screen

#ifndef ANDROID

    QDesktopWidget *screen = QApplication::desktop();
    QPoint l;
    QSize z;
    bool floating;

    // move the main window to it's previous location and size
    // mainwindow may not be here absoutely as the docked widgets sizes may force 
    // qt to move the window and increase size
    
    // if position is off of the screen then return to default location
    l = settings.value(SETTINGSPOS, QPoint(SETTINGSDEFAULT_X, SETTINGSDEFAULT_Y)).toPoint();
    if (l.x() >= screen->width() || l.x() <= 0 ) l.setX(SETTINGSDEFAULT_X);
    if (l.y() >= screen->height() || l.y() <= 0 ) l.setY(SETTINGSDEFAULT_Y);
    move(l);
    // get last size and if it was larger then the screen then reduce to fit
    z = settings.value(SETTINGSSIZE, QSize(SETTINGSDEFAULT_W, SETTINGSDEFAULT_H)).toSize();
    if (z.width() >= screen->width()-l.x()) z.setWidth(screen->width()-l.x());
    if (z.height() >= screen->height()-l.y()) z.setHeight(screen->height()-l.y());
    resize(z);

    // position and size the graphics dock and basicgraph
    floating = settings.value(SETTINGSGRAPHFLOAT, false).toBool();
    graphdock->setFloating(floating);
    if (floating) {
        l = settings.value(SETTINGSGRAPHPOS, QPoint(SETTINGSGRAPHDEFAULT_X, SETTINGSGRAPHDEFAULT_Y)).toPoint();
        if (l.x() >= screen->width() || l.x() <= 0 ) l.setX(SETTINGSGRAPHDEFAULT_X);
        if (l.y() >= screen->height() || l.y() <= 0 ) l.setY(SETTINGSGRAPHDEFAULT_Y);
        graphdock->move(l);
	}
	z = settings.value(SETTINGSGRAPHSIZE, QSize(SETTINGSGRAPHDEFAULT_W, SETTINGSGRAPHDEFAULT_H)).toSize();
    if (z.width() >= screen->width()-l.x()) z.setWidth(screen->width()-l.x());
    if (z.height() >= screen->height()-l.y()) z.setHeight(screen->height()-l.y());
    graphscroll->resize(z);
 
    // position the output text dock
    outdock->setFloating(settings.value(SETTINGSOUTFLOAT, false).toBool());
    if (settings.contains(SETTINGSOUTPOS)) {
        l = settings.value(SETTINGSOUTPOS, QPoint(SETTINGSOUTDEFAULT_X, SETTINGSOUTDEFAULT_Y)).toPoint();
        if (l.x() >= screen->width() || l.x() <= 0 ) l.setX(SETTINGSOUTDEFAULT_X);
        if (l.y() >= screen->height() || l.y() <= 0 ) l.setY(SETTINGSOUTDEFAULT_Y);
        outdock->move(l);
        z = settings.value(SETTINGSOUTSIZE, QSize(SETTINGSOUTDEFAULT_W, SETTINGSOUTDEFAULT_H)).toSize();
        if (z.width() >= screen->width()-l.x()) z.setWidth(screen->width()-l.x());
        if (z.height() >= screen->height()-l.y()) z.setHeight(screen->height()-l.y());
        outdock->resize(z);
    }

    // positon the variable dock
    vardock->setFloating(settings.value(SETTINGSVARFLOAT, false).toBool());
    if (settings.contains(SETTINGSVARPOS)) {
        l = settings.value(SETTINGSVARPOS, QPoint(SETTINGSVARDEFAULT_X, SETTINGSVARDEFAULT_Y)).toPoint();
        if (l.x() >= screen->width() || l.x() <= 0 ) l.setX(SETTINGSVARDEFAULT_X);
        if (l.y() >= screen->height() || l.y() <= 0 ) l.setY(SETTINGSVARDEFAULT_Y);
        vardock->move(l);
        z = settings.value(SETTINGSVARSIZE, QSize(SETTINGSVARDEFAULT_W, SETTINGSVARDEFAULT_H)).toSize();
        if (z.width() >= screen->width()-l.x()) z.setWidth(screen->width()-l.x());
        if (z.height() >= screen->height()-l.y()) z.setHeight(screen->height()-l.y());
        vardock->resize(z);
    }
#endif

    // set initial font
    QFont initialFont;
    QString initialFontString = settings.value(SETTINGSFONT,SETTINGSFONTDEFAULT).toString();
    if (initialFont.fromString(initialFontString)) {
        editwin->setFont(initialFont);
        outwin->setFont(initialFont);
    }


}

MainWindow::~MainWindow() {
    //printf("mwdestroy\n");
    delete rc;
    delete editsyntax;
    delete mymutex;
    delete waitCond;
    delete editwin;
    delete outwin;
    delete graphwin;
    delete maintbar;
    
}

void MainWindow::about() {
    QString title;
    QString message;

#ifdef ANDROID
    // android does not have webkit dialogs - make plain text
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Android");
    message = QObject::tr("BASIC-256") + QObject::tr(" Android") + "\n" +
              QObject::tr("version ") +  VERSION + QObject::tr(" - built with QT ") + QT_VERSION_STR + "\n" +
              QObject::tr("Copyright &copy; 2006-2016, The BASIC-256 Team") + "\n" +
              QObject::tr("Please visit our web site at http://www.basic256.org for tutorials and documentation.") + "\n" +
              QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.") + "\n" +
              QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.");
#else
    // webkit dialogs for all platforms except android
#ifdef WIN32PORTABLE
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Portable");
    message = "<h2>" + QObject::tr("BASIC-256") + QObject::tr(" Portable") + "</h2>";
#else
    title = QObject::tr("About BASIC-256");
    message = "<h2>" + QObject::tr("BASIC-256") + "</h2>";
#endif	// WIN32PORTABLE

    message += QObject::tr("version ") + "<b>" + VERSION + "</b>" + QObject::tr(" - built with QT ") + "<b>" + QT_VERSION_STR + "</b>" +
               "<p>" + QObject::tr("Copyright &copy; 2006-2016, The BASIC-256 Team") + "</p>" +
               "<p>" + QObject::tr("Please visit our web site at <a href=\"http://www.basic256.org\">http://www.basic256.org</a> for tutorials and documentation.") + "</p>" +
               "<p>" + QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.")  + "</p>" +
               "<p><i>" + QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.")  + "</i></p>";
#endif

    QMessageBox::about(this, title, message);
}

void MainWindow::updateRecent() {
    //update recent list on file menu
    if (showRecentList) {
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        for (int i=0; i<9; i++) {
            QString fn = settings.value(QString::number(i), "").toString();
            QString path = QDir::currentPath() + "/";
            if (QString::compare(path, fn.left(path.length()))==0) {
                fn = fn.right(fn.length()-path.length());
            }
            recentact[i]->setEnabled(fn.length()!=0 && newact->isEnabled());
            recentact[i]->setVisible(fn.length()!=0);
            recentact[i]->setText("&" + QString::number(i+1) + " - " + fn);
        }
        settings.endGroup();
    } else {
        for (int i=0; i<9; i++) {
            recentact[i]->setEnabled(false);
            recentact[i]->setVisible(false);
        }
    }
}

void MainWindow::loadAndGoMode() {
    //disable everything except what is needed to quit, stop and run a program.
    // called by Main when -r option is sent
    editWinVisibleAct->setChecked(false);
    newact->setVisible(false);
    openact->setVisible(false);
    saveact->setVisible(false);
    saveasact->setVisible(false);
    printact->setVisible(false);
    showRecentList = false;
    editmenu->setTitle("");
    editmenu->setVisible(false);
    undoact->setVisible(false);
    redoact->setVisible(false);
    cutact->setVisible(false);
    copyact->setVisible(false);
    pasteact->setVisible(false);
    editWinVisibleAct->setVisible(false);
    variableWinVisibleAct->setVisible(false);
    debugact->setVisible(false);
    stepact->setVisible(false);
    bpact->setVisible(false);

    runact->activate(QAction::Trigger);
}

void MainWindow::closeEvent(QCloseEvent *e) {
    // quit the application but ask if there are unsaved changes in buffer
    bool doquit = true;
    if (editwin->codeChanged) {
        QMessageBox msgBox;
        msgBox.setText(tr("Program modifications have not been saved."));
        msgBox.setInformativeText(tr("Do you want to discard your changes?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        doquit = (msgBox.exec() == QMessageBox::Yes);
    }
    if (doquit) {
        // actually quitting
        e->accept();
        // save current screen posision, visibility and floating
        // if we ae not android
#ifndef ANDROID
        SETTINGS;
        settings.setValue(SETTINGSVISIBLE, isVisible());
        settings.setValue(SETTINGSSIZE, size());
        settings.setValue(SETTINGSPOS, pos());
        settings.setValue(SETTINGSTOOLBAR, maintbar->isVisible());
        settings.setValue(SETTINGSOUTVISIBLE, outdock->isVisible());
        settings.setValue(SETTINGSOUTFLOAT, outdock->isFloating());
        settings.setValue(SETTINGSOUTSIZE, outdock->size());
        settings.setValue(SETTINGSOUTPOS, outdock->pos());
        settings.setValue(SETTINGSOUTTOOLBAR, outwinwgt->isVisibleToolBar());
        settings.setValue(SETTINGSEDITWHITESPACE, editWhitespaceAct->isChecked());
		settings.setValue(SETTINGSGRAPHVISIBLE, graphdock->isVisible());
        settings.setValue(SETTINGSGRAPHFLOAT, graphdock->isFloating());
        settings.setValue(SETTINGSGRAPHSIZE, graphwinwgt->size());
        settings.setValue(SETTINGSGRAPHPOS, graphdock->pos());
        settings.setValue(SETTINGSGRAPHTOOLBAR, graphwinwgt->isVisibleToolBar());
        settings.setValue(SETTINGSGRAPHGRIDLINES, graphwin->isVisibleGridLines());
        settings.setValue(SETTINGSVARVISIBLE, vardock->isVisible());
        settings.setValue(SETTINGSVARFLOAT, vardock->isFloating());
        settings.setValue(SETTINGSVARSIZE, vardock->size());
        settings.setValue(SETTINGSVARPOS, vardock->pos());
#endif

    } else {
        // not quitting
        e->ignore();
    }


}

void MainWindow::updateStatusBar(QString status) {
    statusBar()->showMessage(status);
}

void MainWindow::updateWindowTitle(QString title) {
    setWindowTitle(title);
}



