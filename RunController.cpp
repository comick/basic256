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
#include <QMessageBox>
#include <QFile>
#include <QSound>
#include <QApplication>



#include "RunController.h"

#ifdef WIN32
	#include <windows.h> 
#else
	#include <speak_lib.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <math.h>
	#include <linux/soundcard.h>
#endif

QMutex mutex;
QMutex debugmutex;
QWaitCondition waitCond;
QWaitCondition waitDebugCond;
QWaitCondition waitInput;

QSound wavsound(QString(""));

RunController::RunController(MainWindow *mw)
{
	mainwin = mw;
	//i = new Interpreter(mainwin->goutput->image, mainwin->goutput->imask);
	i = new Interpreter(mainwin->goutput);
	te = mainwin->editor;
	output = mainwin->output;
	goutput = mainwin->goutput;
	statusbar = mainwin->statusBar();

	QObject::connect(i, SIGNAL(runFinished()), this, SLOT(stopRun()));
	QObject::connect(i, SIGNAL(goutputReady()), this, SLOT(goutputFilter()));
	QObject::connect(i, SIGNAL(resizeGraph(int, int)), this, SLOT(goutputResize(int, int)));
	QObject::connect(i, SIGNAL(outputReady(QString)), this, SLOT(outputFilter(QString)));
	QObject::connect(i, SIGNAL(clearText()), this, SLOT(outputClear()));

	QObject::connect(this, SIGNAL(runPaused()), i, SLOT(pauseResume()));
	QObject::connect(this, SIGNAL(runResumed()), i, SLOT(pauseResume()));
	QObject::connect(this, SIGNAL(runHalted()), i, SLOT(stop()));

	QObject::connect(i, SIGNAL(inputNeeded()), output, SLOT(getInput()));
	QObject::connect(output, SIGNAL(inputEntered(QString)), this, SLOT(inputFilter(QString)));
	QObject::connect(output, SIGNAL(inputEntered(QString)), i, SLOT(receiveInput(QString)));

	QObject::connect(i, SIGNAL(goToLine(int)), te, SLOT(goToLine(int)));

	QObject::connect(i, SIGNAL(soundReady(int, int)), this, SLOT(playSound(int, int)));
	QObject::connect(i, SIGNAL(speakWords(QString)), this, SLOT(speakWords(QString)));
	QObject::connect(i, SIGNAL(playWAV(QString)), this, SLOT(playWAV(QString)));
	QObject::connect(i, SIGNAL(stopWAV()), this, SLOT(stopWAV()));

	QObject::connect(i, SIGNAL(highlightLine(int)), te, SLOT(highlightLine(int)));
	QObject::connect(i, SIGNAL(varAssignment(QString, QString, int)), mainwin->vardock, SLOT(addVar(QString, QString, int)));
}

void
RunController::playSound(int frequency, int duration)
{
#ifdef WIN32
	Beep(frequency, duration);
#else
	// Code based on idea from TONEGEN - Plays a sine wave via the dsp or standard out.
	// Copyright (C) 2000 Timothy Pozar pozar@lns.com
 
	int stereo = 0;		// mono (stereo - false)
	int rate = 44100;
	int devfh;
	int i, test;
	unsigned short int *p;

	// lets build one cycle from zero back to zero in buffer p
	// nwords = length of p
	int nwords = rate / frequency;
 	p = (unsigned short int *) malloc(nwords * sizeof(unsigned short int));
	for(i = 0; i < nwords; i++) {
		p[i] = (sin(2 * M_PI * i / nwords) + 1) * 0x7fff;
	}

	int cyclesout = frequency * duration / 1000;	// number of complete cycles to output

	if ((devfh = open("/dev/dsp", O_RDWR)) != -1) {
		// Set mono
		test = stereo;
		if(ioctl(devfh, SNDCTL_DSP_STEREO, &stereo) != -1) {
			// set the sample rate
			test = rate;
			if(ioctl( devfh, SNDCTL_DSP_SPEED, &test) != -1) {
				for (i=0; i<cyclesout; i++) {
        				int outwords = write(devfh, p, nwords);
				}
			}
		}
		close(devfh);
	}
#endif
}


void
RunController::speakWords(QString text)
{
#ifdef WIN32
	// bad implementation - should use espeak library and call directly j.m.reneau (2009/10/26)
	// espeak command_line folder needs to be added to system path
	int foo = text.remove(QChar('"'), Qt::CaseInsensitive);
	QString command = QString("espeak \"") + text + QString("\"");
	system(command.toLatin1());
#else
	//*nix variants call espeak library directly

	char *data_path = NULL;   // use default path for espeak-data
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;

	int samplerate = espeak_Initialize(AUDIO_OUTPUT_PLAYBACK,0,data_path,0);
        if (samplerate!=-1) {
		espeak_SetVoiceByName("default");
		int size=text.length()+1;	// buffer length
		espeak_Synth(text.toLatin1(),size,0,POS_CHARACTER,0,synth_flags,NULL,NULL);
		espeak_Synchronize();		// wait to finish
		espeak_Terminate();		// clean up
	}
#endif
}

void RunController::playWAV(QString file)
{
	if(QSound::isAvailable()) {
		wavsound.play(file);
	}
}

void RunController::stopWAV()
{
	if(QSound::isAvailable()) {
		wavsound.stop();
	}
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
RunController::goutputResize(int width, int height)
{
	mutex.lock();
	goutput->resize(width, height);
	goutput->setMinimumSize(goutput->image->width(), goutput->image->height());
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

	stopWAV();

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


