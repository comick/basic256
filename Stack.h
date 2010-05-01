#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


enum b_type {T_INT, T_FLOAT, T_STRING, T_BOOL, T_ARRAY, T_STRARRAY, T_UNUSED};


typedef struct
{
  b_type type;
  union {
    char *string;
    int intval;
    double floatval; 
  } value;
} stackval;


class Stack
{
 public:
  Stack();
  ~Stack();
  void push(char *);
  void push(int);
  void push(double);
  void swap();
  stackval *pop();
  int popint();
  double popfloat();
  char *popstring();
  void clean(stackval *);
  void clear();

 private:
  static const int initialSize = 64;
  stackval *top;
  stackval *bottom;
  stackval *limit;
  void checkLimit();
};
