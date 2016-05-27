#include "Sound.h"


Sound::Sound() {
    soundVolume = 10;	// set default sound volume to max
}

Sound::~Sound() {
}

// play a list of sounds 0,2,4... = frequency & 1,3,5... = duration in ms
// notes is arraylength/2 (number of notes)
void Sound::playSounds(int notes, int* freqdur) {


	int16_t amplititude = SOUND_HALFWAVE /10 * soundVolume; // (half wave height)
	double wave=0.0;		// current radian angle of wave
	double wavebit=0.0;		// delta radians for each byte output in wave
    short s;
    char *cs = (char *) &s;

    // setup the audio format
    QAudioFormat format;
    format.setSampleRate(4400);
    format.setChannelCount(1);
    format.setSampleSize(16);	// 16 bit audio
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    // build sound into buffer
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    for(int tnotes=0; tnotes<notes; tnotes++) {
        // lets build a sine wave
        int length = format.sampleRate() * freqdur[tnotes*2+1] / 1000;
		if(freqdur[tnotes*2]==0) {
			// rest (put out zeros)
			wave=0;
			wavebit=0;
		} else {
			wavebit = 2 * M_PI / ((double) format.sampleRate() / (double) freqdur[tnotes*2]);
		}
		
        for(int i = 0; i < length; i++) {
            s = (int16_t) (amplititude * sin(wave));
            buffer.write(cs,sizeof(int16_t));
            wave+=wavebit;
        }
    }
    buffer.seek(0);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        fprintf(stderr,"Raw audio format not supported by backend, cannot play sound.\n");
        return;
    }

    QAudioOutput audio(format);
    audio.start(&buffer);

    // ...then wait for the sound to finish
    QEventLoop loop;
    QObject::connect(&audio, SIGNAL(stateChanged(QAudio::State)), &loop, SLOT(quit()));
    do {
        loop.exec(QEventLoop::WaitForMoreEvents);
    } while((audio.state() == QAudio::ActiveState));

    buffer.close();

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


