#include "Convert.h"


#include <string>

Convert::Convert(Error *e) {
	error = e;
	typeconverror = SETTINGSTYPECONVDEFAULT;
	decimaldigits = SETTINGSDECDIGSDEFAULT;
}


void Convert::settypeconverror(int e) {
// TypeConvError	0- report no errors
// 					1 - report problems as warning
// 					2 - report problems as an error
	typeconverror = e;
}

int Convert::ec(int errorcode, int warncode) {
	if (typeconverror==0) return ERROR_NONE;
	if (typeconverror==1) return warncode;
	return errorcode;
}

void Convert::setdecimaldigits(int e) {
// DecimalDigits	when converting a double to a string how many decinal
//					digits will we display default 12 maximum should not
//					exceed 15, 14 to be safe
	decimaldigits = e;
}


int Convert::getInt(DataElement *e) {
	int i=0;
	if (e->type == T_FLOAT || e->type == T_VARREF) {
		i = (int) (e->floatval + (e->floatval>0?EPSILON:-EPSILON));
	} else if (e->type == T_STRING) {
		if (e->stringval.length()!=0) {
			bool ok;
			i = e->stringval.toInt(&ok);
			if(!ok) {
				error->q(ec(ERROR_TYPECONV, WARNING_TYPECONV));
			}
		}
	} else if (e->type==T_ARRAY) {
		error->q(ERROR_ARRAYINDEXMISSING);
	} else if (e->type==T_UNUSED) {
		error->q(ec(ERROR_VARNOTASSIGNED, WARNING_VARNOTASSIGNED));
	}
	return i;
}

double Convert::getFloat(DataElement *e) {
    double f=0;
	if (e->type == T_FLOAT || e->type == T_VARREF) {
		f = e->floatval;
	} else if (e->type == T_STRING) {
		if (e->stringval.length()!=0) {
			bool ok;
			f = e->stringval.toDouble(&ok);
			if(!ok) {
				error->q(ec(ERROR_TYPECONV, WARNING_TYPECONV));
			}
		}
	} else if (e->type==T_ARRAY) {
		error->q(ERROR_ARRAYINDEXMISSING);
	} else if (e->type==T_UNUSED) {
		error->q(ec(ERROR_VARNOTASSIGNED, WARNING_VARNOTASSIGNED));
	}
	return f;
}

QString Convert::getString(DataElement *e) {
	return getString(e, decimaldigits);
}

QString Convert::getString(DataElement *e, int ddigits) {
    QString s = "";
    if (e->type == T_STRING) {
        s = e->stringval;
    } else if (e->type == T_VARREF) {
        s = QString::number(e->floatval,'f',0);
    } else if (e->type == T_FLOAT) {
        double xp = log10(e->floatval*(e->floatval<0?-1:1)); // size in powers of 10
        if (xp*2<-ddigits || xp>ddigits) {
            s = QString::number(e->floatval,'g',ddigits);
        } else {
            s = QString::number(e->floatval,'f',ddigits - (xp>0?xp:0));
            // strip trailing zeros and decimal point
            // need to test for locales with a comma as a currency seperator
            if (s.contains('.',Qt::CaseInsensitive)) {
				while(s.endsWith("0")) s.chop(1);
				if(s.endsWith(".")) s.chop(1);
			}
		}
	} else if (e->type==T_ARRAY) {
		error->q(ERROR_ARRAYINDEXMISSING);
	} else if (e->type==T_UNUSED) {
		error->q(ec(ERROR_VARNOTASSIGNED, WARNING_VARNOTASSIGNED));
	}
    return s;
}


int Convert::compareFloats(double one, double two) {
    // return 1 if one>two  0 if one==two or -1 if one<two
    // USE FOR ALL COMPARISON WITH NUMBERS
    // used a small number (epsilon) to make sure that
    // decimal precission errors are ignored
    if (fabs(one - two)<=EPSILON) return 0;
    else if (one < two) return -1;
    else return 1;
}

int Convert::compare(DataElement *one, DataElement* two) {
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	if (one->type == T_STRING && two->type == T_STRING) {
		// if both are strings - [compare them as strings] strcmp
		QString stwo = getString(two);
		QString sone = getString(one);
		int ans = sone.compare(stwo);
		if (ans==0) return 0;
		else if (ans<0) return -1;
		else return 1;
	} else {
		// if either a number then compare as numbers
		double ftwo = getFloat(two);
		double fone = getFloat(one);
		return compareFloats(fone, ftwo);
	}
}
