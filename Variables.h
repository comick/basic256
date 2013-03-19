// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

#include <map>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <QString>

#include "ErrorCodes.h"
#include "Stack.h"


#define VARIABLE_MAXARRAYELEMENTS 1048576
#define MAX_RECURSE_LEVELS	1048576

struct array
{
  int xdim;
  int ydim;
  int size;
  union
  {
    double *fdata;
    QString *sdata;
  } data;
};


struct variable
{
  b_type type;
  QString value_string;
  double value_floatval; 
  array *value_arr;
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
		int getrecurse();
		//
		int type(int);
		int error();
		int errorvarnum();
		//
		void setvarref(int, int);
		//
		void setfloat(int, double);
		double getfloat(int);
		//
		void setstring(int, QString);
		QString getstring(int);
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
		void arraysetstring(int, int, QString);
		void array2dsetstring(int, int, int, QString);
		QString arraygetstring(int, int);
		QString array2dgetstring(int, int, int);
		//
		void makeglobal(int);


	private:
		int lasterror;
		int lasterrorvar;
		int recurselevel;
		std::map<int, std::map<int,variable*> > varmap;
		std::map<int, bool> globals;
		void clearvariable(variable*);
		variable* getvfromnum(int, bool);
		bool isglobal(int);

};
