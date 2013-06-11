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
	while(!stackstack.empty()) {
		 ele = stackstack.top();
		 stackstack.pop();
		 delete(ele);
	}
	errornumber = ERROR_NONE;
}

void
Stack::debug()
{
	// display the contents of the stack
}

int Stack::height()
{
	// return the height of the stack in elements
	// magic of pointer math returns number of elements
	return stackstack.size();
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
	stackstack.push(ele);
}

void
Stack::pushint(int i)
{
	stackdata *ele = new stackdata;
	ele->type = T_FLOAT;
	ele->floatval = (double) i;
	stackstack.push(ele);
}

void
Stack::pushvarref(int i)
{
	stackdata *ele = new stackdata;
    ele->type = T_VARREF;
    ele->floatval = i;
	stackstack.push(ele);
}

void
Stack::pushvarrefstr(int i)
{
	stackdata *ele = new stackdata;
    ele->type = T_VARREFSTR;
    ele->floatval = i;
	stackstack.push(ele);
}

void
Stack::pushfloat(double d)
{
	stackdata *ele = new stackdata;
	ele->type = T_FLOAT;
	ele->floatval = d;
	stackstack.push(ele);
}

int Stack::peekType()
{
	if (stackstack.empty()) {
		errornumber = ERROR_STACKUNDERFLOW;
		return T_FLOAT;
	} else {
		stackdata *ele = stackstack.top();
		return ele->type;
	}
}

stackdata *Stack::popelement() {
	// pop an element but if there is not one on the stack
	// pop a zero and set the error to underflow
	stackdata *e;
	if (stackstack.empty()) {
		errornumber = ERROR_STACKUNDERFLOW;
		pushint(0);
	}
	e = stackstack.top();
	stackstack.pop();
	return e;
}

void Stack::swap2()
{
	// swap top two pairs of elements
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackdata *two = popelement();
	stackdata *three = popelement();
	
	stackstack.push(one);
	stackstack.push(zero);
	stackstack.push(three);
	stackstack.push(two);
}

void Stack::swap()
{
	// swap top two elements
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackstack.push(zero);
	stackstack.push(one);
}

void 
Stack::topto2()
{
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackdata *two = popelement();
	stackstack.push(zero);
	stackstack.push(two);
	stackstack.push(one);
}

void Stack::dup() {
	// make copy of top
	stackdata *zero = popelement();
	stackdata *ele = new stackdata;
	ele->type = zero->type;
	ele->floatval = zero->floatval;
	ele->string = zero->string;
	stackstack.push(ele);
	stackstack.push(zero);
}

void Stack::dup2() {
	stackdata *zero = popelement();
	stackdata *one = popelement();
	stackstack.push(one);
	stackstack.push(zero);
	// make copies of one and zero to dup
	stackdata *ele = new stackdata;
	ele->type = one->type;
	ele->floatval = one->floatval;
	ele->string = one->string;
	stackstack.push(ele);
	ele = new stackdata;
	ele->type = zero->type;
	ele->floatval = zero->floatval;
	ele->string = zero->string;
	stackstack.push(ele);
}

int 
Stack::popint()
{
	int i=0;
	stackdata *top=popelement();
	
	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		i = (int) top->floatval;
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

int Stack::compareTopTwo()
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	stackdata *two = popelement();
	stackdata *one = popelement();
	stackstack.push(one);
	stackstack.push(two);
	
	int ans = 0;	// default equal
	
	if (one->type == T_STRING || two->type == T_STRING)
	{
		// one or both strings - [compare them as strings] strcmp
		QString stwo = popstring();
		QString sone = popstring();
		ans = sone.compare(stwo);
	} else {
		// anything else - compare them as doubles
		double ftwo = popfloat();
		double fone = popfloat();
		if (fone == ftwo) ans = 0;
		else if (fone < ftwo) ans = -1;
		else ans = 1;
	}
	// 
	return ans;
}
