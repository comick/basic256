#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ErrorCodes.h"
#include "Stack.h"


#define VARIABLE_NUMVARS 2000
#define VARIABLE_MAXARRAYELEMENTS 100000

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


class Variables
{
	public:
		Variables();
		~Variables();
		void clear();
		void clearvariable(int);
		//
		int type(int);
		int error();
		//
		void setfloat(int, double);
		double getfloat(int);
		//
		void setstring(int, char *);
		char *getstring(int);
		//
		void arraydimfloat(int, int, int, bool);
		void arraydimstring(int, int, int, bool);
		//
		int arraysize(int);
		int arraysizex(int);
		int arraysizey(int);
		//
		void arraysetfloat(int, int, double);
		void array2dsetfloat(int, int, int, double);
		double arraygetfloat(int, int);
		double array2dgetfloat(int, int, int);
		//
		void arraysetstring(int, int, char *);
		void array2dsetstring(int, int, int, char *);
		char *arraygetstring(int, int);
		char *array2dgetstring(int, int, int);



	private:
		int lasterror;
		variable vars[VARIABLE_NUMVARS];

};
