#pragma once

#include <list>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>

#include "ErrorCodes.h"
#include "Types.h"

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
  
  void pushelement(b_type, double, QString);
  void pushstring(QString);
  void pushint(int);
  void pushfloat(double);
  void swap();
  void swap2();
  void topto2();
  void dup();
  void dup2();
  int peekType();
  stackdata *popelement();
  int popint();
  double popfloat();
  QString popstring();
  void clear();
  QString debug();
  int height();
  int compareTopTwo();
  int compareFloats(double, double);
  int error();
  void clearerror();
  void settypeconverror(int);
  void setdecimaldigits(int);

 private:
  std::list<stackdata*> stacklist;
  int errornumber;		// internal storage of last stack error
  int typeconverror;	// 0-return no errors on type conversion, 1-warn, 2-error
  int decimaldigits;	// display n decinal digits 12 default - 8 to 15 valid
};
