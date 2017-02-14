/** Copyright (C) 2016, Florin Oprea <florinoprea.contact@gmail.com>
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

#pragma once

#include <math.h>
#include <stdio.h>
#include <unordered_map>

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QBuffer>
#include <QEventLoop>
#include <QFileInfo>
#include <QMap>

#include "Error.h"
#include "Constants.h"
#include "Settings.h"
//#include "BasicDownloader.h"

#define SOUND_HALFWAVE 0x7fff

#define SOUNDTYPE_EMPTY         0
#define SOUNDTYPE_GENERATED     1
#define SOUNDTYPE_FILE          2
#define SOUNDTYPE_WEB           3
#define SOUNDTYPE_MEMORY        4

struct LoadedSound{
    QByteArray* byteArray;
    //validate loaded media file into memory if it is a playable file
    //to speed up things, we do this check only at first play
    bool needValidation;
    bool isValidated; //the result of validation

};

class Sound : public QObject
{
    Q_OBJECT
    public:
        Sound();
        ~Sound();

        int type; //0-no sound, 1-generated sound, 2-file, 3-web, 4-memory //(for debug)
        int id; //(for debug)
        QString source;
        int soundState; //
        int soundStateExpected; //
        double individualVolume;
        bool isStopping; //flag
        QMediaPlayer* media;
        QAudioOutput* audio;
        QBuffer *buffer;
        QByteArray *byteArray;
        int state();
        void play();
        void stop();
        void pause();
        void wait();
        double position();
        bool seek(double);
        double length();
        void updatedMasterVolume(double);
        int error();
        void prepareConnections();
        int sound_samplerate;
        bool waitLoadedMediaValidation();

        qint64 media_duration;
signals:
        void exitWaitingLoop();
        void deleteMe(int);

private slots:
    void handleAudioStateChanged(QAudio::State);
    void handleMediaStateChanged(QMediaPlayer::State);
    void handleMediaDurationChanged(qint64);
    void stopsSoundsAndWaiting();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus);
    void handleMediaError(QMediaPlayer::Error);
};






class SoundSystem : public QObject
{
Q_OBJECT
    public:
        SoundSystem();
        ~SoundSystem();

        int lastIdUsed;
        int lastBeepLoaded;
        std::unordered_map<int, Sound*> soundsmap;
        QMap <QString, LoadedSound> loadedsounds;
        QByteArray* generateSound(std::vector<std::vector<double>>);
        int playSound(std::vector<std::vector<double>> sounddata);
        int playSound(QString s);
        bool soundSystemIsStopping;

        void pause(int sound);
        double position(int sound);
        void seek(int sound, double pos);
        int state(int sound);
        void stop(int sound);
        void wait(int sound);
        double length(int sound);
        void volume(int sound, double vol);
        void play(int sound);


        void setMasterVolume(double);
        double getMasterVolume();
        void exit();
        //QString loadSound(QString);
        QString loadSoundFromVector(std::vector<std::vector<double>>);
        QString loadSoundFromArray(QString, QByteArray*);
        bool unloadSound(QString);
        //BasicDownloader *downloader;

        Error **error;

signals:
    void stopsSoundsAndWaiting();

private slots:
    void deleteMe(int);

private:
        QAudioFormat format;
        double masterVolume;
        int sound_samplerate;
        int sound_normalize_ms;
        int sound_fade_ms;
};



