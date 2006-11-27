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

#include "RunController.h"
#include "PauseButton.h"
#include "DockWidget.h"
#include "MainWindow.h"

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
   	goutput->setMinimumSize(300, 300);

 	DockWidget * gdock = new DockWidget(this);
	gdock->setObjectName( "gdock" );
	DockWidget * tdock = new DockWidget(this);
	tdock->setObjectName( "tdock" );
	
	BasicWidget * editorwgt = new BasicWidget();
	editorwgt->setViewWidget(editor);
	BasicWidget * outputwgt = new BasicWidget(QObject::tr("Text Output"));
	outputwgt->setViewWidget(output);
	BasicWidget * goutputwgt = new BasicWidget(QObject::tr("Graphics Output"));
	goutputwgt->setViewWidget(goutput);
	
	RunController *rc = new RunController(this);

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
  	QAction *newact = filemenu->addAction(QIcon(":images/new.png"), QObject::tr("New"));
  	QAction *openact = filemenu->addAction(QIcon(":images/open.png"), QObject::tr("Open"));
  	QAction *saveact = filemenu->addAction(QIcon(":images/save.png"), QObject::tr("Save"));
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
	QMenu *viewmenu = menuBar()->addMenu(QObject::tr("View"));
	QAction *textWinVisibleAct = viewmenu->addAction(QObject::tr("Text Window"));
	QAction *graphWinVisibleAct = viewmenu->addAction(QObject::tr("Graphics Window"));
	textWinVisibleAct->setCheckable(true);
	graphWinVisibleAct->setCheckable(true);
	textWinVisibleAct->setChecked(true);
	graphWinVisibleAct->setChecked(true);
	QObject::connect(textWinVisibleAct, SIGNAL(toggled(bool)), tdock, SLOT(setVisible(bool)));
	QObject::connect(graphWinVisibleAct, SIGNAL(toggled(bool)), gdock, SLOT(setVisible(bool)));

	QMenu *viewtbars = viewmenu->addMenu(QObject::tr("Toolbars"));	
	QAction *maintbaract = viewtbars->addAction(QObject::tr("Main"));
	maintbaract->setCheckable(true);
	maintbaract->setChecked(true);
	QObject::connect(maintbaract, SIGNAL(toggled(bool)), maintbar, SLOT(setVisible(bool)));
	if (outputwgt->usesToolBar())
	{
		QAction *texttbaract = viewtbars->addAction(QObject::tr("Text Output"));
		texttbaract->setCheckable(true);
		texttbaract->setChecked(false);
		outputwgt->slotShowToolBar(false);
		QObject::connect(texttbaract, SIGNAL(toggled(bool)), outputwgt, SLOT(slotShowToolBar(const bool)));
	}
	if (goutputwgt->usesToolBar())
	{
		QAction *graphtbaract = viewtbars->addAction(QObject::tr("Graphics Output"));
		graphtbaract->setCheckable(true);
		graphtbaract->setChecked(false);
		goutputwgt->slotShowToolBar(false);
		QObject::connect(graphtbaract, SIGNAL(toggled(bool)), goutputwgt, SLOT(slotShowToolBar(const bool)));
	}
	
	// Run menu
  	QMenu *runmenu = menuBar()->addMenu(QObject::tr("Run"));
  	runact = runmenu->addAction(QIcon(":images/run.png"), QObject::tr("Run"));
  	debugact = runmenu->addAction(QIcon(":images/debug.png"), QObject::tr("Debug"));
  	stepact = runmenu->addAction(QIcon(":images/step.png"), QObject::tr("Step"));
	stepact->setEnabled(false);
  	stopact = runmenu->addAction(QIcon(":images/stop.png"), QObject::tr("Stop"));
	stopact->setEnabled(false);
  	runmenu->addSeparator();
  	QAction *saveByteCode = runmenu->addAction(QObject::tr("Save Compiled Byte Code"));
  	QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
  	QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
  	QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
  	QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
  	QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

	// About menu
  	QMenu *aboutmenu = menuBar()->addMenu(QObject::tr("About"));
  	QAction *aboutb256 = aboutmenu->addAction(QObject::tr("About BASIC-256"));
  	QObject::connect(aboutb256, SIGNAL(triggered()), aboutdialog, SLOT(show()));

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
  	maintbar->addAction(cutact);
  	maintbar->addAction(copyact);
  	maintbar->addAction(pasteact);
	
  	gdock->setFeatures(gdock->features() ^ QDockWidget::DockWidgetClosable);
  	tdock->setFeatures(tdock->features() ^ QDockWidget::DockWidgetClosable);

	gdock->setWidget(goutputwgt);
  	gdock->setWindowTitle(QObject::tr("Graphics Output"));

	tdock->setWidget(outputwgt);
  	tdock->setWindowTitle(QObject::tr("Text Output"));

	setCentralWidget(editorwgt);
  	addDockWidget(Qt::RightDockWidgetArea, tdock);
  	addDockWidget(Qt::RightDockWidgetArea, gdock);  
}

MainWindow::~MainWindow()
{
	
}
