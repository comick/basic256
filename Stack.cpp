#include "Stack.h"
#include "DataElement.h"
#include <string>

// be doubly sure that when a stack element (DataElement) is popped from
// the stack and NOT PUT BACK it MUST be deleted to keep from causing a memory
// leak!!!!  - 2013-03-25 j.m.reneau

Stack::Stack(Error *e, Convert *c) {
	error = e;	// save error object as private pointer
	convert = c;
	stackpointer = 0;	// height of stack
	stackGrow();
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
	int oldsize = stackdata.size();
	stackdata.resize(oldsize+10);
	for(unsigned int i = oldsize; i< stackdata.size(); i++) {
		stackdata[i] = new DataElement();
	}
}

QString Stack::debug() {
    // return a string representing the stack
    QString s("");
    for (unsigned int i=0; i<stackdata.size(); i++) {
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
	if (stackpointer >= stackdata.size())  stackGrow();
	// push to stack a copy of he dataelement
	if (source) {
		stackdata[stackpointer]->copy(source);
	} else {
		stackdata[stackpointer]->type = T_UNASSIGNED;
	}
	stackpointer++;
}

void Stack::pushlong(long i) {
	if (stackpointer >= stackdata.size())  stackGrow();
	stackdata[stackpointer]->type = T_INT;
	stackdata[stackpointer]->intval = i;
	stackpointer++;
}

void Stack::pushvarref(int i) {
	if (stackpointer >= stackdata.size())  stackGrow();
	stackdata[stackpointer]->type = T_REF;
	stackdata[stackpointer]->intval = i;
	stackpointer++;
}

void Stack::pushfloat(double d) {
	if (stackpointer >= stackdata.size())  stackGrow();
	stackdata[stackpointer]->type = T_FLOAT;
	stackdata[stackpointer]->floatval = d;
	stackpointer++;
}

void Stack::pushstring(QString string) {
	if (stackpointer >= stackdata.size())  stackGrow();
	stackdata[stackpointer]->type = T_STRING;
	stackdata[stackpointer]->stringval = string;
	stackpointer++;
}

//
// Pushes derived from RAW pushes
//

void Stack::pushvariant(QString string) {
	pushvariant(string, T_UNASSIGNED);
}

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
					d = string.toDouble(&ok);
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
				d = string.toDouble(&ok);
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
	
void Stack::pushbool(bool i) {
	if (i) {
		pushlong(1);
	} else {
		pushlong(0);
	}
}

void Stack::pushint(int i) {
    pushlong((long) i);
}


//
// Peek Operations - look but dont touch

int Stack::peekType() {
	return peekType(0);
}

int Stack::peekType(unsigned int i) {
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
        return NULL;
    }
	stackpointer--;
	return stackdata[stackpointer];
}

//
// Pops derivedfrom RAW pop

int Stack::popbool() {
	DataElement *top=popelement();
	return convert->getBool(top);
}

int Stack::popint() {
	DataElement *top=popelement();
	return convert->getInt(top);
}

long Stack::poplong() {
	DataElement *top=popelement();
	return convert->getLong(top);
}

double Stack::popfloat() {
	DataElement *top=popelement();
	return convert->getFloat(top);
}

QString Stack::popstring() {
	DataElement *top=popelement();
	return convert->getString(top);
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

