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
#include <stdio.h>
#include <cmath>
#include <sqlite3.h>
#include "BasicGraph.h"
#include "Stack.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

#ifndef ERROR_NONE
#define ERROR_NONE 0
#define ERROR_NOSUCHLABEL 1
#define ERROR_NOSUCHLABEL_MESSAGE "No such label"
#define ERROR_FOR1 2
#define ERROR_FOR1_MESSAGE "Illegal FOR -- start number > end number"
#define ERROR_FOR2 3
#define ERROR_FOR2_MESSAGE "Illegal FOR -- start number < end number"
#define ERROR_NEXTNOFOR 4
#define ERROR_NEXTNOFOR_MESSAGE "Next without FOR"
#define ERROR_FILENUMBER 5
#define ERROR_FILENUMBER_MESSAGE "Invalid File Number"
#define ERROR_FILEOPEN 6
#define ERROR_FILEOPEN_MESSAGE "Unable to open file"
#define ERROR_FILENOTOPEN 7
#define ERROR_FILENOTOPEN_MESSAGE "File not open."
#define ERROR_FILEWRITE 8
#define ERROR_FILEWRITE_MESSAGE "Unable to write to file"
#define ERROR_FILERESET 9
#define ERROR_FILERESET_MESSAGE "Unable to reset file"
#define ERROR_ARRAYSIZELARGE 10
#define ERROR_ARRAYSIZELARGE_MESSAGE "Array dimension too large"
#define ERROR_ARRAYSIZESMALL 11
#define ERROR_ARRAYSIZESMALL_MESSAGE "Array dimension too small"
#define ERROR_NOSUCHVARIABLE 12
#define ERROR_NOSUCHVARIABLE_MESSAGE "Unknown variable"
#define ERROR_NOTARRAY 13
#define ERROR_NOTARRAY_MESSAGE "Not an array variable"
#define ERROR_NOTSTRINGARRAY 14
#define ERROR_NOTSTRINGARRAY_MESSAGE "Not a string array variable"
#define ERROR_ARRAYINDEX 15
#define ERROR_ARRAYINDEX_MESSAGE "Array index out of bounds"
#define ERROR_STRNEGLEN 16
#define ERROR_STRNEGLEN_MESSAGE "Substring length less that zero"
#define ERROR_STRSTART 17
#define ERROR_STRSTART_MESSAGE "Starting position less than zero"
#define ERROR_STREND 18
#define ERROR_STREND_MESSAGE "String not long enough for given starting character"
#define ERROR_NONNUMERIC 19
#define ERROR_NONNUMERIC_MESSAGE "Non-numeric value in numeric expression"
#define ERROR_RGB 20
#define ERROR_RGB_MESSAGE "RGB Color values must be in the range of 0 to 255."
#define ERROR_PUTBITFORMAT 21
#define ERROR_PUTBITFORMAT_MESSAGE "String input to putbit incorrect."
#define ERROR_POLYARRAY 22
#define ERROR_POLYARRAY_MESSAGE "Argument not an array for poly()/stamp()"
#define ERROR_POLYPOINTS 23
#define ERROR_POLYPOINTS_MESSAGE "Not enough points in array for poly()/stamp()"
#define ERROR_IMAGEFILE 24
#define ERROR_IMAGEFILE_MESSAGE "Unable to load image file."
#define ERROR_SPRITENUMBER 25
#define ERROR_SPRITENUMBER_MESSAGE "Sprite number out of range."
#define ERROR_SPRITENA 26
#define ERROR_SPRITENA_MESSAGE "Sprite has not been assigned."
#define ERROR_SPRITESLICE 27
#define ERROR_SPRITESLICE_MESSAGE "Unable to slice image."
#define ERROR_FOLDER 28
#define ERROR_FOLDER_MESSAGE "Invalid directory name."
#define ERROR_DECIMALMASK 29
#define ERROR_DECIMALMASK_MESSAGE "Decimal mask must be in the range of 0 to 15."
#define ERROR_DBOPEN 30
#define ERROR_DBOPEN_MESSAGE "Unable to open SQLITE database."
#define ERROR_DBQUERY 31
#define ERROR_DBQUERY_MESSAGE "Database query error (message follows)."
#define ERROR_DBNOTOPEN 32
#define ERROR_DBNOTOPEN_MESSAGE "Database must be opened first."
#define ERROR_DBCOLNO 33
#define ERROR_DBCOLNO_MESSAGE "Column number out of range."
#define ERROR_DBNOTSET 34
#define ERROR_DBNOTSET_MESSAGE "Record set must be opened first."
#define ERROR_EXTOPBAD 35
#define ERROR_EXTOPBAD_MESSAGE "Invalid Extended Op-code."
#endif

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED};

#define NUMVARS 2000
#define NUMFILES 8


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
  QImage *image;
  BasicGraph *graph;
  variable vars[NUMVARS];
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
  void clearvars();
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
  sqlite3 *dbconn;
  sqlite3_stmt *dbset;
  int errornum;
  QString errormessage;
  int lasterrornum;
  QString lasterrormessage;
  int lasterrorline;
  int onerroraddress;
};


#endif 
