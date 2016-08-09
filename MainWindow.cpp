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

// key press information from outwin and graphwin
int lastKey;
std::list<int> pressedKeys;



MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring)
    :	QMainWindow(parent, f) {

	localecode = localestring;
	locale = new QLocale(localecode);

    undoButtonValue = false;
    redoButtonValue = false;
	guiState = GUISTATENORMAL;
	
    mainwin = this;

    // create the global mymutexes and waits
    mymutex = new QMutex(QMutex::NonRecursive);
    mydebugmutex = new QMutex(QMutex::NonRecursive);
    waitCond = new QWaitCondition();
    waitDebugCond = new QWaitCondition();

    setWindowIcon(QIcon(":/images/basic256.png"));

    editwin = new BasicEdit();
    editwin->setObjectName( "editor" );
    editwinwgt = new BasicWidget(QObject::tr("Program Editor"));
    editwinwgt->setViewWidget(editwin);
    connect(editwin, SIGNAL(changeStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
    connect(editwin, SIGNAL(changeWindowTitle(QString)), this, SLOT(updateWindowTitle(QString)));
    editdock = new DockWidget();
    editdock->setObjectName( "editdock" );
    editdock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    editdock->setWidget(editwinwgt);
    editdock->setWindowTitle(QObject::tr("Program Editor"));

    outwin = new BasicOutput();
    outwin->setObjectName( "output" );
    outwin->setReadOnly(true);
    outwinwgt = new BasicWidget(QObject::tr("Text Output"));
    outwinwgt->setViewWidget(outwin);
    outdock = new DockWidget();
    outdock->setObjectName( "tdock" );
    outdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    outdock->setWidget(outwinwgt);
    outdock->setWindowTitle(QObject::tr("Text Output"));
 
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
    graphdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    graphdock->setWidget(graphscroll);
    graphdock->setWindowTitle(QObject::tr("Graphics Output"));

    varwin = new VariableWin();
    varwinwgt = new BasicWidget(QObject::tr("Variable Watch"));
    varwinwgt->setViewWidget(varwin);
    vardock = new DockWidget();
    vardock->setObjectName( "vardock" );
    vardock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    vardock->setWidget(varwinwgt);
    vardock->setWindowTitle(QObject::tr("Variable Watch"));

    setCentralWidget(editdock);
    addDockWidget(Qt::RightDockWidgetArea, outdock);
    addDockWidget(Qt::RightDockWidgetArea, graphdock);
    addDockWidget(Qt::LeftDockWidgetArea, vardock);
    setContextMenuPolicy(Qt::NoContextMenu);

    rc = new RunController();
    editsyntax = new EditSyntaxHighlighter(editwin->document());

    // Main window toolbar
    maintbar = new QToolBar();
    addToolBar(maintbar);

    // File menu
    filemenu = menuBar()->addMenu(QObject::tr("&File"));
    newact = filemenu->addAction(QIcon(":images/new.png"), QObject::tr("&New"));
    newact->setShortcuts(QKeySequence::keyBindings(QKeySequence::New));
    openact = filemenu->addAction(QIcon(":images/open.png"), QObject::tr("&Open"));
    openact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Open));
    saveact = filemenu->addAction(QIcon(":images/save.png"), QObject::tr("&Save"));
    saveact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Save));
    saveasact = filemenu->addAction(QIcon(":images/saveas.png"), QObject::tr("Save &As..."));
    saveasact->setShortcuts(QKeySequence::keyBindings(QKeySequence::SaveAs));
    filemenu->addSeparator();
    printact = filemenu->addAction(QIcon(":images/print.png"), QObject::tr("&Print..."));
    printact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));
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
    exitact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Quit));
    //
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
    QObject::connect(editwin, SIGNAL(undoAvailable(bool)), this, SLOT(slotUndoAvailable(bool)));
    QObject::connect(undoact, SIGNAL(triggered()), editwin, SLOT(undo()));
    undoact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Undo));
    undoact->setEnabled(false);
    redoact = editmenu->addAction(QIcon(":images/redo.png"), QObject::tr("&Redo"));
    QObject::connect(editwin, SIGNAL(redoAvailable(bool)), this, SLOT(slotRedoAvailable(bool)));
    QObject::connect(redoact, SIGNAL(triggered()), editwin, SLOT(redo()));
    redoact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Redo));
    redoact->setEnabled(false);
    editmenu->addSeparator();
    cutact = editmenu->addAction(QIcon(":images/cut.png"), QObject::tr("Cu&t"));
    cutact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Cut));
    cutact->setEnabled(false);
    copyact = editmenu->addAction(QIcon(":images/copy.png"), QObject::tr("&Copy"));
    copyact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Copy));
    copyact->setEnabled(false);
    pasteact = editmenu->addAction(QIcon(":images/paste.png"), QObject::tr("&Paste"));
    pasteact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Paste));
    editmenu->addSeparator();
    selectallact = editmenu->addAction(QObject::tr("Select &All"));
    selectallact->setShortcuts(QKeySequence::keyBindings(QKeySequence::SelectAll));
    editmenu->addSeparator();
    findact = editmenu->addAction(QObject::tr("&Find..."));
    findact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Find));
    findagain = editmenu->addAction(QObject::tr("Find &Next"));
    findagain->setShortcuts(QKeySequence::keyBindings(QKeySequence::FindNext));
    replaceact = editmenu->addAction(QObject::tr("&Replace..."));
    replaceact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Replace));
    editmenu->addSeparator();
    beautifyact = editmenu->addAction(QObject::tr("&Beautify"));
    editmenu->addSeparator();
    prefact = editmenu->addAction(QIcon(":images/preferences.png"), QObject::tr("Preferences..."));
    //
    QObject::connect(cutact, SIGNAL(triggered()), editwin, SLOT(cut()));
    QObject::connect(copyact, SIGNAL(triggered()), editwin, SLOT(copy()));
    QObject::connect(editwin, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyCutButtons(bool)));
    QObject::connect(pasteact, SIGNAL(triggered()), editwin, SLOT(paste()));
    QObject::connect(selectallact, SIGNAL(triggered()), editwin, SLOT(selectAll()));
    QObject::connect(findact, SIGNAL(triggered()), rc, SLOT(showFind()));
    QObject::connect(findagain, SIGNAL(triggered()), rc, SLOT(findAgain()));
    QObject::connect(replaceact, SIGNAL(triggered()), rc, SLOT(showReplace()));
    QObject::connect(beautifyact, SIGNAL(triggered()), editwin, SLOT(beautifyProgram()));
    QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updatePasteButton()));

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
    outWinVisibleAct = viewmenu->addAction(QObject::tr("&Text Window"));
    graphWinVisibleAct = viewmenu->addAction(QObject::tr("&Graphics Window"));
    variableWinVisibleAct = viewmenu->addAction(QObject::tr("&Variable Watch Window"));
    editWinVisibleAct->setCheckable(true);
    editWinVisibleAct->setChecked(SETTINGSEDITVISIBLEDEFAULT);
    editdock->setVisible(SETTINGSEDITVISIBLEDEFAULT);
    outWinVisibleAct->setCheckable(true);
    outWinVisibleAct->setChecked(SETTINGSOUTVISIBLEDEFAULT);
    outdock->setVisible(SETTINGSOUTVISIBLEDEFAULT);
    graphWinVisibleAct->setCheckable(true);
    graphWinVisibleAct->setChecked(SETTINGSGRAPHVISIBLEDEFAULT);
    graphdock->setVisible(SETTINGSGRAPHVISIBLEDEFAULT);
    variableWinVisibleAct->setCheckable(true);
    variableWinVisibleAct->setChecked(SETTINGSVARVISIBLEDEFAULT);
    vardock->setVisible(SETTINGSVARVISIBLEDEFAULT);

    QObject::connect(editWinVisibleAct, SIGNAL(toggled(bool)), editdock, SLOT(setVisible(bool)));
    QObject::connect(outWinVisibleAct, SIGNAL(toggled(bool)), outdock, SLOT(setVisible(bool)));
    QObject::connect(graphWinVisibleAct, SIGNAL(toggled(bool)), graphdock, SLOT(setVisible(bool)));
    QObject::connect(variableWinVisibleAct, SIGNAL(toggled(bool)), vardock, SLOT(setVisible(bool)));

    QObject::connect(outdock, SIGNAL(visibilityChanged(bool)), this, SLOT(checkOutMenuVisible()));
    QObject::connect(graphdock, SIGNAL(visibilityChanged(bool)), this, SLOT(checkGraphMenuVisible()));
    QObject::connect(vardock, SIGNAL(visibilityChanged(bool)), this, SLOT(checkVarMenuVisible()));

    // Editor and Output font and Editor settings
    viewmenu->addSeparator();
    fontact = viewmenu->addAction(QObject::tr("&Font..."));
    QObject::connect(fontact, SIGNAL(triggered()), rc, SLOT(dialogFontSelect()));
    editWhitespaceAct = viewmenu->addAction(QObject::tr("Show &Whitespace Characters"));
    editWhitespaceAct->setCheckable(true);
    editWhitespaceAct->setChecked(SETTINGSEDITWHITESPACEDEFAULT);
    editwin->slotWhitespace(SETTINGSEDITWHITESPACEDEFAULT);
    QObject::connect(editWhitespaceAct, SIGNAL(toggled(bool)), editwin, SLOT(slotWhitespace(bool)));

    // Graphics Grid Lines
    viewmenu->addSeparator();
    graphGridVisibleAct = viewmenu->addAction(QObject::tr("Graphics Window Grid &Lines"));
    graphGridVisibleAct->setCheckable(true);
    graphGridVisibleAct->setChecked(SETTINGSGRAPHGRIDLINESDEFAUT);
    graphwin->slotGridLines(SETTINGSGRAPHGRIDLINESDEFAUT);
    QObject::connect(graphGridVisibleAct, SIGNAL(toggled(bool)), graphwin, SLOT(slotGridLines(bool)));

    // Toolbars
    viewmenu->addSeparator();
    QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
    maintbaract = viewtbars->addAction(QObject::tr("&Main"));
    maintbaract->setCheckable(true);
    maintbaract->setChecked(SETTINGSTOOLBARDEFAULT);
    maintbar->setVisible(SETTINGSTOOLBARDEFAULT);
    QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
    if (outwinwgt->usesToolBar()) {
        texttbaract = viewtbars->addAction(QObject::tr("&Text Output"));
        texttbaract->setCheckable(true);
        texttbaract->setChecked(SETTINGSOUTTOOLBARDEFAULT);
        outwinwgt->slotShowToolBar(SETTINGSOUTTOOLBARDEFAULT);
        QObject::connect(texttbaract, SIGNAL(toggled(bool)), outwinwgt, SLOT(slotShowToolBar(const bool)));
    }
    if (graphwinwgt->usesToolBar()) {
        graphtbaract = viewtbars->addAction(QObject::tr("&Graphics Output"));
        graphtbaract->setCheckable(true);
        graphtbaract->setChecked(SETTINGSGRAPHTOOLBARDEFAULT);
        graphwinwgt->slotShowToolBar(SETTINGSGRAPHTOOLBARDEFAULT);
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
    runmenu->addSeparator();
    clearbreakpointsact = runmenu->addAction(QObject::tr("&Clear all breakpoints"));
    //runmenu->addSeparator();
    //QAction *saveByteCode = runmenu->addAction(QObject::tr("Save Compiled &Byte Code"));
    QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
    QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
    QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
    QObject::connect(bpact, SIGNAL(triggered()), rc, SLOT(stepBreakPoint()));
    QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
    QObject::connect(clearbreakpointsact, SIGNAL(triggered()), editwin, SLOT(clearBreakPoints()));
    //QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

    // Help menu
    QMenu *helpmenu = menuBar()->addMenu(QObject::tr("&Help"));
#if defined(WIN32PORTABLE) || defined(ANDROID)
    // in portable or android make doc online and context help online
    QAction *onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help..."));
    onlinehact->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showOnlineContextDocumentation()));
    addAction (helpthis);
#else
    // in installed mode make doc offline and online and context help offline
    QAction *docact = helpmenu->addAction(QIcon(":images/help.png"), QObject::tr("&Help..."));
    docact->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    QObject::connect(docact, SIGNAL(triggered()), rc, SLOT(showDocumentation()));    
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showContextDocumentation()));
    addAction (helpthis);
    QAction *onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help..."));
    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
#endif
    helpmenu->addSeparator();
    QAction *aboutact = helpmenu->addAction(QObject::tr("&About BASIC-256..."));
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
        

	loadCustomizations();

}

void MainWindow::loadCustomizations() {
	// from settings - load the customizations to the screen
	
    SETTINGS;
    bool v;

    // View menuBar
    v = settings.value(SETTINGSEDITVISIBLE, SETTINGSEDITVISIBLEDEFAULT).toBool();
    editWinVisibleAct->setChecked(v);

    v = settings.value(SETTINGSOUTVISIBLE, SETTINGSOUTVISIBLEDEFAULT).toBool();
    outWinVisibleAct->setChecked(v);

    v = settings.value(SETTINGSGRAPHVISIBLE, SETTINGSGRAPHVISIBLEDEFAULT).toBool();
    graphWinVisibleAct->setChecked(v);

    v = settings.value(SETTINGSVARVISIBLE, SETTINGSVARVISIBLEDEFAULT).toBool();
    variableWinVisibleAct->setChecked(v);

    // Editor and Output font and Editor settings
    v = settings.value(SETTINGSEDITWHITESPACE, SETTINGSEDITWHITESPACEDEFAULT).toBool();
    editWhitespaceAct->setChecked(v);

    // Graphics Grid Lines
    v = settings.value(SETTINGSGRAPHGRIDLINES, SETTINGSGRAPHGRIDLINESDEFAUT).toBool();
    graphGridVisibleAct->setChecked(v);

    // Toolbars
    v = settings.value(SETTINGSTOOLBAR, SETTINGSTOOLBARDEFAULT).toBool();
    maintbaract->setChecked(v);
    v = settings.value(SETTINGSOUTTOOLBAR, SETTINGSOUTTOOLBARDEFAULT).toBool();
    texttbaract->setChecked(v);

    if (graphwinwgt->usesToolBar()) {
        v = settings.value(SETTINGSGRAPHTOOLBAR, SETTINGSGRAPHTOOLBARDEFAULT).toBool();
        graphtbaract->setChecked(v);
    }

    // position where the docks and main window were last on screen
    // unless the position is off the screen
    // NOT android - Android - FULL screen

#ifndef ANDROID

	QDesktopWidget *screen = QApplication::desktop();
	QPoint l, mainl;
	QSize z, mainz;
	bool floating;

	// move the main window to it's previous location and size
	// mainwindow may not be here absoutely as the docked widgets sizes may force 
	// qt to move the window and increase size

    // get last size and if it was larger then the screen then reduce to fit
    mainz = settings.value(SETTINGSSIZE, QSize(SETTINGSDEFAULT_W, SETTINGSDEFAULT_H)).toSize();
    mainl = settings.value(SETTINGSPOS, QPoint(SETTINGSDEFAULT_X, SETTINGSDEFAULT_Y)).toPoint();

	// if position is off of the screen then return to default location
    if (mainl.x() >= screen->width() || mainl.x() <= 0 ) mainl.setX(SETTINGSDEFAULT_X);
    if (mainl.y() >= screen->height() || mainl.y() <= 0 ) mainl.setY(SETTINGSDEFAULT_Y);
    if (mainz.width() >= screen->width()-mainl.x()) mainz.setWidth(screen->width()-mainl.x());
    if (mainz.height() >= screen->height()-mainl.y()) mainz.setHeight(screen->height()-mainl.y());
	//proper order
    resize(mainz);
	move(mainl);


	// position and size the graphics dock and basicgraph
	floating = settings.value(SETTINGSGRAPHFLOAT, false).toBool();
	graphdock->setFloating(floating);
	l = settings.value(SETTINGSGRAPHPOS, QPoint(SETTINGSGRAPHDEFAULT_X, SETTINGSGRAPHDEFAULT_Y)).toPoint();
	graphdock->move(l);
	z = settings.value(SETTINGSGRAPHSIZE, QSize(SETTINGSGRAPHDEFAULT_W, SETTINGSGRAPHDEFAULT_H)).toSize();
	graphdock->resize(z);

	// position the output text dock
	floating = settings.value(SETTINGSOUTFLOAT, false).toBool();
	outdock->setFloating(floating);
	l = settings.value(SETTINGSOUTPOS, QPoint(SETTINGSOUTDEFAULT_X, SETTINGSOUTDEFAULT_Y)).toPoint();
	outdock->move(l);
	z = settings.value(SETTINGSOUTSIZE, QSize(SETTINGSOUTDEFAULT_W, SETTINGSOUTDEFAULT_H)).toSize();
	outdock->resize(z);

	// positon the variable dock
	floating = settings.value(SETTINGSVARFLOAT, false).toBool();
	vardock->setFloating(floating);
	l = settings.value(SETTINGSVARPOS, QPoint(SETTINGSVARDEFAULT_X, SETTINGSVARDEFAULT_Y)).toPoint();
	vardock->move(l);
	z = settings.value(SETTINGSVARSIZE, QSize(SETTINGSVARDEFAULT_W, SETTINGSVARDEFAULT_H)).toSize();
	vardock->resize(z);
#endif


    // set initial font
    QFont initialFont;
    QString initialFontString = settings.value(SETTINGSFONT,SETTINGSFONTDEFAULT).toString();
    if (initialFont.fromString(initialFontString)) {
        editwin->setFont(initialFont);
        outwin->setFont(initialFont);
    }
    
}


void MainWindow::saveCustomizations() {
	// save user customizations on close

	SETTINGS;
	settings.setValue(SETTINGSVISIBLE, isVisible());
	settings.setValue(SETTINGSTOOLBAR, maintbar->isVisible());
	if(!guiState==GUISTATEAPP) settings.setValue(SETTINGSEDITVISIBLE, editdock->isVisible());
	settings.setValue(SETTINGSOUTVISIBLE, outdock->isVisible());
	settings.setValue(SETTINGSOUTTOOLBAR, outwinwgt->isVisibleToolBar());
	settings.setValue(SETTINGSEDITWHITESPACE, editWhitespaceAct->isChecked());
	settings.setValue(SETTINGSGRAPHVISIBLE, graphdock->isVisible());
	settings.setValue(SETTINGSGRAPHTOOLBAR, graphwinwgt->isVisibleToolBar());
	settings.setValue(SETTINGSGRAPHGRIDLINES, graphwin->isVisibleGridLines());
	if(guiState==GUISTATENORMAL) settings.setValue(SETTINGSVARVISIBLE, vardock->isVisible());

// android does not use floating size or position
#ifndef ANDROID
	settings.setValue(SETTINGSSIZE, size());
	settings.setValue(SETTINGSPOS, pos());
	settings.setValue(SETTINGSOUTFLOAT, outdock->isFloating());
	settings.setValue(SETTINGSOUTSIZE, outdock->size());
	settings.setValue(SETTINGSOUTPOS, outdock->pos());
	settings.setValue(SETTINGSGRAPHFLOAT, graphdock->isFloating());
	settings.setValue(SETTINGSGRAPHSIZE, graphdock->size());
	settings.setValue(SETTINGSGRAPHPOS, graphdock->pos());
	if(guiState==GUISTATENORMAL) {
		settings.setValue(SETTINGSVARFLOAT, vardock->isFloating());
		settings.setValue(SETTINGSVARSIZE, vardock->size());
		settings.setValue(SETTINGSVARPOS, vardock->pos());
	}
#endif

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
    if (locale) delete(locale);
    
}

void MainWindow::about() {
    QString title;
    QString message;

#ifdef ANDROID
    // android does not have webkit dialogs - make plain text
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Android");
    message = QObject::tr("BASIC-256") + QObject::tr(" Android") + "\n" +
              QObject::tr("version ") +  VERSION + QObject::tr(" - built with QT ") + QT_VERSION_STR + "\n" +
			QObject::tr("Locale Name: ") + locale->name() + QObject::tr("Decimal Point: ")+  "'" +locale->decimalPoint() + "'\n" +
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
			"<br>" + QObject::tr("Locale Name: ") + "<b>" + locale->name() + "</b> "+ QObject::tr("Decimal Point: ") + "<b>'" + locale->decimalPoint() + "'</b>" +
			"<p>" + QObject::tr("Copyright &copy; 2006-2016, The BASIC-256 Team") + "</p>" +
			"<p>" + QObject::tr("Please visit our web site at <a href=\"http://www.basic256.org\">http://www.basic256.org</a> for tutorials and documentation.") + "</p>" +
			"<p>" + QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.")  + "</p>" +
			"<p><i>" + QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.")  + "</i></p>";
#endif

    QMessageBox::about(this, title, message);
}

void MainWindow::updateRecent() {
    //update recent list on file menu
    if (guiState==GUISTATENORMAL) {
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
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
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            recentact[i]->setEnabled(false);
            recentact[i]->setVisible(false);
        }
    }
}

void MainWindow::setGuiState(int state) {
	//disable everything except what is needed to quit, stop and run a program.
	// called by Main when -r or -a option is sent
	guiState = state;
	
	if (state==GUISTATERUN||state==GUISTATEAPP) {
		// common UI changes for both states
		variableWinVisibleAct->setChecked(false);
		newact->setVisible(false);
		openact->setVisible(false);
		saveact->setVisible(false);
		saveasact->setVisible(false);
		printact->setVisible(false);
		editmenu->setTitle("");
		editmenu->setVisible(false);
		undoact->setVisible(false);
		redoact->setVisible(false);
		cutact->setVisible(false);
		copyact->setVisible(false);
		pasteact->setVisible(false);
		variableWinVisibleAct->setVisible(false);
		debugact->setVisible(false);
		stepact->setVisible(false);
		bpact->setVisible(false);
		editWhitespaceAct->setVisible(false);
		clearbreakpointsact->setVisible(false);
		editwin->blockSignals(true);
		findact->blockSignals(true);
		findagain->blockSignals(true);
		replaceact->blockSignals(true);
		helpthis->blockSignals(true);
		vardock->setVisible(false);
		for (int i=0; i<SETTINGSGROUPHISTN; i++) {
			recentact[i]->setEnabled(false);
			recentact[i]->setVisible(false);
			recentact[i]->blockSignals(false);
		}
		
		// run state additional changes
		if (state==GUISTATERUN) {
			editwin->setReadOnly(true);
		}
		
		// application state additional changes
		if (state==GUISTATEAPP) {
			editWinVisibleAct->setChecked(false);
			editWinVisibleAct->setVisible(false);
			runact->setVisible(false);
		}

	}
}


void MainWindow::ifGuiStateRun() {
		// start run if app or run  state
		// called from main if code is loaded
		if (guiState==GUISTATEAPP || guiState==GUISTATERUN) {
			runact->activate(QAction::Trigger);
		}
}

void MainWindow::ifGuiStateClose() {
	// optionally force close if application
	// from runtimecontroller when interperter stopps
	if (guiState==GUISTATEAPP) close();
}


void MainWindow::closeEvent(QCloseEvent *e) {
    // quit the application but ask if there are unsaved changes in buffer
    bool doquit = true;
    if (editwin->document()->isModified()) {
        doquit = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Program modifications have not been saved."),
            tr("Do you want to discard your changes?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No));
    }
    if (doquit) {
        // actually quitting
        e->accept();
        // save current screen posision, visibility and floating
        saveCustomizations();
        qApp->quit();
    } else {
        // not quitting
        e->ignore();
    }


}

//Buttons section
void MainWindow::updateStatusBar(QString status) {
    statusBar()->showMessage(status);
}

void MainWindow::updateWindowTitle(QString title) {
    setWindowTitle(title);
}

void MainWindow::updatePasteButton(){
     pasteact->setEnabled(editwin->canPaste());
}

void MainWindow::updateCopyCutButtons(bool stuffToCopy) {
    // only enable the copy buttons when there is stuff to copy
    cutact->setEnabled(stuffToCopy && editwin->canPaste());
    copyact->setEnabled(stuffToCopy);
}

void MainWindow::slotUndoAvailable(bool val) {
    undoact->setEnabled(val);
    undoButtonValue = val;
}

void MainWindow::slotRedoAvailable(bool val) {
    redoact->setEnabled(val);
    redoButtonValue = val;
}


//Update Paste, Copy, Cut, Undo, Redo buttons for Run/Stop program
void MainWindow::setEnabledEditorButtons(bool val) {
    updatePasteButton();
    QTextCursor curs = editwin->textCursor();
    updateCopyCutButtons(curs.hasSelection());
    undoact->setEnabled(val?undoButtonValue:false);
    redoact->setEnabled(val?redoButtonValue:false);
}


void MainWindow::setRunState(int state) {
    // set the menus, menu options, and tool bar items to
    // correct state based on the stop/run/debug status
    // state see RUNSTATE* constants

    editwin->setReadOnly(state!=RUNSTATESTOP || guiState!=GUISTATENORMAL);
    editwin->runState = state;
	editwin->highlightCurrentLine();

    // file menu
    newact->setEnabled(state==RUNSTATESTOP);
    openact->setEnabled(state==RUNSTATESTOP);
    saveact->setEnabled(state==RUNSTATESTOP);
    saveasact->setEnabled(state==RUNSTATESTOP);
    printact->setEnabled(state==RUNSTATESTOP);
    for(int t=0; t<SETTINGSGROUPHISTN; t++)
		recentact[t]->setEnabled(state==RUNSTATESTOP);
    exitact->setEnabled(true);

    // edit menu
    setEnabledEditorButtons(state==RUNSTATESTOP);
    selectallact->setEnabled(state==RUNSTATESTOP);
    findact->setEnabled(state==RUNSTATESTOP);
    findagain->setEnabled(state==RUNSTATESTOP);
    replaceact->setEnabled(state==RUNSTATESTOP);
    beautifyact->setEnabled(state==RUNSTATESTOP);
    prefact->setEnabled(state==RUNSTATESTOP);

    // run menu
    runact->setEnabled(state==RUNSTATESTOP);
    debugact->setEnabled(state==RUNSTATESTOP);
    stepact->setEnabled(state==RUNSTATEDEBUG);
    bpact->setEnabled(state==RUNSTATEDEBUG);
    stopact->setEnabled(state!=RUNSTATESTOP && state!=RUNSTATESTOPING);
    clearbreakpointsact->setEnabled(state!=RUNSTATERUN);
}

void MainWindow::checkGraphMenuVisible(){
    graphWinVisibleAct->setChecked(graphdock->isVisible());
}
void MainWindow::checkOutMenuVisible(){
    outWinVisibleAct->setChecked(outdock->isVisible());
}
void MainWindow::checkVarMenuVisible(){
    variableWinVisibleAct->setChecked(vardock->isVisible());
}
