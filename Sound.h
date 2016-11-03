#pragma once

#include <math.h>
#include <stdio.h>

#include "Constants.h"
#include "Settings.h"
#include <QObject>
#include <QTimer>

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
  void playSounds(std::vector<std::vector<int>>);
  void setVolume(double);
  double getVolume();
  void stop();

signals:
    void exitLoop();

 private:
  QAudioFormat format;
  bool notsupported;
  double soundVolume;
  bool cancel;
  int sound_samplerate;
  int sound_normalize_ms;
  int sound_fade_ms;

};
