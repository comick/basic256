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



#include "PauseButton.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"
#include "BasicDock.h"

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



MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring, int guistate)
    :	QMainWindow(parent, f) {

    localecode = localestring;
	locale = new QLocale(localecode);
    guiState = guistate;
    mainwin = this;
    undoButtonValue = false;
    redoButtonValue = false;
    setAcceptDrops(true);

    // create the global mymutexes and waits
    mymutex = new QMutex(QMutex::NonRecursive);
    mydebugmutex = new QMutex(QMutex::NonRecursive);
    waitCond = new QWaitCondition();
    waitDebugCond = new QWaitCondition();

    setWindowIcon(QIcon(":/images/basic256.png"));

#ifndef ANDROID
    manager = new QNetworkAccessManager(this);
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SecureProtocols);
    request.setSslConfiguration(config);
#ifdef WIN32PORTABLE
    request.setUrl(QUrl("https://sourceforge.net/projects/basic256prtbl/best_release.json"));
#else
    request.setUrl(QUrl("https://sourceforge.net/projects/kidbasic/best_release.json"));
#endif
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "App/1.0");
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sourceforgeReplyFinished(QNetworkReply*)));
#endif


    // Basic* *win go into BasicWidget *win_widget to get menus and toolbars
	// *win_widget go into BasicDock *win_dock to create the GUI docks
	
    editwin = new BasicEdit();
    editwin->setObjectName( "editwin" );
    editwin_widget = new BasicWidget(QObject::tr("Program Editor"));
    editwin_widget->setObjectName( "editwin_widget" );
    editwin_widget->setViewWidget(editwin);
    editwin_dock = new BasicDock();
    editwin_dock->setObjectName( "editwin_dock" );
    editwin_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    editwin_dock->setWidget(editwin_widget);
    editwin_dock->setWindowTitle(QObject::tr("Program Editor"));

    outwin = new BasicOutput();
    outwin->setObjectName( "outwin" );
    outwin->setReadOnly(true);
    outwin_widget = new BasicWidget(QObject::tr("Text Output"));
	outwin_widget->setObjectName( "outwin_widget" );
    outwin_widget->setViewWidget(outwin);
    outwin_dock = new BasicDock();
    outwin_dock->setObjectName( "outwin_dock" );
    outwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    outwin_dock->setWidget(outwin_widget);
    outwin_dock->setWindowTitle(QObject::tr("Text Output"));
 
    graphwin = new BasicGraph();
    graphwin->setObjectName( "graphwin" );
    graphwin_widget = new BasicWidget(QObject::tr("Graphics Output"));
    graphwin_widget->setObjectName( "graphwin_widget" );
    graphwin_widget->setViewWidget(graphwin);
    graph_scroll = new QScrollArea();
	graph_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
 	graph_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	graph_scroll->setWidget(graphwin_widget);
    graphwin_dock = new BasicDock();
    graphwin_dock->setObjectName( "graphwin_dock" );
    graphwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    graphwin_dock->setWidget(graph_scroll);
    graphwin_dock->setWindowTitle(QObject::tr("Graphics Output"));

    varwin = new VariableWin();
    varwin->setObjectName( "varwin" );
    varwin_widget = new BasicWidget(QObject::tr("Variable Watch"));
    varwin_widget->setObjectName( "varwin_widget" );
    varwin_widget->setViewWidget(varwin);
    varwin_dock = new BasicDock();
    varwin_dock->setObjectName( "varwin_dock" );
    varwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    varwin_dock->setWidget(varwin_widget);
    varwin_dock->setWindowTitle(QObject::tr("Variable Watch"));

    setCentralWidget(editwin_dock);
    addDockWidget(Qt::RightDockWidgetArea, outwin_dock);
    addDockWidget(Qt::RightDockWidgetArea, graphwin_dock);
    addDockWidget(Qt::LeftDockWidgetArea, varwin_dock);
    setContextMenuPolicy(Qt::NoContextMenu);

    rc = new RunController();
    editsyntax = new EditSyntaxHighlighter(editwin->document());

    // Main window toolbar
    main_toolbar = new QToolBar();
    main_toolbar->setObjectName("main_toolbar");
    addToolBar(main_toolbar);

    // File menu
    filemenu = menuBar()->addMenu(QObject::tr("&File"));
    filemenu_new_act = filemenu->addAction(QIcon(":images/new.png"), QObject::tr("&New"));
    filemenu_new_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::New));
    filemenu_open_act = filemenu->addAction(QIcon(":images/open.png"), QObject::tr("&Open..."));
    filemenu_open_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Open));

    // Recent files menu
    filemenu_recentfiles = filemenu->addMenu(QObject::tr("Open &Recent"));
    for(int i=0;i<SETTINGSGROUPHISTN;i++){
        recentfiles_act[i] = filemenu_recentfiles->addAction(QIcon(":images/open.png"), QObject::tr(""));
        if(i<10)
            recentfiles_act[i]->setShortcut(Qt::Key_0 + ((i+1)%SETTINGSGROUPHISTN) + Qt::CTRL);
    }
    filemenu_recentfiles->addSeparator();
    recentfiles_empty_act = filemenu_recentfiles->addAction(QObject::tr("&Clear list"));
    updateRecent();

    // File menu - continue
    filemenu_save_act = filemenu->addAction(QIcon(":images/save.png"), QObject::tr("&Save"));
    filemenu_save_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Save));
    filemenu_saveas_act = filemenu->addAction(QIcon(":images/saveas.png"), QObject::tr("Save &As..."));
    filemenu_saveas_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::SaveAs));
    filemenu->addSeparator();
    filemenu_print_act = filemenu->addAction(QIcon(":images/print.png"), QObject::tr("&Print..."));
    filemenu_print_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));
    filemenu->addSeparator();
    filemenu_exit_act = filemenu->addAction(QIcon(":images/exit.png"), QObject::tr("&Exit"));
    filemenu_exit_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Quit));


    // Edit menu
    editmenu = menuBar()->addMenu(QObject::tr("&Edit"));
    undoact = editmenu->addAction(QIcon(":images/undo.png"), QObject::tr("&Undo"));
    undoact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Undo));
    undoact->setEnabled(false);
    redoact = editmenu->addAction(QIcon(":images/redo.png"), QObject::tr("&Redo"));
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

    bool extraSepAdded = false;
    if (outwin_widget->usesMenu()) {
        editmenu->addSeparator();
        extraSepAdded = true;
        editmenu->addMenu(outwin_widget->getMenu());
    }
    if (graphwin_widget->usesMenu()) {
        if (!extraSepAdded) {
            editmenu->addSeparator();
        }
        editmenu->addMenu(graphwin_widget->getMenu());
    }

    // View menuBar
    viewmenu = menuBar()->addMenu(QObject::tr("&View"));

    editwin_visible_act = viewmenu->addAction(QObject::tr("&Edit Window"));
    editwin_visible_act->setCheckable(true);
    editwin_dock->setActionCheck(editwin_visible_act);
    editwin_visible_act->setChecked(SETTINGSEDITVISIBLEDEFAULT);

    outwin_visible_act = viewmenu->addAction(QObject::tr("&Text Window"));
    outwin_visible_act->setCheckable(true);
    outwin_dock->setActionCheck(outwin_visible_act);
    outwin_visible_act->setChecked(SETTINGSOUTVISIBLEDEFAULT);

    graphwin_visible_act = viewmenu->addAction(QObject::tr("&Graphics Window"));
    graphwin_visible_act->setCheckable(true);
    graphwin_dock->setActionCheck(graphwin_visible_act);
    graphwin_visible_act->setChecked(SETTINGSGRAPHVISIBLEDEFAULT);

    varwin_visible_act = viewmenu->addAction(QObject::tr("&Variable Watch Window"));
    varwin_visible_act->setCheckable(true);
    varwin_dock->setActionCheck(varwin_visible_act);
    varwin_visible_act->setChecked(SETTINGSVARVISIBLEDEFAULT);


    // Editor and Output font and Editor settings
    viewmenu->addSeparator();
    fontact = viewmenu->addAction(QObject::tr("&Font..."));
    edit_whitespace_act = viewmenu->addAction(QObject::tr("Show &Whitespace Characters"));
    edit_whitespace_act->setCheckable(true);
    edit_whitespace_act->setChecked(SETTINGSEDITWHITESPACEDEFAULT);
    editwin->slotWhitespace(SETTINGSEDITWHITESPACEDEFAULT);

    // Graphics Grid Lines
    viewmenu->addSeparator();
    graph_grid_visible_act = viewmenu->addAction(QObject::tr("Graphics Window Grid &Lines"));
    graph_grid_visible_act->setCheckable(true);
    graph_grid_visible_act->setChecked(SETTINGSGRAPHGRIDLINESDEFAUT);
    graphwin->slotGridLines(SETTINGSGRAPHGRIDLINESDEFAUT);

    // Toolbars
    viewmenu->addSeparator();
    QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
    main_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Main"));
    main_toolbar_visible_act->setCheckable(true);
    main_toolbar_visible_act->setChecked(SETTINGSTOOLBARVISIBLEDEFAULT);
    outwin_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Text Output"));
    outwin_toolbar_visible_act->setCheckable(true);
    outwin_toolbar_visible_act->setChecked(SETTINGSOUTTOOLBARVISIBLEDEFAULT);
    graphwin_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Graphics Output"));
    graphwin_toolbar_visible_act->setCheckable(true);
    graphwin_toolbar_visible_act->setChecked(SETTINGSGRAPHTOOLBARVISIBLEDEFAULT);
 
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

    // Help menu
    QMenu *helpmenu = menuBar()->addMenu(QObject::tr("&Help"));
#if defined(WIN32PORTABLE) || defined(ANDROID)
    // in portable or android make doc online and context help online
    onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help..."));
    onlinehact->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    addAction (helpthis);
#else
    // in installed mode make doc offline and online and context help offline
    docact = helpmenu->addAction(QIcon(":images/help.png"), QObject::tr("&Help..."));
    docact->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    addAction (helpthis);
    onlinehact = helpmenu->addAction(QIcon(":images/firefox.png"), QObject::tr("&Online help..."));
#endif
#ifndef ANDROID
    helpmenu->addSeparator();
    checkupdate = helpmenu->addAction(QObject::tr("&Check for update..."));
#endif
    helpmenu->addSeparator();
    QAction *aboutact = helpmenu->addAction(QObject::tr("&About BASIC-256..."));

    // Add actions to main window toolbar
    main_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    main_toolbar->addAction(filemenu_new_act);
    main_toolbar->addAction(filemenu_open_act);
    main_toolbar->addAction(filemenu_save_act);
    main_toolbar->addSeparator();
    main_toolbar->addAction(runact);
    main_toolbar->addAction(debugact);
    main_toolbar->addAction(stepact);
    main_toolbar->addAction(bpact);
    main_toolbar->addAction(stopact);
    main_toolbar->addSeparator();
    main_toolbar->addAction(undoact);
    main_toolbar->addAction(redoact);
    main_toolbar->addAction(cutact);
    main_toolbar->addAction(copyact);
    main_toolbar->addAction(pasteact);
	//

	loadCustomizations();
	configureGuiState();

    //Display windows and toolbars acording their final settings
    editwin_dock->setVisible(editwin_visible_act->isChecked());
    graphwin_dock->setVisible(graphwin_visible_act->isChecked());
    outwin_dock->setVisible(outwin_visible_act->isChecked());
    varwin_dock->setVisible(varwin_visible_act->isChecked());
    main_toolbar->setVisible(main_toolbar_visible_act->isChecked());
    graphwin_widget->slotShowToolBar(graphwin_toolbar_visible_act->isChecked());
    outwin_widget->slotShowToolBar(outwin_toolbar_visible_act->isChecked());


	// connect the signals
	QObject::connect(editwin, SIGNAL(changeStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
	QObject::connect(editwin, SIGNAL(changeWindowTitle(QString)), this, SLOT(updateWindowTitle(QString)));
	QObject::connect(editwin, SIGNAL(undoAvailable(bool)), this, SLOT(slotUndoAvailable(bool)));
	QObject::connect(editwin, SIGNAL(redoAvailable(bool)), this, SLOT(slotRedoAvailable(bool)));
	QObject::connect(editwin, SIGNAL(copyAvailable(bool)), this, SLOT(updateCopyCutButtons(bool)));
	QObject::connect(editwin_visible_act, SIGNAL(triggered(bool)), editwin_dock, SLOT(setVisible(bool)));
    QObject::connect(editwin, SIGNAL(updateRecent()), this, SLOT(updateRecent()));
    QObject::connect(editwin, SIGNAL(addFileToRecentList(QString)), this, SLOT(addFileToRecentList(QString)));


    QObject::connect(filemenu_new_act, SIGNAL(triggered()), editwin, SLOT(newProgram()));
    QObject::connect(filemenu_open_act, SIGNAL(triggered()), editwin, SLOT(loadProgram()));
    QObject::connect(filemenu_save_act, SIGNAL(triggered()), editwin, SLOT(saveProgram()));
    QObject::connect(filemenu_saveas_act, SIGNAL(triggered()), editwin, SLOT(saveAsProgram()));
    QObject::connect(filemenu_print_act, SIGNAL(triggered()), editwin, SLOT(slotPrint()));
    QObject::connect(filemenu_recentfiles, SIGNAL(aboutToShow()), this, SLOT(updateRecent())); //in case that another instance modify history
    for(int i=0;i<SETTINGSGROUPHISTN;i++){
        QObject::connect(recentfiles_act[i], SIGNAL(triggered()), this, SLOT(openRecent()));
    }
    QObject::connect(recentfiles_empty_act, SIGNAL(triggered()), this, SLOT(emptyRecent()));
    QObject::connect(filemenu_exit_act, SIGNAL(triggered()), this, SLOT(close()));

	QObject::connect(graphwin_toolbar_visible_act, SIGNAL(toggled(bool)), graphwin_widget, SLOT(slotShowToolBar(const bool)));
	QObject::connect(graphwin_visible_act, SIGNAL(triggered(bool)), graphwin_dock, SLOT(setVisible(bool)));

	QObject::connect(main_toolbar_visible_act, SIGNAL(toggled(bool)), main_toolbar, SLOT(setVisible(bool)));

	QObject::connect(outwin_visible_act, SIGNAL(triggered(bool)), outwin_dock, SLOT(setVisible(bool)));
	QObject::connect(outwin_toolbar_visible_act, SIGNAL(toggled(bool)), outwin_widget, SLOT(slotShowToolBar(const bool)));

	QObject::connect(varwin_visible_act, SIGNAL(triggered(bool)), varwin_dock, SLOT(setVisible(bool)));


    QObject::connect(undoact, SIGNAL(triggered()), editwin, SLOT(undo()));
    QObject::connect(redoact, SIGNAL(triggered()), editwin, SLOT(redo()));
    QObject::connect(cutact, SIGNAL(triggered()), editwin, SLOT(cut()));
    QObject::connect(copyact, SIGNAL(triggered()), editwin, SLOT(copy()));
    QObject::connect(pasteact, SIGNAL(triggered()), editwin, SLOT(paste()));
    QObject::connect(selectallact, SIGNAL(triggered()), editwin, SLOT(selectAll()));
    QObject::connect(findact, SIGNAL(triggered()), rc, SLOT(showFind()));
    QObject::connect(findagain, SIGNAL(triggered()), rc, SLOT(findAgain()));
    QObject::connect(replaceact, SIGNAL(triggered()), rc, SLOT(showReplace()));
    QObject::connect(beautifyact, SIGNAL(triggered()), editwin, SLOT(beautifyProgram()));
    QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updatePasteButton()));
    QObject::connect(fontact, SIGNAL(triggered()), rc, SLOT(dialogFontSelect()));
    QObject::connect(edit_whitespace_act, SIGNAL(toggled(bool)), editwin, SLOT(slotWhitespace(bool)));
    QObject::connect(graph_grid_visible_act, SIGNAL(toggled(bool)), graphwin, SLOT(slotGridLines(bool)));

    QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
    QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
    QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
    QObject::connect(bpact, SIGNAL(triggered()), rc, SLOT(stepBreakPoint()));
    QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
    QObject::connect(clearbreakpointsact, SIGNAL(triggered()), editwin, SLOT(clearBreakPoints()));
    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
    QObject::connect(aboutact, SIGNAL(triggered()), this, SLOT(about()));

#ifndef ANDROID
    QObject::connect(checkupdate, SIGNAL(triggered()), this, SLOT(checkForUpdate()));
#endif
#if defined(WIN32PORTABLE) || defined(ANDROID)
    QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showOnlineContextDocumentation()));
#else
    QObject::connect(docact, SIGNAL(triggered()), rc, SLOT(showDocumentation()));    
    QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showContextDocumentation()));
#endif

    if(autoCheckForUpdate) checkForUpdate();
}

void MainWindow::loadCustomizations() {
	// from settings - load the customizations to the screen

	SETTINGS;
    bool v, restoreWindows;

    restoreWindows = settings.value(SETTINGSWINDOWSRESTORE, SETTINGSWINDOWSRESTOREDEFAULT).toBool();
    if(restoreWindows){
        restoreGeometry(settings.value(SETTINGSMAINGEOMETRY + QString::number(guiState)).toByteArray());
        QByteArray state = settings.value(SETTINGSMAINSTATE + QString::number(guiState)).toByteArray();
        restoreState(state);
        // edit window
        v = settings.value(SETTINGSEDITVISIBLE + QString::number(guiState), SETTINGSEDITVISIBLEDEFAULT).toBool();
        editwin_visible_act->setChecked(v);
        // graph window
        v = settings.value(SETTINGSGRAPHVISIBLE + QString::number(guiState), SETTINGSGRAPHVISIBLEDEFAULT).toBool();
        graphwin_visible_act->setChecked(v);
        // out window
        v = settings.value(SETTINGSOUTVISIBLE + QString::number(guiState), SETTINGSOUTVISIBLEDEFAULT).toBool();
        outwin_visible_act->setChecked(v);
        // var window - variable watch
        v = settings.value(SETTINGSVARVISIBLE + QString::number(guiState), SETTINGSVARVISIBLEDEFAULT).toBool();
        varwin_visible_act->setChecked(v);
    }

    // main toolbar
    v = settings.value(SETTINGSTOOLBARVISIBLE + QString::number(guiState), SETTINGSTOOLBARVISIBLEDEFAULT).toBool();
    main_toolbar_visible_act->setChecked(v);

    // edit whitespace
    v = settings.value(SETTINGSEDITWHITESPACE + QString::number(guiState), SETTINGSEDITWHITESPACEDEFAULT).toBool();
    edit_whitespace_act->setChecked(v);

    // graph toolbar and grid
    v = settings.value(SETTINGSGRAPHGRIDLINES + QString::number(guiState), SETTINGSGRAPHGRIDLINESDEFAUT).toBool();
    graph_grid_visible_act->setChecked(v);
    v = settings.value(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), SETTINGSGRAPHTOOLBARVISIBLEDEFAULT).toBool();
    graphwin_toolbar_visible_act->setChecked(v);

    // out toolbar
    v = settings.value(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), SETTINGSOUTTOOLBARVISIBLEDEFAULT).toBool();
    outwin_toolbar_visible_act->setChecked(v);

    // set initial font
    QFont initialFont;
    QString initialFontString = settings.value(SETTINGSFONT + QString::number(guiState),SETTINGSFONTDEFAULT).toString();
    if (initialFont.fromString(initialFontString)) {
        editwin->setFont(initialFont);
        outwin->setFont(initialFont);
    }

    autoCheckForUpdate = (guiState==GUISTATENORMAL&&settings.value(SETTINGSCHECKFORUPDATE, SETTINGSCHECKFORUPDATEDEFAULT).toBool());
}


void MainWindow::saveCustomizations() {
    // save user customizations on close

	SETTINGS;
	settings.setValue(SETTINGSMAINGEOMETRY + QString::number(guiState), saveGeometry());
	settings.setValue(SETTINGSMAINSTATE + QString::number(guiState), saveState());

    // main
	settings.setValue(SETTINGSTOOLBARVISIBLE + QString::number(guiState), main_toolbar->isVisible());

    // edit
	settings.setValue(SETTINGSEDITVISIBLE + QString::number(guiState), editwin_visible_act->isChecked());
	settings.setValue(SETTINGSEDITWHITESPACE + QString::number(guiState), edit_whitespace_act->isChecked());

    // graph
	settings.setValue(SETTINGSGRAPHVISIBLE + QString::number(guiState), graphwin_visible_act->isChecked());
	settings.setValue(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), graphwin_widget->isVisibleToolBar());
	settings.setValue(SETTINGSGRAPHGRIDLINES + QString::number(guiState), graphwin->isVisibleGridLines());

    // out
	settings.setValue(SETTINGSOUTVISIBLE + QString::number(guiState), outwin_visible_act->isChecked());
	settings.setValue(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), outwin_widget->isVisibleToolBar());

    // var
	settings.setValue(SETTINGSVARVISIBLE + QString::number(guiState), varwin_visible_act->isChecked());

    // font
	settings.setValue(SETTINGSFONT + QString::number(guiState), editwin->font().toString());
}

MainWindow::~MainWindow() {
    delete rc;
    delete editsyntax;
    delete mymutex;
    delete waitCond;
    delete editwin;
    delete outwin;
    delete graphwin;
    delete main_toolbar;
    if (locale) delete(locale);
}

void MainWindow::about() {
    QString title;
    QString message;
    SETTINGS;
    bool usefloatlocale = settings.value(SETTINGSFLOATLOCALE, SETTINGSFLOATLOCALEDEFAULT).toBool();


#ifdef ANDROID
    // android does not have webkit dialogs - make plain text
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Android");
    message = QObject::tr("BASIC-256") + QObject::tr(" Android") + "\n" +
              QObject::tr("version ") +  VERSION + QObject::tr(" - built with QT ") + QT_VERSION_STR + "\n" +
            QObject::tr("Locale Name: ") + locale->name() + QObject::tr("Decimal Point: ")+  "'" + (usefloatlocale?locale->decimalPoint():'.') + "'\n" +
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
            "<br>" + QObject::tr("Locale Name: ") + "<b>" + locale->name() + "</b> "+ QObject::tr("Decimal Point: ") + "<b>'" + (usefloatlocale?locale->decimalPoint():'.') + "'</b>" +
			"<p>" + QObject::tr("Copyright &copy; 2006-2016, The BASIC-256 Team") + "</p>" +
			"<p>" + QObject::tr("Please visit our web site at <a href=\"http://www.basic256.org\">http://www.basic256.org</a> for tutorials and documentation.") + "</p>" +
			"<p>" + QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.")  + "</p>" +
			"<p><i>" + QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.")  + "</i></p>";
#endif

    QMessageBox::about(this, title, message);
}



void MainWindow::configureGuiState() {
	//disable everything except what is needed to quit, stop and run a program.
	if (guiState==GUISTATERUN||guiState==GUISTATEAPP) {
		// common UI changes for both states
		filemenu_new_act->setVisible(false);
		filemenu_open_act->setVisible(false);
		filemenu_save_act->setVisible(false);
		filemenu_saveas_act->setVisible(false);
		filemenu_print_act->setVisible(false);
        editmenu->menuAction()->setVisible(false);
		undoact->setVisible(false);
		redoact->setVisible(false);
		cutact->setVisible(false);
		copyact->setVisible(false);
		pasteact->setVisible(false);
		debugact->setVisible(false);
		stepact->setVisible(false);
		bpact->setVisible(false);
		edit_whitespace_act->setVisible(false);
		clearbreakpointsact->setVisible(false);

		editwin->blockSignals(true);
		findact->blockSignals(true);
		findagain->blockSignals(true);
		replaceact->blockSignals(true);
		helpthis->blockSignals(true);
		varwin_visible_act->setVisible(false);
        editwin->setReadOnly(true);

		// application state additional changes
		if (guiState==GUISTATEAPP) {
            onlinehact->setVisible(false);
            docact->setVisible(false);
            editwin_visible_act->setVisible(false);
			main_toolbar_visible_act->setVisible(false);
            main_toolbar_visible_act->setChecked(false);
			runact->setVisible(false);
            stopact->setVisible(false);
            runmenu->menuAction()->setVisible(false);
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

void MainWindow::ifGuiStateClose(bool ok) {
	// optionally force close if application
	// from runtimecontroller when interperter stopps
    if (guiState==GUISTATEAPP){
        if(!ok){
            statusBar()->showMessage("Error.");
            QMessageBox::warning(this, tr("Error"), tr("Current program has terminated in an unusual way."));
        }
        close();
    }
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
        // save current screen posision, visibility and floating
        saveCustomizations();
        // actually quitting
        e->accept();
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
    filemenu_new_act->setEnabled(state==RUNSTATESTOP);
    filemenu_open_act->setEnabled(state==RUNSTATESTOP);
    filemenu_save_act->setEnabled(state==RUNSTATESTOP);
    filemenu_saveas_act->setEnabled(state==RUNSTATESTOP);
    filemenu_print_act->setEnabled(state==RUNSTATESTOP);
    for(int t=0; t<SETTINGSGROUPHISTN; t++)
        recentfiles_act[t]->setEnabled(state==RUNSTATESTOP);
    filemenu_exit_act->setEnabled(true);

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

//Check for an update
#ifndef ANDROID
void MainWindow::checkForUpdate(void){
    manager->get(request);
}

void MainWindow::sourceforgeReplyFinished(QNetworkReply* reply){
    QString url;
    QString filename;
    if(reply->error() == QNetworkReply::NoError) {
        QByteArray  strReply = reply->readAll();
        //qDebug() << strReply;
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply);
        QJsonObject jsonObject = jsonResponse.object();
#if defined(WIN32PORTABLE) || defined(WIN32)
        filename = jsonObject["platform_releases"].toObject()["windows"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["windows"].toObject()["url"].toString();
#elif defined(LINUX)
        filename = jsonObject["platform_releases"].toObject()["linux"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["linux"].toObject()["url"].toString();
#elif defined(MACX)
        filename = jsonObject["platform_releases"].toObject()["mac"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["mac"].toObject()["url"].toString();
#endif
        //qDebug() << text;
        QRegExp rx("\\d+\\.\\d+\\.\\d+\\.\\d+");
        rx.indexIn(filename);
        QString siteversion = rx.cap(0);
        rx.indexIn(VERSION);
        QString thisversion = rx.cap(0);
        if(siteversion=="" || thisversion==""){
            //Unknown error
            if(!autoCheckForUpdate)QMessageBox::warning(this, tr("Check for an update"), tr("Unknown error."),QMessageBox::Ok, QMessageBox::Ok);
        }else if(siteversion>thisversion){
            //New version to download
            if(QMessageBox::information(this, tr("Check for an update"), tr("BASIC-256") + " " + siteversion + " " + tr("is now available - you have") + " " + thisversion + ". " + tr("Would you like to download the new version now?"),QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)==QMessageBox::Yes){
                QDesktopServices::openUrl(QUrl(url));
            }
        }else if(thisversion>siteversion){
            //Beta version
            if(!autoCheckForUpdate)QMessageBox::information(this, tr("Check for an update"), tr("You are currently using a development version."),QMessageBox::Ok, QMessageBox::Ok);
        }else{
            //No new version to download
            if(!autoCheckForUpdate)QMessageBox::information(this, tr("Check for an update"), tr("You are using the latest software version for your current OS."),QMessageBox::Ok, QMessageBox::Ok);
        }
        autoCheckForUpdate = false; //do automatically check for update only once

    } else {
        //Network error
        QMessageBox::warning(this, tr("Check for an update"), tr("We are unable to connect right now. Please check your network connection and try again."),QMessageBox::Ok, QMessageBox::Ok);
    }
    reply->deleteLater();
}
#endif




void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    if (event->mimeData()->hasFormat("text/uri-list")){
        event->acceptProposedAction();
    }else{
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event){
        if(event->mimeData()->hasFormat("text/uri-list")){
        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.isEmpty())
            return;
        QString fileName = urls.first().toLocalFile();
        if (fileName.isEmpty())
            return;
        rc->stopRun();
        editwin->loadFile(fileName);
    }
        return;
}


void MainWindow::updateRecent() {
    //update recent list on file menu
    int counter=0;

    if (guiState==GUISTATENORMAL) {
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            QString fn = settings.value(QString::number(i), "").toString().trimmed();
            if(!fn.isEmpty()){
                recentfiles_act[counter]->setData(fn);
                QString path = QDir::currentPath() + "/";
                if (QString::compare(path, fn.left(path.length()))==0) {
                    fn = fn.right(fn.length()-path.length());
                }
                recentfiles_act[counter]->setEnabled(fn.length()!=0 && filemenu_new_act->isEnabled());
                recentfiles_act[counter]->setVisible(fn.length()!=0);
                recentfiles_act[counter]->setText((counter>=9?QString::number(int((counter+1)/10)):QString("")) + "&" + QString::number((counter+1)%10) + " - " + fn);
                counter++; //next slot - skip empty values deleted manually by user
            }
        }
        settings.endGroup();
        //hide unused slots
        for (int i=counter; i<SETTINGSGROUPHISTN; i++) {
            recentfiles_act[i]->setEnabled(false);
            recentfiles_act[i]->setVisible(false);
        }
    }else{
        filemenu_recentfiles->menuAction()->setVisible(false);
    }

    //if(guiState!=GUISTATENORMAL) or we have unused slots
    for (int i=counter; i<SETTINGSGROUPHISTN; i++) {
        recentfiles_act[i]->setEnabled(false);
        recentfiles_act[i]->setVisible(false);
    }
    //if we have no history to show
    if(counter==0){
        recentfiles_empty_act->setEnabled(false);
        recentfiles_empty_act->setVisible(false);
        filemenu_recentfiles->setEnabled(false);
    }else{
        recentfiles_empty_act->setEnabled(true);
        recentfiles_empty_act->setVisible(true);
        filemenu_recentfiles->setEnabled(true);
    }
}

void MainWindow::openRecent(){
    QAction *action = qobject_cast<QAction *>(sender());
    if (action){
        QString f = action->data().toString().trimmed();
        if(!f.isEmpty()){
            editwin->loadFile(f);
        }
    }
}

void MainWindow::emptyRecent(){
    SETTINGS;
    settings.beginGroup(SETTINGSGROUPHIST);
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            settings.setValue(QString::number(i), "");
        }
    settings.endGroup();
    updateRecent();
}

void MainWindow::addFileToRecentList(QString fn) {
    // keep list of recently open or saved files
    // put file name at position 0 on list
    fn=fn.trimmed();
    if(!fn.isEmpty()){
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        // if program is at top then do nothing
        if (settings.value(QString::number(0), "").toString() != fn) {
            // find end of scootdown
            int e;
            for(e=1; e<SETTINGSGROUPHISTN && settings.value(QString::number(e), "").toString() != fn; e++) {}
            // scoot entries down
            for (int i=(e<SETTINGSGROUPHISTN?e:SETTINGSGROUPHISTN-1); i>=1; i--) {
                settings.setValue(QString::number(i), settings.value(QString::number(i-1), ""));
            }
            settings.setValue(QString::number(0), fn);
        }
        settings.endGroup();
        updateRecent();
    }
}
