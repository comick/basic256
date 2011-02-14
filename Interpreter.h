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
#include <QDir>
#include <QTime>
#include <stdio.h>
#include <cmath>
#include <sqlite3.h>
#include <dirent.h>
#include "BasicGraph.h"
#include "Stack.h"
#include "Variables.h"
#include "ErrorCodes.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED};

#define NUMFILES 8
#define NUMSOCKETS 8

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

typedef struct {
	QImage *image;
	QImage *underimage;
	double x;
	double y;
	bool visible;
	bool active;
} sprite;

class Interpreter : public QThread
{
  Q_OBJECT;
 public:
  Interpreter(BasicGraph *);
  ~Interpreter();
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
  void executeSystem(char*);
  void speakWords(QString);
  void playWAV(QString);
  void waitWAV();
  void stopWAV();
  void goToLine(int);
  void highlightLine(int);
  void varAssignment(QString name, QString value, int arraylen);
  void resizeGraph(int, int);

 private:
  int compareTwoStackVal(stackval *, stackval *);
  void waitForGraphics();
  void printError(int, QString);
  QString getErrorMessage(int);
  int netSockClose(int);
  QImage *image;
  BasicGraph *graph;
  Variables variables;
  Stack stack;
  QFile **stream;
  unsigned char *op;
  frame *callstack;
  forframe *forstack;
  QColor pencolor;
  run_status status;
  run_status oldstatus;
  bool fastgraphics;
  QString inputString;
  bool once;
  int currentLine;
  QString fontfamily;
  int fontpoint;
  int fontweight;
  void clearsprites();
  void spriteundraw(int);
  void spriteredraw(int);
  bool spritecollide(int, int);
  sprite *sprites;
  int nsprites;
  void closeDatabase();
  sqlite3 *dbconn;
  sqlite3_stmt *dbset;
  int errornum;
  QString errormessage;
  int lasterrornum;
  QString lasterrormessage;
  int lasterrorline;
  int onerroraddress;
  int netsockfd[NUMSOCKETS];
  DIR *directorypointer;			// used by dir function
  QTime runtimer;				// used by 

};


#endif 
