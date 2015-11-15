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
  


struct arrayvariable
{
  b_type type;
  QString stringval;
  double floatval; 
};
  
typedef struct VariableArrayPart
{
  int xdim;
  int ydim;
  int size;
  std::map<int,arrayvariable*> datamap;
} VariableArrayPart;

struct variable
{
  b_type type;
  QString stringval;
  double floatval; 
  VariableArrayPart *arr;
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
		variable* get(int);
		void set(int, b_type, double, QString);
 		//
		void arraydim(b_type, int, int, int, bool);
		arrayvariable* arrayget(int, int, int);
		void arrayset(int, int, int, b_type, double, QString);
		//
		int arraysize(int);
		int arraysizex(int);
		int arraysizey(int);
		//
		void makeglobal(int);


	private:
		int lasterror;
		int lasterrorvar;
		int recurselevel;
		std::map<int, std::map<int,variable*> > varmap;
		std::map<int, bool> globals;
		void clearvariable(variable*);
		bool isglobal(int);

};
