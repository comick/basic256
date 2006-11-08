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

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED};
enum b_type {T_INT, T_FLOAT, T_STRING, T_BOOL, T_ARRAY, T_STRARRAY, T_UNUSED};

#define NUMVARS 2000


struct array
{
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

class stackval
{
 public:
  stackval *next;
  b_type type;
  union {
    char *string;
    int intval;
    double floatval; 
  } value;
  stackval();
  ~stackval();
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


class Stack
{
 public:
  Stack();
  void push(char *);
  void push(int);
  void push(double);
  stackval *pop();
  
 private:
  stackval *top;
};


class Interpreter : public QThread
{
  Q_OBJECT;
 public:
  Interpreter(QImage *, QImage *);
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
  void goToLine(int);
  void highlightLine(int);

 private:
  void printError(QString);
  QImage *image;
  QImage *imask;
  variable vars[NUMVARS];
  Stack stack;
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
};


#endif 
