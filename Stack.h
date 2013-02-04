#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


enum b_type {T_FLOAT, T_STRING, T_BOOL, T_ARRAY, T_STRARRAY, T_UNUSED, T_VARREF, T_VARREFSTR};
// stack types T_VARREF, T_VARREFSTR are to pass a variable reference to a subroutine or function (BYREF passing)

typedef struct
{
  b_type type;
  union {
    char *string;
    double floatval; 
  } value;
} stackval;


class Stack
{
 public:
  Stack();
  ~Stack();
  void pushstring(char *);
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
  stackval *pop();
  int popint();
  double popfloat();
  char *popstring();
  int toint(stackval *);
  double tofloat(stackval *);
  char *tostring(stackval *);
  void clean(stackval *);
  void clear();
  void debug();
  int height();

  static const int defaultFToAMask = 6;
  int fToAMask;

 private:
  static const int initialSize = 64;
  stackval *top;
  stackval *bottom;
  stackval *limit;
  void checkLimit();
  void dup(int);
};
