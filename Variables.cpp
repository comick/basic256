#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to the following types:
// T_UNUSED, T_FLOAT, T_STRING, T_ARRAY, and T_STRARRAY

Variables::Variables()
{
	lasterror = ERROR_NONE;
	// initialize variable storage
	for (int i = 0; i < VARIABLE_NUMVARS; i++)
	{
		vars[i].type = T_UNUSED;
		vars[i].value.floatval = 0;
		vars[i].value.string = NULL;
		vars[i].value.arr = NULL;
	}
}

Variables::~Variables()
{
	lasterror = ERROR_NONE;
	// free variables and all storage

}

int Variables::error() {
	return(lasterror);
}

void
Variables::clear()
{
	lasterror = ERROR_NONE;
	// free all variables - string and arrays
	for (int i = 0; i < VARIABLE_NUMVARS; i++)
	{
		if (vars[i].type != T_UNUSED) clearvariable(i);
	}
}


void
Variables::clearvariable(int varnum)
{
	// free a single variable
	if (vars[varnum].type == T_STRING && vars[varnum].value.string != NULL)
	{
		free(vars[varnum].value.string);
		vars[varnum].value.string = NULL;
	}
	else if (vars[varnum].type == T_ARRAY && vars[varnum].value.arr != NULL)
	{
		delete(vars[varnum].value.arr->data.fdata);
		delete(vars[varnum].value.arr);
	}
	else if (vars[varnum].type == T_STRARRAY && vars[varnum].value.arr != NULL)
	{
		for (int j = 0; j < vars[varnum].value.arr->size; j++)
		{
			if (vars[varnum].value.arr->data.sdata[j])
			{
				free(vars[varnum].value.arr->data.sdata[j]);
				vars[varnum].value.arr->data.sdata[j] = NULL;
			}
		}
		delete(vars[varnum].value.arr->data.sdata);
		delete(vars[varnum].value.arr);
	}
	vars[varnum].type = T_UNUSED;
	vars[varnum].value.floatval = 0;
	vars[varnum].value.string = NULL;
	vars[varnum].value.arr = NULL;
}


int Variables::type(int varnum)
{
	return(vars[varnum].type);
}


void Variables::setfloat(int varnum, double value)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type != T_UNUSED) clearvariable(varnum);
	vars[varnum].type = T_FLOAT;
	vars[varnum].value.floatval = value;
}

double Variables::getfloat(int varnum)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_FLOAT) {
		return(vars[varnum].value.floatval);
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		return(0);
	}
}

void Variables::setstring(int varnum, char *value)
{
	// pass pointer - copied when put on stack so this is a good pointer
	if (vars[varnum].type != T_UNUSED) clearvariable(varnum);
	vars[varnum].type = T_STRING;
	vars[varnum].value.string = value;
}

char *Variables::getstring(int varnum)
{
	// just return pointer - copied when put on stack so this is a good pointer
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_STRING) {
		return(vars[varnum].value.string);
	} else {
		lasterror = ERROR_NOSUCHVARIABLE;
		return(NULL);
	}
}

void Variables::arraydimfloat(int varnum, int xdim, int ydim, bool redim)
{
	lasterror = ERROR_NONE;

	// max number of elements to save on a redim
	int redimsize = 0;
	if(redim && vars[varnum].type==T_ARRAY) {
		redimsize = vars[varnum].value.arr->size;	
	}
	
	int size = xdim * ydim;
	if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (size >= 1) {
			array *a = new array;
			double *d = new double[size];
			for (int j = 0; j < size; j++) {
				if(j < redimsize) {
					d[j] = vars[varnum].value.arr->data.fdata[j];						
				} else {
					d[j] = 0;
				}
			}

			if (vars[varnum].type != T_UNUSED) clearvariable(varnum);

			vars[varnum].type = T_ARRAY;
			a->data.fdata = d;
			a->size = size;
			a->xdim = xdim;
			a->ydim = ydim;
			vars[varnum].value.arr = a;
		} else {
			lasterror = ERROR_ARRAYSIZESMALL;
		}
	} else {
		lasterror = ERROR_ARRAYSIZELARGE;
	}
}

void Variables::arraydimstring(int varnum, int xdim, int ydim, bool redim)
{
	lasterror = ERROR_NONE;

	// max number of elements to save on a redim
	int redimsize = 0;
	if(redim && vars[varnum].type==T_STRARRAY) {
		redimsize = vars[varnum].value.arr->size;	
	}
	
	int size = xdim * ydim;
	if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (size >= 1) {
			array *a = new array;
			char **c = new char*[size];
			for (int j = 0; j < size; j++) {
				if(j < redimsize) {
					c[j] = vars[varnum].value.arr->data.sdata[j];						
					vars[varnum].value.arr->data.sdata[j]=NULL;	// so not free-d with moved to new array
				} else {
					c[j] = strdup("");
				}
			}

			if (vars[varnum].type != T_UNUSED) clearvariable(varnum);

			vars[varnum].type = T_STRARRAY;
			a->data.sdata = c;
			a->size = size;
			a->xdim = xdim;
			a->ydim = ydim;
			vars[varnum].value.arr = a;
		} else {
			lasterror = ERROR_ARRAYSIZESMALL;
		}
	} else {
		lasterror = ERROR_ARRAYSIZELARGE;
	}
}

int Variables::arraysize(int varnum)
{
	// return length of array as if it was a one dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY || vars[varnum].type == T_STRARRAY)
	{
		return(vars[varnum].value.arr->size);
	} else {
		lasterror = ERROR_NOTARRAY;
	}
	return(0);
}

int Variables::arraysizex(int varnum)
{
	// return number of columns of array as if it was a two dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY || vars[varnum].type == T_STRARRAY)
	{
		return(vars[varnum].value.arr->xdim);
	} else {
		lasterror = ERROR_NOTARRAY;
	}
	return(0);
}

int Variables::arraysizey(int varnum)
{
	// return number of rows of array as if it was a two dimensional array - 0 = not an array		
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY || vars[varnum].type == T_STRARRAY)
	{
		return(vars[varnum].value.arr->ydim);
	} else {
		lasterror = ERROR_NOTARRAY;
	}
	return(0);
}

void Variables::arraysetfloat(int varnum, int index, double value)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY) {
		if(index >= 0 && index < vars[varnum].value.arr->size) {
			vars[varnum].value.arr->data.fdata[index] = value;
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTARRAY;
	}
}

void Variables::array2dsetfloat(int varnum, int x, int y, double value)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY) {
		if(x >= 0 && x < vars[varnum].value.arr->xdim && y >= 0 && y < vars[varnum].value.arr->ydim ) {
			vars[varnum].value.arr->data.fdata[x * vars[varnum].value.arr->ydim + y] = value;
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTARRAY;
	}
}

double Variables::arraygetfloat(int varnum, int index)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY) {
		if(index >= 0 && index < vars[varnum].value.arr->size) {
			return(vars[varnum].value.arr->data.fdata[index]);
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTARRAY;
	}
	return(0);
}

double Variables::array2dgetfloat(int varnum, int x, int y)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_ARRAY) {
		if(x >= 0 && x < vars[varnum].value.arr->xdim && y >= 0 && y < vars[varnum].value.arr->ydim ) {
			return(vars[varnum].value.arr->data.fdata[x * vars[varnum].value.arr->ydim + y]);
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTARRAY;
	}
	return(0);
}

void Variables::arraysetstring(int varnum, int index, char *value)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_STRARRAY) {
		if(index >= 0 && index < vars[varnum].value.arr->size) {
			if (vars[varnum].value.arr->data.sdata[index])
			{
				free(vars[varnum].value.arr->data.sdata[index]);
			}
			vars[varnum].value.arr->data.sdata[index] = value;
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTSTRINGARRAY;
	}
}

void Variables::array2dsetstring(int varnum, int x, int y, char *value)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_STRARRAY) {
		if(x >= 0 && x < vars[varnum].value.arr->xdim && y >= 0 && y < vars[varnum].value.arr->ydim ) {
			int index = x * vars[varnum].value.arr->ydim + y;
			if (vars[varnum].value.arr->data.sdata[index])
			{
				free(vars[varnum].value.arr->data.sdata[index]);
			}
			vars[varnum].value.arr->data.sdata[index] = value;
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTSTRINGARRAY;
	}
}

char *Variables::arraygetstring(int varnum, int index)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_STRARRAY) {
		if(index >= 0 && index < vars[varnum].value.arr->size) {
			return(vars[varnum].value.arr->data.sdata[index]);
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTSTRINGARRAY;
	}
	return(NULL);
}

char *Variables::array2dgetstring(int varnum, int x, int y)
{
	lasterror = ERROR_NONE;
	if (vars[varnum].type == T_STRARRAY) {
		if(x >= 0 && x < vars[varnum].value.arr->xdim && y >= 0 && y < vars[varnum].value.arr->ydim ) {
			int index = x * vars[varnum].value.arr->ydim + y;
			return(vars[varnum].value.arr->data.sdata[index]);
		} else {
			lasterror = ERROR_ARRAYINDEX;
		}
	} else {
		lasterror = ERROR_NOTSTRINGARRAY;
	}
	return(NULL);
}



