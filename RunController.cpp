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

#include <math.h>
#include <iostream>

#include <QProcess>
#include <QDesktopServices>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>
#include <QDir>
#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QFileDialog>
	#include <QtWidgets/QMessageBox>
	#include <QtWidgets/QInputDialog>
	#include <QtWidgets/QFontDialog>
	#include <QtWidgets/QApplication>
#else
	#include <QFileDialog>
	#include <QMessageBox>
	#include <QInputDialog>
	#include <QFontDialog>
	#include <QApplication>
#endif


using namespace std;

#include "RunController.h"
#include "MainWindow.h"
#include "DocumentationWin.h"
#include "Settings.h"
#include "md5.h"

#ifdef WIN32
	#include <windows.h> 
	#include <servprov.h>
	#include <string>
	#include <cstdlib>
//	#include <mmsystem.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <time.h>
	#include <unistd.h>
#endif

#ifdef LINUX
   // sys/soundcard.h instead of linux/soundcard.h in Debian so that we don't
   // FTBFS on kfreebsd (See Debian bug #594164).
   #include <sys/soundcard.h>
#endif

#ifdef USEQSOUND
	#include <QSound>
	QSound *wavsound;
#endif

#ifdef ESPEAK
	#include <speak_lib.h>
#endif

extern QMutex* mymutex;
extern QMutex* mydebugmutex;
extern QWaitCondition* waitCond;
extern QWaitCondition* waitDebugCond;

extern MainWindow * mainwin;
extern BasicEdit * editwin;
extern BasicOutput * outwin;
extern BasicGraph * graphwin;
extern VariableWin * varwin;



RunController::RunController()
{
	i = new Interpreter();

	replacewin = NULL;
	docwin = NULL;
	
	#ifdef USEQSOUND
		wavsound = new QSound(QString(""));
	#endif

	//signals for the Interperter (i)
	QObject::connect(i, SIGNAL(goToLine(int)), editwin, SLOT(goToLine(int)));
	QObject::connect(i, SIGNAL(highlightLine(int)), editwin, SLOT(highlightLine(int)));
	
	QObject::connect(i, SIGNAL(outputClear()), this, SLOT(outputClear()));
	QObject::connect(i, SIGNAL(dialogAlert(QString)), this, SLOT(dialogAlert(QString)));
	QObject::connect(i, SIGNAL(dialogConfirm(QString, int)), this, SLOT(dialogConfirm(QString, int)));
	QObject::connect(i, SIGNAL(dialogPrompt(QString, QString)), this, SLOT(dialogPrompt(QString, QString)));
	QObject::connect(i, SIGNAL(executeSystem(QString)), this, SLOT(executeSystem(QString)));
	QObject::connect(i, SIGNAL(goutputReady()), this, SLOT(goutputReady()));
	QObject::connect(i, SIGNAL(mainWindowsResize(int, int, int)), this, SLOT(mainWindowsResize(int, int, int)));
	QObject::connect(i, SIGNAL(mainWindowsVisible(int, bool)), this, SLOT(mainWindowsVisible(int, bool)));
	QObject::connect(i, SIGNAL(outputReady(QString)), this, SLOT(outputReady(QString)));
	QObject::connect(i, SIGNAL(playWAV(QString)), this, SLOT(playWAV(QString)));
	QObject::connect(i, SIGNAL(stopRun()), this, SLOT(stopRun()));
	QObject::connect(i, SIGNAL(speakWords(QString)), this, SLOT(speakWords(QString)));
	QObject::connect(i, SIGNAL(stopWAV()), this, SLOT(stopWAV()));
	QObject::connect(i, SIGNAL(waitWAV()), this, SLOT(waitWAV()));
	
	QObject::connect(i, SIGNAL(getInput()), outwin, SLOT(getInput()));

	QObject::connect(i, SIGNAL(varAssignment(int, QString, QString, int, int)), varwin, SLOT(varAssignment(int, QString, QString, int, int)));

	QObject::connect(this, SIGNAL(runPaused()), i, SLOT(runPaused()));
	QObject::connect(this, SIGNAL(runResumed()), i, SLOT(runResumed()));
	QObject::connect(this, SIGNAL(runHalted()), i, SLOT(runHalted()));

	QObject::connect(outwin, SIGNAL(inputEntered(QString)), this, SLOT(inputEntered(QString)));
	QObject::connect(outwin, SIGNAL(inputEntered(QString)), i, SLOT(inputEntered(QString)));


}

RunController::~RunController()
{
	if(replacewin!=NULL) replacewin->close();
	if(docwin!=NULL) docwin->close();

	#ifdef USEQSOUND
		delete wavsound;
	#endif
	delete i;
	
	//printf("rcdestroy\n");
}


void
RunController::speakWords(QString text)
{
	mymutex->lock();
	#ifdef ESPEAK
	    SETTINGS;
		espeak_ERROR err;
		
		// espeak tts library
		int synth_flags = espeakCHARS_UTF8 | espeakPHONEMES | espeakENDPAUSE;
		#ifdef WIN32
			// use program install folder
			int samplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK,0,(char *) QFileInfo(QCoreApplication::applicationFilePath()).absolutePath().toUtf8().data(),0);
		#else
			// use default path for espeak-data
			int samplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK,0,NULL,0);
		#endif
		if (samplerate!=-1) {
			QString voicename = settings.value(SETTINGSESPEAKVOICE,SETTINGSESPEAKVOICEDEFAULT).toString();
			err = espeak_SetVoiceByName(voicename.toUtf8());
			if(err==EE_OK) {
				int size=text.length()+1;	// buffer length
				err = espeak_Synth(text.toUtf8(),size,0,POS_CHARACTER,0,synth_flags,NULL,NULL);
				if (err==EE_OK) {
					espeak_Synchronize();		// wait to finish
					espeak_Terminate();		// clean up
				} else {
					printf("espeak synth error %i\n", err);
				}
			} else {
				printf("espeak st voice error %i\n", err);
			}
		} else {
			printf("Unable to initialize espeak\n");
		}
	#endif
	#ifdef ESPEAK_EXECUTE
		// easy espeak implementation when all else fails
		text.replace("\""," quote ");
		text.prepend("espeak \"");
		text.append("\"");
		executeSystem(text);
	#endif
	#ifdef MACX_SAY
		// easy macosX implementation - call the command line say statement
		text.replace("\""," quote ");
		text.prepend("say \"");
		text.append("\"");
		executeSystem(text);
	#endif
	waitCond->wakeAll();
	mymutex->unlock();

}

void
RunController::executeSystem(QString text)
{
	// need to implement system as a function to return process output
	// and to handle input
	
	QProcess sy;
	//fprintf(stderr,"system b4 %s\n", text);
	mymutex->lock();
	
	
     sy.start(text);
     if (sy.waitForStarted()) {

		//sy.write("Qt rocks!");
		//sy.closeWriteChannel();

		if (!sy.waitForFinished()) {
			//QByteArray result = sy.readAll();
		}
	 }
	
	
	//system(text);
	waitCond->wakeAll();
	mymutex->unlock();
	//fprintf(stderr,"system af %s\n", text);
}

void RunController::playWAV(QString file)
{
	mymutex->lock();
	#ifdef USEQSOUND
		wavsound->play(file);
	#endif
	#ifdef USESDL
		Mix_HaltChannel(SDL_CHAN_WAV);
		Mix_Chunk *music;
    	music = Mix_LoadWAV((char *) file.toUtf8().data());
    	Mix_PlayChannel(SDL_CHAN_WAV,music,0);
	#endif
	waitCond->wakeAll();
	mymutex->unlock();
}


void RunController::waitWAV()
{
	mymutex->lock();
	#ifdef USEQSOUND
		while(!wavsound->isFinished())
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1000);
		#endif
	#endif
	#ifdef USESDL
		while(Mix_Playing(SDL_CHAN_WAV))
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1000);
		#endif
	#endif
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::stopWAV()
{
	mymutex->lock();
	#ifdef USEQSOUND
		wavsound->stop();
	#endif
	#ifdef USESDL
		Mix_HaltChannel(SDL_CHAN_WAV);
	#endif
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::startDebug()
{
	if (i->isStopped())
	{
		int result = i->compileProgram((editwin->toPlainText() + "\n").toUtf8().data());
		if (result < 0)
		{
			i->debugMode = false;
			emit(runHalted());
			return;
		}
		i->initialize();
		i->debugMode = true;
		outputClear();
		mainwin->statusBar()->showMessage(tr("Running"));
		graphwin->setFocus();
		i->start();
		varwin->clear();
		mainWindowSetRunning(2);
		emit(debugStarted());
	}
}

void
RunController::startRun()
{
	if (i->isStopped())
	{
		int result = i->compileProgram((editwin->toPlainText() + "\n").toUtf8().data());
		i->debugMode = false;
		if (result < 0)
		{
			emit(runHalted());
			return;
		}
		i->initialize();
		outputClear();
		mainwin->statusBar()->showMessage(tr("Running"));
		graphwin->setFocus();
		i->start();
		varwin->clear();
		mainWindowSetRunning(1);
		emit(runStarted());
	}
}


void
RunController::inputEntered(QString text)
{
	graphwin->setFocus();
	mymutex->lock();
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::outputClear()
{
	mymutex->lock();
	outwin->clear();
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::outputReady(QString text)
{
	mymutex->lock();
	outwin->insertPlainText(text);
	outwin->ensureCursorVisible();
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::goutputReady()
{
	mymutex->lock();
	graphwin->repaint();
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::stepThrough()
{
	mydebugmutex->lock();
	waitDebugCond->wakeAll();
	mydebugmutex->unlock();
}

void
RunController::stopRun()
{
	mainwin->statusBar()->showMessage(tr("Ready."));

	mainWindowSetRunning(0);

	stopWAV();

	mymutex->lock();
	outwin->setReadOnly(true);
	waitCond->wakeAll();
	mymutex->unlock();

	mydebugmutex->lock();
	i->debugMode = false;
	waitDebugCond->wakeAll();
	mydebugmutex->unlock();

	emit(runHalted());
}


void
RunController::pauseResume()
{
	if (paused)
	{
		mainwin->statusBar()->showMessage(tr("Running"));
		paused = false;
		emit(runResumed());
	}
	else
	{
		mainwin->statusBar()->showMessage(tr("Paused"));
		paused = true;
		emit(runPaused());
	}
}



void
RunController::saveByteCode()
{
    byteCodeData *bc = i->getByteCode((editwin->toPlainText() + "\n").toUtf8().data());
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

void
RunController::showDocumentation()
{
	if (!docwin) docwin = new DocumentationWin(mainwin);
	docwin->show();
	docwin->raise();
	docwin->go("");
	docwin->activateWindow();
}


void RunController::showOnlineDocumentation()
{
	QDesktopServices::openUrl(QUrl("http://doc.basic256.org"));
}

void
RunController::showContextDocumentation()
{
	QString w = editwin->getCurrentWord();
	if (!replacewin) docwin = new DocumentationWin(mainwin);
	docwin->show();
	docwin->raise();
	docwin->go(w);
	docwin->activateWindow();
}

void RunController::showOnlineContextDocumentation()
{
	QString w = editwin->getCurrentWord();
	QDesktopServices::openUrl(QUrl("http://doc.basic256.org/doku.php?id=en:" + w));
}

void
RunController::showPreferences()
{
	bool good = true;
    SETTINGS;
    QString prefpass = settings.value(SETTINGSPREFPASSWORD,"").toString();
	if (prefpass.length()!=0) {
		char * digest;
		QString text = QInputDialog::getText(mainwin, tr("BASIC-256 Preferences and Settings"),
			tr("Password:"), QLineEdit::Password, QString:: null);
		digest = MD5(text.toUtf8().data()).hexdigest();
		good = (QString::compare(digest, prefpass)==0);
		free(digest);
	}
	if (good) {
		PreferencesWin *w = new PreferencesWin(mainwin);
		w->show();
		w->raise();
		w->activateWindow();
	} else {
		QMessageBox msgBox;
		msgBox.setText("Incorrect password.");
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
	}
}

void
RunController::showPrefPrinter()
{
	PrefPrinterWin *w = new PrefPrinterWin(mainwin);
	w->show();
	w->raise();
	w->activateWindow();
}

void RunController::showReplace()
{
     if (!replacewin) replacewin = new ReplaceWin();
     replacewin->setReplaceMode(true);
     replacewin->show();
     replacewin->raise();
     replacewin->activateWindow();
}

void RunController::showFind()
{
     if (!replacewin) replacewin = new ReplaceWin();
     replacewin->setReplaceMode(false);
     replacewin->show();
     replacewin->raise();
     replacewin->activateWindow();
}

void RunController::findAgain()
{
     if (!replacewin) replacewin = new ReplaceWin();
     replacewin->setReplaceMode(false);
     replacewin->show();
     replacewin->raise();
     replacewin->activateWindow();
     replacewin->findAgain();
}

void
RunController::mainWindowsVisible(int w, bool v)
{
	if (w==0) mainwin->editWinVisibleAct->setChecked(v);
	if (w==1) mainwin->graphWinVisibleAct->setChecked(v);
	if (w==2) mainwin->textWinVisibleAct->setChecked(v);
}

void
RunController::mainWindowsResize(int w, int width, int height)
{
	// only resize graphics window now - may add other windows later
	mymutex->lock();
	if (w==1) {
		graphwin->resize(width, height);
		graphwin->setMinimumSize(graphwin->image->width(), graphwin->image->height());
	}
	waitCond->wakeAll();
	mymutex->unlock();
}


void
RunController::dialogAlert(QString prompt)
{
	QMessageBox msgBox(mainwin);
	msgBox.setText(prompt);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	// actualy show alert (take exclusive control)
	mymutex->lock();
	msgBox.exec();
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::dialogConfirm(QString prompt, int dflt)
{
	QMessageBox msgBox(mainwin);
	msgBox.setText(prompt);
	msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
	if (dflt!=-1) {
		if (dflt!=0) {
			msgBox.setDefaultButton(QMessageBox::Yes);
		} else {
			msgBox.setDefaultButton(QMessageBox::No);
		}
	}
	// actualy show confirm (take exclusive control)
	mymutex->lock();
	if (msgBox.exec()==QMessageBox::Yes) {
		i->returnInt = 1;
	} else {
		i->returnInt = 0;
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void
RunController::dialogPrompt(QString prompt, QString dflt)
{
	QInputDialog in(mainwin);
	in.setLabelText(prompt);
	in.setTextValue(dflt);
	// actualy show prompt (take exclusive control)
	mymutex->lock();
	if (in.exec()==QDialog::Accepted) {
		i->returnString = in.textValue();
	} else {
		i->returnString = dflt;
	}
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::dialogFontSelect()
{
    bool ok;
    SETTINGS;
    QFont newf = QFontDialog::getFont(&ok, editwin->font(), mainwin);
    if (ok) {
        settings.setValue(SETTINGSFONT, newf.toString());

		mymutex->lock();
        editwin->setFont(newf);
        outwin->setFont(newf);
		waitCond->wakeAll();
		mymutex->unlock();
    }
}

void RunController::mainWindowSetRunning(int type)
{
    // set the menus, menu options, and tool bar items to
    // correct state based on the stop/run/debug status
    // type 0 - stop, 1-run, 2-debug
    
	editwin->setReadOnly(type!=0);
	QTextCursor textCursor = editwin->textCursor();
	textCursor.clearSelection();
	editwin->setTextCursor( textCursor );
    
    // file menu
    mainwin->newact->setEnabled(type==0);
    mainwin->openact->setEnabled(type==0);
    mainwin->saveact->setEnabled(type==0);
    mainwin->saveasact->setEnabled(type==0);
    mainwin->printact->setEnabled(type==0);
    for(int t=0; t<SETTINGSGROUPHISTN; t++) mainwin->recentact[t]->setEnabled(type==0);
    mainwin->exitact->setEnabled(true);
    
	// edit menu
    mainwin->undoact->setEnabled(type==0);
    mainwin->redoact->setEnabled(type==0);
    mainwin->cutact->setEnabled(false);
    mainwin->copyact->setEnabled(false);
    mainwin->pasteact->setEnabled(type==0);
    mainwin->selectallact->setEnabled(true);
    mainwin->findact->setEnabled(type==0);
    mainwin->findagain1->setEnabled(type==0);
    mainwin->findagain2->setEnabled(type==0);
    mainwin->replaceact->setEnabled(type==0);
    mainwin->beautifyact->setEnabled(type==0);
    mainwin->prefact->setEnabled(type==0);
    mainwin->prefprinteract->setEnabled(type==0);
    
	// run menu    
    mainwin->runact->setEnabled(type==0);
	mainwin->debugact->setEnabled(type==0);
	mainwin->stepact->setEnabled(type==2);
	mainwin->stopact->setEnabled(type!=0);
    
}

void RunController::mainWindowEnableCopy(bool stuffToCopy)
{
	// only enable the copy buttons when there is stuff to copy
    mainwin->cutact->setEnabled(stuffToCopy && mainwin->pasteact->isEnabled());
    mainwin->copyact->setEnabled(stuffToCopy);
}
