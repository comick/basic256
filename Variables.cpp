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
	if (v->type == T_UNUSED) {
	} else if (v->type == T_STRING && v->value_string != NULL) {
		v->value_string = NULL;
	}
	else if (v->type == T_ARRAY && v->value_arr != NULL) {
		delete(v->value_arr);
		v->value_arr = NULL;
	}
	else if (v->type == T_STRARRAY && v->value_arr != NULL)	{
		for (int j = 0; j < v->value_arr->size; j++) {
			if (v->value_arr->data.sdata[j]) {
				v->value_arr->data.sdata[j] = NULL;
			}
		}
		delete(v->value_arr);
		v->value_arr = NULL;
	}
    v->type = T_UNUSED;
}

variable* Variables::getvfromnum(int varnum, bool makenew) {
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
			v = getvfromnum((int) v->value_floatval, makenew);
        	        recurselevel++;
		}
	} else {
		if(makenew) {
			v = new variable;
			v->type = T_UNUSED;
			v->value_floatval = 0;
			v->value_string = NULL;
			v->value_arr = NULL;
			varmap[level][varnum] = v;
			//printf("lastvar=%i size=%i\n", varnum, varmap.size());
		} else {
			v = NULL;
		}
	}
	return(v);
}


int Variables::type(int varnum)
{
	lasterror = ERROR_NONE;
	variable *v = getvfromnum(varnum,false);
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
    variable *v = getvfromnum(varnum,true);
    clearvariable(v);
    v->type = T_FLOAT;
    v->value_floatval = value;
}

void Variables::setvarref(int varnum, int value)
{
    lasterror = ERROR_NONE;
    variable *v = getvfromnum(varnum,true);
    clearvariable(v);
    v->type = T_VARREF;
    v->value_floatval = value;
}

double Variables::getfloat(int varnum)
{
	lasterror = ERROR_NONE;
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_FLOAT) {
			return(v->value_floatval);
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
	lasterror = ERROR_NONE;
	variable *v = getvfromnum(varnum,true);
	clearvariable(v);
	v->type = T_STRING;
	v->value_string = value;
}

QString Variables::getstring(int varnum)
{
	// just return pointer - copied when put on stack so this is a good pointer
	lasterror = ERROR_NONE;
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_STRING) {
			return(v->value_string);
		} else {
			lasterror = ERROR_NOSUCHVARIABLE;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		lasterrorvar = varnum;
	}
	return null;
}

void Variables::arraydimfloat(int varnum, int xdim, int ydim, bool redim)
{
	variable *v = getvfromnum(varnum,true);
	lasterror = ERROR_NONE;

	// max number of elements to save on a redim
	int redimsize = 0;
	if(redim && v->type==T_ARRAY) {
		redimsize = v->value_arr->size;	
	}
	
	int size = xdim * ydim;
	if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (size >= 1) {
			array *a = new array;
			double *d = new double[size];
			for (int j = 0; j < size; j++) {
				if(j < redimsize) {
					d[j] = v->value_arr->data.fdata[j];						
				} else {
					d[j] = 0;
				}
			}

			clearvariable(v);

			v->type = T_ARRAY;
			a->data.fdata = d;
			a->size = size;
			a->xdim = xdim;
			a->ydim = ydim;
			v->value_arr = a;
		} else {
			lasterror = ERROR_ARRAYSIZESMALL;
			lasterrorvar = varnum;
		}
	} else {
		lasterror = ERROR_ARRAYSIZELARGE;
		lasterrorvar = varnum;
	}
}

void Variables::arraydimstring(int varnum, int xdim, int ydim, bool redim)
{
	variable *v = getvfromnum(varnum,true);
	lasterror = ERROR_NONE;

	// max number of elements to save on a redim
	int redimsize = 0;
	if(redim && v->type==T_STRARRAY) {
		redimsize = v->value_arr->size;	
	}
	
	int size = xdim * ydim;
	if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (size >= 1) {
			array *a = new array;
			char **c = new char*[size];
			for (int j = 0; j < size; j++) {
				if(j < redimsize) {
					c[j] = v->value_arr->data.sdata[j];						
				} else {
					c[j] = null;
				}
			}

			clearvariable(v);

			v->type = T_STRARRAY;
			a->data.sdata = c;
			a->size = size;
			a->xdim = xdim;
			a->ydim = ydim;
			v->value_arr = a;
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->value_arr->size);
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->value_arr->xdim);
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY || v->type == T_STRARRAY)
		{
			return(v->value_arr->ydim);
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(index >= 0 && index < v->value_arr->size) {
				v->value_arr->data.fdata[index] = value;
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(x >= 0 && x < v->value_arr->xdim && y >= 0 && y < v->value_arr->ydim ) {
				v->value_arr->data.fdata[x * v->value_arr->ydim + y] = value;
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(index >= 0 && index < value_arr->size) {
				return(v->value_arr->data.fdata[index]);
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_ARRAY) {
			if(x >= 0 && x < v->value_arr->xdim && y >= 0 && y < v->value_arr->ydim ) {
				return(v->value_arr->data.fdata[x * v->value_arr->ydim + y]);
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(index >= 0 && index < v->value_arr->size) {
				if (v->value_arr->data.sdata[index])
				{
					free(v->value_arr->data.sdata[index]);
				}
				v->value_arr->data.sdata[index] = value;
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(x >= 0 && x < v->value_arr->xdim && y >= 0 && y < v->value_arr->ydim ) {
				int index = x * v->value_arr->ydim + y;
				if (v->value_arr->data.sdata[index])
				{
					free(v->value_arr->data.sdata[index]);
				}
				v->value_arr->data.sdata[index] = value;
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
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(index >= 0 && index < v->value_arr->size) {
				return(v->value_arr->data.sdata[index]);
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
	return null;
}

QString Variables::array2dgetstring(int varnum, int x, int y)
{
	lasterror = ERROR_NONE;
	variable *v = getvfromnum(varnum,false);
	if(v) {
		if (v->type == T_STRARRAY) {
			if(x >= 0 && x < v->value_arr->xdim && y >= 0 && y < v->value_arr->ydim ) {
				int index = x * v->value_arr->ydim + y;
				return(v->value_arr->data.sdata[index]);
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
	return null;
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

