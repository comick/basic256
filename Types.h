#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>

#include "Error.h"

// the Types.h and Types.cpp include an object called dataElement that is
// used in stack and variable to store  single element of data and the
// definitions of the various types of values that may be stored

// internally we can use the stringval and floatval but for all
// external output and such - use the getint/getfloat/getstring methods

// stack and variable type T_VARREF used to pass a variable reference to a subroutine or function (BYREF passing)
enum b_type {T_UNUSED, T_FLOAT, T_STRING, T_ARRAY, T_VARREF};

class DataElement
{
	public:
		b_type type;
		QString stringval;
		double floatval; 

		DataElement();
		DataElement(DataElement *);
		DataElement(QString);
		DataElement(double);
		DataElement(int);
		
		QString debug();
		
};
