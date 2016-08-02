#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_REF is a variable number in the previous recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable(int v) {
    data = new DataElement();
    data->intval = v;			// store variable number in intval for unassigned
    arr = NULL;
}

Variable::~Variable() {
    if (data) delete(data);
    if (arr) delete(arr);
}


// ************************************************************************
// * Variables contain the individual variable(s) for the recursion level *
// ************************************************************************

Variables::Variables(int n, Error *e) {
	// n is the size of the symbol table
	recurselevel = 0;
	numsyms = n;
    allocateRecurseLevel();
    error = e;
}

Variables::~Variables() {
    // free variables and all storage
    do {
		freeRecurseLevel();
		recurselevel--;
	} while (recurselevel>=0);
}

QString Variables::debug() {
    // return a string representing the variables
    QString s("");
    if(!varmap.empty()) {
        for(int i=0;i<=recurselevel;i++) {
            s += "recurse Level " + QString::number(i);
            for(unsigned int j=0; j < varmap[i].size(); j++) {
                s += " varnum " + QString::number(j);
                s += " " + (varmap[i][j]?(varmap[i][j]->data?varmap[i][j]->data->debug():"NULL"):"UNUSED") + "\n";
            }
            s += "\n";
        }
    }
    return s;
}

void Variables::allocateRecurseLevel() {
    // initialize a new recurse level
    // create a NULL pointer for each symbol at this recurse level
    // wasteful for the labels and jump points but vector is so fast
    // to access an element it does not matter
    varmap[recurselevel].resize(numsyms);
    for(int i=0; i<numsyms; i++) {
		varmap[recurselevel][i] = new Variable(i);
	}
}

void Variables::freeRecurseLevel() {
    // delete a recurse level being sure to delete
    // the variables in the runlevel efore removing it
    unsigned int size = varmap[recurselevel].size();
    for(unsigned int i=0; i<size; i++) {
		if (varmap[recurselevel][i]) {
			delete(varmap[recurselevel][i]);
			varmap[recurselevel][i]=NULL;
		}
	}
	varmap[recurselevel].clear();
	varmap.erase(recurselevel);
}

void
Variables::increaserecurse() {
    recurselevel++;
    if (recurselevel>=MAX_RECURSE_LEVELS) {
        recurselevel--;
        error->q(ERROR_MAXRECURSE);
    } else {
		allocateRecurseLevel();
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
        freeRecurseLevel();
        // return back to prev variable context
        recurselevel--;
    }
}

Variable* Variables::get(int varnum) {
	// get v from map or follow REF to previous recurse level if needed

	Variable *v;
	int level = recurselevel;
	if(level!=0 && isglobal(varnum)) level = 0;

	v = varmap[level][varnum];
	if (v->data->type==T_REF && recurselevel>0) {
		recurselevel--;
		v = get(v->data->intval);
		recurselevel++;
	}
	return(v);
}


VariableInfo* Variables::getInfo(int varnum) {
	// used ONLY by VariableWin to get the real variable being effected
	// by the assignment
	// returns the actual variable number and recurse level the variable pointed
	// to
	//
	VariableInfo  *vi;
	Variable *v;
	int level = recurselevel;
	if(level!=0 && isglobal(varnum)) level = 0;

	v = varmap[level][varnum];
	if (v->data && v->data->type==T_REF && recurselevel>0) {
		// recurse for a reference
		recurselevel--;
		vi = getInfo(v->data->intval);
		recurselevel++;
		return(vi);
	}
	// has data or not send back current recurse info
	vi = new VariableInfo();
	vi->level = level;
	vi->varnum = varnum;
	return(vi);
}

DataElement* Variables::getdata(int varnum) {
    // get data from v - return varnum's data
    Variable *v = get(varnum); //get variable if exist or create a new one
    return v->data;

}

void Variables::setdata(int varnum, DataElement* e) {
    // recieves a DataElement pointed pulled from the stack
    // e is a pointer in the stack vector AND MUST NOT BE DELETED
    Variable *v = get(varnum);
    v->data->copy(e);
}

void Variables::setdata(int varnum, long l) {
    Variable *v = get(varnum);
    v->data->type = T_INT;
    v->data->intval = l;
}

void Variables::setdata(int varnum, double f) {
    Variable *v = get(varnum);
    v->data->type = T_FLOAT;
    v->data->floatval = f;
}

void Variables::setdata(int varnum, QString s) {
    Variable *v = get(varnum);
    v->data->type = T_STRING;
    v->data->stringval = s;
}

void Variables::unassign(int varnum) {
    Variable *v = get(varnum);
    v->data->type = T_UNASSIGNED;
    v->data->intval = varnum;
}

void Variables::arraydim(int varnum, int xdim, int ydim, bool redim) {
    Variable *v = get(varnum);
    int size = xdim * ydim;
    
    if (size <= VARIABLE_MAXARRAYELEMENTS) {
        if (size >= 1) {
            if (v->data->type != T_ARRAY || !redim || !v->arr) {
                // if array data is dim or redim without a dim then create a new one (clear the old)
                if (v->arr){
					int oldsize = v->arr->xdim * v->arr->ydim;
					for(int i=0;i<oldsize;i++) {
						if(v->arr->datavector[i]) {
							delete(v->arr->datavector[i]);
							v->arr->datavector[i] = NULL;
						}
					}
                    v->arr->datavector.clear();
                    delete(v->arr);
                }
                v->arr = new VariableArrayPart;
                v->arr->datavector.resize(size);
				for(int i=0;i<size;i++) {
					v->arr->datavector[i] = new DataElement();
					v->arr->datavector[i]->intval = varnum;
				}
            }else{
				// redim - resize the vector
				int oldsize = v->arr->xdim * v->arr->ydim;
				if (size<oldsize) {
					// free elements from end and resize
					for(int i=size;i<oldsize;i++) {
						if(v->arr->datavector[i]) {
							delete(v->arr->datavector[i]);
							v->arr->datavector[i] = NULL;
						}
					}
					v->arr->datavector.resize(size);
				}
				if (size>oldsize) {
					// resize and create empty elements
					v->arr->datavector.resize(size);
					for(int i=oldsize;i<size;i++) {
						v->arr->datavector[i] = new DataElement();
						v->arr->datavector[i]->intval = varnum;
					}
				}
            }
            v->data->type = T_ARRAY;
            v->data->intval = varnum;	// put the variable number as the intval so that if it ends up on the stack we can get the original number
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

    if (v->data->type == T_ARRAY) {
        return(v->arr->xdim * v->arr->ydim);
    } else if (v->data->type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

int Variables::arraysizex(int varnum) {
    // return number of columns of array as if it was a two dimensional array - 0 = not an array
    Variable *v = get(varnum);

    if (v->data->type == T_ARRAY) {
        return(v->arr->xdim);
    } else if (v->data->type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

int Variables::arraysizey(int varnum) {
    // return number of rows of array as if it was a two dimensional array - 0 = not an array
    Variable *v = get(varnum);

    if (v->data->type == T_ARRAY) {
        return(v->arr->ydim);
    } else if (v->data->type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

DataElement* Variables::arraygetdata(int varnum, int x, int y) {
	// get data from array elements in v from map (using x, y)
	// if there is an error return an unassigned value
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			return v->arr->datavector[i];
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
	return v->data;
}

//This function sould be used to access data using just one index
//this is usefull expecially for statements that accept array (SOUND, POLY etc.) by OP_ARRAY2STACK or OP_IMPLODE
//In this way can be passed arrays with two dimensions without problems. (+speed)

DataElement* Variables::arraygetdata(int varnum, int y) {
	// get data from array elements in v from map (using items index)
	// if there is an error return an unassigned value
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (y >=0 && y < v->arr->ydim * v->arr->ydim) {
			return v->arr->datavector[y];
		} else {
		error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
	return v->data;
}

void Variables::arraysetdata(int varnum, int x, int y, DataElement *e) {
	// DataElement's data is copied to the variable and it should be deleted
	// by whomever created it
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			v->arr->datavector[i]->copy(e);
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, long l) {
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			v->arr->datavector[i]->type = T_INT;
			v->arr->datavector[i]->intval = l;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, double f) {
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			v->arr->datavector[i]->type = T_FLOAT;
			v->arr->datavector[i]->floatval = f;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, QString s) {
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			v->arr->datavector[i]->type = T_STRING;
			v->arr->datavector[i]->stringval = s;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arrayunassign(int varnum, int x, int y) {
	Variable *v = get(varnum);
	if (v->data->type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
			v->arr->datavector[i]->type = T_UNASSIGNED;
			v->arr->datavector[i]->intval = varnum;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
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
    return (globals.count(varnum)>0);
}
