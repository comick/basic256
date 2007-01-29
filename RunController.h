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


#ifndef __RUNCONTROLLER_H
#define __RUNCONTROLLER_H
 
#include <QTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include "BasicEdit.h"
#include "BasicOutput.h"
#include "BasicGraph.h"
#include "Interpreter.h"
#include "MainWindow.h"


class RunController : public QObject
{
  Q_OBJECT;
 public:
  RunController(MainWindow *);

 signals:
  void debugStarted();
  void runStarted();
  void runHalted();
  void runPaused();
  void runResumed();

 public slots:
  void playSound(int, int);
  void inputFilter(QString text);
  void outputFilter(QString text);
  void outputClear();
  void goutputFilter();
  void startDebug();
  void startRun();
  void stopRun();
  void pauseResume();
  void saveByteCode();
  void stepThrough();

 private:
  Interpreter *i;
  BasicEdit *te;
  BasicOutput *output;
  BasicGraph *goutput;
  QStatusBar *statusbar;
  bool paused;
  run_status oldStatus;
  QString bytefilename;
  MainWindow *mainwin;
};



#endif
