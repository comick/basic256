#include "Variables.h"
#include <QDebug>

// manage storage of variables

// variables are limited to types (defined in Types.h):

// T_REF is a variable number in the specified recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable() {
	data = new DataElement;
}

Variable::~Variable() {
	delete(data);
}


// ************************************************************************
// * Variables contain the individual variable(s) for the recursion level *
// ************************************************************************

Variables::Variables(int n) :  numsyms(n){
	// n is the size of the symbol table
    recurselevel = 0;
	maxrecurselevel = -1;
	allocateRecurseLevel();
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
			s += QStringLiteral("recurse Level ") + QString::number(i) + "\n";
			for(int j=0; j < numsyms; j++) {
				s += QStringLiteral(" varnum ") + QString::number(j) + QStringLiteral(" ") ;
				s += varmap[i][j]->data->debug();
				s += "\n";
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
            varmap[recurselevel][i] = new Variable();
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
		varmap[level][i]->data->clear();
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
	if(isglobal[varnum]) level=0;
	v = varmap[level][varnum];
	real_varnum = varnum;
	real_level = level;
	if(v->data->type==T_REF) {
		// reference to other level - recurse down the rabitty hole
		return get(v->data->intval, v->data->level);
	} else {
		return(v);
	}
}

Variable* Variables::get(const int varnum) {
    return get(varnum, recurselevel);
}

Variable* Variables::getAt(const int varnum, const int level) {
	// get variable from map without follow REF
	// need for variable window too
	real_varnum = varnum;
	if(isglobal[varnum]){
		real_level = 0;
		return(varmap[0][varnum]);
	}else{
		real_level = level;
		return(varmap[level][varnum]);
	}
}

Variable* Variables::getAt(const int varnum) {
	// get variable without recurse from this level
	return getAt(varnum, recurselevel);
}

DataElement* Variables::getdata(const int varnum) {
	// throw unassigned error by default
	return getdata(varnum,false);
}

DataElement* Variables::getdata(const int varnum, const bool unassignederror) {
	// get data from variable - return varnum's data
	Variable *v = get(varnum);
	if (unassignederror && v->data->type==T_UNASSIGNED){
		error->q(ERROR_VARNOTASSIGNED, varnum);
	}
	return v->data;
}


void Variables::setdata(const int varnum, DataElement* e) {
    // recieves a DataElement pointed pulled from the stack
    // e is a pointer in the stack vector AND MUST NOT BE DELETED
	Variable *v;
	if (e->type==T_REF) {
		// if we are assigning a "REF" dont recurse down the previous ref is there is one
		// just assign it at the current run level.
		v = getAt(varnum);
	} else {
		v = get(varnum, recurselevel);
	}
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

void Variables::unassign(const int varnum) {
    //unassign true variable, not the reference (unlink)
    Variable *v = varmap[(recurselevel==0||isglobal[varnum])?0:recurselevel][varnum];
    v->data->clear();
}

void Variables::makeglobal(const int varnum) {
    // make a variable global - if there is a value in a run level greater than 0 then
    // that value will be lost to the program
    isglobal[varnum] = true;
}
