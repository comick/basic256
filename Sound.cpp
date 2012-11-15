#include "Sound.h"

Sound::Sound()
{
	soundVolume = 5;	// set default sound volume to 1/2

	#ifdef SOUND_SDL
		//mono
		Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16SYS, 1, 2048);
	#endif

}

Sound::~Sound()
{
}

// play a list of counds 0,2,4... = frequency & 1,3,5... = duration in ms
// notes is arraylength/2 (number of notes)
void Sound::playSounds(int notes, int* freqdur)
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

#ifdef SOUND_DSP
	// Code loosely based on idea from TONEGEN by Timothy Pozar
	// - Plays a sine wave via the dsp or standard out.
 
	int stereo = 0;		// mono (stereo - false)
	int rate = 11025;/dev/dsp
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

#ifdef SOUND_SDL
	Mix_Chunk c;

	c.allocated = 1; // sdl needs to free chunk data
	c.volume = (unsigned char) (tan((double) soundVolume / 10) * 0x7f);
	
	// lets build the output
	double wave = 0;
	int totaltime = 0;
	for(int tnotes=0;tnotes<notes;tnotes++) {
		totaltime += freqdur[tnotes*2+1];
	}
        if (totaltime>0) {
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
        }
#endif

#ifdef SOUND_QMOBILITY
	// use the QT library Audio Output
	// part of the QtMobility project
	double amplititude = tan((double) soundVolume / (double) 10) * (double) 0x7f;  // (half wave height)
	double wave = 0;	

 	// setup the audio format
	QAudioFormat format;
 	format.setFrequency(11025);
 	format.setChannels(1);
 	format.setSampleSize(8);
 	format.setCodec("audio/pcm");
 	format.setByteOrder(QAudioFormat::LittleEndian);
 	format.setSampleType(QAudioFormat::SignedInt);

	// build sound into buffer
	QBuffer buffer;
	buffer.open(QBuffer::ReadWrite);
	for(int tnotes=0;tnotes<notes;tnotes++) {
		// lets build a sine wave
		int length = format.sampleRate() * freqdur[tnotes*2+1] / 1000;
		double wavebit = 2 * M_PI / ((double) format.sampleRate() / (double) freqdur[tnotes*2]);
		for(int i = 0; i < length; i++) {
			buffer.putChar((char) (amplititude * sin(wave)));
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
#endif

}

void Sound::setVolume(int volume)
{
  // volume MUST be betwen 0(mute) and 10(all the way)
  soundVolume = volume;
}


