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
#include <QMutex>
#include <QWaitCondition>
#include <QFileDialog>
#include <QFile>

#include "RunController.h"

QMutex mutex;
QMutex debugmutex;
QWaitCondition waitCond;
QWaitCondition waitDebugCond;
QWaitCondition waitInput;

RunController::RunController(MainWindow *mw)
{
  mainwin = mw;
  i = new Interpreter(mainwin->goutput->image, mainwin->goutput->imask);
  te = mainwin->editor;
  output = mainwin->output;
  goutput = mainwin->goutput;
  statusbar = mainwin->statusBar();

  QObject::connect(i, SIGNAL(runFinished()), this, SLOT(stopRun()));
  QObject::connect(i, SIGNAL(goutputReady()), this, SLOT(goutputFilter()));
  QObject::connect(i, SIGNAL(outputReady(QString)), this, SLOT(outputFilter(QString)));
  QObject::connect(i, SIGNAL(clearText()), this, SLOT(outputClear()));

  QObject::connect(this, SIGNAL(runPaused()), i, SLOT(pauseResume()));
  QObject::connect(this, SIGNAL(runResumed()), i, SLOT(pauseResume()));
  QObject::connect(this, SIGNAL(runHalted()), i, SLOT(stop()));

  QObject::connect(i, SIGNAL(inputNeeded()), output, SLOT(getInput()));
  QObject::connect(output, SIGNAL(inputEntered(QString)), this, SLOT(inputFilter(QString)));
  QObject::connect(output, SIGNAL(inputEntered(QString)), i, SLOT(receiveInput(QString)));

  QObject::connect(i, SIGNAL(goToLine(int)), te, SLOT(goToLine(int)));

  QObject::connect(i, SIGNAL(highlightLine(int)), te, SLOT(highlightLine(int)));
  QObject::connect(i, SIGNAL(varAssignment(QString, QString, int)), mainwin->vardock, SLOT(addVar(QString, QString, int)));
}


void
RunController::startDebug()
{
  if (i->isStopped())
    {
      int result = i->compileProgram((te->toPlainText() + "\n").toAscii().data());
      if (result < 0)
	{
	  i->debugMode = false;
	  emit(runHalted());
	  return;
	}
      i->initialize();
      i->debugMode = true;
      output->clear();
      statusbar->showMessage(tr("Running"));
      goutput->setFocus();
      i->start();
      mainwin->vardock->clearTable();
      mainwin->runact->setEnabled(false);
      mainwin->debugact->setEnabled(false);
      mainwin->stepact->setEnabled(true);
      mainwin->stopact->setEnabled(true);
      emit(debugStarted());
    }
}

void 
RunController::startRun()
{
  if (i->isStopped())
    {
      int result = i->compileProgram((te->toPlainText() + "\n").toAscii().data());
      i->debugMode = false;
      if (result < 0)
	{
	  emit(runHalted());
	  return;
	}
      i->initialize();
      output->clear();
      statusbar->showMessage(tr("Running"));
      goutput->setFocus();
      i->start();
      mainwin->vardock->clearTable();
      mainwin->runact->setEnabled(false);
      mainwin->debugact->setEnabled(false);
      mainwin->stepact->setEnabled(false);
      mainwin->stopact->setEnabled(true);
      emit(runStarted());
    }
}


void
RunController::inputFilter(QString text)
{
  goutput->setFocus();
  mutex.lock();
  waitInput.wakeAll();
  mutex.unlock();
}

void
RunController::outputClear()
{
  mutex.lock();
  output->clear();
  waitCond.wakeAll();
  mutex.unlock();
}

void
RunController::outputFilter(QString text)
{
  mutex.lock();
  output->insertPlainText(text);
  output->ensureCursorVisible();
  waitCond.wakeAll();
  mutex.unlock();
}

void
RunController::goutputFilter()
{
  mutex.lock();
  goutput->repaint();
  waitCond.wakeAll();
  mutex.unlock();
}

void
RunController::stepThrough()
{
  debugmutex.lock();
  waitDebugCond.wakeAll();
  debugmutex.unlock();
}

void 
RunController::stopRun()
{
  statusbar->showMessage(tr("Ready."));

  mainwin->runact->setEnabled(true);
  mainwin->debugact->setEnabled(true);
  mainwin->stepact->setEnabled(false);
  mainwin->stopact->setEnabled(false);

  //need to fix waiting for input here.
  mutex.lock();
  waitInput.wakeAll();
  waitCond.wakeAll();
  mutex.unlock();

  debugmutex.lock();
  i->debugMode = false;
  waitDebugCond.wakeAll();
  debugmutex.unlock();

  emit(runHalted());
}


void
RunController::pauseResume()
{
  if (paused)
    {
      statusbar->showMessage(tr("Running"));
      paused = false;
      emit(runResumed());
    }
  else 
    {
      statusbar->showMessage(tr("Paused"));
      paused = true;
      emit(runPaused());
    }
}



void
RunController::saveByteCode()
{
  byteCodeData *bc = i->getByteCode((te->toPlainText() + "\n").toAscii().data());
  if (bc == NULL)
    {
      return;
    }

  if (bytefilename == "")
    {
      bytefilename = QFileDialog::getSaveFileName(NULL, tr("Save file as"), ".", tr("BASIC-256 Compiled File ") + "(*.kbc);;" + tr("Any File ") + "(*.*)");
    }

  if (bytefilename != "")
    {
      QRegExp rx("\\.[^\\/]*$");
      if (rx.indexIn(bytefilename) == -1)
	{
	  bytefilename += ".kbc";
	  //FIXME need to test for existence here
	}
      QFile f(bytefilename);
      f.open(QIODevice::WriteOnly | QIODevice::Truncate);
      f.write((const char *) bc->data, bc->size);
      f.close();
    }
  delete bc;
}


