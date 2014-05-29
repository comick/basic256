/** Copyright (C) 2014, James Reneau.
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

#ifndef USEQSOUND

#include "BasicMediaPlayer.h"

void BasicMediaPlayer::loadFile(QString file) {
    // blocking load adapted from http://qt-project.org/wiki/seek_in_sound_file
    setMedia(QUrl::fromLocalFile(QFileInfo(file).absoluteFilePath()));
    waitForSeekable(2000);
}

void BasicMediaPlayer::waitForSeekable(int ms) {
	// wait for isSeekable up to the time in ms
    if(!isSeekable()) {
		QEventLoop loop;
		QTimer timer;
		timer.setSingleShot(true);
		timer.setInterval(ms);
		loop.connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()) );
        loop.connect(this, SIGNAL(seekableChanged(bool)), &loop, SLOT(quit()));
		loop.exec();
	}
}

void BasicMediaPlayer::waitForState(QMediaPlayer::State newstate, int ms) {
	// if player state is not newstate then wait up to ms
	if(state()!=newstate) {
		// thanks http://qt-project.org/forums/viewthread/19869
		QTimer *timer = new QTimer(this);
		timer->start (ms);
		connect (timer, SIGNAL (timeout()), this, SIGNAL (QMediaPlayer::stateChanged()));
	}
}

int BasicMediaPlayer::state() {
    // media state 0-stop, 1-play, 2-pause
    // this eventually needs to be removed and the base class state() should work
	//
	// with QT5.1 it has been observed that
	// some media files do not report stop at the end of the file
	// and continue to report playing forever
	// if playing check of the position increases after a short sleep time
	
	int s;
	qint64 starttime, endtime;
	Sleeper *sleeper = new Sleeper();
    s = QMediaPlayer::state();
	if (s==QMediaPlayer::PlayingState) {
        starttime = QMediaPlayer::position();
		sleeper->sleepMS(30);
        endtime = QMediaPlayer::position();
		if (starttime==endtime) {
			stop();
			s = QMediaPlayer::StoppedState; // stopped
		}
	}
	return(s);
}

void BasicMediaPlayer::stop() {
	// force stop to reset position at the begining of the file
	// and to totally stop
	setPosition(0);
	QMediaPlayer::stop();
//	waitForState(QMediaPlayer::StoppedState, 1000);
}

void BasicMediaPlayer::wait() {
	// wait for the media file to complete
	Sleeper *sleeper = new Sleeper();
    do {
		sleeper->sleepMS(100);
	} while (state()==QMediaPlayer::PlayingState);
	setPosition(0);
	QMediaPlayer::pause();
}

bool BasicMediaPlayer::seek(double time) {
	long int ms = time * 1000L;
	waitForSeekable(500);
	//qDebug ("mediaStatus %d\n",mediaStatus()); 
	if(isSeekable()) {
		QMediaPlayer::setPosition(ms);
		return true;
	} else {
		return false;
	}
}

double BasicMediaPlayer::length() {
	return QMediaPlayer::duration() / 1000.0d;
}

double BasicMediaPlayer::position() {
	return QMediaPlayer::position() / 1000.0d;
}

void BasicMediaPlayer::play() {
	QMediaPlayer::play();
// not needed for w7 qt 5.2
// need to check xp vistz ubuntu
//waitForState(QMediaPlayer::PlayingState, 500);
}

void BasicMediaPlayer::pause() {
	QMediaPlayer::pause();
// not needed for w7 qt 5.2
// need to check xp vistz ubuntu
//	waitForState(QMediaPlayer::PausedState, 500);
}

int BasicMediaPlayer::error() {
	return QMediaPlayer::error();
}


#endif

