#pragma once

#include <math.h>
#include <stdio.h>

#ifdef SOUND_DSP
#endif

#ifdef SOUND_SDL
	#include <unistd.h>
	#include <SDL/SDL.h>
	#include <SDL/SDL_mixer.h>
	#define SDL_CHAN_WAV 1
	#define SDL_CHAN_SOUND 2
#endif

#ifdef SOUND_QMOBILITY
	#include <QAudioOutput>
	#include <QBuffer>
	#include <QEventLoop>
#endif

#ifdef WIN32
	#include <windows.h> 
	#include <mmsystem.h>
#endif

class Sound
{
 public:
  Sound();
  ~Sound();
  void playSounds(int, int*);
  void setVolume(int);

 private:
  int soundVolume;
};
