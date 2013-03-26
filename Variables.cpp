#include <map>
#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to the following types (defined in Stack.h):
// T_UNUSED, T_FLOAT, T_STRING, T_ARRAY, T_STRARRAY, T_VARREF, and T_VARREFSTR

// T_VARREF[STR] is a variable number in the previous recursion level

Variables::Variables()
{
	lasterror = ERROR_NONE;
	lasterrorvar = 0;
	// initialize variable storage
}

Variables::~Variables()
{
	lasterror = ERROR_NONE;
	// free variables and all storage
	clear();
}

int Variables::error() {
	return(lasterror);
}

int Variables::errorvarnum() {
	return(lasterrorvar);
}

void
Variables::clear()
{
	lasterror = ERROR_NONE;
	// erase all variables
	while(!varmap.empty()) {
		std::map<int, std::map<int,variable*> >::iterator i=varmap.begin();
		while(!(*i).second.empty()) {
			std::map<int,variable*>::iterator j=(*i).second.begin();
			clearvariable((*j).second);
			(*i).second.erase((*j).first);
		}
		varmap.erase((*i).first);
	}
	globals.clear();
	recurselevel = 0;
}

void
Variables::increaserecurse()
{
	lasterror = ERROR_NONE;
	recurselevel++;
	if (recurselevel>=MAX_RECURSE_LEVELS) {
		recurselevel--;
		lasterror = ERROR_MAXRECURSE;
		lasterrorvar = 0;
	}
}

int
Variables::getrecurse()
{
	return	recurselevel;
}

void
Variables::decreaserecurse()
{
	lasterror = ERROR_NONE;
	if (recurselevel>0) {
		// erase all variables in the current recurse level before we return
		// to the previous level
		if(varmap.find(recurselevel)!=varmap.end()) {
			while(!varmap[recurselevel].empty()) {
				std::map<int,variable*>::iterator j=varmap[recurselevel].begin();
				clearvariable((*j).second);
				varmap[recurselevel].erase((*j).first);
			}
			varmap.erase(recurselevel);
		}
		// return back to prev variable context
		recurselevel--;
	}
}

void Variables::clearvariable(variable* v)
{
	// free a variable's current value to allow it to be reassigned
	if (v->type == T_ARRAY)	{
		delete(v->arr);
    }
    v->type = T_UNUSED;
}

variable* Variables::getv(int varnum, bool makenew) {
	// get v from map else return NULL
	// if makenew then make a new one if not exist
	// followref - follow variable reference
	variable *v;
	int level=recurselevel;
	if (isglobal(varnum)) {
		level = 0;
	}
       	if (varmap.find(level) != varmap.end() && varmap[level].find(varnum) != varmap[level].end()) {
		v = varmap[level][varnum];
		if (v->type==T_VARREF && recurselevel>0) {
	                recurselevel--;
			v = getv((int) v->floatval, makenew);
        	        recurselevel++;
		}
	} else {
		if(makenew) {
			v = new variable;
			v->type = T_UNUSED;
			v->floatval = 0;
			v->string = QString("");
			v->arr = NULL;
			varmap[level][varnum] = v;
			//printf("lastvar=%i size=%i\n", varnum, varmap.size());
		} else {
			v = NULL;
		}
	}
	return(v);
}

arraydata* Variables::getarraydata(variable *v, int i) {
	// get arraydata from array elements [i] in v from map	arraydata *d;
	arraydata *d;
	if (v->arr->datamap.find(i) != v->arr->datamap.end()) {
		d = v->arr->datamap[i];
	} else {
		d = new arraydata;
		d->floatval = 0;
		d->string = QString("");
		v->arr->datamap[i] = d;
	}
	return(d);
}

int Variables::type(int varnum)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		return(v->type);
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(T_UNUSED);
}


void Variables::setfloat(int varnum, double value)
{
    lasterror = ERROR_NONE;
    variable *v = getv(varnum,true);
    clearvariable(v);
    v->type = T_FLOAT;
    v->floatval = value;
}

void Variables::setvarref(int varnum, int value)
{
    lasterror = ERROR_NONE;
    variable *v = getv(varnum,true);
    clearvariable(v);
    v->type = T_VARREF;
    v->floatval = value;
}

double Variables::getfloat(int varnum)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_FLOAT) {
			return(v->floatval);
		} else {
			lasterror = ERROR_NOSUCHVARIABLE;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

void Variables::setstring(int varnum, QString value)
{
	// pass pointer - copied when put on stack so this is a good pointer
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,true);
	clearvariable(v);
	v->type = T_STRING;
	v->string = value;
}

QString Variables::getstring(int varnum)
{
	// just return pointer - copied when put on stack so this is a good pointer
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_STRING) {
			return(v->string);
		} else {
			lasterror = ERROR_NOSUCHVARIABLE;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return strdup("");
}

void Variables::arraydim(b_type type, int varnum, int xdim, int ydim, bool redim)
{
	variable *v = getv(varnum,true);
	int size = xdim * ydim;

	lasterror = ERROR_NONE;

	if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (size >= 1) {
			v->type = type;
			if (!redim || !v->arr) {
				// if array data is dim or redim without a dim then create a new one (clear the old)
				v->arr = new array;
			}
			v->arr->size = size;
			v->arr->xdim = xdim;
			v->arr->ydim = ydim;
		} else {
			lasterror = ERROR_ARRAYSIZESMALL;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_ARRAYSIZELARGE;
		lasterrorvar = varnum;
	}
}

int Variables::arraysize(int varnum)
{
	// return length of array as if it was a one dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->arr->size);
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

int Variables::arraysizex(int varnum)
{
	// return number of columns of array as if it was a two dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->arr->xdim);
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

int Variables::arraysizey(int varnum)
{
	// return number of rows of array as if it was a two dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->arr->ydim);
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

void Variables::arraysetfloat(int varnum, int index, double value)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(index >= 0 && index < v->arr->size) {
				getarraydata(v,index)->floatval = value;
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
}

void Variables::array2dsetfloat(int varnum, int x, int y, double value)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(x >= 0 && x < v->arr->xdim && y >= 0 && y < v->arr->ydim ) {
				getarraydata(v,x * v->arr->ydim + y)->floatval = value;
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
}

double Variables::arraygetfloat(int varnum, int index)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(index >= 0 && index < v->arr->size) {
				return(getarraydata(v,index)->floatval);
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

double Variables::array2dgetfloat(int varnum, int x, int y)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(x >= 0 && x < v->arr->xdim && y >= 0 && y < v->arr->ydim ) {
				return(getarraydata(v,x * v->arr->ydim + y)->floatval);
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return(0);
}

void Variables::arraysetstring(int varnum, int index, QString value)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(index >= 0 && index < v->arr->size) {
				getarraydata(v,index)->string = value;
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTSTRINGARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
}

void Variables::array2dsetstring(int varnum, int x, int y, QString value)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(x >= 0 && x < v->arr->xdim && y >= 0 && y < v->arr->ydim ) {
				int index = x * v->arr->ydim + y;
				getarraydata(v,index)->string = value;
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTSTRINGARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
}

QString Variables::arraygetstring(int varnum, int index)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(index >= 0 && index < v->arr->size) {
				return(getarraydata(v,index)->string);
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTSTRINGARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return strdup("");
}

QString Variables::array2dgetstring(int varnum, int x, int y)
{
	lasterror = ERROR_NONE;
	variable *v = getv(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(x >= 0 && x < v->arr->xdim && y >= 0 && y < v->arr->ydim ) {
				int index = x * v->arr->ydim + y;
				return(getarraydata(v,index)->string);
			} else {
				lasterror = ERROR_ARRAYINDEX;
				lasterrorvar = varnum;
			}
		} else {
			lasterror = ERROR_NOTSTRINGARRAY;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return strdup("");
}

void Variables::makeglobal(int varnum)
{
	// make a variable global - if there is a value in a run level greater than 0 then
	// that value will be lost to the program
	// globals list are erased when variables are reset
	globals[varnum] = true;
}

bool Variables::isglobal(int varnum)
{
	// return true if the variable is on the list of globals
	if(globals.find(varnum)!=globals.end()) {
		return globals[varnum];
	}
	return false;
}

