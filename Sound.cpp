#include "Sound.h"
#include <QDebug>

Sound::Sound() :
    QObject()
{
    SETTINGS;
    setVolume(settings.value(SETTINGSSOUNDVOLUME, SETTINGSSOUNDVOLUMEDEFAULT).toInt());
    sound_samplerate = settings.value(SETTINGSSOUNDSAMPLERATE, SETTINGSSOUNDSAMPLERATEDEFAULT).toInt();
    sound_normalize_ms = settings.value(SETTINGSSOUNDNORMALIZE, SETTINGSSOUNDNORMALIZEDEFAULT).toInt();
    sound_fade_ms = settings.value(SETTINGSSOUNDVOLUMERESTORE, SETTINGSSOUNDVOLUMERESTOREDEFAULT).toInt();


    cancel=false;

    // setup the audio format
    format.setSampleRate(sound_samplerate);
    format.setChannelCount(1);
    format.setSampleSize(16);	// 16 bit audio
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        notsupported = true;
    }else{
        notsupported = false;
    }

}

Sound::~Sound() {}

// play lists of sounds in the same time (mixed sounds)
// the vector or vectors soundata contains a vector for each voice
// a voice contains freq/dir pairs in order to play them
void Sound::playSounds(std::vector<std::vector<int>> sounddata) {
    if (notsupported) {
        fprintf(stderr,"Raw audio format not supported by backend, cannot play sound.\n");
        return;
    }
    cancel=false;
	int16_t amplititude = SOUND_HALFWAVE /10 * soundVolume; // (half wave height)
    double wave;		// current radian angle of wave
    double wavebit;		// delta radians for each byte output in wave
    short s;
    char *cs = (char *) &s;
    QBuffer buffer;
    QAudioOutput* audio;
    QEventLoop* loop;
    const double pi = 4*atan(1.0);
    std::vector<double> raw;
    long int rawlenght=0, x;

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
                    s = (int16_t) (amplititude * sin(wave));
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
                if(x+length>rawlenght){
                    rawlenght=x+length;
                    raw.resize(rawlenght,0);
                }
                if(sounddata[voice][index]==0) {
                    //silance
                    x+=length;
                } else {
                    wavebit = 2 * pi / ((double) sound_samplerate / (double) sounddata[voice][index]);
                    for(int i = 0; i < length; i++) {
                        raw[x] += amplititude * sin(wave);
                        x++;
                        wave+=wavebit;
                    }
                }
            }
        }

        int counter=0; //used to count the normalize standby period
        long int lastexceeded = - 10;
        //the magic of mixing sounds (fit sounds in maximum volume and keep ratio)
        for(x=0;x<rawlenght;x++){
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

/* for debug
        buffer.seek(0);
        QByteArray z = buffer.readAll();
        QFile file("some_name.ext");
        file.open(QIODevice::WriteOnly);
        file.write(z);
        //file.write(cs,sizeof(int16_t));
        file.close();
*/

    // play ...then wait for the sound to finish

	
	buffer.seek(0);
	
 	loop = new QEventLoop();

    audio = new QAudioOutput(format);
    audio->setNotifyInterval(100);
    QObject::connect(audio, SIGNAL(notify()), loop, SLOT(quit()));
    QObject::connect(audio, SIGNAL(stateChanged(QAudio::State)), loop, SLOT(quit()));

    //
    QObject::connect(this, SIGNAL(exitLoop()), loop, SLOT(quit()));
    audio->start(&buffer);
    if(!cancel){
        do {
            loop->exec();
        } while((audio->state() == QAudio::ActiveState)&&!cancel);
    }


    //Stops the audio output, detaching from the system resource
	audio->stop();
	buffer.close();
	delete audio;
}

void Sound::setVolume(double volume) {
    // volume MUST be betwen 0(mute) and 10(all the way)
    if (volume < 0.0) volume = 0;
    if (volume > 10.0) volume = 10.0;
    soundVolume = volume;
}

double Sound::getVolume() {
    return soundVolume;
}

void Sound::stop() {
    cancel=true;
    emit(exitLoop());
}


