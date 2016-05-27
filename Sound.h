#pragma once

#include <math.h>
#include <stdio.h>

#include "Constants.h"

#include <QAudioOutput>
#include <QBuffer>
#include <QEventLoop>
#define SOUND_HALFWAVE 0x7fff

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
