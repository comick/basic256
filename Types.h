#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>

#include "ErrorCodes.h"

// the Types.h and Types.cpp include an object called dataElement that is
// used in stack and variable to store  single element of data and the
// definitions of the various types of values that may be stored

// internally we can use the stringval and floatval but for all
// external output and such - use the getint/getfloat/getstring methods

#define BASIC256EPSILON 0.00000001

// stack and variable types T_VARREF, T_VARREFSTR are to pass a variable reference to a subroutine or function (BYREF passing)
enum b_type {T_UNUSED, T_FLOAT, T_STRING, T_ARRAY, T_STRARRAY, T_VARREF, T_VARREFSTR};

class DataElement
{
	public:
		b_type type;
		QString stringval;
		double floatval; 

		DataElement();
		DataElement(DataElement *);
		DataElement(b_type , double , QString);
		
		int getint();
		int getint(bool*);
		double getfloat();
		double getfloat(bool*);
		QString getstring();
		QString getstring(bool*);
		QString getstring(bool*, int);
		QString debug();
};
