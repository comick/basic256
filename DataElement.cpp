#include "DataElement.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
	// create an empty dataelement
	type = T_UNASSIGNED;
}

DataElement::DataElement(DataElement *source) {
	// create a new DataElement from as a copy of another

    type = source->type;
    switch (type){
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
    type = source->type;
    switch (type){
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

