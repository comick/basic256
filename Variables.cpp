#include "Variables.h"
#include <QDebug>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_REF is a variable number in the specified recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable(const int v, const int level) {
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

Variables::Variables(int n, Error *e) :  numsyms(n){
	// n is the size of the symbol table
    recurselevel = 0;
    maxrecurselevel = -1;
    allocateRecurseLevel();
    error = e;
    isglobal = new bool[numsyms];
    int i=numsyms;
    while(i-- > 0) {
        isglobal[i]=false;
    }
}

Variables::~Variables() {
    // free variables and all storage
    do {
        // delete a recurse level being sure to delete
        // the variables in the runlevel before removing it
        int i=numsyms;
        while(i-- > 0) {
            delete(varmap[recurselevel][i]);
        }
        delete[] varmap[recurselevel];
        recurselevel--;
    } while (recurselevel>=0);
    varmap.clear();
    delete[] isglobal;
}

QString Variables::debug() {
    // return a string representing the variables
    QString s("");
    if(!varmap.empty()) {
        for(int i=0;i<=recurselevel;i++) {
            s += QStringLiteral("recurse Level ") + QString::number(i);
            for(int j=0; j < numsyms; j++) {
                s += QStringLiteral(" varnum ") + QString::number(j);
                s += QStringLiteral(" ") + (varmap[i][j]?varmap[i][j]->data.debug():QStringLiteral("UNUSED")) + "\n";
            }
            s += "\n";
        }
    }
    return s;
}

void Variables::allocateRecurseLevel() {
    // initialize a new recurse level
    // create a new Variable for each symbol at this recurse level
    // wasteful for the labels and jump points but vector is so fast
    // to access an element it does not matter
    if(maxrecurselevel<recurselevel){
        varmap.resize(recurselevel+1);
        varmap[recurselevel] = new Variable*[numsyms];
        int i=numsyms;
        while(i-- > 0) {
            varmap[recurselevel][i] = new Variable(i, recurselevel);
        }
        maxrecurselevel=recurselevel;
    }
}

void Variables::clearRecurseLevel() {
    // clear a recurse level
    //it can be reused faster than create/delete all variables
    const int level = recurselevel;
    int i=numsyms;
    while(i-- > 0) {
        Variable *v = varmap[level][i];
        v->data.type=T_UNASSIGNED;
        v->data.level=level; // in case we got a T_REF defined in this level
        //free also long texts - do not use QString("") or "" because of the low speed
        v->data.stringval.clear();
        //clear array if exists
        if(v->arr){
            //free memory from arrays
            delete(v->arr);
            v->arr=NULL;
        }
    }
}

void Variables::increaserecurse() {
    recurselevel++;
    if (recurselevel>=MAX_RECURSE_LEVELS) {
        recurselevel--;
        error->q(ERROR_MAXRECURSE);
    } else {
		allocateRecurseLevel();
	}
}

int Variables::getrecurse() {
    return recurselevel;
}

void Variables::decreaserecurse() {
    if (recurselevel>0) {
        // clear all variables in the current recurse level before we return
        // to the previous level
        clearRecurseLevel();
        // return back to prev variable context
        recurselevel--;
    }
}


Variable* Variables::get(const int varnum, int level) {
    // get v from map or follow REF to recurse level if needed
    Variable *v;
    if(level==0 || isglobal[varnum]){
        v = varmap[0][varnum];
        if(v->data.type!=T_REF)
            return(v);
        level=0;
    }else{
        v = varmap[level][varnum];
        if(v->data.type!=T_REF)
            return(v);
    }



    // make a stack of varnum/level needed to check for circular reference and put the original first
    std::vector<std::pair<int, int>> track = {{varnum, level}};
    std::vector<std::pair<int, int>>::iterator it;
    std::pair <int,int> item;
    int levelback = level;

    do{
        level=v->data.level;
        if(level!=0){
            if(isglobal[v->data.intval]){
                level=0;
            }else if(level>recurselevel){
                //error: Variable points to a non-existent variable
                //this can be done by assign a global variable inside a subroutine and return to previous level
                //where we use the value of global var
                error->q(ERROR_VARNULL, varnum);
                //return the original variable to successfully pass over an OnError situation or to display a specific error message
                return varmap[levelback][varnum];
            }
        }

        // check in stack for v->data.intval/v->data.level and if it is founded then trow error for circular reference
        // store v->data.intval/v->data.level in stack

        item = std::make_pair(v->data.intval, v->data.level);
        it = std::find(track.begin(), track.end(), item);
        if (it != track.end()){
            error->q(ERROR_VARCIRCULAR, varnum);
            //return the original variable to successfully pass over an OnError situation or to display a specific error message
            return varmap[levelback][varnum];
        }
        track.push_back(item);
        v = varmap[level][v->data.intval];


    }while(v->data.type==T_REF);

    return(v);
}

Variable* Variables::get(const int varnum) {
    return get(varnum, recurselevel);
}

Variable* Variables::getRawVariable(const int varnum) {
    // get variable from map without follow REF
    // need for variable window too
    if(isglobal[varnum]){
        return(varmap[0][varnum]);
    }else{
        return(varmap[recurselevel][varnum]);
    }
}

DataElement* Variables::getdata(const int varnum) {
    // get data from variable - return varnum's data
    Variable *v = get(varnum, recurselevel);
    //the correct behaviour is to check for unassigned variable when program try to get its content
    //not when program set a variable content and try to backtrack the source of content from stack
    //to identify if this was an array element or a variable or, worse, a reference (impossible to track)
    //yes, getdata() is more often used than setdata() so this check is more expensive
    if (v->data.type==T_UNASSIGNED){
        //error
        //check if the original variable was a reference to show appropriate error message
        Variable* vv = getRawVariable(varnum);
        if(vv->data.type==T_REF)
            error->q(ERROR_REFNOTASSIGNED, varnum);
        else
            error->q(ERROR_VARNOTASSIGNED, varnum);
    }
    return &v->data;
}

DataElement* Variables::getdata(DataElement* e) {
    // get data from a REF() element pulled from stack - return final data
    Variable *v = get(e->intval, e->level);
    //the correct behaviour is to check for unassigned variable when program try to a variable content
    if (v->data.type==T_UNASSIGNED){
        error->q(ERROR_REFNOTASSIGNED, e->intval);
    }
    return &v->data;
}

void Variables::setdata(const int varnum, DataElement* e) {
    // recieves a DataElement pointed pulled from the stack
    // e is a pointer in the stack vector AND MUST NOT BE DELETED
    Variable *v = get(varnum, recurselevel);
        v->data.copy(e);

// expensive part... to have the maximum speed the best choice
// is not to try to clean the matrix part at every assing action
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, long l) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_INT;
    v->data.intval = l;
// expensive part... to have the maximum speed the best choice
// is not to try to clean the matrix part at every assing action
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, double f) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_FLOAT;
    v->data.floatval = f;
// expensive part... to have the maximum speed the best choice
// is not to try to clean the matrix part at every assing action
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::setdata(int varnum, QString s) {
    Variable *v = get(varnum, recurselevel);
    v->data.type = T_STRING;
    v->data.stringval = s;
// expensive part... to have the maximum speed the best choice
// is not to try to clean the matrix part at every assing action
//    if (v->arr){
//        delete(v->arr);
//        v->arr=NULL;
//    }
}

void Variables::unassign(const int varnum) {
    //unassign true variable, not the reference (unlink)
    Variable *v = varmap[(recurselevel==0||isglobal[varnum])?0:recurselevel][varnum];
    v->data.type = T_UNASSIGNED;
    v->data.intval = varnum;
    v->data.stringval.clear();
    v->data.level = recurselevel;
    if (v->arr){
        delete(v->arr);
        v->arr=NULL;
    }
}

void Variables::copyarray(const int varnum1, DataElement* e) {
    // fast copy arrays a[]=b[] or as argument for function/subroutine
    // already checked for e->type == T_ARRAY

    Variable *v1 = get(varnum1, recurselevel); //get the destination variable
    Variable *v2 = get(e->intval, e->level); //get the source variable (it is already checked for T_ARRAY)
    v1->data.type = T_ARRAY;
    if (!v1->arr)
        v1->arr = new VariableArrayPart;
//    else
//        v1->arr->datavector.clear();
    v1->arr->xdim = v2->arr->xdim;
    v1->arr->ydim = v2->arr->ydim;
    int i = v2->arr->xdim * v2->arr->ydim;
    v1->arr->datavector.resize(i);
    while(i-- > 0) {
        v1->arr->datavector[i].copy(&v2->arr->datavector[i], varnum1);
    }
}


void Variables::arraydim(const int varnum, const int xdim, const int ydim, const bool redim) {
    Variable *v = get(varnum, recurselevel);
    const int size = xdim * ydim;
    
    if (size <= VARIABLE_MAXARRAYELEMENTS) {
        if (size >= 1) {
            if (v->data.type != T_ARRAY || !redim || !v->arr) {
                // if array data is dim or redim without a dim then create a new one (clear the old)
                if (!v->arr){
                    v->arr = new VariableArrayPart;
                }else{
                    v->arr->datavector.clear();
                }
                v->arr->datavector.resize(size);
                int i = size;
                while(i-- > 0) {
                    v->arr->datavector[i].intval = varnum;
				}
                v->data.type = T_ARRAY;
            }else{
				// redim - resize the vector
                const int oldsize = v->arr->xdim * v->arr->ydim;
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

int Variables::arraysize(const int varnum) {
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

int Variables::arraysizerows(const int varnum) {
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

int Variables::arraysizecols(const int varnum) {
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

DataElement* Variables::arraygetdata(const int varnum, const int x, const int y) {
	// get data from array elements in v from map (using x, y)
	// if there is an error return an unassigned value
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
            const int i = x * v->arr->ydim + y;
            DataElement *e = &v->arr->datavector[i];
            //the correct behaviour is to check for unassigned content
            //when program try to use it not when it try to convert to int, string and so...
            if (e->type==T_UNASSIGNED){
                error->q(ERROR_ARRAYELEMENT, varnum);
            }
            return e;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
    return &v->data;
}

void Variables::arraysetdata(const int varnum, const int x, const int y, DataElement *e) {
	// DataElement's data is copied to the variable and it should be deleted
	// by whomever created it
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
            const int i = x * v->arr->ydim + y;
            v->arr->datavector[i].copy(e, varnum);
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::arrayunassign(const int varnum, const int x, const int y) {
    Variable *v = get(varnum, recurselevel);
    if (v->data.type == T_ARRAY) {
		if (x >=0 && x < v->arr->xdim && y >=0 && y < v->arr->ydim) {
            const int i = x * v->arr->ydim + y;
            v->arr->datavector[i].type = T_UNASSIGNED;
            v->arr->datavector[i].intval = varnum;
		} else {
			error->q(ERROR_ARRAYINDEX, varnum);
		}
	} else {
		error->q(ERROR_NOTARRAY, varnum);
	}
}

void Variables::makeglobal(const int varnum) {
    // make a variable global - if there is a value in a run level greater than 0 then
    // that value will be lost to the program
    isglobal[varnum] = true;
}
