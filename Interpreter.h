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
#include <dirent.h>
#include "BasicGraph.h"
#include "Stack.h"
#include "Variables.h"
#include "ErrorCodes.h"
#include "Sound.h"


#if QT_VERSION >= 0x050000
    #include <QtPrintSupport/QPrinter>
    #include <QtPrintSupport/QPrinterInfo>
#else
    #include <QPrinter>
    #include <QPrinterInfo>
#endif

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

#ifndef M_PI
    #define M_PI 3.14159265
#endif

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED};

#define NUMFILES 8
#define NUMSOCKETS 8
#define NUMDBCONN 8
#define NUMDBSET 8

#define STRINGMAXLEN 16777216


struct byteCodeData
{
  unsigned int size;
  void *data;
};


// used by function calls, subroutine calls, and gosubs for return location
struct frame {
  frame *next;
  unsigned char *returnAddr;
};

// used to track nested on-error and try/catch definitions
struct onerrorframe {
  onerrorframe *next;
  int onerroraddress;
  bool onerrorgosub;
};

// structure for the nested for statements  
struct forframe {
  forframe *prev;
  forframe *next;
  unsigned int variable;
  unsigned char *returnAddr;
  double endNum;
  double step;
  int recurselevel;
};

typedef struct {
	QImage *image;
	QImage *underimage;
	double x;
	double y;
	double r;	// rotate
	double s;	// scale
	bool visible;
} sprite;

class Interpreter : public QThread
{
  Q_OBJECT;
 public:
  Interpreter();
  ~Interpreter();
  int compileProgram(char *);
  void initialize();
  bool isRunning();
  bool isStopped();
  bool isAwaitingInput();
  void setInputReady();
  void cleanup();
  void run();
  bool debugMode;
  QString returnString;		// return value from runcontroller emit
  int returnInt;			// return value from runcontroller emit

 public slots:
  int execByteCode();
  void runPaused();
  void runResumed();
  void runHalted();
  void inputEntered(QString);

 signals:
  void fastGraphics();
  void stopRun();
  void goutputReady();
  void outputReady(QString);
  void getInput();
  void outputClear();
  void getKey();
  void playSounds(int, int*);
  void setVolume(int);
  void executeSystem(QString);
  void speakWords(QString);
  void playWAV(QString);
  void waitWAV();
  void stopWAV();
  void goToLine(int);
  void highlightLine(int);
  void varAssignment(int, QString, QString, int, int);
  void mainWindowsResize(int, int, int);
  void mainWindowsVisible(int, bool);
  void dialogAlert(QString);
  void dialogConfirm(QString, int);
  void dialogPrompt(QString, QString);

 private:
  int optype(int op);
  QString opname(int);
  QString opxname(int);
  void waitForGraphics();
  void printError(int, QString);
  QString getErrorMessage(int);
  QString getWarningMessage(int);
  int netSockClose(int);
  Variables variables;
  Stack stack;
  QFile **stream;
  unsigned char *op;
  frame *callstack;
  forframe *forstack;
  onerrorframe *onerrorstack;
  QPen drawingpen;
  QBrush drawingbrush;
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
  void closeDatabase(int);
  int errornum;
  int errorvarnum;
  QString errormessage;
  int lasterrornum;
  QString lasterrormessage;
  int lasterrorline;
  int netsockfd[NUMSOCKETS];
  DIR *directorypointer;		// used by DIR function
  QTime runtimer;				// used by MSEC function
  Sound sound;
  QString currentIncludeFile;	// set to current included file name for runtime error messages

  bool printing;
  QPrinter *printdocument;
  QPainter *printdocumentpainter;

  QSqlQuery *dbSet[NUMDBCONN][NUMDBSET];		// allow NUMDBSET number of sets on a database connection
  bool dbSetRow[NUMDBCONN][NUMDBSET];			// have we moved to the first row yet?


};


#endif 
