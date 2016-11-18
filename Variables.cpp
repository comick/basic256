#include "Variables.h"
#include <string>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_REF is a variable number in the specified recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable(int v, int level) {
    data.intval = v;			// store variable number in intval for unassigned
    data.level = level;
    arr = NULL;
}

Variable::~Variable() {
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
    globals.resize(numsyms, false);
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
                s += " " + (varmap[i][j]?varmap[i][j]->data.debug():"UNUSED") + "\n";
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
        varmap[recurselevel][i] = new Variable(i, recurselevel);
	}
}

void Variables::freeRecurseLevel() {
    // delete a recurse level being sure to delete
    // the variables in the runlevel efore removing it
    for(int i=0; i<numsyms; i++) {
        delete(varmap[recurselevel][i]);
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


Variable* Variables::get(int varnum, int level) {
    // get v from map or follow REF to recurse level if needed
    if(level!=0){
        if(isglobal(varnum)) level=0;
    }

    Variable *v = varmap[level][varnum];
    if((v->data.type!=T_REF))
        return(v);

    // make a stack of varnum/level needed to check for circular reference and put the original first
    std::vector<std::pair<int, int>> track = {{varnum, level}};
    std::vector<std::pair<int, int>>::iterator it;
    std::pair <int,int> item;

    do{
        level=v->data.level;
        if(level!=0){
            if(isglobal(v->data.intval)){
                level=0;
            }else if(level>recurselevel){
                //error: Variable points to a non-existent variable
                //this can be done by assign a global variable inside a subroutine and return to previous level
                //where we use the value of global var
                error->q(ERROR_VARNULL);
                return v;
            }
        }

        // check in stack for v->data.intval/v->data.level and if it is founded then trow error for circular reference
        // store v->data.intval/v->data.level in stack

        item = std::make_pair(v->data.intval, v->data.level);
        it = std::find(track.begin(), track.end(), item);
        if (it != track.end()){
            error->q(ERROR_VARCIRCULAR);
            return v;
        }
        track.push_back(item);
        v = varmap[level][v->data.intval];


    }while(v->data.type==T_REF);

    return(v);
}

Variable* Variables::get(int varnum) {
    return get(varnum, recurselevel);
}

VariableInfo* Variables::getInfo(int varnum) {
	// used ONLY by VariableWin to get the real variable being effected
	// by the assignment
	// returns the actual variable number and recurse level the variable pointed to

    Variable *v = get(varnum, recurselevel);
    VariableInfo *vi = new VariableInfo();
    vi->level = v->data.level; //recurselevel==0?0:(isglobal(varnum)?0:recurselevel);
    vi->varnum = varnum;
    //vi->varnum = varnum;
    return(vi);
}

DataElement* Variables::getdata(int varnum) {
    // get data from v - return varnum's data
    Variable *v = get(varnum, recurselevel);
    return &v->data;
}

DataElement* Variables::getdata(DataElement* e) {
    // get data from a REF() - already check for type==T_REF
    if(e->level>recurselevel){
        error->q(ERROR_VARNULL);
        return e;
    }
    Variable *v = get(e->intval, e->level);
    return &v->data;
}

void Variables::setdata(int varnum, DataElement* e) {
    // recieves a DataElement pointed pulled from the stack
    // e is a pointer in the stack vector AND MUST NOT BE DELETED
    Variable *v = get(varnum, recurselevel);
    v->data.copy(e);
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, long l) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_INT;
    v->data.intval = l;
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, double f) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_FLOAT;
    v->data.floatval = f;
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, QString s) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_STRING;
    v->data.stringval = s;
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::unassign(int varnum) { //unassign true variable, not the reference (unlink)
    int level=recurselevel;
    if(level!=0){
        if(isglobal(varnum)) level=0;
    }

    Variable *v = varmap[level][varnum];
    v->data.type = T_UNASSIGNED;
    v->data.intval = varnum;
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::copyarray(int varnum1, DataElement* e) {
    // suitable for copy arrays and any type of variable
    // for T_REF
    if(e->level>recurselevel){
        error->q(ERROR_VARNULL);
        return;
    }
    Variable *v1 = get(varnum1, recurselevel); //get the destination variable
    Variable *v2 = get(e->intval, e->level); //get the source variable
    v1->data.type = T_ARRAY;
    if (v1->arr){
        delete(v1->arr);
        v1->arr=NULL;
    }
    if (v2->arr){
        v1->arr = new VariableArrayPart;
        v1->arr->xdim = v2->arr->xdim;
        v1->arr->ydim = v2->arr->ydim;
        int size = v2->arr->xdim * v2->arr->ydim;
        v1->arr->datavector.resize(size);
        for(int i=0;i<size;i++) {
            v1->arr->datavector[i].copy(&v2->arr->datavector[i]);
        }
    }
}


void Variables::arraydim(int varnum, int xdim, int ydim, bool redim) {
    Variable *v = get(varnum, recurselevel);
    int size = xdim * ydim;
    
    if (size <= VARIABLE_MAXARRAYELEMENTS) {
        if (size >= 1) {
            if (v->data.type != T_ARRAY || !redim || !v->arr) {
                // if array data is dim or redim without a dim then create a new one (clear the old)
                if (v->arr){
                    v->arr->datavector.clear();
                }else{
                    v->arr = new VariableArrayPart;
                }
                v->arr->datavector.resize(size);
				for(int i=0;i<size;i++) {
                    v->arr->datavector[i].intval = varnum;
				}
                v->data.type = T_ARRAY;
            }else{
				// redim - resize the vector
				int oldsize = v->arr->xdim * v->arr->ydim;
                v->arr->datavector.resize(size);
                for(int i=oldsize;i<size;i++) {
                    v->arr->datavector[i].intval = varnum;
                }
            }

            v->data.intval = varnum;	// put the variable number as the intval so that if it ends up on the stack we can get the original number
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
    Variable *v = get(varnum, recurselevel);

    if (v->data.type == T_ARRAY) {
        return(v->arr->xdim * v->arr->ydim);
    } else if (v->data.type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

int Variables::arraysizerows(int varnum) {
    // return number of columns of array as if it was a two dimensional array - 0 = not an array
    Variable *v = get(varnum, recurselevel);

    if (v->data.type == T_ARRAY) {
        return(v->arr->xdim);
    } else if (v->data.type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

int Variables::arraysizecols(int varnum) {
    // return number of rows of array as if it was a two dimensional array - 0 = not an array
    Variable *v = get(varnum, recurselevel);

    if (v->data.type == T_ARRAY) {
        return(v->arr->ydim);
    } else if (v->data.type==T_UNASSIGNED){
        error->q(ERROR_VARNOTASSIGNED, varnum);
    } else {
        error->q(ERROR_NOTARRAY, varnum);
    }
    return(0);
}

DataElement* Variables::arraygetdata(int varnum, int x, int y) {
	// get data from array elements in v from map (using x, y)
	// if there is an error return an unassigned value
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            return &v->arr->datavector[i];
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
    return &v->data;
}

void Variables::arraysetdata(int varnum, int x, int y, DataElement *e) {
	// DataElement's data is copied to the variable and it should be deleted
	// by whomever created it
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            v->arr->datavector[i].copy(e);
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, long l) {
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            v->arr->datavector[i].type = T_INT;
            v->arr->datavector[i].intval = l;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, double f) {
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            v->arr->datavector[i].type = T_FLOAT;
            v->arr->datavector[i].floatval = f;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arraysetdata(int varnum, int x, int y, QString s) {
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            v->arr->datavector[i].type = T_STRING;
            v->arr->datavector[i].stringval = s;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arrayunassign(int varnum, int x, int y) {
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
			int i = x * v->arr->ydim + y;
            v->arr->datavector[i].type = T_UNASSIGNED;
            v->arr->datavector[i].intval = varnum;
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
    return globals[varnum];
}
