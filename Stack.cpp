#include "Stack.h"
#include <string>

// be doubly sure that when a stack element (stackdata) is popped from
// the stack and NOT PUT BACK it MUST be deleted to keep from causing a memory
// leak!!!!  - 2013-03-25 j.m.reneau

Stack::Stack()
{
	errornumber = ERROR_NONE;
}

Stack::~Stack()
{
	clear();
}


void
Stack::clear()
{
	stackdata *ele;
	while(!stacklist.empty()) {
		 ele = stacklist.front();
		 stacklist.pop_front();
		 delete(ele);
	}
	errornumber = ERROR_NONE;
}

QString Stack::debug()
{
	// return a string representing the stack
	QString s("");
	stackdata *ele;
	for (std::list<stackdata*>::iterator it = stacklist.begin(); it != stacklist.end(); it++) {
		ele = *it;
		if(ele->type==T_FLOAT) s += "float(" + QString::number(ele->floatval) + ") ";
		if(ele->type==T_STRING) s += "string(" + ele->string + ") ";
		if(ele->type==T_BOOL) s += "bool=" + QString::number(ele->floatval) + " ";
		if(ele->type==T_ARRAY) s += "array=" + QString::number(ele->floatval) + " ";
		if(ele->type==T_STRARRAY) s += "strarray=" + QString::number(ele->floatval) + " ";
		if(ele->type==T_UNUSED) s += "unused ";
		if(ele->type==T_VARREF) s += "varref=" + QString::number(ele->floatval) + " ";
		if(ele->type==T_VARREFSTR) s += "varrefstr=" + QString::number(ele->floatval) + " ";
	}
	return s;
}

int Stack::height()
{
	// return the height of the stack in elements
	// magic of pointer math returns number of elements
	return stacklist.size();
}

int Stack::error()
{
	return errornumber;
}

void
Stack::pushstring(QString string)
{
	stackdata *ele = new stackdata;
	ele->type = T_STRING;
	ele->string = string;
	stacklist.push_front(ele);
}

void
Stack::pushint(int i)
{
	stackdata *ele = new stackdata;
	ele->type = T_FLOAT;
	ele->floatval = (double) i;
	stacklist.push_front(ele);
}

void
Stack::pushvarref(int i)
{
	stackdata *ele = new stackdata;
    ele->type = T_VARREF;
    ele->floatval = i;
	stacklist.push_front(ele);
}

void
Stack::pushvarrefstr(int i)
{
	stackdata *ele = new stackdata;
    ele->type = T_VARREFSTR;
    ele->floatval = i;
	stacklist.push_front(ele);
}

void
Stack::pushfloat(double d)
{
	stackdata *ele = new stackdata;
	ele->type = T_FLOAT;
	ele->floatval = d;
	stacklist.push_front(ele);
}

int Stack::peekType()
{
	if (stacklist.empty()) {
		errornumber = ERROR_STACKUNDERFLOW;
		return T_FLOAT;
	} else {
		stackdata *ele = stacklist.front();
		return ele->type;
	}
}

stackdata *Stack::popelement() {
	// pop an element but if there is not one on the stack
	// pop a zero and set the error to underflow
	stackdata *e;
	if (stacklist.empty()) {
		errornumber = ERROR_STACKUNDERFLOW;
		pushint(0);
	}
	e = stacklist.front();
	stacklist.pop_front();
	return e;
}

void Stack::swap2()
{
	// swap top two pairs of elements
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackdata *two = popelement();
	stackdata *three = popelement();
	
	stacklist.push_front(one);
	stacklist.push_front(zero);
	stacklist.push_front(three);
	stacklist.push_front(two);
}

void Stack::swap()
{
	// swap top two elements
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stacklist.push_front(zero);
	stacklist.push_front(one);
}

void 
Stack::topto2()
{
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackdata *two = popelement();
	stacklist.push_front(zero);
	stacklist.push_front(two);
	stacklist.push_front(one);
}

void Stack::dup() {
	// make copy of top
	stackdata *zero = popelement();
	stackdata *ele = new stackdata;
	ele->type = zero->type;
	ele->floatval = zero->floatval;
	ele->string = zero->string;
	stacklist.push_front(ele);
	stacklist.push_front(zero);
}

void Stack::dup2() {
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stacklist.push_front(one);
	stacklist.push_front(zero);
	// make copies of one and zero to dup
	stackdata *ele = new stackdata;
	ele->type = one->type;
	ele->floatval = one->floatval;
	ele->string = one->string;
	stacklist.push_front(ele);
	ele = new stackdata;
	ele->type = zero->type;
	ele->floatval = zero->floatval;
	ele->string = zero->string;
	stacklist.push_front(ele);
}

int 
Stack::popint()
{
	int i=0;
	stackdata *top=popelement();
	
	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		i = (int) (top->floatval + (top->floatval>0?BASIC256EPSILON:-BASIC256EPSILON));
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		i = top->string.toInt(&ok);
	}
	delete(top);
	return i;
}

double 
Stack::popfloat()
{
	double f=0;
	stackdata *top=popelement();

	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		f = top->floatval;
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		f = top->string.toDouble(&ok);
	}
	delete(top);
	return f;
}

QString Stack::popstring()
{
	stackdata *top=popelement();
	QString s;

	if (top->type == T_STRING) {
		s = top->string;
	}
    else if (top->type == T_VARREF || top->type == T_VARREFSTR)
	{
		s = QString::number(top->floatval,'f',0);
	}
	else if (top->type == T_FLOAT)
	{
		double xp = log10(top->floatval*(top->floatval<0?-1:1));
		if (xp<-6.0 || xp>10.0) {
			s = QString::number(top->floatval,'g',10);
		} else {
			int decimal = 12-xp;		// show up to 12 digits of precission
			if (decimal>7) decimal=7;	// up to 7 decimal digits
			//char buffer[64];
			//sprintf(buffer, "%#.*f", decimal, top->floatval);
			//QString s = QString::fromUtf8(buffer);
			s = QString::number(top->floatval,'f',decimal);
			//strip trailing zeros and decimal point
			while(s.endsWith("0")) s.chop(1);
			if(s.endsWith(".")) s.chop(1);
		}
	}
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

int Stack::compareTopTwo()
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	stackdata *two = popelement();
	stackdata *one = popelement();
	stacklist.push_front(one);
	stacklist.push_front(two);
	
	if (one->type == T_STRING || two->type == T_STRING)
	{
		// one or both strings - [compare them as strings] strcmp
		QString stwo = popstring();
		QString sone = popstring();
		int ans = sone.compare(stwo);
		if (ans==0) return 0;
		else if (ans<0) return -1;
		else return 1;
	} else {
		// anything else - compare them as doubles
		double ftwo = popfloat();
		double fone = popfloat();
		return compareFloats(fone, ftwo);
	}
}
