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
	floatval = source->floatval;
	intval = source->intval;
	stringval = source->stringval;
}

DataElement::DataElement(QString s) {
	type = T_STRING;
	stringval = s;
}

DataElement::DataElement(int i) {
	DataElement((long) i);
}

DataElement::DataElement(long l) {
	type = T_INT;
	intval = l;
}

DataElement::DataElement(double d) {
	type = T_FLOAT;
	floatval = d;
}

QString DataElement::debug() {
    // return a string representing the DataElement contents
        if(type==T_INT) return("int(" + QString::number(intval) + ") ");
        if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
        if(type==T_STRING) return("string(" + stringval + ") ");
        if(type==T_ARRAY) return("array=" + QString::number(floatval) + " ");
        if(type==T_UNASSIGNED) return("unused ");
        if(type==T_REF) return("varref=" + QString::number(floatval) + " ");
        return("badtype ");
}

