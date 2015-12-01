#include <map>
#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_VARREF is a variable number in the previous recursion level

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
				s += " " + j->second->debug() + "\n";
			}
			s += "\n";  
		}
	}
	return s;
}

void
Variables::clear() {
    // erase all variables and delete them
    while(!varmap.empty()) {
        std::map<int, std::map<int,Variable*> >::iterator i=varmap.begin();
        while(!(*i).second.empty()) {
            std::map<int,Variable*>::iterator j=(*i).second.begin();
            clearvariable((*j).second);
            delete((*j).second);						// delete the variable object
            (*i).second.erase((*j).first);
        }
        varmap.erase((*i).first);
    }
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
                clearvariable((*j).second);
                delete((*j).second);						// delete the variable object
                varmap[recurselevel].erase((*j).first);		// delete it from the recurse map
            }
            varmap.erase(recurselevel);
        }
        // return back to prev variable context
        recurselevel--;
    }
}

void Variables::clearvariable(Variable* v) {
    // free a variable's current value to allow it to be reassigned
    // but do not delete it
    if (v->type == T_ARRAY)	{
        while(!v->arr->datamap.empty()) {
            std::map<int,DataElement*>::iterator j=v->arr->datamap.begin();
            delete((*j).second);					// delete the array element object
            v->arr->datamap.erase((*j).first);		// delete it from the map
        }
        delete(v->arr);
        v->arr = NULL;
    }
    v->type = T_UNUSED;
}

Variable* Variables::get(int varnum) {
    // get v from map else make a new one if not exist
    // followref - follow variable reference
    Variable *v;
    int level=recurselevel;
    if (isglobal(varnum)) {
        level = 0;
    }
    if (varmap.find(level) != varmap.end() && varmap[level].find(varnum) != varmap[level].end()) {
        v = varmap[level][varnum];
        if (v->type==T_VARREF && recurselevel>0) {
            recurselevel--;
            v = get((int) v->floatval);
            recurselevel++;
        }
    } else {
        v = new Variable();
        v->arr = NULL;
        varmap[level][varnum] = v;
        //printf("lastvar=%i size=%i\n", varnum, varmap.size());
    }
    return(v);
}

void Variables::set(int varnum, b_type type, double value, QString stringval) {
    // pass string pointer - copied when put on stack so this is a good pointer
    Variable *v = get(varnum);
    if (v) {
		clearvariable(v);
		v->type = type;
		v->floatval = value;
		v->stringval = stringval;
	}
}

void Variables::arraydim(int varnum, int xdim, int ydim, bool redim) {
    Variable *v = get(varnum);
    int size = xdim * ydim;

    if (size <= VARIABLE_MAXARRAYELEMENTS) {
        if (size >= 1) {
            if (v->type != T_ARRAY || !redim || !v->arr) {
                // if array data is dim or redim without a dim then create a new one (clear the old)
                clearvariable(v);
                v->arr = new VariableArrayPart;
            }
            v->type = T_ARRAY;
            v->floatval = varnum;	// put the variable number as the floatval so that if it ends up on the stack we can get the original number
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
        if (v->type == T_ARRAY) {
            return(v->arr->size);
        } else {
            error->q(ERROR_NOTARRAY, varnum);
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
        if (v->type == T_ARRAY) {
            return(v->arr->xdim);
        } else {
            error->q(ERROR_NOTARRAY, varnum);
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
        if (v->type == T_ARRAY) {
            return(v->arr->ydim);
        } else {
            error->q(ERROR_NOTARRAY, varnum);
        }
    } else {
        error->q(ERROR_NOSUCHVARIABLE, varnum);
    }
    return(0);
}

DataElement* Variables::arrayget(int varnum, int x, int y) {
    // get variable from array elements in v from map	arraydata *d;
    DataElement *d = NULL;
    Variable *v = get(varnum);
    if(v) {
        if (v->type == T_ARRAY) {
			if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
				int i = x * v->arr->ydim + y;
				if (v->arr->datamap.find(i) != v->arr->datamap.end()) {
					d = v->arr->datamap[i];
				} else {
					d = new DataElement(T_UNUSED, 0, QString());
					v->arr->datamap[i] = d;
				}
           } else {
                error->q(ERROR_ARRAYINDEX, varnum);
				d = new DataElement(T_UNUSED, 0, QString());
            }
        } else {
            error->q(ERROR_NOTARRAY, varnum);
			d = new DataElement(T_UNUSED, 0, QString());
        }
    } else {
        error->q(ERROR_NOSUCHVARIABLE, varnum);
		d = new DataElement(T_UNUSED, 0, QString());
    }
	return(d);
 }


void Variables::arrayset(int varnum, int x, int y, b_type type, double value, QString stringval) {
    DataElement *d = arrayget(varnum, x, y);
    if(d) {
		d->type = type;
		d->floatval = value;
		if (!stringval.isNull()) d->stringval = stringval;
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

