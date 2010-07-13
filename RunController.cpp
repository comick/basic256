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
#include <QMutex>
#include <QWaitCondition>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QApplication>
using namespace std;

#include "RunController.h"

#ifdef WIN32
	#include <windows.h> 
	#include <servprov.h>
	#include <sapi.h>
	#include <string>
	#include <cstdlib>
	#include <mmsystem.h>
#else
	#ifdef LINUX_ESPEAK
		#include <speak_lib.h>
	#endif
	#ifdef LINUX_FLITE
		#include <flite.h>
		extern "C"
		{
			cst_voice* register_cmu_us_kal();
		}
	#endif
	#include <stdio.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <time.h>
	#include <linux/soundcard.h>
#endif

#ifdef USEQSOUND
	#include <QSound>
	QSound wavsound(QString(""));
#endif
#ifdef USESDL
	#include <SDL/SDL.h>
	#include <SDL/SDL_mixer.h>
	#define SDL_CHAN_WAV 1
	#define SDL_CHAN_SOUND 2
#endif

QMutex mutex;
QMutex debugmutex;
QWaitCondition waitCond;
QWaitCondition waitDebugCond;
QWaitCondition waitInput;

RunController::RunController(MainWindow *mw)
{
	mainwin = mw;
	//i = new Interpreter(mainwin->goutput->image, mainwin->goutput->imask);
	i = new Interpreter(mainwin->goutput);
	te = mainwin->editor;
	output = mainwin->output;
	goutput = mainwin->goutput;
	statusbar = mainwin->statusBar();

	soundVolume = 5;	// set default sound volume to 1/2

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

	QObject::connect(i, SIGNAL(setVolume(int)), this, SLOT(setVolume(int)));
	QObject::connect(i, SIGNAL(system(char*)), this, SLOT(system(char*)));
	QObject::connect(i, SIGNAL(playSounds(int, int*)), this, SLOT(playSounds(int, int*)));
	QObject::connect(i, SIGNAL(speakWords(QString)), this, SLOT(speakWords(QString)));
	QObject::connect(i, SIGNAL(playWAV(QString)), this, SLOT(playWAV(QString)));
	QObject::connect(i, SIGNAL(waitWAV()), this, SLOT(waitWAV()));
	QObject::connect(i, SIGNAL(stopWAV()), this, SLOT(stopWAV()));

	QObject::connect(i, SIGNAL(highlightLine(int)), te, SLOT(highlightLine(int)));
	QObject::connect(i, SIGNAL(varAssignment(QString, QString, int)), mainwin->vardock, SLOT(addVar(QString, QString, int)));

#ifdef USESDL
	//mono
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 1, 2048);
#endif
}

// play a list of counds 0,2,4... = frequency & 1,3,5... = duration in ms
// notes is arraylength/2 (number of notes)
void
RunController::playSounds(int notes, int* freqdur)
{

#ifdef WIN32
	
	// code uses Microsoft's waveOut process to create a wave on the default sound card
	// if soundcard is unopenable then use the system speaker
	unsigned long errorcode;
	HWAVEOUT      outHandle;
	WAVEFORMATEX  waveFormat;
	
	double amplititude = tan((double) soundVolume / (double) 10) * (double) 0x7f;  // (half wave height)
	
	// Initialize the WAVEFORMATEX for 8-bit, 11.025KHz, mono
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 1;
	waveFormat.nSamplesPerSec = 11025;
	waveFormat.wBitsPerSample = 8;
	waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// Open the preferred Digital Audio Out device
	errorcode = waveOutOpen(&outHandle, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
	if (!errorcode) {
	
		double wave = 0;
		int totaltime = 0;
		for(int tnotes=0;tnotes<notes;tnotes++) {
			totaltime += freqdur[tnotes*2+1];
		}
		int totallength = waveFormat.nSamplesPerSec * totaltime / 1000;
		unsigned char * p = (unsigned char *) malloc(totallength * sizeof(unsigned char));
		
		int pos = 0; // current position
		
		for(int tnotes=0;tnotes<notes;tnotes++) {
			// lets build a sine wave
			int length = waveFormat.nSamplesPerSec * freqdur[tnotes*2+1] / 1000;
			double wavebit = 2 * M_PI / ((double) waveFormat.nSamplesPerSec / (double) freqdur[tnotes*2]);
		
			for(int i = 0; i < length; i++) {
				p[pos++] =  (unsigned char) (amplititude * (sin(wave)+1));
				wave+=wavebit;
			}
		}

		// create block header with sound data, length and repeat instructions
		WAVEHDR header;
		ZeroMemory(&header, sizeof(WAVEHDR));
		header.dwBufferLength = totallength;
		header.lpData = (CHAR*) p;

		// get block ready for playback
		errorcode = waveOutPrepareHeader(outHandle, &header, sizeof(WAVEHDR));
		if (!errorcode) {
			// write block now that it is ready
			errorcode = waveOutWrite(outHandle, &header, sizeof(WAVEHDR));
		}
		//
		while(waveOutClose(outHandle)==WAVERR_STILLPLAYING) {	
			Sleep(10);
		}
	} 
	
#endif

#ifdef LINUX_DSPSOUND
	// Code loosely based on idea from TONEGEN by Timothy Pozar
	// - Plays a sine wave via the dsp or standard out.
 
	int stereo = 0;		// mono (stereo - false)
	int rate = 11025;
	int devfh;
	int i, test;
	double amplititude = tan((double) soundVolume / 10) * 0x7f;  // (half wave height)
	
	// lets build the output
	double wave = 0;
	int totaltime = 0;
	for(int tnotes=0;tnotes<notes;tnotes++) {
		totaltime += freqdur[tnotes*2+1];
	}
	int totallength = rate * totaltime / 1000;
	unsigned char * p = (unsigned char *) malloc(totallength * sizeof(unsigned char));
	
	int pos = 0; // current position
	
	for(int tnotes=0;tnotes<notes;tnotes++) {
		// lets build a sine wave
		int length = rate * freqdur[tnotes*2+1] / 1000;
		double wavebit = 2 * M_PI / ((double) rate / (double) freqdur[tnotes*2]);
	
		for(int i = 0; i < length; i++) {
			p[pos++] = (unsigned char) ((sin(wave) + 1) * amplititude);
			wave+=wavebit;
		}
	}

	if ((devfh = open("/dev/dsp", O_WRONLY|O_SYNC)) != -1) {
		// Set mono
		test = stereo;
		if(ioctl(devfh, SNDCTL_DSP_STEREO, &stereo) != -1) {
			// set the sample rate
			test = rate;
			if(ioctl( devfh, SNDCTL_DSP_SPEED, &test) != -1) {
   				int outwords = write(devfh, p, totallength);
			}
		}
		close(devfh);
	} else {
		fprintf(stderr,"Unable to open /dev/dsp\n");
	}
#endif

#ifdef USESDL
	Mix_Chunk c;

	c.allocated = 1; // sdl needs to free chunk data
	c.volume = (unsigned char) (tan((double) soundVolume / 10) * 0x7f);
	
	// lets build the output
	double wave = 0;
	int totaltime = 0;
	for(int tnotes=0;tnotes<notes;tnotes++) {
		totaltime += freqdur[tnotes*2+1];
	}
	c.alen = MIX_DEFAULT_FREQUENCY * sizeof(short) * totaltime / 1000;
	c.abuf = (unsigned char *) malloc(c.alen);
	
	int pos = 0; // current position
	
	for(int tnotes=0;tnotes<notes;tnotes++) {
		// lets build a sine wave
		int length = MIX_DEFAULT_FREQUENCY * freqdur[tnotes*2+1] / 1000;
		double wavebit = 2 * M_PI / ((double) MIX_DEFAULT_FREQUENCY / (double) freqdur[tnotes*2]);
		short s;
		char *cs = (char *) &s;
		for(int i = 0; i < length; i++) {
			
			s = (short) (sin(wave) * 0x7fff);
			c.abuf[pos++] = cs[0];
			c.abuf[pos++] = cs[1];
			wave+=wavebit;
		}
	}

    	Mix_PlayChannel(SDL_CHAN_SOUND,&c,0);
	while(Mix_Playing(SDL_CHAN_SOUND)) usleep(1000);

#endif
}


void
RunController::speakWords(QString text)
{
#ifdef WIN32

	// use microsoft sapi with the default voice
	ISpVoice * pVoice = NULL;

    if (FAILED(::CoInitialize(NULL))) return;

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if( SUCCEEDED( hr ) )
    {
		// convert qchar* wo wchar*
		WCHAR *wutext = new WCHAR[text.length() + 1];
		for (int i = 0; i < text.length(); ++i) wutext[i] = text[i].unicode();
		wutext[text.length()] = 0;
		
        hr = pVoice->Speak(wutext, 0, NULL);
        pVoice->Release();
        pVoice = NULL;
		
		delete wutext;	// send to gc - just in case
    }

    ::CoUninitialize();
#endif
#ifdef LINUX_ESPEAK
	//QMessageBox::information( 0, "BASIC-256", QString("ESpeak"));
    
	// espeak tts library
	char *data_path = NULL;   // use default path for espeak-data
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;

	int samplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK,0,data_path,0);
       	if (samplerate!=-1) {
		espeak_SetVoiceByName("default");
		int size=text.length()+1;	// buffer length
		espeak_ERROR err = espeak_Synth(text.toLatin1(),size,0,POS_CHARACTER,0,synth_flags,NULL,NULL);
		if (err!=EE_OK) {
			fprintf(stderr,"espeak synth error %i\n", err);
		}
		espeak_Synchronize();		// wait to finish
		espeak_Terminate();		// clean up

	} else {
		fprintf(stderr,"Unable to initialize espeak\n");
	}
#endif
#ifdef LINUX_FLITE
	// CMU flite (compiled festival voices) from http://www.speech.cs.cmu.edu/flite/
	cst_voice *v;
	flite_init();
	v = register_cmu_us_kal();
	int length = flite_text_to_speech(text.toLatin1(),v,"play");
	// wait for finish
	//clock_t endwait = clock() + (length + 1) * CLOCKS_PER_SEC;
  	//while (clock() < endwait) {}

#endif
}

void
RunController::setVolume(int volume)
{
  // volume MUST be betwen 0(mute) and 10(all the way)
  soundVolume = volume;
}

void
RunController::system(char* text)
{
	system(text);
}

void RunController::playWAV(QString file)
{
#ifdef USEQSOUND
	if(QSound::isAvailable()) {
		wavsound.play(file);
	}
#endif
#ifdef USESDL
	Mix_HaltChannel(SDL_CHAN_WAV);
	Mix_Chunk *music;
    	music = Mix_LoadWAV((char *) file.toUtf8().data());
    	Mix_PlayChannel(SDL_CHAN_WAV,music,0);
#endif
}


void RunController::waitWAV()
{
#ifdef USEQSOUND
	if(QSound::isAvailable()) {
		while(!wavsound.isFinished())
#ifdef WIN32
	Sleep(1);
#else
	usleep(1000);
#endif
	}
#endif
#ifdef USESDL
	while(Mix_Playing(SDL_CHAN_WAV))
#ifdef WIN32
	Sleep(1);
#else
	usleep(1000);
#endif
#endif
}

void RunController::stopWAV()
{
#ifdef USEQSOUND
	if(QSound::isAvailable()) {
		wavsound.stop();
	}
#endif
#ifdef USESDL
	Mix_HaltChannel(SDL_CHAN_WAV);
#endif
}

void
RunController::startDebug()
{
	if (i->isStopped())
	{
		int result = i->compileProgram((te->toPlainText() + "\n").toUtf8().data());
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
		int result = i->compileProgram((te->toPlainText() + "\n").toUtf8().data());
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

void
RunController::showDocumentation()
{
     DocumentationWin *docwin = new DocumentationWin(mainwin);
     docwin->show();
     docwin->raise();
     docwin->activateWindow();
}

