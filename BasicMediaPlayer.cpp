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

#include "BasicMediaPlayer.h"

void BasicMediaPlayer::loadBlocking(QString file) {
	// blocking load adapted from http://qt-project.org/wiki/seek_in_sound_file
	setMedia(QUrl::fromLocalFile(QFileInfo(file).absoluteFilePath()));
	if(!isSeekable()) {
		QEventLoop loop;
		QTimer timer;
		timer.setSingleShot(true);
		timer.setInterval(2000);
		loop.connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()) );
		loop.connect(this, SIGNAL(seekableChanged(bool)), &loop, SLOT(quit()));
		loop.exec();
	}
}
		
int BasicMediaPlayer::state() {
	// this eventually needs to be removed and the base class state() should work
	//
	// with QT5.1 it has been observed that
	// some media files do not report stop at the end of the file
	// and continue to report playing forever
	// if playing check of the position increases after a short sleep time
	
	// media state 0-stop, 1-play, 2-pause
	int s = QMediaPlayer::StoppedState;
	s = QMediaPlayer::state();
	if (s==QMediaPlayer::PlayingState) {
		struct timespec tim, tim2;
		qint64 starttime, endtime;
		tim.tv_sec = 0;
		tim.tv_nsec = 30000000L;	// 30 ms
		starttime = position();
		nanosleep(&tim, &tim2);
		endtime = position();
		if (starttime==endtime) s = QMediaPlayer::StoppedState; // stopped
	}
	return(s);
}
		
void BasicMediaPlayer::wait() {
	// wait for the media file to complete
	struct timespec tim, tim2;
	while(state()==QMediaPlayer::PlayingState) {
		tim.tv_sec = 0;
		tim.tv_nsec = 300000000L;	// 300 ms
		nanosleep(&tim, &tim2);
	}
}
		
