#include "DataElement.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
	// create an empty dataelement
	type = T_UNASSIGNED;
	arr = NULL;
}

DataElement::DataElement(QString s) {
	type = T_STRING;
	stringval = s;
	arr = NULL;
}

DataElement::DataElement(int i) {
    type = T_INT;
    intval = i;
	arr = NULL;
}

DataElement::DataElement(long l) {
	type = T_INT;
	intval = l;
	arr = NULL;
}

DataElement::DataElement(double d) {
	type = T_FLOAT;
	floatval = d;
	arr = NULL;
}

DataElement::~DataElement() {
	stringval.clear();
	if (arr){
		delete(arr);
		arr=NULL;
	}
}




void DataElement::copy(DataElement *source) {
    copy(source,-1);
}

void DataElement::copy(DataElement *source, int varnum) {
	// fill an existing from as a copy of another and set the variable number (used to copy array elements)
	// it is as fast as it can be
	// unused values of strings can be found as garbage but this is the cost of speed
	// source->stringval.clear() is too expensive to be used for each assignment/copy
	type = source->type;
	switch (source->type){
		case T_UNASSIGNED:
			intval = (long) varnum; //variable number for error output
			break;
		case T_FLOAT:
			floatval = source->floatval;
			break;
		case T_STRING:
			stringval = source->stringval;
			break;
		case T_ARRAY:
			{
				// copy array elements from one old array to a new one
				type = T_ARRAY;
				if (!arr)
					arr = new DataElementArray;
				int i = source->arr->xdim * source->arr->ydim;
				arr->datavector.resize(i);
				while(i-- > 0) {
					arr->datavector[i].copy(&source->arr->datavector[i]);
				}
			}
			break;
		case T_REF:
			intval = source->intval;
			level = source->level;
			break;
		case T_INT:
			intval = source->intval;
	}
}

void DataElement::clear() {
	type = T_UNASSIGNED;
	intval = 0;
	stringval.clear();
	level = 0;
	if (arr){
		delete(arr);
		arr=NULL;
	}
}


QString DataElement::debug() {
	// return a string representing the DataElement contents
	if(type==T_INT) return("int(" + QString::number(intval) + ") ");
	if(type==T_REF) return("varref(" + QString::number(intval) + ", level: " + QString::number(level) + ") ");
	if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
	if(type==T_STRING) return("string(" + stringval + ") ");
	if(type==T_ARRAY) return("array(" + QString::number(intval) + ")");
	if(type==T_UNASSIGNED) return("unused ");
	return("badtype ");
}

void DataElement::arraydim(const int xdim, const int ydim, const bool redim) {
	const int size = xdim * ydim;
	
	if (size <= MAXARRAYSIZE) {
		if (size >= 1) {
			if (type != T_ARRAY || !redim) {
				// if array data is dim or redim without a dim then create a new one (clear the old)
				if (!arr){
					arr = new DataElementArray;
				}else{
					arr->datavector.clear();
				}
				arr->datavector.resize(size);
				//int i = size;
				//while(i-- > 0) {
				//	arr->datavector[i].intval = 0;
				//}
				type = T_ARRAY;
			}else{
				// redim - resize the vector
				//const int oldsize = arr->xdim * arr->ydim;
				arr->datavector.resize(size);
				//for(int i=oldsize;i<size;i++) {
				//	arr->datavector[i].intval = 0;
				//}
			}

			arr->xdim = xdim;
			arr->ydim = ydim;
		} else {
			error->q(ERROR_ARRAYSIZESMALL, -1);
		}
	} else {
		error->q(ERROR_ARRAYSIZELARGE, -1);
	}
}

int DataElement::arraysize() {
	if (type == T_ARRAY) {
		return(arr->xdim * arr->ydim);
	} else if (type==T_UNASSIGNED){
		error->q(ERROR_VARNOTASSIGNED, -1);
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
	return(0);
}

int DataElement::arraysizerows() {
	// return number of columns of array as if it was a two dimensional array - 0 = not an array
	if (type == T_ARRAY) {
		return(arr->xdim);
	} else if (type==T_UNASSIGNED){
		error->q(ERROR_VARNOTASSIGNED, -1);
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
	return(0);
}

int DataElement::arraysizecols() {
	// return number of rows of array as if it was a two dimensional array - 0 = not an array
	if (type == T_ARRAY) {
		return(arr->ydim);
	} else if (type==T_UNASSIGNED){
		error->q(ERROR_VARNOTASSIGNED, -1);
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
	return(0);
}

DataElement* DataElement::arraygetdata(const int x, const int y) {
	return arraygetdata(x, y, true);
}

DataElement* DataElement::arraygetdata(const int x, const int y, const bool unassignederror) {
	// get data from array elements from map (using x, y)
	// if there is an error return an unassigned value
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			DataElement *e = &arr->datavector[i];
			//the correct behaviour is to check for unassigned content
			//when program try to use it not when it try to convert to int, string and so...
			if (unassignederror && type==T_UNASSIGNED){
				error->q(ERROR_ARRAYELEMENT, -1);
			}
			return e;
		} else {
			error->q(ERROR_ARRAYINDEX, -1);
		}
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
	return this;
}

void DataElement::arraysetdata(const int x, const int y, DataElement *e) {
	// DataElement's data is copied and it should be deleted
	// by whomever created it
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			arr->datavector[i].copy(e);
		} else {
			error->q(ERROR_ARRAYINDEX, -1);
		}
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
}

void DataElement::arrayunassign(const int x, const int y) {
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			arr->datavector[i].type = T_UNASSIGNED;
		} else {
			error->q(ERROR_ARRAYINDEX, -1);
		}
	} else {
		error->q(ERROR_NOTARRAY, -1);
	}
}
