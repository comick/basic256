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


using namespace std;

#include <QApplication>
#include <QGridLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QDialog>
#include <QLabel>

#include "BasicWidget.h"
#include "BasicOutput.h"
#include "BasicEdit.h"
#include "BasicGraph.h"
#include "RunController.h"
#include "GhostButton.h"
#include "PauseButton.h"
#include "DockWidget.h"
#include "MainWindow.h"

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f)
:	QMainWindow(parent, f)
{
  	QWidget *centerWidget = new QWidget();
	centerWidget->setObjectName( "centerWidget" );
  	QGridLayout *grid = new QGridLayout();
	
  	BasicEdit *editor = new BasicEdit(this);
	editor->setObjectName( "editor" );
  	BasicOutput *output = new BasicOutput();
	output->setObjectName( "output" );
	output->setReadOnly(true);
	BasicGraph *goutput = new BasicGraph(output);
	goutput->setObjectName( "goutput" );
   	goutput->setMinimumSize(300, 300);

 	DockWidget * gdock = new DockWidget(this);
	gdock->setObjectName( "gdock" );
	DockWidget * tdock = new DockWidget(this);
	tdock->setObjectName( "tdock" );
	
	BasicWidget * editorwgt = new BasicWidget();
	editorwgt->setViewWidget(editor);
	BasicWidget * outputwgt = new BasicWidget();
	outputwgt->setViewWidget(output);
	BasicWidget * goutputwgt = new BasicWidget();
	goutputwgt->setViewWidget(goutput);
	
	RunController *rc = new RunController(editor, output, goutput, statusBar());
  	GhostButton *run = new GhostButton(QObject::tr("Run"));
  	run->setObjectName( "run" );
  	PauseButton *pause = new PauseButton(QObject::tr("Pause"));
  	pause->setObjectName( "pause" );
  	GhostButton *stop = new GhostButton(QObject::tr("Stop"));
  	stop->setObjectName( "stop" );
  	GhostButton *step = new GhostButton(QObject::tr("Step"));
  	step->setObjectName( "step" );

	QDialog *aboutdialog = new QDialog();
  	QLabel *aboutlabel = new QLabel(QObject::tr("<h2 align='center'>BASIC-256 -- Version 0.8</h2> \
											  <p>Copyright &copy; 2006, Ian Larsen</p>	\
											  <p><strong>Thanks to our translators:</strong> Immo-Gert Birn \
											  <p><i>You should have received a copy of the GNU General Public License along<br> \
											  with this program; if not, write to the Free Software Foundation, Inc.,<br> \
											  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.</i></p>"), aboutdialog);
  	QGridLayout *aboutgrid = new QGridLayout();
  
  	aboutgrid->addWidget(aboutlabel, 0, 0);
  	aboutdialog->setLayout(aboutgrid);
	
	// Main window toolbar
	QToolBar *maintbar = new QToolBar();
  	addToolBar(maintbar);
	
	// File menu
  	QMenu *filemenu = menuBar()->addMenu(QObject::tr("File"));
  	QAction *newact = filemenu->addAction(QObject::tr("New"));
  	QAction *openact = filemenu->addAction(QObject::tr("Open"));
  	QAction *saveact = filemenu->addAction(QObject::tr("Save"));
  	QAction *saveasact = filemenu->addAction(QObject::tr("Save As"));
	filemenu->addSeparator();
	QAction *printact = filemenu->addAction(QObject::tr("Print"));
	filemenu->addSeparator();
  	QAction *exitact = filemenu->addAction(QObject::tr("Exit"));
  	QObject::connect(newact, SIGNAL(triggered()), editor, SLOT(newProgram()));
  	QObject::connect(openact, SIGNAL(triggered()), editor, SLOT(loadProgram()));
  	QObject::connect(saveact, SIGNAL(triggered()), editor, SLOT(saveProgram()));
  	QObject::connect(saveasact, SIGNAL(triggered()), editor, SLOT(saveAsProgram()));
	QObject::connect(printact, SIGNAL(triggered()), editor, SLOT(slotPrint()));
  	QObject::connect(exitact, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

	// Edit menu
  	QMenu *editmenu = menuBar()->addMenu(QObject::tr("Edit"));
  	QAction *cutact = editmenu->addAction(QObject::tr("Cut"));
  	QAction *copyact = editmenu->addAction(QObject::tr("Copy"));
  	QAction *pasteact = editmenu->addAction(QObject::tr("Paste"));
  	editmenu->addSeparator();
  	QAction *selectallact = editmenu->addAction(QObject::tr("Select All"));
  	QObject::connect(cutact, SIGNAL(triggered()), editor, SLOT(cut()));
  	QObject::connect(copyact, SIGNAL(triggered()), editor, SLOT(copy()));
  	QObject::connect(pasteact, SIGNAL(triggered()), editor, SLOT(paste()));
  	QObject::connect(selectallact, SIGNAL(triggered()), editor, SLOT(selectAll()));

	// View menuBar
	QMenu *viewmenu = menuBar()->addMenu(QObject::tr("View"));
	QMenu *viewtbars = viewmenu->addMenu(QObject::tr("Toolbars"));	
	QAction *maintbaract = viewtbars->addAction(QObject::tr("Main"));
	maintbaract->setCheckable(true);
	maintbaract->setChecked(true);
	QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
	if (outputwgt->usesToolBar())
	{
		QAction *texttbaract = viewtbars->addAction(QObject::tr("Text Output"));
		texttbaract->setCheckable(true);
		texttbaract->setChecked(true);
		QObject::connect(texttbaract, SIGNAL(toggled(bool)), outputwgt, SLOT(slotShowToolBar(const bool)));
	}
	if (goutputwgt->usesToolBar())
	{
		QAction *graphtbaract = viewtbars->addAction(QObject::tr("Graphics Output"));
		graphtbaract->setCheckable(true);
		graphtbaract->setChecked(true);
		QObject::connect(graphtbaract, SIGNAL(toggled(bool)), goutputwgt, SLOT(slotShowToolBar(const bool)));
	}
	
	// Advanced menu
  	QMenu *advancedmenu = menuBar()->addMenu(QObject::tr("Advanced"));
  	QAction *debug = advancedmenu->addAction(QObject::tr("Debug"));
  	QAction *saveByteCode = advancedmenu->addAction(QObject::tr("Save Compiled Byte Code"));
  	QObject::connect(debug, SIGNAL(triggered()), rc, SLOT(startDebug()));
  	QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

	// About menu
  	QMenu *aboutmenu = menuBar()->addMenu(QObject::tr("About"));
  	QAction *aboutb256 = aboutmenu->addAction(QObject::tr("About BASIC-256"));
  	QObject::connect(aboutb256, SIGNAL(triggered()), aboutdialog, SLOT(show()));

	// Add actions to main window toolbar
  	maintbar->addAction(newact);
  	maintbar->addAction(openact);
  	maintbar->addAction(saveact);
	maintbar->addSeparator();
  	maintbar->addAction(cutact);
  	maintbar->addAction(copyact);
  	maintbar->addAction(pasteact);
	
	// Step button
  	QObject::connect(step, SIGNAL(pressed()), rc, SLOT(stepThrough()));
  	QObject::connect(rc, SIGNAL(debugStarted()), step, SLOT(enableButton()));
  	QObject::connect(rc, SIGNAL(runHalted()), step, SLOT(disableButton()));
  	step->disableButton();

	// Run button
  	QObject::connect(run, SIGNAL(clicked()), rc, SLOT(startRun()));
  	QObject::connect(rc, SIGNAL(runStarted()), run, SLOT(disableButton()));
  	QObject::connect(rc, SIGNAL(debugStarted()), run, SLOT(disableButton()));
  	QObject::connect(rc, SIGNAL(runHalted()), run, SLOT(enableButton()));

	// Stop button
  	QObject::connect(stop, SIGNAL(pressed()), rc, SLOT(stopRun()));
  	QObject::connect(rc, SIGNAL(debugStarted()), stop, SLOT(enableButton()));
  	QObject::connect(rc, SIGNAL(runStarted()), stop, SLOT(enableButton()));
  	QObject::connect(rc, SIGNAL(runHalted()), stop, SLOT(disableButton()));
  	stop->disableButton();

	// Pause button
  	QObject::connect(pause, SIGNAL(pressed()), rc, SLOT(pauseResume()));
  	QObject::connect(rc, SIGNAL(runStarted()), pause, SLOT(enableButton()));
  	QObject::connect(rc, SIGNAL(runHalted()), pause, SLOT(disableButton()));
  	pause->disableButton();

	grid->addWidget(editorwgt, 0, 0, 1, 4);
  	grid->addWidget(run, 2, 0);
  	grid->addWidget(pause, 2, 1);
  	grid->addWidget(stop, 2, 2);
  	grid->addWidget(step, 2, 3);

  	gdock->setFeatures(gdock->features() ^ QDockWidget::DockWidgetClosable);
  	tdock->setFeatures(tdock->features() ^ QDockWidget::DockWidgetClosable);

	gdock->setWidget(goutputwgt);
  	gdock->setWindowTitle(QObject::tr("Graphics Output"));

	tdock->setWidget(outputwgt);
  	tdock->setWindowTitle(QObject::tr("Text Output"));

  	centerWidget->setLayout(grid);
	setCentralWidget(centerWidget);
  	addDockWidget(Qt::RightDockWidgetArea, tdock);
  	addDockWidget(Qt::RightDockWidgetArea, gdock);  
}

MainWindow::~MainWindow()
{
	
}
