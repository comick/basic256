#include "Convert.h"

#include <string>



Convert::Convert(Error *e, QLocale *applocale) {
	error = e;
	SETTINGS;
	decimaldigits = settings.value(SETTINGSDECDIGS, SETTINGSDECDIGSDEFAULT).toInt();
	floattail = settings.value(SETTINGSFLOATTAIL, SETTINGSFLOATTAILDEFAULT).toBool();
    replaceDecimalPoint = settings.value(SETTINGSFLOATLOCALE, SETTINGSFLOATLOCALEDEFAULT).toBool();

	// build international safe regular expression for numbers
	locale = applocale;
    replaceDecimalPoint = replaceDecimalPoint && locale->decimalPoint()!='.'; //use locale decimal point only if !="."
    decimalPoint = (replaceDecimalPoint?locale->decimalPoint():'.');
    isnumeric = new QRegExp(QString("^[-+]?[0-9]*") + decimalPoint + QString("?[0-9]+([eE][-+]?[0-9]+)?$"));
}

Convert::~Convert() {
	delete(isnumeric);
}

		
bool Convert::isNumeric(DataElement *e) {
	// return true if this is a number or can convert to one
	// if there is white space around the string number IT IS NOT NUMERIC
	if (e) {
		if (e->type == T_INT || e->type == T_REF || e->type == T_FLOAT) {
			return true;
		} else if (e->type == T_STRING) {
			if (e->stringval.length()!=0) {
				return isnumeric->indexIn(e->stringval) != -1;
			}
		}
	}
	return false;
}


int Convert::getBool(DataElement *e) {
	// BASIC56 uses 1 for true and 0 for false
	// a number not zero is 1 otherwise 0
	// a non empty string is 1 otherwise 0
	// an unassigned variable is 0
	// an array throws an error and is 0
	if (e) {
		if (e->type == T_INT || e->type == T_REF) {
			return ((e->intval!=0)?1:0);
		} else if (e->type == T_FLOAT) {
			return ((e->floatval < -EPSILON || e->floatval > EPSILON)?1:0);
		} else if (e->type == T_STRING) {
			return ((e->stringval.length()!=0)?1:0);
		} else if (e->type==T_UNASSIGNED) {
			return 0;
		} else if (e->type==T_ARRAY) {
			if (error) error->q(ERROR_ARRAYINDEXMISSING,e->intval);
		}
	}
	return 0;
}
		
int Convert::getInt(DataElement *e) {
	long l=getLong(e);
	if (l<INT_MIN||l>INT_MAX) {
		if (error) error->q(ERROR_INTEGERRANGE);
		l = 0;
	}
	return (int) l;
}

long Convert::getLong(DataElement *e) {
	long i=0;
	if (e) {
		if (e->type == T_INT || e->type == T_REF) {
			i = e->intval;
		} else if (e->type == T_FLOAT) {
			double f = e->floatval + (e->floatval>0?EPSILON:-EPSILON);
			if (f<LONG_MIN||f>LONG_MAX) {
				if (error) error->q(ERROR_LONGRANGE);
			} else {
				i = (long) f;
			}
		} else if (e->type == T_STRING) {
			if (e->stringval.length()!=0) {
				bool ok;
				i = e->stringval.toLong(&ok);
				if(!ok) {
					if (error) error->q(ERROR_TYPECONV);
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
		if (e->type == T_FLOAT) {
			f = e->floatval;
		} else if (e->type == T_INT || e->type == T_REF) {
			f = e->intval;
		} else if (e->type == T_STRING) {
			if (e->stringval.length()!=0) {
				bool ok;
                if(replaceDecimalPoint){
                    f = locale->toDouble(e->stringval, &ok);
                }else{
                    f = e->stringval.toDouble(&ok);
                }
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
            double xp = log10(e->floatval*(e->floatval<0?-1:1)+(e->floatval>9.0?1.0:0)); // size in powers of 10
            //check if adding of ".0" will exceed the number of digits to print numbers
            if(((int)xp)==ddigits-1 && floattail){
                s.setNum(e->floatval,'e',ddigits - (xp>0?xp:1));
                s.replace(QRegExp("0+e"), "e");
                s.replace(".e", ".0e");
                if(replaceDecimalPoint){
                    s.replace('.', decimalPoint);
                }
            }else if (xp*2<-ddigits || xp>ddigits) {
                s.setNum(e->floatval,'g',ddigits);
                if(replaceDecimalPoint){
                    s.replace('.', decimalPoint);
                }
            } else {
                s.setNum(e->floatval,'f',ddigits - (xp>0?xp:1));
                if (s.contains('.',Qt::CaseInsensitive)) {
                    while(s.endsWith("0")) s.chop(1);
                    if(s.endsWith('.')){
                        s.chop(1);
                        if(floattail)
                            s.append(decimalPoint + '0');
                    }else if(replaceDecimalPoint){
                        s.replace('.', decimalPoint);
                    }
                }else if(floattail){
                        s.append(decimalPoint + '0');
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
