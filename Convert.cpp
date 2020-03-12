#include "Convert.h"
#include "Error.h"

#include <string>



Convert::Convert(QLocale *applocale) {
    SETTINGS;
    decimaldigits = settings.value(SETTINGSDECDIGS, SETTINGSDECDIGSDEFAULT).toInt();
    if(decimaldigits < SETTINGSDECDIGSMIN || decimaldigits > SETTINGSDECDIGSMAX)
        decimaldigits = SETTINGSDECDIGSDEFAULT;
    floattail = settings.value(SETTINGSFLOATTAIL, SETTINGSFLOATTAILDEFAULT).toBool();
    replaceDecimalPoint = settings.value(SETTINGSFLOATLOCALE, SETTINGSFLOATLOCALEDEFAULT).toBool();

    // build international safe regular expression for numbers
    locale = applocale;
    replaceDecimalPoint = replaceDecimalPoint && locale->decimalPoint()!='.'; //use locale decimal point only if !="."
    decimalPoint = (replaceDecimalPoint?locale->decimalPoint():'.');
    isnumericexpression = QString("^[-+]?[0-9]*") + decimalPoint + QString("?[0-9]+([eE][-+]?[0-9]+)?$");
    musicalnote.setPattern("^(do|re|mi|fa|sol|la|si|c|d|e|f|g|a|b|h|ni|pa|vu|ga|di|ke|zo)([-]?[0-9]+)?(#{1,2}|b{1,2})?$");
    musicalnote.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

Convert::~Convert() {
}


bool Convert::isNumeric(DataElement *e) {
    // return true if this is a number or can convert to one
    // if there is white space around the string number IT IS NOT NUMERIC
    if (e) {
        if (e->type == T_INT || e->type == T_FLOAT) {
            return true;
        } else if (e->type == T_STRING) {
            if (e->stringval.length()!=0) {
                // using isnumeric as private (like musicalnote) increase time for all functions
                // eg. getBool() increased time elapsed from 10200ms to 12500ms
                // Benchmark: convert->getBool() for 2.700.000.000 times
                // ...I have no explanation for this...
                QRegularExpression isnumeric(isnumericexpression);
                QRegularExpressionMatch match = isnumeric.match(e->stringval);
                return match.hasMatch();
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
        if (e->type == T_INT) {
            return (e->intval!=0l);
        } else if (e->type == T_FLOAT) {
            return ((e->floatval < -EPSILON || e->floatval > EPSILON)?1:0);
        } else if (e->type == T_STRING) {
            return ((e->stringval.length()!=0)?1:0);
        } else if (e->type==T_UNASSIGNED) {
            error->q(ERROR_VARNOTASSIGNED,e->intval);
        } else if (e->type==T_ARRAY) {
            error->q(ERROR_ARRAYINDEXMISSING,e->intval);
        }
    }
    return 0;
}

int Convert::getInt(DataElement *e) {
    long l=getLong(e);
    if (l<INT_MIN||l>INT_MAX) {
        error->q(ERROR_INTEGERRANGE);
        l = 0;
    }
    return (int) l;
}

long Convert::getLong(DataElement *e) {
    long i=0;
    if (e) {
        if (e->type == T_INT) {
            i = e->intval;
        } else if (e->type == T_FLOAT) {
            double f = e->floatval + (e->floatval>0.0?EPSILON:-EPSILON);
            if (f<LONG_MIN||f>LONG_MAX) {
                error->q(ERROR_LONGRANGE);
            } else {
                i = (long) f;
            }
        } else if (e->type == T_STRING) {
            if (e->stringval.length()!=0) {
                bool ok;
                i = e->stringval.toLong(&ok);
                if(!ok) {
                    i = 0;
                    error->q(ERROR_TYPECONV);
                }
            } else {
                error->q(ERROR_TYPECONV);
            }
        } else if (e->type==T_ARRAY) {
            error->q(ERROR_ARRAYINDEXMISSING,e->intval);
        } else if (e->type==T_UNASSIGNED) {
            error->q(ERROR_VARNOTASSIGNED, e->intval);
        }
    }
    return i;
}

double Convert::getFloat(DataElement *e) {
    double f = 0.0;
    if (e) {
        if (e->type == T_FLOAT) {
            return(e->floatval);
        } else if (e->type == T_INT) {
            return(e->intval);
        } else if (e->type == T_STRING) {
            if (e->stringval.length()!=0) {
                bool ok;
                if(replaceDecimalPoint){
                    f = locale->toDouble(e->stringval, &ok);
                }else{
                    f = e->stringval.toDouble(&ok);
                }
                if(!ok) {
                    f = 0.0;
                    error->q(ERROR_TYPECONV);
                }
            }else {
                error->q(ERROR_TYPECONV);
            }
        } else if (e->type==T_ARRAY) {
            error->q(ERROR_ARRAYINDEXMISSING,e->intval);
        } else if (e->type==T_UNASSIGNED) {
            error->q(ERROR_VARNOTASSIGNED,e->intval);
        }
    }
    return f;
}

double Convert::getMusicalNote(DataElement *e) {
    double f = 0.0;
    if (e) {
        if (e->type == T_FLOAT) {
            f = e->floatval;
        } else if (e->type == T_INT) {
            f = e->intval;
        }else if (e->type == T_STRING) {
            QRegularExpressionMatch match = musicalnote.match(e->stringval);
            if (match.hasMatch()) {
                QString note, octave, accidentals;
                int oct = 4, acc = 0, n;
                double s;
                note = match.captured(1).toLower();
                n = notesmap[note];
                octave = match.captured(2);
                if(octave!="") oct = octave.toInt();
                accidentals = match.captured(3).toLower();
                if(accidentals=="#") acc = 1;
                if(accidentals=="##") acc = 2;
                if(accidentals=="b") acc = -1;
                if(accidentals=="bb") acc = -2;

                s=oct*12+n+acc-57; //if s==0 then s==440Hz
                f=pow(2.0,s/12.0)*440;
            }else{
                if (e->stringval.length()!=0) {
                    bool ok;
                    if(replaceDecimalPoint){
                        f = locale->toDouble(e->stringval, &ok);
                    }else{
                        f = e->stringval.toDouble(&ok);
                    }
                    if(!ok) {
                        f = 0.0;
                        error->q(ERROR_STRING2NOTE);
                    }
                }else {
                    error->q(ERROR_STRING2NOTE);
                }
            }
            } else if (e->type==T_UNASSIGNED) {
                error->q(ERROR_VARNOTASSIGNED,e->intval);
            } else if (e->type==T_ARRAY) {
                error->q(ERROR_ARRAYINDEXMISSING,e->intval);
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
        } else if (e->type == T_INT) {
            s = QString::number(e->intval);
        } else if (e->type == T_FLOAT) {
            if(e->floatval >= -EPSILON && e->floatval <= EPSILON){
                //this is almost 0.0
                //If you add 0.1/-0.1 you will never get a perfect 0.0 again
                if(floattail)
                    s = QStringLiteral("0") + decimalPoint + QStringLiteral("0");
                else
                    s = QStringLiteral("0");
            }else{
                double xp = log10(e->floatval*(e->floatval<0.0?-1.0:1.0)); // size in powers of 10
                //check if adding of ".0" will exceed the number of digits to print numbers
                if(((int)xp)==ddigits-1 && floattail){
                    s.setNum(e->floatval,'e',ddigits - 1);
                    s.replace(QRegExp(QStringLiteral("0+e")), QStringLiteral("e"));
                    s.replace(QStringLiteral(".e"), QStringLiteral(".0e"));
                    if(replaceDecimalPoint){
                        s.replace('.', decimalPoint);
                    }
                }else if (xp*2<-ddigits || (int)xp>=(ddigits-(floattail?1:0))) { //number is too small or too big to show in original form
                    s.setNum(e->floatval,'g',ddigits);
                    if(replaceDecimalPoint){
                        s.replace('.', decimalPoint);
                    }
                } else {
                    int d = ddigits - (xp>0.0?(int)xp+1:1);
                    //double precision sucks
                    //if we need to print more than 7 decimals there are chances to print numbers as 2.300000001 instead of 2.3
                    if(d>7){
                        s.setNum(e->floatval,'f',7);
                        if(!s.endsWith(QStringLiteral("0"))) // we cannot reduce it
                            s.setNum(e->floatval,'f',d);
                    }else{
                        s.setNum(e->floatval,'f',d);
                    }
                    if (s.contains('.',Qt::CaseInsensitive)) {
                        while(s.endsWith(QStringLiteral("0"))) s.chop(1);
                        if(s.endsWith(QStringLiteral("."))){
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
            }
        } else if (e->type==T_ARRAY) {
            error->q(ERROR_ARRAYINDEXMISSING,e->intval);
        } else if (e->type==T_UNASSIGNED) {
           error->q(ERROR_VARNOTASSIGNED, e->intval);
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
        if (one->type == T_INT && two->type == T_INT) {
            // if both are integers comapre as integers
            long ans = one->intval - two->intval;
            if (ans==0) return 0;
            else if (ans<0) return -1;
            else return 1;
        } else if (one->type == T_STRING && two->type == T_STRING) {
            // if both are strings - [compare them as strings] strcmp
            int ans = one->stringval.compare(two->stringval);
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
