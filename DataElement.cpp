#include "DataElement.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
	// create an empty dataelement
	type = T_UNASSIGNED;
}



DataElement::DataElement(QString s) {
	type = T_STRING;
	stringval = s;
}

DataElement::DataElement(int i) {
    type = T_INT;
    intval = i;
}

DataElement::DataElement(long l) {
	type = T_INT;
	intval = l;
}

DataElement::DataElement(double d) {
	type = T_FLOAT;
	floatval = d;
}

void DataElement::copy(DataElement *source) {
    // fill an existing from as a copy of another
    // it is as fast as it can be
    // unused values of strings can be found as garbage but this is the cost of speed
    // source->stringval.clear() is too expensive to be used for each assignment/copy
    type = source->type;
    switch (type){
        case T_FLOAT:
            floatval = source->floatval;
            break;
        case T_STRING:
            stringval = source->stringval;
            break;
        case T_ARRAY: //need to copy level and variable number when push an array to stack to access originals elements when pop it (for subroutines and functions)
        case T_REF:
            level = source->level;
        default: //T_INT or T_UNASSIGNED (variable number)
            intval = source->intval;
    }
}

void DataElement::copy(DataElement *source, int varnum) {
    // fill an existing from as a copy of another and set the variable number (used to copy array elements)
    // it is as fast as it can be
    // unused values of strings can be found as garbage but this is the cost of speed
    // source->stringval.clear() is too expensive to be used for each assignment/copy
    type = source->type;
    switch (type){
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
        case T_REF:
            level = source->level;
        case T_INT:
            intval = source->intval;
    }
}

QString DataElement::debug() {
    // return a string representing the DataElement contents
        if(type==T_INT) return("int(" + QString::number(intval) + ") ");
        if(type==T_REF) return("varref(" + QString::number(intval) + ", level: " + QString::number(level) + ") ");
        if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
        if(type==T_STRING) return("string(" + stringval + ") ");
        if(type==T_ARRAY) return("array=" + QString::number(floatval) + " ");
        if(type==T_UNASSIGNED) return("unused ");
        return("badtype ");
}

