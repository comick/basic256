// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

#include <map>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ErrorCodes.h"
#include "Stack.h"


#define VARIABLE_MAXARRAYELEMENTS 1048576
#define MAX_RECURSE_LEVELS	1024

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
		//
		void clear();
		void increaserecurse();
		void decreaserecurse();
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
		int recurselevel;
		std::map<int, variable*> varmap;
		void clearvariable(variable*);
		variable* getvfromnum(int, bool);

};
