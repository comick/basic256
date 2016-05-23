#pragma once

#include <math.h>
#include <stdio.h>

#include "Constants.h"

#ifdef SOUND_DSP
#endif

#ifdef SOUND_SDL
	#include <unistd.h>
	#include <SDL/SDL.h>
	#include <SDL/SDL_mixer.h>
	#define SDL_CHAN_WAV 1
	#define SDL_CHAN_SOUND 2
	#define SOUND_HALFWAVE 0x7f
#endif

#ifdef SOUND_QMOBILITY
	#include <QAudioOutput>
	#include <QBuffer>
	#include <QEventLoop>
	#define SOUND_HALFWAVE 0x7fff
#endif

#ifdef SOUND_WIN32
	#include <windows.h>
	#include <mmsystem.h>
#endif

class Sound
{
 public:
  Sound();
  ~Sound();
  void playSounds(int, int*);
  void setVolume(double);
  double getVolume();

 private:
  double soundVolume;
};
