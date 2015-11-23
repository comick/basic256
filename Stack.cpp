#include "Stack.h"
#include "Types.h"
#include <string>

// be doubly sure that when a stack element (DataElement) is popped from
// the stack and NOT PUT BACK it MUST be deleted to keep from causing a memory
// leak!!!!  - 2013-03-25 j.m.reneau

Stack::Stack() {
    errornumber = ERROR_NONE;
    typeconverror = SETTINGSTYPECONVDEFAULT;
    decimaldigits = SETTINGSDECDIGSDEFAULT;
}

Stack::~Stack() {
    clear();
}


void Stack::settypeconverror(int e) {
// TypeConvError	0- report no errors
// 					1 - report problems as warning
// 					2 - report problems as an error
	typeconverror = e;
}


void Stack::setdecimaldigits(int e) {
// DecimalDigits	when converting a double to a string how many decinal
//					digits will we display default 12 maximum should not
//					exceed 15, 14 to be safe
	decimaldigits = e;
}

int Stack::gettypeconverror() {
	return typeconverror;
}

void
Stack::clear() {
    DataElement *ele;
    while(!stacklist.empty()) {
        ele = stacklist.front();
        stacklist.pop_front();
        delete(ele);
    }
    errornumber = ERROR_NONE;
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

int Stack::error() {
    // return error
    return errornumber;
}

void Stack::clearerror() {
    // clear error
    errornumber = ERROR_NONE;
}

void Stack::pushelement(b_type type, double floatval, QString stringval) {
	// create a new data element and push to stack
    DataElement *ele = new DataElement(type, floatval, stringval);
    stacklist.push_front(ele);
}

void Stack::pushelement(DataElement *source) {
	// create a new data element and push to stack
    DataElement *ele = new DataElement(source);
    stacklist.push_front(ele);
}


void
Stack::pushstring(QString string) {
    pushelement(T_STRING, 0, string);
}

void
Stack::pushint(int i) {
    pushelement(T_FLOAT, i, QString());
}

void
Stack::pushfloat(double d) {
    pushelement(T_FLOAT, d, QString());
}

int Stack::peekType() {
	return peekType(0);
}

int Stack::peekType(int i) {
	if (stacklist.empty()) {
		errornumber = ERROR_STACKUNDERFLOW;
		return T_UNUSED;
	}
	if (i>0) {
		// need to iterate down the stack to peek out value
		std::list<DataElement*>::iterator it = stacklist.begin();
		while(i>0) {
			i--;
			it++;
			if ( it == stacklist.end()) {
				errornumber = ERROR_STACKUNDERFLOW;
				return T_UNUSED;
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
        errornumber = ERROR_STACKUNDERFLOW;
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

void Stack::seterror(int status) {
	// take status from DataElement.get* and set the stack error/warning as needed
	// PRIVATE
	if(status!=DataElement::STATUSOK) {
		if (status==DataElement::STATUSERROR) {
			if (typeconverror==SETTINGSTYPECONVWARN) errornumber = WARNING_TYPECONV;
			if (typeconverror==SETTINGSTYPECONVERROR) errornumber = ERROR_TYPECONV;
		}
		if (status==DataElement::STATUSUNUSED) {
			if (typeconverror==SETTINGSTYPECONVWARN) errornumber = WARNING_VARNOTASSIGNED;
			if (typeconverror==SETTINGSTYPECONVERROR) errornumber = ERROR_VARNOTASSIGNED;
		}
	}
}


int Stack::popint() {
	int status;
	DataElement *top=popelement();
	int i = top->getint(&status);
	seterror(status);
	delete(top);
	return i;
}

double Stack::popfloat() {
	int status;
	DataElement *top=popelement();
	double f = top->getfloat(&status);
	seterror(status);
	delete(top);
	return f;
}

QString Stack::popstring() {
	int status;
	DataElement *top=popelement();
	QString s = top->getstring(&status, decimaldigits);
	seterror(status);
	delete(top);
	return s;
}

int Stack::compareFloats(double one, double two) {
    // return 1 if one>two  0 if one==two or -1 if one<two
    // USE FOR ALL COMPARISON WITH NUMBERS
    // used a small number (epsilon) to make sure that
    // decimal precission errors are ignored
    if (fabs(one - two)<=BASIC256EPSILON) return 0;
    else if (one < two) return -1;
    else return 1;
}

int Stack::compareTopTwo() {
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	int status;
	DataElement *two = popelement();
	DataElement *one = popelement();
	if (one->type == T_STRING && two->type == T_STRING) {
		// if both are strings - [compare them as strings] strcmp
		QString stwo = two->getstring(&status,decimaldigits);
		seterror(status);
		QString sone = one->getstring(&status,decimaldigits);
		seterror(status);
		int ans = sone.compare(stwo);
		if (ans==0) return 0;
		else if (ans<0) return -1;
		else return 1;
	} else {
		// if either a number then compare as numbers
		double ftwo = two->getfloat(&status);
		seterror(status);
		double fone = one->getfloat(&status);
		seterror(status);
		return compareFloats(fone, ftwo);
	}
}
