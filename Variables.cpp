#include <map>
#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_REF is a variable number in the previous recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable() {
	data = new DataElement();
	arr = NULL;
}

Variable::~Variable() {
}


// ************************************************************************ 
// * Variables contain the individual variable(s) for the recursion level *
// ************************************************************************ 

Variables::Variables(Error *e) {
	error = e;
    // initialize variable storage
}

Variables::~Variables() {
    // free variables and all storage
    clear();
}

QString Variables::debug() {
	// return a string representing the variables
	QString s("");
	if(!varmap.empty()) {
		for(std::map<int, std::map<int, Variable*> >::iterator i = varmap.begin(); i != varmap.end(); ++i) {
			s += "recurse Level " + QString::number(i->first);
			for(std::map<int, Variable*>::iterator j = i->second.begin(); j != i->second.end(); ++j) {
				s += " varnum " + QString::number(j->first);
				s += " " + (j->second->data?j->second->data->debug():"NULL") + "\n";
			}
			s += "\n";  
		}
	}
	return s;
}

void
Variables::clear() {
    // erase all variables and delete them
    varmap.clear();
    globals.clear();
    recurselevel = 0;
}

void
Variables::increaserecurse() {
    recurselevel++;
    if (recurselevel>=MAX_RECURSE_LEVELS) {
        recurselevel--;
        error->q(ERROR_MAXRECURSE);
    }
}

int
Variables::getrecurse() {
    return	recurselevel;
}

void
Variables::decreaserecurse() {
    if (recurselevel>0) {
        // erase all variables in the current recurse level before we return
        // to the previous level
        if(varmap.find(recurselevel)!=varmap.end()) {
            while(!varmap[recurselevel].empty()) {
                std::map<int,Variable*>::iterator j=varmap[recurselevel].begin();
                delete((*j).second);
                varmap[recurselevel].erase((*j).first);
            }
            varmap.erase(recurselevel);
        }
        // return back to prev variable context
        recurselevel--;
    }
}

Variable* Variables::get(int varnum) {
	// get v from map else make a new one if not exist with NULL data and arr
	// foll REF to previous recurse level if needed
	Variable *v;
	int level=recurselevel;
	if (isglobal(varnum)) {
		level = 0;
	}
	if (varmap.find(level) != varmap.end() && varmap[level].find(varnum) != varmap[level].end()) {
		v = varmap[level][varnum];
		if (v->data) {
			if (v->data->type==T_REF && recurselevel>0) {
				recurselevel--;
				v = get(v->data->intval);
				recurselevel++;
			}
		} else {
			v = new Variable();
			varmap[level][varnum] = v;
		}
	} else {
		v = new Variable();
		varmap[level][varnum] = v;
	}
	return(v);
}

DataElement* Variables::getdata(int varnum) {
    // get data from v -return NULL if not assigned
    DataElement *e = NULL;
    Variable *v = get(varnum);
 	if (v->data) {
		e = v->data;
	} else {
		error->q(ERROR_VARNOTASSIGNED, varnum);
	}
	return e;
}

void Variables::setdata(int varnum, DataElement* e) {
	// recieves a DataElement pointed pulled from the stack
	// DO NOT DELETE WHEN PULLED FROM STACK AS it lives here now
	// COPY THIS DataElement WHNE PUSHING ONTO STACK
	Variable *v = get(varnum);
	if (v->data) delete(v->data);
	v->data = e;
}

void Variables::arraydim(int varnum, int xdim, int ydim, bool redim) {
    Variable *v = get(varnum);
    int size = xdim * ydim;

    if (size <= VARIABLE_MAXARRAYELEMENTS) {
		if (!v->data) v->data = new DataElement();
        if (size >= 1) {
            if (v->data->type != T_ARRAY || !redim || !v->arr) {
                // if array data is dim or redim without a dim then create a new one (clear the old)
                if (v->arr) delete(v->arr);
                v->arr = new VariableArrayPart;
            }
            v->data->type = T_ARRAY;
            v->data->intval = varnum;	// put the variable number as the intval so that if it ends up on the stack we can get the original number
            v->arr->size = size;
            v->arr->xdim = xdim;
            v->arr->ydim = ydim;
        } else {
            error->q(ERROR_ARRAYSIZESMALL, varnum);
        }
    } else {
        error->q(ERROR_ARRAYSIZELARGE, varnum);
    }
}

int Variables::arraysize(int varnum) {
	// return length of array as if it was a one dimensional array - 0 = not an array
	Variable *v = get(varnum);
	if(v) {
		if (v->data) {
			if (v->data->type == T_ARRAY) {
				return(v->arr->size);
			} else {
				error->q(ERROR_NOTARRAY, varnum);
			}
		} else {
			error->q(ERROR_VARNOTASSIGNED, varnum);
		}
	} else {
		error->q(ERROR_NOSUCHVARIABLE, varnum);
	}
	return(0);
}

int Variables::arraysizex(int varnum) {
    // return number of columns of array as if it was a two dimensional array - 0 = not an array
	Variable *v = get(varnum);
	if(v) {
		if (v->data) {
			if (v->data->type == T_ARRAY) {
				return(v->arr->xdim);
			} else {
				error->q(ERROR_NOTARRAY, varnum);
			}
		} else {
			error->q(ERROR_VARNOTASSIGNED, varnum);
		}
	} else {
		error->q(ERROR_NOSUCHVARIABLE, varnum);
	}
	return(0);
}

int Variables::arraysizey(int varnum) {
    // return number of rows of array as if it was a two dimensional array - 0 = not an array
	Variable *v = get(varnum);
	if(v) {
		if (v->data) {
			if (v->data->type == T_ARRAY) {
				return(v->arr->ydim);
			} else {
				error->q(ERROR_NOTARRAY, varnum);
			}
		} else {
			error->q(ERROR_VARNOTASSIGNED, varnum);
		}
	} else {
		error->q(ERROR_NOSUCHVARIABLE, varnum);
	}
	return(0);
}

DataElement* Variables::arraygetdata(int varnum, int x, int y) {
	// get data from array elements in v from map
	// if there is an error return NULL
	Variable *v = get(varnum);
	if(v) {
		if(v->data) {
			if (v->data->type == T_ARRAY) {
				if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
					int i = x * v->arr->ydim + y;
					if (v->arr->datamap.find(i) != v->arr->datamap.end()) {
						return v->arr->datamap[i];
					} else {
						error->q(ERROR_VARNOTASSIGNED, varnum);
					}
			   } else {
					error->q(ERROR_ARRAYINDEX, varnum);
				}
			} else {
				error->q(ERROR_NOTARRAY, varnum);
			}
		} else {
			error->q(ERROR_VARNOTASSIGNED, varnum);
		}
	} else {
		error->q(ERROR_NOSUCHVARIABLE, varnum);
	}
	return(NULL);
}


void Variables::arraysetdata(int varnum, int x, int y, DataElement *e) {
	// be sure this is popped from the stack and not deleted
	Variable *v = get(varnum);
	if(v) {
		if(v->data) {
			if (v->data->type == T_ARRAY) {
				if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
					int i = x * v->arr->ydim + y;
					if (v->arr->datamap.find(i) != v->arr->datamap.end()) {
						delete(v->arr->datamap[i]);
					}
					v->arr->datamap[i] = e;
			   } else {
					error->q(ERROR_ARRAYINDEX, varnum);
				}
			} else {
				error->q(ERROR_NOTARRAY, varnum);
			}
		} else {
			error->q(ERROR_VARNOTASSIGNED, varnum);
		}
	} else {
		error->q(ERROR_NOSUCHVARIABLE, varnum);
	}
}
 


void Variables::makeglobal(int varnum) {
    // make a variable global - if there is a value in a run level greater than 0 then
    // that value will be lost to the program
    // globals list are erased when variables are reset
    globals[varnum] = true;
}

bool Variables::isglobal(int varnum) {
    // return true if the variable is on the list of globals
    if(globals.find(varnum)!=globals.end()) {
        return globals[varnum];
    }
    return false;
}

