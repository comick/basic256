#pragma once

#include <stack>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>

#include <QString>


enum b_type {T_FLOAT, T_STRING, T_BOOL, T_ARRAY, T_STRARRAY, T_UNUSED, T_VARREF, T_VARREFSTR};
// stack types T_VARREF, T_VARREFSTR are to pass a variable reference to a subroutine or function (BYREF passing)

struct stackdata
{
  b_type type;
  QString string;
  double floatval; 
};


class Stack
{
 public:
  Stack();
  ~Stack();
  void pushstring(QString);
  void pushint(int);
  void pushfloat(double);
  void pushvarref(int);
  void pushvarrefstr(int);
  void swap();
  void swap2();
  void topto2();
  void dup();
  void dup2();
  int peekType();
  int popint();
  double popfloat();
  QString popstring();
  void clear();
  void debug();
  int height();
  int compareTopTwo();

 private:
  std::stack<stackdata*> stackstack;

};
