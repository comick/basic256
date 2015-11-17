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
#include "Types.h"

#define VARIABLE_MAXARRAYELEMENTS 1048576
#define MAX_RECURSE_LEVELS	1048576

  
class VariableArrayPart
{
	public:
		int xdim;
		int ydim;
		int size;
		std::map<int,DataElement*> datamap;
};

class Variable : public DataElement
{
	public:
		VariableArrayPart *arr;
};

class Variables
{
	public:
		Variables();
		~Variables();
		//
		QString debug();
		void clear();
		void increaserecurse();
		void decreaserecurse();
		int getrecurse();
		//
		int type(int);
		int error();
		int errorvarnum();
		//
		Variable* get(int);
		void set(int, b_type, double, QString);
 		//
		void arraydim(b_type, int, int, int, bool);
		DataElement* arrayget(int, int, int);
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
		std::map<int, std::map<int,Variable*> > varmap;
		std::map<int, bool> globals;
		void clearvariable(Variable*);
		bool isglobal(int);

};
