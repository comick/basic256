#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>

#include "Error.h"
#include "BasicTypes.h"

// the Types.h and Types.cpp include an object called dataElement that is
// used in stack and variable to store  single element of data and the
// definitions of the various types of values that may be stored

// internally we can use the stringval and floatval but for all
// external output and such - use the getint/getfloat/getstring methods

class DataElement
{
	public:
		int type;	// type from BasicTypes.h
		QString stringval;
		double floatval; 
		int intval;

		DataElement();
		DataElement(DataElement *);
		DataElement(QString);
		DataElement(double);
		DataElement(int);
		
		QString debug();
		
};
