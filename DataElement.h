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

// the DataElement.h and DataElements.cpp define a class that is
// used in stack and variable to store  single element of data and the
// definitions of the various types of values that may be stored

// we will pass a pointer to a DataElement to the functions of the
// Convert class for all output

class DataElement
{
	public:
		int type;	// type from BasicTypes.h
		QString stringval;
		double floatval;
		long intval;
        int level;

		DataElement();
		
		DataElement(DataElement *);
		DataElement(QString);
		DataElement(double);
		DataElement(long);
		DataElement(int);

		QString debug();
		void copy(DataElement *);
};
