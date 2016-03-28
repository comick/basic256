#include "Stack.h"
#include "DataElement.h"
#include <string>

// be doubly sure that when a stack element (DataElement) is popped from
// the stack and NOT PUT BACK it MUST be deleted to keep from causing a memory
// leak!!!!  - 2013-03-25 j.m.reneau

Stack::Stack(Error *e, Convert *c) {
	error = e;	// save error object as private pointer
	convert = c;

}

Stack::~Stack() {
    clear();
}


void Stack::clear() {
    DataElement *ele;
    while(!stacklist.empty()) {
        ele = stacklist.front();
        stacklist.pop_front();
        delete(ele);
    }
}

QString Stack::debug() {
    // return a string representing the stack
    QString s("");
    DataElement *ele;
    for (std::list<DataElement*>::iterator it = stacklist.begin(); it != stacklist.end(); it++) {
        ele = *it;
        s += ele->debug() +  " ";
    }
    return s;
}

int Stack::height() {
    // return the height of the stack in elements
    // magic of pointer math returns number of elements
    return stacklist.size();
}


void Stack::pushdataelement(DataElement *source) {
	// push to stack a duplicate or a  new nothing
	DataElement *ele;
	if (source) {
		ele = new DataElement(source);
	} else {
		ele = new DataElement();
	}
    stacklist.push_front(ele);
}


void Stack::pushstring(QString string) {
    DataElement *ele = new DataElement(string);
    stacklist.push_front(ele);
}

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
	
void Stack::pushint(int i) {
    pushlong((long) i);
}

void Stack::pushlong(long i) {
    DataElement *ele = new DataElement(i);
    stacklist.push_front(ele);
}

void Stack::pushvarref(int i) {
    DataElement *ele = new DataElement();
    ele->type = T_REF;
    ele->intval = (long) i;
    stacklist.push_front(ele);
}

void Stack::pushfloat(double d) {
    DataElement *ele = new DataElement(d);
    stacklist.push_front(ele);
}

int Stack::peekType() {
	return peekType(0);
}

int Stack::peekType(int i) {
	if (stacklist.empty()) {
		error->q(ERROR_STACKUNDERFLOW);
		return T_UNASSIGNED;
	}
	if (i>0) {
		// need to iterate down the stack to peek out value
		std::list<DataElement*>::iterator it = stacklist.begin();
		while(i>0) {
			i--;
			it++;
			if ( it == stacklist.end()) {
				error->q(ERROR_STACKUNDERFLOW);
				return T_UNASSIGNED;
			}
		}
		return (*it)->type;
	} else {
		// peek at top element
		return stacklist.front()->type;
	}
}

DataElement *Stack::popelement() {
    // pop an element but if there is not one on the stack
    // pop a zero and set the error to underflow
    DataElement *e;
    if (stacklist.empty()) {
        error->q(ERROR_STACKUNDERFLOW);
        pushint(0);
    }
    e = stacklist.front();
    stacklist.pop_front();
    return e;
}

void Stack::swap2() {
    // swap top two pairs of elements
    DataElement *zero = popelement();
    DataElement *one = popelement();
    DataElement *two = popelement();
    DataElement *three = popelement();

    stacklist.push_front(one);
    stacklist.push_front(zero);
    stacklist.push_front(three);
    stacklist.push_front(two);
}

void Stack::swap() {
    // swap top two elements
    DataElement *zero = popelement();
    DataElement *one = popelement();
    stacklist.push_front(zero);
    stacklist.push_front(one);
}

void
Stack::topto2() {
    // move the top of the stack under the next two
    // 0, 1, 2, 3...  becomes 1, 2, 0, 3...
    DataElement *zero = popelement();
    DataElement *one = popelement();
    DataElement *two = popelement();
    stacklist.push_front(zero);
    stacklist.push_front(two);
    stacklist.push_front(one);
}

void Stack::dup() {
    // make copy of top
    DataElement *zero = popelement();
    DataElement *ele = new DataElement(zero);
    stacklist.push_front(ele);
    stacklist.push_front(zero);
}

void Stack::dup2() {
    DataElement *zero = popelement();
    DataElement *one = popelement();
    stacklist.push_front(one);
    stacklist.push_front(zero);
    // make copies of one and zero to dup
    DataElement *ele = new DataElement(one);
    stacklist.push_front(ele);
    ele = new DataElement(zero);
    stacklist.push_front(ele);
}


int Stack::popint() {
	DataElement *top=popelement();
	long i = convert->getInt(top);
	delete(top);
	return i;
}

long Stack::poplong() {
	DataElement *top=popelement();
	long i = convert->getLong(top);
	delete(top);
	return i;
}

double Stack::popfloat() {
	DataElement *top=popelement();
	double f = convert->getFloat(top);
	delete(top);
	return f;
}

QString Stack::popstring() {
	DataElement *top=popelement();
	QString s = convert->getString(top);
	delete(top);
	return s;
}
