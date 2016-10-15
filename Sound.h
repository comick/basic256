#pragma once

#include <math.h>
#include <stdio.h>

#include "Constants.h"
#include <QObject>

#include <QAudioOutput>
#include <QBuffer>
#include <QEventLoop>

#define SOUND_HALFWAVE 0x7fff
#define SOUND_SAMPLERATE 4400

class Sound : public QObject
{
    Q_OBJECT
public:
    explicit Sound();
    virtual ~Sound();
  void playSounds(int voices, int*);
  void setVolume(double);
  double getVolume();
  void stop();

signals:
    void exitLoop();

 private:
  double soundVolume;
  bool cancel;

};
