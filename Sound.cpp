#include "Sound.h"


Sound::Sound() :
    QObject()
{
    soundVolume = 10;	// set default sound volume to max
    cancel=false;
}

Sound::~Sound() {}

// play lists of sounds in the same time (mixed sounds)
// each list of sonds is treated as an individual voice
// list of sounds: [counter_of_items], [frequency], [duration in ms], [frequency], [duration in ms]...
void Sound::playSounds(int voices, int* freqdur) {
    cancel=false;


	int16_t amplititude = SOUND_HALFWAVE /10 * soundVolume; // (half wave height)
    double wave;		// current radian angle of wave
    double wavebit;		// delta radians for each byte output in wave
    short s;
    char *cs = (char *) &s;
    int notes;
    int voice;
    int index=0, maxvoice=0;
    long maxsize=0;
    QBuffer buffer[voices];
    QAudioOutput* audio[voices];


    // setup the audio format
    QAudioFormat format;
    format.setSampleRate(SOUND_SAMPLERATE);
    format.setChannelCount(1);
    format.setSampleSize(16);	// 16 bit audio
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        fprintf(stderr,"Raw audio format not supported by backend, cannot play sound.\n");
        return;
    }



    for(voice=0; voice<voices; voice++){
        wave=0.0;
        wavebit=0.0;
        notes=freqdur[index++]/2;

        // build sound into buffer
        buffer[voice].open(QBuffer::ReadWrite);
        for(int tnotes=0; tnotes<notes; tnotes++) {
            // lets build a sine wave
            int length = SOUND_SAMPLERATE * freqdur[index+1] / 1000;
            if(freqdur[index]==0) {
                // rest (put out zeros)
                wave=0;
                wavebit=0;
            } else {
                wavebit = 2 * M_PI / ((double) SOUND_SAMPLERATE / (double) freqdur[index]);
            }
            index+=2;

            for(int i = 0; i < length; i++) {
                s = (int16_t) (amplititude * sin(wave));
                buffer[voice].write(cs,sizeof(int16_t));
                wave+=wavebit;
            }
        }
        buffer[voice].seek(0);
        //store longest voice
        if(buffer[voice].size()>maxsize){
            maxsize=buffer[voice].size();
            maxvoice=voice;
        }
        audio[voice]=new QAudioOutput(format);
    }




    for(voice=0; voice<voices; voice++){
        audio[voice]->start(&buffer[voice]);
    }

    // ...then wait for the sound to finish
    QEventLoop loop;
    QObject::connect(audio[maxvoice], SIGNAL(stateChanged(QAudio::State)), &loop, SLOT(quit()));
    QObject::connect(this, SIGNAL(exitLoop()), &loop, SLOT(quit()));
    if(!cancel){
        do {
            loop.exec(QEventLoop::WaitForMoreEvents);
        } while((audio[maxvoice]->state() == QAudio::ActiveState)&&!cancel);
    }


    //Stops the audio output, detaching from the system resource
    for(voice=0; voice<voices; voice++){
        audio[voice]->stop();
        buffer[voice].close();
        delete audio[voice];
    }
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


