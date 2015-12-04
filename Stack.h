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

#include "Error.h"
#include "Convert.h"
#include "DataElement.h"
#include "Settings.h"



class Stack
{
 public:
  Stack(Error *, Convert *);
  ~Stack();
  
  Error *error;
  Convert *convert;
  void pushdataelement(DataElement*);
  void pushstring(QString);
  void pushint(int);
  void pushvarref(int);
  void pushfloat(double);
  void swap();
  void swap2();
  void topto2();
  void dup();
  void dup2();
  int peekType();
  int peekType(int);
  DataElement *popelement();
  int popint();
  double popfloat();
  QString popstring();
  void clear();
  QString debug();
  int height();


 private:
  std::list<DataElement*> stacklist;
};
