/** Copyright (C) 2006, Ian Paul Larsen.
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

#ifndef __INTERPRETER_H
#define __INTERPRETER_H

#include <QPixmap>
#include <QImage>
#include <QThread>
#include <QFile>
#include <stdio.h>
#include <cmath>
#include "BasicGraph.h"
#include "Stack.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED};

#define NUMVARS 2000


struct array
{
  int xdim;
  int ydim;
  int size;
  union
  {
    double *fdata;
    char **sdata;
  } data;
};


struct variable
{
  b_type type;
  union {
    char *string;
    double floatval; 
    array *arr;
  } value;
};

struct byteCodeData
{
  unsigned int size;
  void *data;
};


struct frame {
  frame *next;
  unsigned char *returnAddr;
};

  
struct forframe {
  forframe *prev;
  forframe *next;
  unsigned int variable;
  unsigned char *returnAddr;
  double endNum;
  double step;
};



class Interpreter : public QThread
{
  Q_OBJECT;
 public:
  Interpreter(BasicGraph *);
  int compileProgram(char *);
  void initialize();
  byteCodeData *getByteCode(char *);
  bool isRunning();
  bool isStopped();
  bool isAwaitingInput();
  void setInputReady();
  void cleanup();
  void run();
  bool debugMode;

 public slots:
  int execByteCode();
  void pauseResume();
  void stop();
  void receiveInput(QString);

 signals:
  void fastGraphics();
  void runFinished();
  void goutputReady();
  void outputReady(QString);
  void inputNeeded();
  void clearText();
  void getKey();
  void playSounds(int, int*);
  void setVolume(int);
  void system(char*);
  void speakWords(QString);
  void playWAV(QString);
  void stopWAV();
  void goToLine(int);
  void highlightLine(int);
  void varAssignment(QString name, QString value, int arraylen);
  void resizeGraph(int, int);

 private:
  void waitForGraphics();
  void printError(QString);
  QImage *image;
  BasicGraph *graph;
  variable vars[NUMVARS];
  Stack stack;
  QFile *stream;
  unsigned char *op;
  frame *callstack;
  forframe *forstack;
  QColor pencolor;
  run_status status;
  run_status oldstatus;
  bool fastgraphics;
  QString inputString;
  void clearvars();
  bool once;
  int currentLine;
  QString fontfamily;
  int fontpoint;
  int fontweight;
};


#endif 
