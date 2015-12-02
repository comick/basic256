#include "Types.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
	// create an empty dataelement
	type = T_UNUSED;
    floatval = 0;
    stringval = "";
}

DataElement::DataElement(DataElement *source) {
	// create a new DataElement from as a copy of another
	type = source->type;
    floatval = source->floatval;
    stringval = source->stringval;
}

DataElement::DataElement(QString s) {
	type = T_STRING;
	floatval = 0;
	stringval = s;
}

DataElement::DataElement(int i) {
	type = T_FLOAT;
	floatval = i;
}

DataElement::DataElement(double d) {
	type = T_FLOAT;
	floatval = d;
}

QString DataElement::debug() {
    // return a string representing the DataElement contents
        if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
        if(type==T_STRING) return("string(" + stringval + ") ");
        if(type==T_ARRAY) return("array=" + QString::number(floatval) + " ");
        if(type==T_UNUSED) return("unused ");
        if(type==T_VARREF) return("varref=" + QString::number(floatval) + " ");
        return("badtype ");
}

