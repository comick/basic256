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

#include "Sound.h"
#include <qDebug>


Sound::Sound() :
    QObject()
{
    type = SOUNDTYPE_EMPTY;
    soundState = 0;
    soundStateExpected = 0;
    //We need soundStateExpected because media sounds that are resumed are a bit lazy to
    //update state to QMediaPlayer::PlayingState
    //And if we call wait() just after pause()/play() we can have a chance to miss the waiting
    //Sequence: play(), pause(), play()/resume, wait()
    //A solution was to wait after each play() or pause() by loosing speed
    byteArray = NULL;
    buffer = NULL;
    audio = NULL;
    media = NULL;
    source = "";
    isStopping = false;
    individualVolume = 1.0;
    media_duration = 0; //used only by mediaplayer because duration may not be available when initial playback begins
}

Sound::~Sound() {
    //qDebug() << id << "- Sound::~Sound()";
    if(soundState!=0){
        if (audio) audio->stop();
        if (media) media->stop();
    }
    if(buffer){
        buffer->close();
        delete(buffer);
        buffer=NULL;
    }
    delete(byteArray);
    byteArray=NULL;
    if(audio){
        delete(audio);
        audio=NULL;
    }
    if(media){
        delete(media);
        media=NULL;
    }
}

int Sound::state() {
    return soundState;
}

void Sound::play() {
    soundStateExpected = 1;
    if(audio){
        if(audio->state()==QAudio::SuspendedState){
            audio->resume();
        }else{
            audio->start(buffer);
        }
    }else if(media){
        //qDebug() << id << "- play()";
        media->play();
        //qDebug() << id << "- end play()";
    }
}

void Sound::stop() {
    isStopping = true;
    soundStateExpected = 0;
    if(audio){
        audio->stop();
    }else if(media){
        media->stop();
    }
}

void Sound::pause() {
    soundStateExpected = 2;
    if(audio){
        audio->suspend();
    }else if(media){
        //qDebug() << id << "- pause()";
        media->pause();
    }
}

void Sound::wait() {
    //Wait till the end of sound or till the end of program
    QEventLoop *loop = new QEventLoop();
    QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
    if(audio){
        QObject::connect(audio, SIGNAL(stateChanged(QAudio::State)), loop, SLOT(quit()));
        while(!isStopping && audio && audio->state() == QAudio::ActiveState){
            loop->exec(QEventLoop::WaitForMoreEvents);
        }
    }else if(media){
        //qDebug() << id << "- wait()";
        QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
        while(!isStopping && media && (media->state() == QMediaPlayer::PlayingState || (media->state() == QMediaPlayer::PausedState && soundStateExpected==1))){
            loop->exec(QEventLoop::WaitForMoreEvents);
        }
    }
    delete (loop);
}

bool Sound::waitLoadedMediaValidation() {
    //this function waits a bit afer play command to check if buffer contain a valid media file
    //because QMediaPlayer::play remains in Play state when a fake sound is loaded into memory and played
    //The only trick I found is that when first mediaStatusChanged is emitted, the duration is 0 for fake media
    //http://stackoverflow.com/questions/42168280/qmediaplayer-play-a-sound-loaded-into-memory
    //PS This validation shuld be fired only once, at first play
    QTimer *timer = new QTimer();
    timer->setSingleShot(true);
    QEventLoop *loop = new QEventLoop();
    QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
    QObject::connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
    timer->start(2000);
    //QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
    QObject::connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), loop, SLOT(quit()));
    QObject::connect(media, SIGNAL(durationChanged(qint64)), loop, SLOT(quit()));
    if(!isStopping){
        //qDebug() << "loop->exec";
        loop->exec(QEventLoop::ExcludeUserInputEvents);
    }
    //qDebug() << "exit loop->exec";
    delete (loop);
    delete (timer);
    return (media_duration!=0);
}

double Sound::position() {
    //Return the position of the played sound
    //Attention: elapsedUSecs returns the microseconds since start() was called, including time
    //in Idle and Suspend states. So, in this case, we gonna use buffer->pos() instead.
    if(audio){
        return ((double)buffer->pos() / ((double)sound_samplerate * (double) sizeof(int16_t)));
    }else if(media){
        return media->position() / (double)(1000.0);
    }
    return 0.0;
}

bool Sound::seek(double sec) {
    //For QAudio the seeking must be done by using the buffer. Attention: the new position must be divisible
    //with sizeof(int16_t) - the sample size of generated sound is 16 bit
    //For QMediaPlayer we need to wait a bit if the sound is not seekable for the moment, but not loger than 2 seconds
    if(audio){
        return (buffer->seek( (qint64)((double)sound_samplerate * sec)  * (qint64) sizeof(int16_t)));
    }else if(media){
        if(media->isSeekable()) {
            media->setPosition(sec * 1000L);
            return true;
        } else {
            //qDebug() << id << "- not isSeekable";
            QTimer *timer = new QTimer();
            timer->setSingleShot(true);
            QEventLoop *loop = new QEventLoop();
            bool timeisup = false;
            connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
            timer->start(2000);
            QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
            QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
            QObject::connect(media, SIGNAL(seekableChanged(bool)), loop, SLOT(quit()));
            while(!isStopping && media && !media->isSeekable() && media->state() != QMediaPlayer::StoppedState && timer->isActive()){
                loop->exec(QEventLoop::WaitForMoreEvents);
            }
            if(!timer->isActive()) timeisup=true;
            delete (loop);
            delete (timer);
            if(media->isSeekable()) {
                media->setPosition(sec * 1000L);
                return true;
            } else {
                return !timeisup;
            }
        }
    }
    return true;
}

double Sound::length() {
    //For QMediaPlayer the length of played sound may not be available when initial playback begins (asyncron mode).
    //If we need the length for a media we may wait a bit right after sound starts to play, but not longer than 2 seconds.
    if(audio){
        return ( buffer->size() / (double) sizeof(int16_t) / (double)sound_samplerate );
    }else if(media){
        //return (double)media->duration() / (double)(1000.0);
        bool timeisup = false;
        if(media_duration==0.0 && soundState!=0){
            QTimer *timer = new QTimer();
            timer->setSingleShot(true);
            QEventLoop *loop = new QEventLoop();
            connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));
            timer->start(2000);
            QObject::connect(this, SIGNAL(exitWaitingLoop()), loop, SLOT(quit()));
            QObject::connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), loop, SLOT(quit()));
            QObject::connect(media, SIGNAL(durationChanged(qint64)), loop, SLOT(quit()));
            //qDebug() << id << "- soundState" << soundState;
            while(!isStopping && media && media->state() != QMediaPlayer::StoppedState && timer->isActive() && media_duration==0.0){
                loop->exec(QEventLoop::WaitForMoreEvents);
            }
            if(!timer->isActive()) timeisup=true;
            delete (loop);
            delete (timer);
        }
        if(timeisup) return (-1.0);
        return (media_duration/1000.0);
    }
    return 0;
}


//tell that master volume has been changed
void Sound::updatedMasterVolume(double v) {
    if(audio){
        audio->setVolume((v/10.0)*individualVolume);
    }else if(media){
        media->setVolume((v*10)*individualVolume);
    }
}

int Sound::error() {
    if(audio){
        return audio->error();
    }else if(media){
        return media->error();
    }
    return 0;
}

void Sound::prepareConnections() {
    if(audio){
        connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleAudioStateChanged(QAudio::State)));
    }else if(media){
        connect(media, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(onMediaStatusChanged(QMediaPlayer::MediaStatus)));
        connect(media, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleMediaStateChanged(QMediaPlayer::State)));
        connect(media, SIGNAL(durationChanged(qint64)), this, SLOT(handleMediaDurationChanged(qint64)));
        connect(media, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleMediaError(QMediaPlayer::Error)));
    }
}

void Sound::onMediaStatusChanged(QMediaPlayer::MediaStatus s){
    //qDebug() << id << "- Media Status Changed:" << s << " " << media->error() << " " << media->state() << media->duration();
}

void Sound::handleMediaDurationChanged(qint64 d){
    media_duration=d;
    //qDebug() << id << "- Media Duration Changed:" << media->error() << " " << media->state() << media_duration;
}

void Sound::handleMediaError(QMediaPlayer::Error err) {
    //qDebug() << id << "- ERROR:" << err;
}

void Sound::handleMediaStateChanged(QMediaPlayer::State newState){
    //qDebug() << id << newState;
    soundState = newState;
    if(newState==QMediaPlayer::StoppedState){
        isStopping=true;
        source=QString("");
        media->disconnect();
        media->setMedia(QMediaContent());
        type = SOUNDTYPE_EMPTY;
        emit(exitWaitingLoop());
        emit(deleteMe(id));
        this->disconnect();
        /*
        if(buffer){
            buffer->close();
            delete(buffer);
            buffer=NULL;
        }
        delete(byteArray);
        byteArray=NULL;
        media->deleteLater();
        media = NULL;
        */

    }
}

void Sound::handleAudioStateChanged(QAudio::State newState){
    switch (newState) {
        case QAudio::ActiveState:
            soundState = 1;
            break;
        case QAudio::SuspendedState:
            soundState = 2;
            break;
        case QAudio::StoppedState:
            soundState = 0;
            isStopping=true;
            source=QString("");
            audio->disconnect();
            type = SOUNDTYPE_EMPTY;
            emit(exitWaitingLoop());
            emit(deleteMe(id));
            this->disconnect();
/* deleteOnStop? if wait, delete manual after wait
            buffer->close();
            delete(buffer);
            buffer=NULL;
            delete(byteArray);
            byteArray=NULL;
            audio->deleteLater();
            audio = NULL;
*/
            break;
        case QAudio::IdleState:
            isStopping=true;
            audio->stop();
            break;
    }
}


void Sound::stopsSoundsAndWaiting(){
    //qDebug() << id << "- emit(exitWaitingLoop());";
    stop();
    emit(exitWaitingLoop());
}



SoundSystem::SoundSystem() :
    QObject()
{
    error=NULL;
    soundSystemIsStopping = false;
    lastIdUsed=0;
    lastBeepLoaded=0;
    //downloader = new BasicDownloader();
    //downloader = NULL;

    SETTINGS;
    setMasterVolume(settings.value(SETTINGSSOUNDVOLUME, SETTINGSSOUNDVOLUMEDEFAULT).toInt());
    sound_samplerate = settings.value(SETTINGSSOUNDSAMPLERATE, SETTINGSSOUNDSAMPLERATEDEFAULT).toInt();
    sound_normalize_ms = settings.value(SETTINGSSOUNDNORMALIZE, SETTINGSSOUNDNORMALIZEDEFAULT).toInt();
    sound_fade_ms = settings.value(SETTINGSSOUNDVOLUMERESTORE, SETTINGSSOUNDVOLUMERESTOREDEFAULT).toInt();

    // setup the audio format
    format.setSampleRate(sound_samplerate);
    format.setChannelCount(1);
    format.setSampleSize(16);	// 16 bit audio
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        fprintf(stderr,"Raw audio format not supported by backend, try default sample rate.\n");
    }else{
        QAudioFormat nearest = info.nearestFormat(format);
        sound_samplerate = nearest.sampleRate();
        format.setSampleRate(sound_samplerate);
        if (!info.isFormatSupported(format)) {
            fprintf(stderr,"Raw audio format not supported by backend, cannot play sound.\n");
        }
    }
}

SoundSystem::~SoundSystem() {
    //qDebug() << "~SoundSystem()";
    exit();
}


int SoundSystem::playSound(QString s){
    //qDebug() << "SoundSystem::playSound(QString s)";
    if(soundSystemIsStopping) return 0;
    QUrl url(s);
    if(s.startsWith("sound:")){
        //qDebug() << "sound:";
        if(loadedsounds.count(s)){
            lastIdUsed++;
            if(loadedsounds[s].needValidation || loadedsounds[s].isValidated){
                //qDebug() << "new Sound();";
                soundsmap[lastIdUsed] = new Sound();
                soundsmap[lastIdUsed]->id = lastIdUsed;
                connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
                connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
                soundsmap[lastIdUsed]->media = new QMediaPlayer();
                soundsmap[lastIdUsed]->type = SOUNDTYPE_MEMORY;
                soundsmap[lastIdUsed]->source = s;
                soundsmap[lastIdUsed]->buffer = new QBuffer(loadedsounds[s].byteArray);
                soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadOnly);
                soundsmap[lastIdUsed]->buffer->seek(0);
                soundsmap[lastIdUsed]->media->setMedia(QMediaContent(), soundsmap[lastIdUsed]->buffer);
                soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
                soundsmap[lastIdUsed]->prepareConnections();
                soundsmap[lastIdUsed]->play();
                //Validate loaded file only once.
                //See waitLoadedMediaValidation() for details.
                if(loadedsounds[s].needValidation){
                    //qDebug() << "enter waitLoadedMediaValidation";
                    loadedsounds[s].needValidation = false;
                    loadedsounds[s].isValidated = soundsmap[lastIdUsed]->waitLoadedMediaValidation();
                    //qDebug() << "waitLoadedMediaValidation" << loadedsounds[s].isValidated;
                    if(!loadedsounds[s].isValidated) soundsmap[lastIdUsed]->stop();
                }
            }
        }else{
            //there is no resource loaded with that ID
            if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
            return 0;
        }
    }else if(s.startsWith("beep:")){
        if(loadedsounds.count(s)){
            lastIdUsed++;
            soundsmap[lastIdUsed] = new Sound();
            soundsmap[lastIdUsed]->id = lastIdUsed;
            connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
            connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
            soundsmap[lastIdUsed]->type = SOUNDTYPE_MEMORY;
            soundsmap[lastIdUsed]->source = s;
            soundsmap[lastIdUsed]->buffer = new QBuffer(loadedsounds[s].byteArray);
            soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadOnly);
            soundsmap[lastIdUsed]->buffer->seek(0);
            soundsmap[lastIdUsed]->audio = new QAudioOutput(format);
            soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
            soundsmap[lastIdUsed]->sound_samplerate=sound_samplerate;
            soundsmap[lastIdUsed]->prepareConnections();
            soundsmap[lastIdUsed]->play();
        }else{
            //there is no resource loaded with that ID
            if(*error)(*error)->q(ERROR_SOUNDRESOURCE);
        }
    }else if(QFileInfo(s).exists()){
        lastIdUsed++;
        //qDebug() << lastIdUsed << "- enter creation mode (FILE)";
        soundsmap[lastIdUsed] = new Sound();
        soundsmap[lastIdUsed]->id = lastIdUsed;
        connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
        connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
        soundsmap[lastIdUsed]->media = new QMediaPlayer();
        soundsmap[lastIdUsed]->type = SOUNDTYPE_FILE;
        soundsmap[lastIdUsed]->source = s;
        soundsmap[lastIdUsed]->media->setMedia(QUrl::fromLocalFile(QFileInfo(s).absoluteFilePath()));
        soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
        soundsmap[lastIdUsed]->prepareConnections();
        soundsmap[lastIdUsed]->play();
    }else if (url.isValid() && (url.scheme()=="http" || url.scheme()=="https" || url.scheme()=="ftp")){
        lastIdUsed++;
        //qDebug() << lastIdUsed << "- enter creation mode (WWW)";
        soundsmap[lastIdUsed] = new Sound();
        soundsmap[lastIdUsed]->id = lastIdUsed;
        connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
        connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
        soundsmap[lastIdUsed]->media = new QMediaPlayer();
        soundsmap[lastIdUsed]->type = SOUNDTYPE_WEB;
        soundsmap[lastIdUsed]->source = s;
        soundsmap[lastIdUsed]->media->setMedia(QUrl::fromUserInput(s));
        soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
        soundsmap[lastIdUsed]->prepareConnections();
        soundsmap[lastIdUsed]->play();
    }else{
        //Unable to load sound file
        if(*error)(*error)->q(ERROR_SOUNDFILE);
        //////qDebug () << "ERROR: no file to play! -" << s;
        return 0;
    }
    //qDebug() << "exit SoundSystem::playSound(QString s)";

    return lastIdUsed;

}


int SoundSystem::playSound(std::vector<std::vector<double>> sounddata) {
    if(soundSystemIsStopping) return 0;
    lastIdUsed++;
    soundsmap[lastIdUsed] = new Sound();
    soundsmap[lastIdUsed]->id = lastIdUsed;
    connect(this, SIGNAL(stopsSoundsAndWaiting()), soundsmap[lastIdUsed], SLOT(stopsSoundsAndWaiting()));
    connect(soundsmap[lastIdUsed], SIGNAL(deleteMe(int)), this, SLOT(deleteMe(int)));
    soundsmap[lastIdUsed]->byteArray  = generateSound(sounddata);
    soundsmap[lastIdUsed]->buffer = new QBuffer(soundsmap[lastIdUsed]->byteArray);
    soundsmap[lastIdUsed]->buffer->open(QIODevice::ReadWrite);
    soundsmap[lastIdUsed]->buffer->seek(0);
    soundsmap[lastIdUsed]->type = SOUNDTYPE_GENERATED;
    soundsmap[lastIdUsed]->audio = new QAudioOutput(format);
    soundsmap[lastIdUsed]->updatedMasterVolume(masterVolume);
    soundsmap[lastIdUsed]->sound_samplerate=sound_samplerate;
    soundsmap[lastIdUsed]->prepareConnections();
    soundsmap[lastIdUsed]->play();
    return lastIdUsed;
}

/*
QString SoundSystem::loadSound(QString s){
    //load or download a sound file, store it into memory and return ID of loaded resource like "sound:boom.mp3"
    QString id = QString("sound:") + s;
    if(!loadedsounds.count(id)){
        QUrl url(s);
        if(QFileInfo(s).exists()){
            if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
            QFile file(s);
            file.open(QIODevice::ReadOnly);
            loadedsounds[id].byteArray = new QByteArray(file.readAll());
            file.close();
            //check if is a playable file at first play
            loadedsounds[id].needValidation = true;
            loadedsounds[id].isValidated = false;
            return (id);
        }else if (url.isValid() && (url.scheme()=="http" || url.scheme()=="https" || url.scheme()=="ftp")){
            if(downloader==NULL)
                downloader = new BasicDownloader();
            downloader->download(QUrl::fromUserInput(s));

            if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
            loadedsounds[id].byteArray = new QByteArray(downloader->data());
            //check if is a playable file at first play
            loadedsounds[id].needValidation = true;
            loadedsounds[id].isValidated = false;
            delete downloader;
            downloader=NULL;
            return (id);
        }else{
            if(*error)(*error)->q(ERROR_SOUNDFILE);
        }
    }
    return id;
}
*/

QString SoundSystem::loadSoundFromArray(QString s, QByteArray* arr){
    QString id = QString("sound:") + s;
    if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
    loadedsounds[id].byteArray = new QByteArray(*arr);
    loadedsounds[id].needValidation = true;
    loadedsounds[id].isValidated = false;
    return (id);
}



QString SoundSystem::loadSoundFromVector(std::vector<std::vector<double> > sounddata) {
    //generate a sound, load it into memory and return ID of loaded resource like "beep:23"
    lastBeepLoaded++;
    QString id = QString("beep:") + QString::number(lastBeepLoaded);
    if (loadedsounds.contains(id)) delete(loadedsounds[id].byteArray);
    loadedsounds[id].byteArray = generateSound(sounddata);
    //we don´t need a validation - it is not a media file
    loadedsounds[id].needValidation = false;
    loadedsounds[id].isValidated = false;
    return id;
}

bool SoundSystem::unloadSound(QString id){
    if (loadedsounds.contains(id)){
        for (auto it = soundsmap.begin(); it != soundsmap.end(); )
        {
            if(it->second->source == id){
                if(it->second->soundState != 0) it->second->stop();
                delete(it->second);
                soundsmap.erase(it++);
            }else{
                ++it;
            }
        }
        delete(loadedsounds[id].byteArray);
        loadedsounds.remove(id);
        return true;
    }
    return false;
}

QByteArray*  SoundSystem::generateSound(std::vector<std::vector<double>> sounddata) {
    // mix lists of sounds in the to be played in the same time (mixed sounds)
    // the vector or vectors soundata contains a vector for each voice
    // a voice contains freq/dir pairs in order to play them

        double wave;		// current radian angle of wave
        double wavebit;		// delta radians for each byte output in wave
        short s;
        char *cs = (char *) &s;
        QByteArray* array = new QByteArray();
        QBuffer buffer(array);
        const double pi = 4*atan(1.0);
        std::vector<double> raw;
        long int rawlength=0, x;

        double max=SOUND_HALFWAVE; // max value of mixed sounds. If is greater, then normalize
        const int normalizestandby = sound_samplerate * sound_normalize_ms / 1000; // the period to keep normalize (1 sec)
        const int normalizefade = sound_samplerate * sound_fade_ms / 1000; //the time period  to fade volume to normal value after a normalize period (eceeded sound)
        double fadebit; // this will keep the value to substract from max (volume) in the fade sequence

        buffer.open(QIODevice::ReadWrite|QIODevice::Truncate);

        if(sounddata.size() == 1){

            //one voice - there is no need to mix voices, so we can write directly to buffer
            wave=0.0;
            wavebit=0.0;

            // build sound into buffer
            for(unsigned int index=0; index<sounddata[0].size(); index+=2) {
                // lets build a sine wave
                int length = sound_samplerate * sounddata[0][index+1] / 1000;
                if(sounddata[0][index]==0) {
                    // rest (put out zeros)
                    wave=0.0;
                    wavebit=0.0;
                    s=0;
                    for(int i = 0; i < length; i++) {
                        buffer.write(cs,sizeof(int16_t));
                    }
                } else {
                    wavebit = 2 * pi / ((double) sound_samplerate / (double) sounddata[0][index]);
                    for(int i = 0; i < length; i++) {
                        s = (int16_t) (SOUND_HALFWAVE * sin(wave));
                        buffer.write(cs,sizeof(int16_t));
                        wave+=wavebit;
                    }
                }
            }

        }else{

            //mix multiple voices by adding values
            for(unsigned int voice=0; voice<sounddata.size(); voice++){
                wave=0.0;
                wavebit=0.0;
                x=0;

                // mix sounds into vector
                for(unsigned int index=0; index<sounddata[voice].size(); index+=2) {
                    // lets build a sine wave
                    int length = sound_samplerate * sounddata[voice][index+1] / 1000;
                    if(x+length>rawlength){
                        rawlength=x+length;
                        raw.resize(rawlength,0);
                    }
                    if(sounddata[voice][index]==0) {
                        //silance
                        x+=length;
                    } else {
                        wavebit = 2 * pi / ((double) sound_samplerate / (double) sounddata[voice][index]);
                        for(int i = 0; i < length; i++) {
                            raw[x] += SOUND_HALFWAVE * sin(wave);
                            x++;
                            wave+=wavebit;
                        }
                    }
                }
            }

            int counter=0; //used to count the normalize standby period
            long int lastexceeded = - 10;
            //the magic of mixing sounds (fit sounds in maximum volume and keep ratio)
            for(x=0;x<rawlength;x++){
                //check if volume is exceeding and change max value to scale sound into range
                if(raw[x]<=-max){
                    max = raw[x]*-1;
                    counter = normalizestandby;
                    fadebit = (max-(double)SOUND_HALFWAVE)/(double)normalizefade;
                    lastexceeded=x;
                }else if(raw[x]>=max){
                    max = raw[x];
                    counter = normalizestandby;
                    fadebit = (max-(double)SOUND_HALFWAVE)/(double)normalizefade;
                    lastexceeded=x;
                }else if(lastexceeded==x-1){
                    //normalize also the previous zone to avoid flat top of sound wave
                    long int pos=lastexceeded;
                    //rewind to the begining of the exceeding wave (negative/positive)
                    if(raw[lastexceeded]>0){
                        while(pos>=0 && raw[pos]>0) pos--;
                    }else{
                        while(pos>=0 && raw[pos]<0) pos--;
                    }
                    buffer.seek(pos*sizeof(int16_t));
                    // set the new counter to the true begining of normalized zone
                    counter = normalizestandby-(lastexceeded-pos);
                    //write the new values
                    for(;pos<=lastexceeded;pos++){
                        s = (int16_t) (raw[pos]* (double) SOUND_HALFWAVE / max);
                        buffer.write(cs,sizeof(int16_t));
                    }
                }

                //wait a period of time (normalizestandby) before to fade to normal volume again
                //just in case the exceeding volume is still used
                if(counter>0){
                    counter--;
                }else{
                    //fade to normal volume
                    if(max>SOUND_HALFWAVE){
                        max-=fadebit;
                    }
                }
                s = (int16_t) (raw[x]* (double) SOUND_HALFWAVE / max);
                buffer.write(cs,sizeof(int16_t));
            }
        }
 //for debug
/*
        buffer.seek(0);
        QByteArray z = buffer.readAll();
        QFile file("some_name.ext");
        file.open(QIODevice::WriteOnly);
        file.write(z);
        //file.write(cs,sizeof(int16_t));
        file.close();
*/
        buffer.seek(0);
        return array;
}

void SoundSystem::setMasterVolume(double volume) {
    // volume MUST be betwen 0(mute) and 10(all the way)
    if (volume < 0.0) volume = 0;
    if (volume > 10.0) volume = 10.0;
    masterVolume = volume;
    for ( auto it = soundsmap.begin(); it != soundsmap.end(); ++it )
        it->second->updatedMasterVolume(masterVolume);
}

double SoundSystem::getMasterVolume() {
    return masterVolume;
}

void SoundSystem::exit(){
    //qDebug() << "stop everything - EEEEEEEEEEXXXXXXXXXXXXXXXXIIIIIIIIIIIIIIIITTTTTTTTTTTTTTT!";
    if(soundSystemIsStopping){
        //qDebug() << "exit - already stopped by user";
        return;
    }
    soundSystemIsStopping = true; // no more  new sounds

    //stop everything
    emit(stopsSoundsAndWaiting());
    //delete (downloader);
    //downloader=NULL;
/*
    for ( auto it = soundsmap.begin(); it != soundsmap.end(); ++it ){
        it->second->deleteLater();
        it->second=NULL;
    }
    soundsmap.clear();

    QMap<QString, QByteArray*>::const_iterator i = loadedsounds.constBegin();
    while (i != loadedsounds.constEnd()) {
        delete(i.value());
        ++i;
    }
    loadedsounds.clear();
    */
}

void SoundSystem::deleteMe(int id){
    //qDebug() << id << "deleteMe" << "active:" << soundsmap.size();
    if(soundsmap.count(id)){
        disconnect(this, 0, soundsmap[id], 0);
        soundsmap[id]->deleteLater();
        soundsmap.erase(id);
    }
}

void SoundSystem::pause(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) soundsmap[sound]->pause();
}

double SoundSystem::position(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) return soundsmap[sound]->position();
    return 0.0;
}

void SoundSystem::seek(int sound, double pos){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)){
        bool s = soundsmap[sound]->seek(pos);
        if(!s && *error)(*error)->q(WARNING_SOUNDNOTSEEKABLE);
    }
}

int SoundSystem::state(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) return soundsmap[sound]->state();
    return 0;
}

void SoundSystem::stop(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) soundsmap[sound]->stop();
}

void SoundSystem::wait(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) soundsmap[sound]->wait();
}

double SoundSystem::length(int sound) {
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)){
        double l = soundsmap[sound]->length();
        if(l>=0.0) return l;
        if(*error)(*error)->q(WARNING_SOUNDLENGTH);
    }
    return 0.0;
}

void SoundSystem::volume(int sound, double vol){
    vol=vol/10.0;
    if(vol<0.0) vol=0.0;
    if(vol>1.0) vol=1.0;
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)){
        soundsmap[sound]->individualVolume=vol;
        soundsmap[sound]->updatedMasterVolume(masterVolume);
    }
}

void SoundSystem::play(int sound){
    if(sound<0) sound=lastIdUsed;
    if(soundsmap.count(sound)) soundsmap[sound]->play();
}





