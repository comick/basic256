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

#include <QApplication>
#include <QGridLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QDialog>
#include <QLabel>
#include <QString>
#include <stdio.h>
using namespace std;

#include "RunController.h"
#include "PauseButton.h"
#include "DockWidget.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f)
		:	QMainWindow(parent, f)
{
	QWidget * centerWidget = new QWidget();
	centerWidget->setObjectName( "centerWidget" );

	editor = new BasicEdit(this);
	editor->setObjectName( "editor" );

	output = new BasicOutput();
	output->setObjectName( "output" );
	output->setReadOnly(true);

	goutput = new BasicGraph(output);
	goutput->setObjectName( "goutput" );

	DockWidget * gdock = new DockWidget(this);
	gdock->setObjectName( "gdock" );
	DockWidget * tdock = new DockWidget(this);
	tdock->setObjectName( "tdock" );

	vardock = new VariableWin(this);

	BasicWidget * editorwgt = new BasicWidget();
	editorwgt->setViewWidget(editor);
	BasicWidget * outputwgt = new BasicWidget(QObject::tr("Text Output"));
	outputwgt->setViewWidget(output);
	BasicWidget * goutputwgt = new BasicWidget(QObject::tr("Graphics Output"));
	goutputwgt->setViewWidget(goutput);

	RunController *rc = new RunController(this);
	editsyntax = new EditSyntaxHighlighter(editor->document());

	QDialog *aboutdialog = new QDialog();
	char* abouttext = (char *) malloc(2048);
	sprintf(abouttext,"<h2 align='center'>BASIC-256 -- Version %s</h2> \
		<p>Copyright &copy; 2006, The BASIC-256 Team</p> \
		<p>Please visit our web site at <a href=http://www.basic256.org>www.basic256.org</a> for tutorials and documentation.</p> \
		<p>Please see the CONTRIBUTORS file for a list of developers and translators for this project.</p>\
		<p><i>You should have received a copy of the GNU General Public License along<br> \
		with this program; if not, write to the Free Software Foundation, Inc.,<br> \
		51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.</i></p>",VERSION);

	QLabel *aboutlabel = new QLabel(QObject::tr(abouttext), aboutdialog);
	QGridLayout *aboutgrid = new QGridLayout();
	free(abouttext);

	aboutgrid->addWidget(aboutlabel, 0, 0);
	aboutdialog->setLayout(aboutgrid);

	// Main window toolbar
	QToolBar *maintbar = new QToolBar();
	addToolBar(maintbar);

	// File menu
	QMenu *filemenu = menuBar()->addMenu(QObject::tr("&File"));
	QAction *newact = filemenu->addAction(QIcon(":images/new.png"), QObject::tr("&New"));
	newact->setShortcut(Qt::Key_N + Qt::CTRL);
	QAction *openact = filemenu->addAction(QIcon(":images/open.png"), QObject::tr("&Open"));
	openact->setShortcut(Qt::Key_O + Qt::CTRL);
	QAction *saveact = filemenu->addAction(QIcon(":images/save.png"), QObject::tr("&Save"));
	saveact->setShortcut(Qt::Key_S + Qt::CTRL);
	QAction *saveasact = filemenu->addAction(QIcon(":images/saveas.png"), QObject::tr("Save &As"));
	saveasact->setShortcut(Qt::Key_S + Qt::CTRL + Qt::SHIFT);
	filemenu->addSeparator();
	QAction *printact = filemenu->addAction(QIcon(":images/print.png"), QObject::tr("&Print"));
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
	QAction *exitact = filemenu->addAction(QIcon(":images/exit.png"), QObject::tr("&Exit"));
	exitact->setShortcut(Qt::Key_Q + Qt::CTRL);
	//
	QObject::connect(filemenu, SIGNAL(aboutToShow()), this, SLOT(updateRecent()));
	QObject::connect(newact, SIGNAL(triggered()), editor, SLOT(newProgram()));
	QObject::connect(openact, SIGNAL(triggered()), editor, SLOT(loadProgram()));
	QObject::connect(saveact, SIGNAL(triggered()), editor, SLOT(saveProgram()));
	QObject::connect(saveasact, SIGNAL(triggered()), editor, SLOT(saveAsProgram()));
	QObject::connect(printact, SIGNAL(triggered()), editor, SLOT(slotPrint()));
	QObject::connect(recentact[0], SIGNAL(triggered()), editor, SLOT(loadRecent0()));
	QObject::connect(recentact[1], SIGNAL(triggered()), editor, SLOT(loadRecent1()));
	QObject::connect(recentact[2], SIGNAL(triggered()), editor, SLOT(loadRecent2()));
	QObject::connect(recentact[3], SIGNAL(triggered()), editor, SLOT(loadRecent3()));
	QObject::connect(recentact[4], SIGNAL(triggered()), editor, SLOT(loadRecent4()));
	QObject::connect(recentact[5], SIGNAL(triggered()), editor, SLOT(loadRecent5()));
	QObject::connect(recentact[6], SIGNAL(triggered()), editor, SLOT(loadRecent6()));
	QObject::connect(recentact[7], SIGNAL(triggered()), editor, SLOT(loadRecent7()));
	QObject::connect(recentact[8], SIGNAL(triggered()), editor, SLOT(loadRecent8()));
	QObject::connect(exitact, SIGNAL(triggered()), this, SLOT(close()));

	// Edit menu
	QMenu *editmenu = menuBar()->addMenu(QObject::tr("&Edit"));
	QAction *undoact = editmenu->addAction(QIcon(":images/undo.png"), QObject::tr("&Undo"));
	QObject::connect(undoact, SIGNAL(triggered()), editor, SLOT(undo()));
	undoact->setShortcut(Qt::Key_U + Qt::CTRL);
	QAction *redoact = editmenu->addAction(QIcon(":images/redo.png"), QObject::tr("&Redo"));
	QObject::connect(redoact, SIGNAL(triggered()), editor, SLOT(redo()));
	redoact->setShortcut(Qt::Key_R + Qt::CTRL);
	editmenu->addSeparator();
	QAction *cutact = editmenu->addAction(QIcon(":images/cut.png"), QObject::tr("Cu&t"));
	cutact->setShortcut(Qt::Key_X + Qt::CTRL);
	QAction *copyact = editmenu->addAction(QIcon(":images/copy.png"), QObject::tr("&Copy"));
	copyact->setShortcut(Qt::Key_C + Qt::CTRL);
	QAction *pasteact = editmenu->addAction(QIcon(":images/paste.png"), QObject::tr("&Paste"));
	pasteact->setShortcut(Qt::Key_P + Qt::CTRL);
	editmenu->addSeparator();
	QAction *selectallact = editmenu->addAction(QObject::tr("Select &All"));
	selectallact->setShortcut(Qt::Key_A + Qt::CTRL);
	editmenu->addSeparator();
	QAction *beautifyact = editmenu->addAction(QObject::tr("&Beautify"));
	editmenu->addSeparator();
	QAction *prefact = editmenu->addAction(QIcon(":images/preferences.png"), QObject::tr("Preferences"));
	QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));
	//
	QObject::connect(cutact, SIGNAL(triggered()), editor, SLOT(cut()));
	QObject::connect(copyact, SIGNAL(triggered()), editor, SLOT(copy()));
	QObject::connect(pasteact, SIGNAL(triggered()), editor, SLOT(paste()));
	QObject::connect(selectallact, SIGNAL(triggered()), editor, SLOT(selectAll()));
	QObject::connect(beautifyact, SIGNAL(triggered()), editor, SLOT(beautifyProgram()));

	bool extraSepAdded = false;
	if (outputwgt->usesMenu())
	{
		editmenu->addSeparator();
		extraSepAdded = true;
		editmenu->addMenu(outputwgt->getMenu());
	}
	if (goutputwgt->usesMenu())
	{
		if (!extraSepAdded)
		{
			editmenu->addSeparator();
		}
		editmenu->addMenu(goutputwgt->getMenu());
	}

	// View menuBar
	QMenu *viewmenu = menuBar()->addMenu(QObject::tr("&View"));
	QAction *textWinVisibleAct = viewmenu->addAction(QObject::tr("&Text Window"));
	QAction *graphWinVisibleAct = viewmenu->addAction(QObject::tr("&Graphics Window"));
	QAction *variableWinVisibleAct = viewmenu->addAction(QObject::tr("&Variable Watch Window"));
	textWinVisibleAct->setCheckable(true);
	graphWinVisibleAct->setCheckable(true);
	variableWinVisibleAct->setCheckable(true);
	textWinVisibleAct->setChecked(true);
	graphWinVisibleAct->setChecked(true);
	variableWinVisibleAct->setChecked(false);
	QObject::connect(textWinVisibleAct, SIGNAL(toggled(bool)), tdock, SLOT(setVisible(bool)));
	QObject::connect(graphWinVisibleAct, SIGNAL(toggled(bool)), gdock, SLOT(setVisible(bool)));
	QObject::connect(variableWinVisibleAct, SIGNAL(toggled(bool)), vardock, SLOT(setVisible(bool)));

	QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
	QAction *maintbaract = viewtbars->addAction(QObject::tr("&Main"));
	maintbaract->setCheckable(true);
	maintbaract->setChecked(true);
	QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
	if (outputwgt->usesToolBar())
	{
		QAction *texttbaract = viewtbars->addAction(QObject::tr("&Text Output"));
		texttbaract->setCheckable(true);
		texttbaract->setChecked(false);
		outputwgt->slotShowToolBar(false);
		QObject::connect(texttbaract, SIGNAL(toggled(bool)), outputwgt, SLOT(slotShowToolBar(const bool)));
	}
	if (goutputwgt->usesToolBar())
	{
		QAction *graphtbaract = viewtbars->addAction(QObject::tr("&Graphics Output"));
		graphtbaract->setCheckable(true);
		graphtbaract->setChecked(false);
		goutputwgt->slotShowToolBar(false);
		QObject::connect(graphtbaract, SIGNAL(toggled(bool)), goutputwgt, SLOT(slotShowToolBar(const bool)));
	}
	QMenu *fontsize = viewmenu->addMenu(QObject::tr("&Font Size"));
	QAction *fontSmallAct = fontsize->addAction(QObject::tr("&Small"));
	QAction *fontMediumAct = fontsize->addAction(QObject::tr("&Medium"));
	QAction *fontLargeAct = fontsize->addAction(QObject::tr("&Large"));
	QAction *fontHugeAct = fontsize->addAction(QObject::tr("&Huge"));
	QObject::connect(fontSmallAct, SIGNAL(triggered()), editor, SLOT(fontSmall()));
	QObject::connect(fontMediumAct, SIGNAL(triggered()), editor, SLOT(fontMedium()));
	QObject::connect(fontLargeAct, SIGNAL(triggered()), editor, SLOT(fontLarge()));
	QObject::connect(fontHugeAct, SIGNAL(triggered()), editor, SLOT(fontHuge()));
	QObject::connect(fontSmallAct, SIGNAL(triggered()), output, SLOT(fontSmall()));
	QObject::connect(fontMediumAct, SIGNAL(triggered()), output, SLOT(fontMedium()));
	QObject::connect(fontLargeAct, SIGNAL(triggered()), output, SLOT(fontLarge()));
	QObject::connect(fontHugeAct, SIGNAL(triggered()), output, SLOT(fontHuge()));

	// Run menu
	QMenu *runmenu = menuBar()->addMenu(QObject::tr("&Run"));
	runact = runmenu->addAction(QIcon(":images/run.png"), QObject::tr("&Run"));
	runact->setShortcut(Qt::Key_F5);
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
	QAction *docact = helpmenu->addAction(QIcon(":images/help.png"), QObject::tr("&Help"));
	docact->setShortcut(Qt::Key_F1);
	QObject::connect(docact, SIGNAL(triggered()), rc, SLOT(showDocumentation()));
	QAction *aboutact = helpmenu->addAction(QObject::tr("&About BASIC-256"));
	QObject::connect(aboutact, SIGNAL(triggered()), aboutdialog, SLOT(show()));

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

	gdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	gdock->setWidget(goutputwgt);
	gdock->setWindowTitle(QObject::tr("Graphics Output"));

	tdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	tdock->setWidget(outputwgt);
	tdock->setWindowTitle(QObject::tr("Text Output"));

	vardock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	vardock->setVisible(false);
	vardock->setFloating(true);

	setCentralWidget(editorwgt);
	addDockWidget(Qt::RightDockWidgetArea, tdock);
	addDockWidget(Qt::RightDockWidgetArea, gdock);
	addDockWidget(Qt::LeftDockWidgetArea, vardock);
	setContextMenuPolicy(Qt::NoContextMenu);

	// position where it was last on screen
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	resize(settings.value(SETTINGSSIZE, QSize(800, 600)).toSize());
	move(settings.value(SETTINGSPOS, QPoint(100, 100)).toPoint());

}

void MainWindow::updateRecent()
{
	//update recent list on file menu
	QSettings settings(SETTINGSORG, SETTINGSAPP);
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

}


MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *e) {
	// quit the application but ask if there are unsaved changes in buffer
	bool doquit = true;
	if (editor->codeChanged) {
		QMessageBox msgBox;
		msgBox.setText(tr("Program modifications have not been saved."));
		msgBox.setInformativeText(tr("Do you want to discard your changes?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		doquit = (msgBox.exec() == QMessageBox::Yes);
	}
	if (doquit) {
         e->accept();
     } else {
         e->ignore();
     }
     
     // save current screen posision
     QSettings settings(SETTINGSORG, SETTINGSAPP);
     settings.setValue(SETTINGSSIZE, size());
     settings.setValue(SETTINGSPOS, pos());

}
