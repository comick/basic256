#include "Convert.h"


#include <string>


Convert::Convert(Error *e) {
	error = e;
	SETTINGS;
	decimaldigits = settings.value(SETTINGSDECDIGS, SETTINGSDECDIGSDEFAULT).toInt();
}
		
		
int Convert::getInt(DataElement *e) {
	return (int) getLong(e);
}

long Convert::getLong(DataElement *e) {
	long i=0;
	if (e) {
		if (e->type == T_INT || e->type == T_REF) {
			i = e->intval;
		} else if (e->type == T_FLOAT) {
			i = (long) (e->floatval + (e->floatval>0?EPSILON:-EPSILON));
		} else if (e->type == T_STRING) {
			if (e->stringval.length()!=0) {
				bool ok;
				i = e->stringval.toLong(&ok);
				if(!ok) {
					if (error) if (error) error->q(ERROR_TYPECONV);
				}
			}
		} else if (e->type==T_ARRAY) {
			if (error) error->q(ERROR_ARRAYINDEXMISSING,e->intval);
		} else if (e->type==T_UNASSIGNED) {
			if (error) error->q(ERROR_VARNOTASSIGNED, e->intval);
		}
	}
	return i;
}

double Convert::getFloat(DataElement *e) {
    double f=0;
	if (e) {
		if (e->type == T_INT || e->type == T_REF) {
			f = e->intval;
		} else if (e->type == T_FLOAT) {
			f = e->floatval;
		} else if (e->type == T_STRING) {
			if (e->stringval.length()!=0) {
				bool ok;
				f = e->stringval.toDouble(&ok);
				if(!ok) {
					if (error) error->q(ERROR_TYPECONV);
				}
			}
		} else if (e->type==T_ARRAY) {
			if (error) error->q(ERROR_ARRAYINDEXMISSING,e->intval);
		} else if (e->type==T_UNASSIGNED) {
			if (error) error->q(ERROR_VARNOTASSIGNED,e->intval);
		}
	}
	return f;
}

QString Convert::getString(DataElement *e) {
	return getString(e, decimaldigits);
}

QString Convert::getString(DataElement *e, int ddigits) {
    QString s = "";
    if (e) {
		if (e->type == T_STRING) {
			s = e->stringval;
		} else if (e->type == T_INT || e->type == T_REF) {
			s = QString::number(e->intval);
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
			if (error) error->q(ERROR_ARRAYINDEXMISSING,e->intval);
		} else if (e->type==T_UNASSIGNED) {
			if (error) error->q(ERROR_VARNOTASSIGNED, e->intval);
		}
	}
    return s;
}


int Convert::compareFloats(double fone, double ftwo) {
	// return 1 if one>two  0 if one==two or -1 if one<two
	// USE FOR ALL COMPARISON WITH FLOATS
	// used a small number (epsilon) to make sure that
	// decimal precission errors are ignored
	if (fabs(fone - ftwo)<=EPSILON) return 0;
	else if (fone < ftwo) return -1;
	else return 1;
}

int Convert::compare(DataElement *one, DataElement* two) {
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	if (one&&two) {
		if (one->type == T_STRING && two->type == T_STRING) {
			// if both are strings - [compare them as strings] strcmp
			int ans = one->stringval.compare(two->stringval);
			if (ans==0) return 0;
			else if (ans<0) return -1;
			else return 1;
		} else if (one->type == T_INT && two->type == T_INT) {
			// if both are integers comapre as integers
			long ans = one->intval - two->intval;
			if (ans==0) return 0;
			else if (ans<0) return -1;
			else return 1;
		} else {
			// if either a float number then compare as floats
			double ftwo = getFloat(two);
			double fone = getFloat(one);
			return compareFloats(fone, ftwo);
		}
	} else {
		return 0;
	}
}
