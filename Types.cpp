#include "Types.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
		DataElement(T_UNUSED, 0, QString());
}

DataElement::DataElement(DataElement *source) {
	// create a new DataElement from parts
	type = source->type;
    floatval = source->floatval;
    stringval = source->stringval;
}

DataElement::DataElement(b_type typein, double floatvalin, QString stringvalin) {
	// copy a DataElement to another
	type = typein;
    floatval = floatvalin;
    stringval = stringvalin;
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

