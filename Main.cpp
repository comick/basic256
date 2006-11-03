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
#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QMainWindow>
#include <QGridLayout>
#include <QMenuBar>
#include <QDockWidget>
#include <QPushButton>
#include <QStatusBar>
#include "BasicOutput.h"
#include "BasicEdit.h"
#include "BasicGraph.h"
#include "RunController.h"
#include "GhostButton.h"
#include "PauseButton.h"


int main(int argc, char *argv[])
{
  QApplication qapp(argc, argv);

  QTranslator qtTranslator;
  qtTranslator.load("qt_" + QLocale::system().name());
  qapp.installTranslator(&qtTranslator);
  
  QTranslator kbTranslator;
  kbTranslator.load("basic256_" + QLocale::system().name(), "./Translations/");
  qapp.installTranslator(&kbTranslator);

  QMainWindow *mainwin = new QMainWindow();
  QWidget *centerWidget = new QWidget();
  QGridLayout *grid = new QGridLayout();
  BasicEdit *editor = new BasicEdit(mainwin);
  BasicOutput *output = new BasicOutput();
  BasicGraph *goutput = new BasicGraph(output);
  QDockWidget *gdock = new QDockWidget();
  QDockWidget *tdock = new QDockWidget();
  RunController *rc = new RunController(editor, output, goutput, mainwin->statusBar());
  GhostButton *run = new GhostButton(QObject::tr("Run"));
  PauseButton *pause = new PauseButton(QObject::tr("Pause"));
  GhostButton *stop = new GhostButton(QObject::tr("Stop"));
  QPushButton *step = new QPushButton("Step");


  QMenu *filemenu = mainwin->menuBar()->addMenu(QObject::tr("File"));
  QAction *newact = filemenu->addAction(QObject::tr("New"));
  QAction *openact = filemenu->addAction(QObject::tr("Open"));
  QAction *saveact = filemenu->addAction(QObject::tr("Save"));
  QAction *saveasact = filemenu->addAction(QObject::tr("Save As"));
  QAction *exitact = filemenu->addAction(QObject::tr("Exit"));
  QObject::connect(newact, SIGNAL(triggered()), editor, SLOT(newProgram()));
  QObject::connect(openact, SIGNAL(triggered()), editor, SLOT(loadProgram()));
  QObject::connect(saveact, SIGNAL(triggered()), editor, SLOT(saveProgram()));
  QObject::connect(saveasact, SIGNAL(triggered()), editor, SLOT(saveAsProgram()));
  QObject::connect(exitact, SIGNAL(triggered()), &qapp, SLOT(closeAllWindows()));

  QMenu *editmenu = mainwin->menuBar()->addMenu(QObject::tr("Edit"));
  QAction *cutact = editmenu->addAction(QObject::tr("Cut"));
  QAction *copyact = editmenu->addAction(QObject::tr("Copy"));
  QAction *pasteact = editmenu->addAction(QObject::tr("Paste"));
  editmenu->addSeparator();
  QAction *selectallact = editmenu->addAction(QObject::tr("Select All"));
  QObject::connect(cutact, SIGNAL(triggered()), editor, SLOT(cut()));
  QObject::connect(copyact, SIGNAL(triggered()), editor, SLOT(copy()));
  QObject::connect(pasteact, SIGNAL(triggered()), editor, SLOT(paste()));
  QObject::connect(selectallact, SIGNAL(triggered()), editor, SLOT(selectAll()));

  QMenu *advancedmenu = mainwin->menuBar()->addMenu(QObject::tr("Advanced"));
  QAction *saveByteCode = advancedmenu->addAction(QObject::tr("Save Compiled Byte Code"));
  QObject::connect(saveByteCode, SIGNAL(triggered()), rc, SLOT(saveByteCode()));

  QObject::connect(step, SIGNAL(pressed()), rc, SLOT(stepThrough()));

  QObject::connect(run, SIGNAL(pressed()), rc, SLOT(startRun()));
  QObject::connect(rc, SIGNAL(runStarted()), run, SLOT(disableButton()));
  QObject::connect(rc, SIGNAL(runHalted()), run, SLOT(enableButton()));

  QObject::connect(stop, SIGNAL(pressed()), rc, SLOT(stopRun()));
  QObject::connect(rc, SIGNAL(runStarted()), stop, SLOT(enableButton()));
  QObject::connect(rc, SIGNAL(runHalted()), stop, SLOT(disableButton()));
  stop->disableButton();

  QObject::connect(pause, SIGNAL(pressed()), rc, SLOT(pauseResume()));
  QObject::connect(rc, SIGNAL(runStarted()), pause, SLOT(enableButton()));
  QObject::connect(rc, SIGNAL(runHalted()), pause, SLOT(disableButton()));
  pause->disableButton();

  grid->addWidget(editor, 0, 0, 1, 4);
  grid->addWidget(run, 2, 0);
  grid->addWidget(pause, 2, 1);
  grid->addWidget(stop, 2, 2);
  grid->addWidget(step, 2, 3);

  gdock->setFeatures(gdock->features() ^ QDockWidget::DockWidgetClosable);
  tdock->setFeatures(tdock->features() ^ QDockWidget::DockWidgetClosable);

  gdock->setWidget(goutput);
  gdock->setWindowTitle(QObject::tr("Graphics Output"));
  tdock->setWidget(output);
  tdock->setWindowTitle(QObject::tr("Text Output"));

  centerWidget->setLayout(grid);
  mainwin->setCentralWidget(centerWidget);
  mainwin->addDockWidget(Qt::RightDockWidgetArea, tdock);
  mainwin->addDockWidget(Qt::RightDockWidgetArea, gdock);
  
  goutput->setMinimumSize(300, 300);
  mainwin->setWindowState(mainwin->windowState() & Qt::WindowMaximized);
  
  output->setReadOnly(true);
  
  mainwin->resize(800,600);
  mainwin->statusBar()->showMessage(QObject::tr("Ready."));
  mainwin->setWindowTitle(QObject::tr("Untitled - BASIC-256"));
  mainwin->show();
  return qapp.exec();
}
