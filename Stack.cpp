#include "Stack.h"
#include "DataElement.h"
#include <string>


Stack::Stack(Convert *c, QLocale *applocale) {
	convert = c;
	stackpointer = 0;	// height of stack
    stacksize = 0;       //max size of stack to avoid calling stackdata.size()
	stackGrow();
	locale = applocale;

}

Stack::~Stack() {
	for(unsigned int i = 0; i< stackdata.size(); i++) {
		delete(stackdata[i]);
		stackdata[i] = NULL;
	}
	stackdata.clear();
}

void Stack::stackGrow() {
	// add 10 elements to the size of the stack
    int i = stacksize;
    stackdata.resize(stacksize+10);
    stacksize=stackdata.size();
    while(i< stacksize) {
		stackdata[i] = new DataElement();
        i++;
	}
}

QString Stack::debug() {
    // return a string representing the stack
    QString s("");
    for (int i=0; i<stackpointer; i++) {
        s += stackdata[i]->debug() +  " ";
    }
    return s;
}

int Stack::height() {
    // return the height of the stack in elements
    // magic of pointer math returns number of elements
    return stackpointer;
}

//
// RAW Push Operations
//

void Stack::pushdataelement(DataElement *source) {
    if (stackpointer >= stacksize)  stackGrow();
	// push to stack a copy of he dataelement
	if (source) {
		stackdata[stackpointer]->copy(source);
	} else {
		stackdata[stackpointer]->type = T_UNASSIGNED;
	}
	stackpointer++;
}

void Stack::pushlong(long i) {
    if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer]->type = T_INT;
	stackdata[stackpointer]->intval = i;
	stackpointer++;
}

void Stack::pushvarref(int i, int level) {
    if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer]->type = T_REF;
	stackdata[stackpointer]->intval = i;
    stackdata[stackpointer]->level = level;
    stackpointer++;
}

void Stack::pushfloat(double d) {
    if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer]->type = T_FLOAT;
	stackdata[stackpointer]->floatval = d;
	stackpointer++;
}

void Stack::pushstring(QString string) {
    if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer]->type = T_STRING;
	stackdata[stackpointer]->stringval = string;
	stackpointer++;
}

void Stack::pushint(int i) {
    if (stackpointer >= stacksize)  stackGrow();
    stackdata[stackpointer]->type = T_INT;
    stackdata[stackpointer]->intval = (long)i;
    stackpointer++;
}

void Stack::pushbool(bool i) {
    if (stackpointer >= stacksize)  stackGrow();
    stackdata[stackpointer]->type = T_INT;
    if (i)
        stackdata[stackpointer]->intval = 1;
    else
        stackdata[stackpointer]->intval = 0;
    stackpointer++;
}


//
// Pushes derived from RAW pushes
//

void Stack::pushvariant(QString string, int type) {
	// try to convert a string to an int or float and push that type
	// if unable then push a string
	switch (type) {
		case T_UNASSIGNED:
			{
				bool ok;
				long i;
				i = string.toLong(&ok);
				if (ok) {
					pushlong(i);
				} else {
					double d;
					d = locale->toDouble(string,&ok);
					if (ok) {
						pushfloat(d);
					} else {
						// not an integer or double - push string
						pushstring(string);
					}
				}
			}
			break;
		case T_INT:
			{
				bool ok;
				long i=0;
				i = string.toLong(&ok);
				if (!ok) {
					error->q(ERROR_TYPECONV);
				}
				pushlong(i);
			}
			break;
		case T_FLOAT:
			{
				bool ok;
				double d=0.0;
				d = locale->toDouble(string,&ok);
				if (!ok) {
					error->q(ERROR_TYPECONV);
				}
				pushfloat(d);
			}
			break;
		case T_STRING:
			{
				pushstring(string);
			}
			break;
	}
}
	


//
// Peek Operations - look but dont touch

int Stack::peekType() {
	return peekType(0);
}

int Stack::peekType(int i) {
	if (stackpointer<=i) {
		error->q(ERROR_STACKUNDERFLOW);
		return T_UNASSIGNED;
	}
	return stackdata[stackpointer - i - 1]->type;
}

//
// Raw Pop Operations

DataElement *Stack::popelement() {
    // pop an element - a POINTER to the data on the stack
    // WILL CHANGE ON NEXT PUSH!!!!
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        // return a fake element instead of NULL
        // to handle a potential error in Interpreter
        stackdata[0]->type = T_INT;
        stackdata[0]->intval = 0l;
        return stackdata[0];
    }
	stackpointer--;
	return stackdata[stackpointer];
}

int Stack::popbool() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return 0;
    }
    return convert->getBool(stackdata[--stackpointer]);
}

int Stack::popint() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return 0;
    }
    return convert->getInt(stackdata[--stackpointer]);
}

long Stack::poplong() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return 0;
    }
    return convert->getLong(stackdata[--stackpointer]);
}

double Stack::popfloat() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return 0.0;
    }
    return convert->getFloat(stackdata[--stackpointer]);
}

double Stack::popnote() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return 0.0;
    }
    return convert->getMusicalNote(stackdata[--stackpointer]);
}

QString Stack::popstring() {
    if (stackpointer==0) {
        error->q(ERROR_STACKUNDERFLOW);
        return QString("");
    }
    return convert->getString(stackdata[--stackpointer]);
}

//
// SWAP and DUP opeations to the stack

void Stack::swap2() {
    // swap top two pairs of elements
    // if top of stack is A,B,C,D make it C,D,A,B
    DataElement *t;

	if (stackpointer<4) {
		error->q(ERROR_STACKUNDERFLOW);
		return;
	}
    
    t = stackdata[stackpointer-3];
    stackdata[stackpointer-3] = stackdata[stackpointer-1];
    stackdata[stackpointer-1] = t;

    t = stackdata[stackpointer-4];
    stackdata[stackpointer-4] = stackdata[stackpointer-2];
    stackdata[stackpointer-2] = t;
}

void Stack::swap() {
    // swap top two elements
    // if top of stack is A,B,C,D make it B,A,C,D
    DataElement *t;

	if (stackpointer<2) {
		error->q(ERROR_STACKUNDERFLOW);
		return;
	}
    
    t = stackdata[stackpointer-2];
    stackdata[stackpointer-2] = stackdata[stackpointer-1];
    stackdata[stackpointer-1] = t;
}

void
Stack::topto2() {
    // move the top of the stack under the next two
    // 0, 1, 2, 3...  becomes 1, 2, 0, 3...
    DataElement *t;

	if (stackpointer<3) {
		error->q(ERROR_STACKUNDERFLOW);
		return;
	}
    
    t = stackdata[stackpointer-1];
    stackdata[stackpointer-1] = stackdata[stackpointer-2];
    stackdata[stackpointer-2] = stackdata[stackpointer-3];
    stackdata[stackpointer-3] = t;
}

void Stack::dup() {
    // make copy of top
    // if top of stack is A,B,C,D make it A,A,B,C,D
	if (stackpointer<1) {
		error->q(ERROR_STACKUNDERFLOW);
		return;
	}
    pushdataelement(stackdata[stackpointer-1]);
}

void Stack::dup2() {
    // make copy of top two
    // if top of stack is A,B,C,D make it A,B,A,B,C,D
	if (stackpointer<2) {
		error->q(ERROR_STACKUNDERFLOW);
		return;
	}
    pushdataelement(stackdata[stackpointer-2]);
    pushdataelement(stackdata[stackpointer-2]);
}

void Stack::drop(int n){
    //quick drop a number of elements from stack
    //usefull to clear the stack when an array from stack is not needed anymore
    //in case that error is catched and we want to pass over that (ONERROR or TRY/CATCH)
    stackpointer-=n;
    if (stackpointer<0) {
        stackpointer=0;
        error->q(ERROR_STACKUNDERFLOW);
    }
}
