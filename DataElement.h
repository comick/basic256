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

#define MAXARRAYSIZE 1048576
class DataElement;

class DataElementArray
{
    public:
        int xdim;
        int ydim;
        std::vector<DataElement> datavector;
};

class DataElement
{
	public:
		int type;	// type from BasicTypes.h
		QString stringval;
		double floatval;
		long intval;
		int level;
        DataElementArray *arr;
        

		DataElement();
		~DataElement();
		
		DataElement(QString);
		DataElement(double);
		DataElement(long);
		DataElement(int);

		QString debug();
		void copy(DataElement *);
		
		void clear();
		
		void arraydim(const int, const int, const bool);
		DataElement* arraygetdata(const int, const int);
		void arraysetdata(const int, const int, DataElement *);
		void arrayunassign(const int, const int);
		int arraysize();
		int arraysizerows();
		int arraysizecols();

		
		static int getError() {
			return getError(false);
		}

		static int getError(int clear) {
			int olde = e;
			if (clear) e = ERROR_NONE;
			return olde;
		}

	private:
		static int e;		// error number thrown - will be 0 if no error
		Error *error;
};



