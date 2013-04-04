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

#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QApplication>
	#include <QtWidgets/QGridLayout>
	#include <QtWidgets/QMenuBar>
	#include <QtWidgets/QStatusBar>
	#include <QtWidgets/QDialog>
	#include <QtWidgets/QLabel>
	#include <QtWidgets/QShortcut>
#else
	#include <QApplication>
	#include <QGridLayout>
	#include <QMenuBar>
	#include <QStatusBar>
	#include <QDialog>
	#include <QLabel>
	#include <QShortcut>
#endif

using namespace std;

#include "PauseButton.h"
#include "DockWidget.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"

// global mutexes and timers
QMutex* mutex;
QMutex* debugmutex;
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
		:	QMainWindow(parent, f)
{

	mainwin = this;

	// create the global mutexes and waits
	mutex = new QMutex(QMutex::NonRecursive);
	debugmutex = new QMutex(QMutex::NonRecursive);
	waitCond = new QWaitCondition();
	waitDebugCond = new QWaitCondition();

	setWindowIcon(QIcon(":/images/basic256.png"));

	QWidget * centerWidget = new QWidget();
	centerWidget->setObjectName( "centerWidget" );

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
	DockWidget * outdock = new DockWidget();
	outdock->setObjectName( "tdock" );
	outdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	outdock->setWidget(outwinwgt);
	outdock->setWindowTitle(QObject::tr("Text Output"));
   	connect(editwin, SIGNAL(changeWindowTitle(QString)), this, SLOT(updateWindowTitle(QString)));

	graphwin = new BasicGraph();
	graphwin->setObjectName( "goutput" );
	graphwinwgt = new BasicWidget(QObject::tr("Graphics Output"));
	graphwinwgt->setViewWidget(graphwin);
	DockWidget * graphdock = new DockWidget();
	graphdock->setObjectName( "graphdock" );
	graphdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	graphdock->setWidget(graphwinwgt);
	graphdock->setWindowTitle(QObject::tr("Graphics Output"));


	varwin = new VariableWin();
	varwinwgt = new BasicWidget(QObject::tr("Variable Watch"));
	varwinwgt->setViewWidget(varwin);
	DockWidget * vardock = new DockWidget();
	vardock->setObjectName( "vardock" );
	vardock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	vardock->setWidget(varwinwgt);
	vardock->setWindowTitle(QObject::tr("Variable Watch"));
	vardock->setVisible(false);
	vardock->setFloating(true);

	rc = new RunController();
	editsyntax = new EditSyntaxHighlighter(editwin->document());

	// Main window toolbar
	QToolBar *maintbar = new QToolBar();
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
	QShortcut* findagain1 = new QShortcut(Qt::Key_F3, this);
	QShortcut* findagain2 = new QShortcut(Qt::Key_G + Qt::CTRL, this);
	replaceact = editmenu->addAction(QObject::tr("&Replace"));
	editmenu->addSeparator();
	beautifyact = editmenu->addAction(QObject::tr("&Beautify"));
	editmenu->addSeparator();
	prefact = editmenu->addAction(QIcon(":images/preferences.png"), QObject::tr("Preferences"));
	//
	QObject::connect(cutact, SIGNAL(triggered()), editwin, SLOT(cut()));
	QObject::connect(editwin, SIGNAL(copyAvailable(bool)), cutact, SLOT(setEnabled(bool)));
	QObject::connect(copyact, SIGNAL(triggered()), editwin, SLOT(copy()));
	QObject::connect(editwin, SIGNAL(copyAvailable(bool)), copyact, SLOT(setEnabled(bool)));
	QObject::connect(pasteact, SIGNAL(triggered()), editwin, SLOT(paste()));
	QObject::connect(selectallact, SIGNAL(triggered()), editwin, SLOT(selectAll()));
	QObject::connect(findact, SIGNAL(triggered()), rc, SLOT(showFind()));
	QObject::connect(findagain1, SIGNAL(activated()), rc, SLOT(findAgain())); 
	QObject::connect(findagain2, SIGNAL(activated()), rc, SLOT(findAgain())); 
	QObject::connect(replaceact, SIGNAL(triggered()), rc, SLOT(showReplace()));
	QObject::connect(beautifyact, SIGNAL(triggered()), editwin, SLOT(beautifyProgram()));
	QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));

	bool extraSepAdded = false;
	if (outwinwgt->usesMenu())
	{
		editmenu->addSeparator();
		extraSepAdded = true;
		editmenu->addMenu(outwinwgt->getMenu());
	}
	if (graphwinwgt->usesMenu())
	{
		if (!extraSepAdded)
		{
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
	editWinVisibleAct->setChecked(true);
	textWinVisibleAct->setChecked(true);
	graphWinVisibleAct->setChecked(true);
	variableWinVisibleAct->setChecked(false);
	QObject::connect(editWinVisibleAct, SIGNAL(toggled(bool)), editwinwgt, SLOT(setVisible(bool)));
	QObject::connect(textWinVisibleAct, SIGNAL(toggled(bool)), outdock, SLOT(setVisible(bool)));
	QObject::connect(graphWinVisibleAct, SIGNAL(toggled(bool)), graphdock, SLOT(setVisible(bool)));
	QObject::connect(variableWinVisibleAct, SIGNAL(toggled(bool)), vardock, SLOT(setVisible(bool)));

    // Editor and Output font
    viewmenu->addSeparator();
    QAction *fontSelect = viewmenu->addAction(QObject::tr("&Font"));
    QObject::connect(fontSelect, SIGNAL(triggered()), rc, SLOT(dialogFontSelect()));

    // Graphics Grid Lines
    viewmenu->addSeparator();
    graphGridVisibleAct = viewmenu->addAction(QObject::tr("Graphics Window Grid &Lines"));
	graphGridVisibleAct->setCheckable(true);
	graphGridVisibleAct->setChecked(false);
	QObject::connect(graphGridVisibleAct, SIGNAL(toggled(bool)), graphwin, SLOT(slotGridLines(bool)));

	// view bars
	viewmenu->addSeparator();
	QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
	QAction *maintbaract = viewtbars->addAction(QObject::tr("&Main"));
	maintbaract->setCheckable(true);
	maintbaract->setChecked(true);
	QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
	if (outwinwgt->usesToolBar())
	{
		QAction *texttbaract = viewtbars->addAction(QObject::tr("&Text Output"));
		texttbaract->setCheckable(true);
		texttbaract->setChecked(false);
		outwinwgt->slotShowToolBar(false);
		QObject::connect(texttbaract, SIGNAL(toggled(bool)), outwinwgt, SLOT(slotShowToolBar(const bool)));
	}
	if (graphwinwgt->usesToolBar())
	{
		QAction *graphtbaract = viewtbars->addAction(QObject::tr("&Graphics Output"));
		graphtbaract->setCheckable(true);
		graphtbaract->setChecked(false);
		graphwinwgt->slotShowToolBar(false);
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
	stopact = runmenu->addAction(QIcon(":images/stop.png"), QObject::tr("&Stop"));
	stopact->setShortcut(Qt::Key_F5 + Qt::SHIFT);
	stopact->setEnabled(false);
	//runmenu->addSeparator();
	//QAction *saveByteCode = runmenu->addAction(QObject::tr("Save Compiled &Byte Code"));
	QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
	QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
	QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
	QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
	//QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

	// Help menu
	QMenu *helpmenu = menuBar()->addMenu(QObject::tr("&Help"));
	#ifdef WIN32PORTABLE
		// in portable mode make doc online and context help online
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

	// position where it was last on screen
    SETTINGS;
	resize(settings.value(SETTINGSSIZE, QSize(800, 600)).toSize());
	move(settings.value(SETTINGSPOS, QPoint(100, 100)).toPoint());

    // set initial font
    QFont initialFont;
    QString initialFontString = settings.value(SETTINGSFONT,SETTINGSFONTDEFAULT).toString();
    if (initialFont.fromString(initialFontString)) {
        editwin->setFont(initialFont);
        outwin->setFont(initialFont);
    }


}

MainWindow::~MainWindow()
{
	//printf("mwdestroy\n");
	delete rc;
	delete editsyntax;
	delete mutex;
	delete waitCond;
	delete editwin;
	delete outwin;
	delete graphwin;
}

void MainWindow::about()
{
	#ifdef WIN32PORTABLE
		#define PORTABLE QObject::tr(" Portable")
	#else
		#define PORTABLE ""
	#endif
		
	QMessageBox::about(this, QObject::tr("About BASIC-256") +  PORTABLE,
		QObject::tr("<h2>BASIC-256") + PORTABLE + QObject::tr("</h2>") +
		QObject::tr("version <b>") +  VERSION + QObject::tr("</b> - built with QT <b>") + QT_VERSION_STR +QObject::tr("</b>") +
		QObject::tr("<p>Copyright &copy; 2006-2010, The BASIC-256 Team</p>") + 
		QObject::tr("<p>Please visit our web site at <a href=http://www.basic256.org>basic256.org</a> for tutorials and documentation.</p>") +
		QObject::tr("<p>Please see the CONTRIBUTORS file for a list of developers and translators for this project.</p>") +
		QObject::tr("<p><i>You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.</i></p>")
		);

   #undef PORTABLE
}

void MainWindow::updateRecent()
{
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
			recentact[i]->setEnabled(fn.length()!=0);		
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

void MainWindow::loadAndGoMode()
{
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
		// save current screen posision
		SETTINGS;
		settings.setValue(SETTINGSSIZE, size());
		settings.setValue(SETTINGSPOS, pos());

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


	
