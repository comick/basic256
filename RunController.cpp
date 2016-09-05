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
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QApplication>



#include "RunController.h"
#include "MainWindow.h"
#include "DocumentationWin.h"
#include "Settings.h"
#include "md5.h"
#include "Sleeper.h"

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

#ifdef ESPEAK
#include <speak_lib.h>
#endif

#ifdef ANDROID
#include "android/AndroidTTS.h"
AndroidTTS *androidtts;
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



RunController::RunController() {
    i = new Interpreter(mainwin->locale);

    replacewin = NULL;
    docwin = NULL;

#ifdef ANDROID
    androidtts = new AndroidTTS();
#endif

    //signals for the Interperter (i)
    QObject::connect(i, SIGNAL(goToLine(int)), editwin, SLOT(goToLine(int)));
    QObject::connect(i, SIGNAL(seekLine(int)), editwin, SLOT(seekLine(int)));

    QObject::connect(i, SIGNAL(debugNextStep()), this, SLOT(debugNextStep()));
    QObject::connect(i, SIGNAL(outputClear()), this, SLOT(outputClear()));
    QObject::connect(i, SIGNAL(dialogAlert(QString)), this, SLOT(dialogAlert(QString)));
    QObject::connect(i, SIGNAL(dialogConfirm(QString, int)), this, SLOT(dialogConfirm(QString, int)));
    QObject::connect(i, SIGNAL(dialogPrompt(QString, QString)), this, SLOT(dialogPrompt(QString, QString)));
    QObject::connect(i, SIGNAL(executeSystem(QString)), this, SLOT(executeSystem(QString)));
    QObject::connect(i, SIGNAL(goutputReady()), this, SLOT(goutputReady()));
    QObject::connect(i, SIGNAL(mainWindowsResize(int, int, int)), this, SLOT(mainWindowsResize(int, int, int)));
    QObject::connect(i, SIGNAL(mainWindowsVisible(int, bool)), this, SLOT(mainWindowsVisible(int, bool)));
    QObject::connect(i, SIGNAL(outputReady(QString)), this, SLOT(outputReady(QString)));
    QObject::connect(i, SIGNAL(outputError(QString)), this, SLOT(outputError(QString)));
    QObject::connect(i, SIGNAL(stopRun()), this, SLOT(stopRun()));
    QObject::connect(i, SIGNAL(stopRunFinalized()), this, SLOT(stopRunFinalized()));
    QObject::connect(i, SIGNAL(speakWords(QString)), this, SLOT(speakWords(QString)));
#ifdef ANDROID
	QObject::connect(i, SIGNAL(playWAV(QString)), this, SLOT(playWAV(QString)));
	QObject::connect(i, SIGNAL(stopWAV()), this, SLOT(stopWAV()));
	QObject::connect(i, SIGNAL(waitWAV()), this, SLOT(waitWAV()));
#endif


    QObject::connect(i, SIGNAL(getInput()), outwin, SLOT(getInput()));

	// for debugging and controlling the variable watch window
	QObject::connect(i, SIGNAL(varWinAssign(Variables*, int)), varwin,
		SLOT(varWinAssign(Variables*, int)));
	QObject::connect(i, SIGNAL(varWinAssign(Variables*, int, int, int)), varwin,
		SLOT(varWinAssign(Variables*, int, int, int)));
	QObject::connect(i, SIGNAL(varWinDropLevel(int)), varwin,
		SLOT(varWinDropLevel(int)));
	QObject::connect(i, SIGNAL(varWinDimArray(Variables*, int, int, int)), varwin,
		SLOT(varWinDimArray(Variables*, int, int, int)));

    QObject::connect(this, SIGNAL(runHalted()), i, SLOT(runHalted()));

    QObject::connect(outwin, SIGNAL(inputEntered(QString)), i, SLOT(inputEntered(QString)));
    QObject::connect(outwin, SIGNAL(inputEntered(QString)), this, SLOT(inputEntered(QString)));


}

RunController::~RunController() {
    if(replacewin!=NULL) replacewin->close();
    if(docwin!=NULL) docwin->close();
    stopRun();
    i->wait();
    delete i;
}


void
RunController::speakWords(QString text) {
#ifdef ESPEAK
    SETTINGS;
    espeak_ERROR err;

    mymutex->lock();

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
        if (voicename == SETTINGSESPEAKVOICEDEFAULT)
                voicename = QLocale::languageToString(QLocale::system().language());  // language of system locale
        err = espeak_SetVoiceByName(voicename.toUtf8());
        if (err != EE_OK) err = espeak_SetVoiceByName(SETTINGSESPEAKVOICEDEFAULT);  // fallback to english if locale language not found
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
    waitCond->wakeAll();
    mymutex->unlock();

#endif
#ifdef ESPEAK_EXECUTE
    // easy espeak implementation when all else fails
    // mutex handled by executeSystem
    text.replace("\""," quote ");
    text.prepend("espeak \"");
    text.append("\"");
    executeSystem(text);
#endif
#ifdef MACX_SAY
    // easy macosX implementation - call the command line say statement
    // mutex handled by executeSystem
    text.replace("\""," quote ");
    text.prepend("say \"");
    text.append("\"");
    //fprintf(stderr,"MACX_SAY %s\n", text.toStdString().c_str());
    executeSystem(text);
#endif
#ifdef ANDROID
    mymutex->lock();
    androidtts->say(text);
    waitCond->wakeAll();
    mymutex->unlock();
#endif

}

void
RunController::executeSystem(QString text) {
    // need to implement system as a function to return process output
    // and to handle input

    QProcess sy;
    mymutex->lock();

    sy.start(text);
    if (sy.waitForStarted()) {
        if (!sy.waitForFinished()) {
            //QByteArray result = sy.readAll();
        }
    }

    waitCond->wakeAll();
    mymutex->unlock();
}


void
RunController::startDebug() {
    if (i->isStopped()) {
        outputClear();
        int result = i->compileProgram((editwin->toPlainText() + "\n").toUtf8().data());
        if (result < 0) {
            i->debugMode = 0;
            stopRunFinalized();
            return;
        }
        i->initialize();
        i->debugMode = 1;
        editwin->updateBreakPointsList();
        i->debugBreakPoints = editwin->breakPoints;
        mainwin->statusBar()->showMessage(tr("Running"));
        graphwin->setFocus();
        i->start();
        varwin->clear();
        if (replacewin) replacewin->close();
        mainwin->setRunState(RUNSTATEDEBUG);
    }
}

void
RunController::debugNextStep() {
	// show step buttons for next debug step
    mainwin->setRunState(RUNSTATEDEBUG);
}


void
RunController::startRun() {
    if (i->isStopped()) {
        //
        outputClear();
        // Start Compile
        int result = i->compileProgram((editwin->toPlainText() + "\n").toUtf8().data());
        i->debugMode = 0;
        if (result < 0) {
            stopRunFinalized();
            return;
        }
        // if successful compile see if we need to save it
        SETTINGS;
        if(settings.value(SETTINGSIDESAVEONRUN, SETTINGSIDESAVEONRUNDEFAULT).toBool()) {
            editwin->saveFile(true);
            mainwin->statusBar()->showMessage(tr("Saved"));
        }
        //
        // now setup and start the run
        i->initialize();
        mainwin->statusBar()->showMessage(tr("Running"));
        graphwin->setFocus();
        i->start();
        varwin->clear();
        if (replacewin) replacewin->close();
        mainwin->setRunState(RUNSTATERUN);
     }
}


void
RunController::inputEntered(QString text) {
    (void) text;
    graphwin->setFocus();
    mymutex->lock();
    waitCond->wakeAll();
    mymutex->unlock();
}

void
RunController::outputClear() {
    mymutex->lock();
	outwin->setTextColor(Qt::black);
    outwin->clear();
    waitCond->wakeAll();
    mymutex->unlock();
}

void RunController::outputReady(QString text) {
	mymutex->lock();
	mainWindowsVisible(2,true);
	QTextCursor t(outwin->textCursor());
	if(!t.atEnd() || t.hasSelection()){
		t.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
		outwin->setTextCursor(t);
	}
	outwin->insertPlainText(text);
	outwin->ensureCursorVisible();
	waitCond->wakeAll();
	mymutex->unlock();
}

void RunController::outputError(QString text) {
	outwin->setTextColor(Qt::red); //color red
	outputReady(text);
	outwin->setTextColor(Qt::black); //back to black color
}

void
RunController::goutputReady() {
    mymutex->lock();
    graphwin->repaint();
    waitCond->wakeAll();
    mymutex->unlock();
}

void
RunController::stepThrough() {
	i->debugMode = 1; // step through debugging
	mainwin->setRunState(RUNSTATERUNDEBUG);
    mydebugmutex->lock();
    waitDebugCond->wakeAll();
    mydebugmutex->unlock();
}
void

RunController::stepBreakPoint() {
	i->debugMode = 2; // run to break point debugging
	mainwin->setRunState(RUNSTATERUNDEBUG);
    mydebugmutex->lock();
    waitDebugCond->wakeAll();
    mydebugmutex->unlock();
}

void RunController::stopRun() {
	// event when the user clicks on the stop button
	mainwin->statusBar()->showMessage(tr("Stopping."));

	mainwin->setRunState(RUNSTATESTOPING);

	mymutex->lock();
	outwin->stopInput();
	waitCond->wakeAll();
	mymutex->unlock();

	mydebugmutex->lock();
	i->debugMode = 0;
	waitDebugCond->wakeAll();
	mydebugmutex->unlock();

	emit(runHalted());
}

void RunController::stopRunFinalized() {
	// event when the interperter actually finishes the run
	mainwin->statusBar()->showMessage(tr("Ready."));

	mainwin->setRunState(RUNSTATESTOP);

	mainwin->ifGuiStateClose();
}

void
RunController::showDocumentation() {
	if (!docwin) docwin = new DocumentationWin(mainwin);
	docwin->show();
	if(docwin->windowState()&Qt::WindowMinimized) docwin->setWindowState(docwin->windowState()^Qt::WindowMinimized);    docwin->raise();
	docwin->go("");
	docwin->activateWindow();
}


void RunController::showOnlineDocumentation() {
    QDesktopServices::openUrl(QUrl("http://doc.basic256.org"));
}

void
RunController::showContextDocumentation() {
	QString w = editwin->getCurrentWord();
	if (!docwin) docwin = new DocumentationWin(mainwin);
	docwin->show();
	if(docwin->windowState()&Qt::WindowMinimized) docwin->setWindowState(docwin->windowState()^Qt::WindowMinimized);    docwin->raise();
	docwin->raise();
	docwin->go(w);
	docwin->activateWindow();
}

void RunController::showOnlineContextDocumentation() {
    QString w = editwin->getCurrentWord();
    QDesktopServices::openUrl(QUrl("http://doc.basic256.org/doku.php?id=en:" + w));
}

void
RunController::showPreferences() {
    bool advanced = true;
    SETTINGS;
    QString prefpass = settings.value(SETTINGSPREFPASSWORD,"").toString();
    if (prefpass.length()!=0) {
        char * digest;
        QString text = QInputDialog::getText(mainwin, tr("BASIC-256 Advanced Preferences and Settings"),
                                             tr("Password:"), QLineEdit::Password, QString:: null);
        digest = MD5(text.toUtf8().data()).hexdigest();
        advanced = (QString::compare(digest, prefpass)==0);
        free(digest);
    }
    PreferencesWin *w = new PreferencesWin(mainwin, advanced);
    w->show();
    w->raise();
    w->activateWindow();
}


void RunController::showReplace() {
    if (!replacewin) replacewin = new ReplaceWin();
    replacewin->setReplaceMode(true);
    QTextCursor cursor = editwin->textCursor();
    if(cursor.hasSelection()){
        replacewin->findText->setText(cursor.selectedText());
    }
    replacewin->findText->selectAll();
    replacewin->show();
    replacewin->raise();
    replacewin->activateWindow();
}

void RunController::showFind() {
    if (!replacewin) replacewin = new ReplaceWin();
    replacewin->setReplaceMode(false);
    QTextCursor cursor = editwin->textCursor();
    if(cursor.hasSelection()){
        replacewin->findText->setText(cursor.selectedText());
    }
    replacewin->findText->selectAll();
    replacewin->show();
    replacewin->raise();
    replacewin->activateWindow();
}

void RunController::findAgain() {
    if (!replacewin) replacewin = new ReplaceWin();
    replacewin->setReplaceMode(false);
    replacewin->show();
    replacewin->raise();
    replacewin->activateWindow();
    replacewin->findAgain();
}

void
RunController::mainWindowsVisible(int w, bool v) {
    if (w==0) mainwin->editwin_visible_act->setChecked(v);
    if (w==1) mainwin->graphwin_visible_act->setChecked(v);
    if (w==2) mainwin->outwin_visible_act->setChecked(v);
}

void
RunController::mainWindowsResize(int w, int width, int height) {
    // only resize graphics window now - may add other windows later
    mymutex->lock();
    if (w==0) editwin->resize(width, height);
    if (w==1) graphwin->resize(width, height);
    if (w==2) outwin->resize(width, height);
    waitCond->wakeAll();
    mymutex->unlock();
}


void
RunController::dialogAlert(QString prompt) {
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
RunController::dialogConfirm(QString prompt, int dflt) {
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
RunController::dialogPrompt(QString prompt, QString dflt) {
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

void RunController::dialogFontSelect() {
	bool ok;
	SETTINGS;
	QFont newf = QFontDialog::getFont(&ok, editwin->font(), mainwin, QString(), QFontDialog::MonospacedFonts);

	if (ok) {
		mymutex->lock();
		editwin->setFont(newf);
		outwin->setFont(newf);
		waitCond->wakeAll();
		mymutex->unlock();
	}
}
