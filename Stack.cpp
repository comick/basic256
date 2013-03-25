#include "Stack.h"
#include <string>

Stack::Stack()
{
}

Stack::~Stack()
{
	clear();
}


void
Stack::clear()
{
	while(!stackstack.empty()) stackstack.pop();
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
	stackdata *ele = stackstack.top();
	return ele->type;
}

void Stack::swap2()
{
	// swap top two pairs of elements
	stackdata *zero = stackstack.top();
	stackstack.pop();
	stackdata *one = stackstack.top();
	stackstack.pop();
	stackdata *two = stackstack.top();
	stackstack.pop();
	stackdata *three = stackstack.top();
	stackstack.pop();
	
	stackstack.push(one);
	stackstack.push(zero);
	stackstack.push(three);
	stackstack.push(two);
}

void Stack::swap()
{
	// swap top two elements
	stackdata *zero = stackstack.top();
	stackstack.pop();
	stackdata *one = stackstack.top();
	stackstack.pop();
	stackstack.push(zero);
	stackstack.push(one);
}

void 
Stack::topto2()
{
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
	stackdata *zero = stackstack.top();
	stackstack.pop();
	stackdata *one = stackstack.top();
	stackstack.pop();
	stackdata *two = stackstack.top();
	stackstack.pop();
	stackstack.push(zero);
	stackstack.push(two);
	stackstack.push(one);
}

void Stack::dup() {
	stackdata *zero = stackstack.top();
	stackstack.push(zero);
}

void Stack::dup2() {
	stackdata *zero = stackstack.top();
	stackstack.pop();
	stackdata *one = stackstack.top();
	stackstack.pop();
	stackstack.push(one);
	stackstack.push(zero);
	stackstack.push(one);
	stackstack.push(zero);
}

int 
Stack::popint()
{
	int i=0;
	stackdata *top=stackstack.top();
	stackstack.pop();
	
	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		i = (int) top->floatval;
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		i = top->string.toInt(&ok);
	}
	return i;
}

double 
Stack::popfloat()
{
	double f=0;
	stackdata *top=stackstack.top();
	stackstack.pop();

	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		f = top->floatval;
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		f = top->string.toDouble(&ok);
	}
	return f;
}

QString Stack::popstring()
{
	stackdata *top=stackstack.top();
	stackstack.pop();

	if (top->type == T_STRING) {
		return top->string;
	}
    else if (top->type == T_VARREF || top->type == T_VARREFSTR)
	{
		return QString::number(top->floatval,'f',0);
	}
	else if (top->type == T_FLOAT)
	{
		double xp = log10(top->floatval*(top->floatval<0?-1:1));
		if (xp<-6.0 || xp>10.0) {
			return QString::number(top->floatval,'g',10);
		} else {
			int decimal = 12-xp;		// show up to 12 digits of precission
			if (decimal>7) decimal=7;	// up to 7 decimal digits
			//char buffer[64];
			//sprintf(buffer, "%#.*f", decimal, top->floatval);
			//QString s = QString::fromUtf8(buffer);
			QString s = QString::number(top->floatval,'f',decimal);
			//strip trailing zeros and decimal point
			while(s.endsWith("0")) s.chop(1);
			if(s.endsWith(".")) s.chop(1);
			return s;
		}
	}
	return QString("");
}

int Stack::compareTopTwo()
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	stackdata *two = stackstack.top();
	stackstack.pop();
	stackdata *one = stackstack.top();
	stackstack.pop();
	stackstack.push(one);
	stackstack.push(two);
	
	if (one->type == T_STRING || two->type == T_STRING)
	{
		// one or both strings - [compare them as strings] strcmp
		QString stwo = popstring();
		QString sone = popstring();
		return sone.compare(stwo);
	} else {
		// anything else - compare them as doubles
		double ftwo = popfloat();
		double fone = popfloat();
		if (fone == ftwo) return 0;
		else if (fone < ftwo) return -1;
		else return 1;
	}
	// default equal
	return 0;
}
