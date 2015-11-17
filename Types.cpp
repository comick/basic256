#include "Types.h"
#include "Settings.h"

#include <string>

DataElement::DataElement() {
		DataElement(T_UNUSED, 0, NULL);
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
        if(type==T_STRARRAY) return("strarray=" + QString::number(floatval) + " ");
        if(type==T_UNUSED) return("unused ");
        if(type==T_VARREF) return("varref=" + QString::number(floatval) + " ");
        if(type==T_VARREFSTR) return("varrefstr=" + QString::number(floatval) + " ");
        return("badtype ");
}

int DataElement::getint() {
	return getint(NULL);
}

int DataElement::getint(bool *ok) {
    int i=0;
    if (ok) *ok = true;
    if (type == T_FLOAT || type == T_VARREF || type == T_VARREFSTR) {
        i = (int) (floatval + (floatval>0?BASIC256EPSILON:-BASIC256EPSILON));
    } else if (type == T_STRING) {
        i = stringval.toInt(&*ok);
    } else if (type==T_UNUSED) {
		if (ok) *ok = false;
    }
    return i;
}

double DataElement::getfloat() {
		return getfloat(NULL);
}

double DataElement::getfloat(bool *ok) {
    double f=0;
    if (ok) *ok = true;
    if (type == T_FLOAT || type == T_VARREF || type == T_VARREFSTR) {
        f = floatval;
    } else if (type == T_STRING) {
        f = stringval.toDouble(&*ok);
    } else if (type==T_UNUSED) {
		if (ok) *ok = false;
	}
    return f;
}

QString DataElement::getstring() {
	return getstring(NULL, SETTINGSDECDIGSDEFAULT);
}

QString DataElement::getstring(bool *ok) {
	return getstring(&*ok, SETTINGSDECDIGSDEFAULT);
}

QString DataElement::getstring(bool *ok, int decimaldigits) {
    QString s;
	if (ok) *ok=true;
    if (type == T_STRING) {
        s = stringval;
    } else if (type == T_VARREF || type == T_VARREFSTR) {
        s = QString::number(floatval,'f',0);
    } else if (type == T_FLOAT) {
        double xp = log10(floatval*(floatval<0?-1:1)); // size in powers of 10
        if (xp*2<-decimaldigits || xp>decimaldigits) {
            s = QString::number(floatval,'g',decimaldigits);
        } else {
            s = QString::number(floatval,'f',decimaldigits - (xp>0?xp:0));
            // strip trailing zeros and decimal point
            // need to test for locales with a comma as a currency seperator
            if (s.contains('.',Qt::CaseInsensitive)) {
				while(s.endsWith("0")) s.chop(1);
				if(s.endsWith(".")) s.chop(1);
			}
        }
    } else if (type==T_UNUSED) {
		s = "unused";
		if (ok) *ok = false;
    }
    return s;
}
