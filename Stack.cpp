#include "Stack.h"

Stack::Stack()
{
	top = bottom = (stackval *) malloc (sizeof(stackval) * initialSize);
	limit = bottom + initialSize;
	top->type = T_UNUSED;
	top->floatval = 0;
}

Stack::~Stack()
{
	free(bottom);
}


void
Stack::clear()
{
	while (top > bottom)
	{
		top->string = NULL;
		top->type = T_UNUSED;
		top->floatval = 0;
		top--;
	}
	top->string = NULL;
	top->type = T_UNUSED;
	top->floatval = 0;
}

void
Stack::debug()
{
	// display the contents of the stack
	stackval *temp=top;
	while (temp >= bottom)
	{
		if (temp->type == T_STRING) printf(">>S.%s",temp->string.toUtf8().data());
        if (temp->type == T_VARREF) printf(">>V.%f",temp->floatval);
        if (temp->type == T_VARREFSTR) printf(">>VS.%f",temp->floatval);
        if (temp->type == T_FLOAT) printf(">>F.%f",temp->floatval);
		if (temp->type == T_UNUSED) printf(">>U.");
		temp--;
	}
	printf("\n");
}

int Stack::height()
{
	// return the height of the stack in elements
	// magic of pointer math returns number of elements
	return ((int) (top - bottom));
}

void
Stack::checkLimit()
{
	while (top + 1 >= limit)
	{
		limit += limit - bottom;
		stackval *newbottom = (stackval *) realloc(bottom, sizeof(stackval) * (limit - bottom));
		if (!newbottom)
		{
			exit(1);
		}
		int diff = newbottom - bottom;
		bottom = newbottom;
		top += diff;
		limit += diff;
	}
}

void
Stack::pushstring(QString c)
{
	checkLimit();
	top++;
	top->type = T_STRING;
	top->string = c;
}

void
Stack::pushint(int i)
{
	checkLimit();
	top++;
	top->type = T_FLOAT;
	top->floatval = (double) i;
}

void
Stack::pushvarref(int i)
{
    checkLimit();
    top++;
    top->type = T_VARREF;
    top->floatval = i;
}

void
Stack::pushvarrefstr(int i)
{
    checkLimit();
    top++;
    top->type = T_VARREFSTR;
    top->floatval = i;
}

void
Stack::pushfloat(double d)
{
	checkLimit();
	top++;
	top->type = T_FLOAT;
	top->floatval = d;
}

int Stack::peekType()
{
	return top->type;
}

stackval *
Stack::pop()
{
	stackval *temp = top;
	if (top!=bottom) {
		top--;
	}
	return temp;
}

void Stack::swap2()
{
	// swap top two pairs of elements
	stackval temp;
	stackval *one = top - 1;
	stackval *two = top - 2;
	stackval *three = top - 3;
	
	temp.type = two->type;
	temp.floatval = two->floatval;
	temp.string = two->string;
	two->type = top->type;
	two->floatval = top->floatval;
	two->string = top->string;
	top->type = temp.type;
	top->floatval = temp.floatval;
	top->string = temp.string;

	temp.type = three->type;
	temp.floatval = three->floatval;
	temp.string = three->string;
	three->type = one->type;
	three->floatval = one->floatval;
	three->string = one->string;
	one->type = temp.type;
	one->floatval = temp.floatval;
	one->string = temp.string;
}

void Stack::swap()
{
	// swap top two elements
	stackval temp;
	stackval *two = top - 1;
	
	temp.type = two->type;
	temp.floatval = two->floatval;
	temp.string = two->string;
	
	two->type = top->type;
	two->floatval = top->floatval;
	two->string = top->string;

	top->type = temp.type;
	top->floatval = temp.floatval;
	top->string = temp.string;
}

void 
Stack::topto2()
{
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...

	stackval temp;
	stackval *two = top - 1;
	stackval *three = top - 2;
	
	temp.type = top->type;
	temp.floatval = top->floatval;
	temp.string = top->string;
	
	top->type = two->type;
	top->floatval = two->floatval;
	top->string = two->string;

	two->type = three->type;
	two->floatval = three->floatval;
	two->string = three->string;

	three->type = temp.type;
	three->floatval = temp.floatval;
	three->string = temp.string;
}

void Stack::dup() {
	dup(0);
}

void Stack::dup2() {
	dup(1);
	dup(1);
}

void Stack::dup(int n) {
	// internal duplicate entry 0-top n-ndown
	stackval *orig = top - n;
    if (orig->type==T_VARREF) {
        pushvarref(orig->floatval);
    }
    if (orig->type==T_VARREFSTR) {
        pushvarrefstr(orig->floatval);
    }
    if (orig->type==T_FLOAT) {
		pushfloat(orig->floatval);
	}
	if (orig->type==T_STRING) {
		pushstring(orig->string);
	}
}

int 
Stack::popint()
{
	int i=0;
	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		i = (int) top->floatval;
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		i = top->string.toInt(&ok,10);
		top->string = NULL;
		top->type = T_UNUSED;
	}
	if (top > bottom) top--;
	return i;
}

double 
Stack::popfloat()
{
	double f=0;
	if (top->type == T_FLOAT || top->type == T_VARREF || top->type == T_VARREFSTR) {
		f = top->floatval;
	}
	else if (top->type == T_STRING)
	{
		bool ok;
		f = top->string.toDouble(&ok);
		top->string = NULL;
		top->type = T_UNUSED;
	}
	if (top > bottom) top--;
	return f;
}

char* 
Stack::popstring()
{
	QString s;
	// don't forget to free() the string returned by this function when you are done with it
	if (top->type == T_STRING) {
		s = top->string;
		top->string = NULL;
	}
    else if (top->type == T_VARREF || top->type == T_VARREFSTR)
	{
		s.setNumber(top->floatval,"f",0);
	}
	else if (top->type == T_FLOAT)
	{
		double xp = log10(top->floatval*(top->floatval<0?-1:1));
		if (xp<-6.0 || xp>10.0) {
			s.setNumber(top->floatval,"g",10);
		} else {
			int decimal = 12-xp;		// show up to 12 digits of precission
			if (decimal>7) decimal=7;	// up to 7 decimal digits
			s.setNumber(top->floatval,"f",decimal);
			//strip trailing zeros and decimal point
			//while(buffer[strlen(buffer)-1]=='0') buffer[strlen(buffer)-1] = 0x00;
			//if(buffer[strlen(buffer)-1]=='.') buffer[strlen(buffer)-1] = 0x00;
		}
	}


	top->type = T_UNUSED;
	if (top > bottom) top--;
	return s;
}

int Stack::compareTopTwo()
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	stackval *two = top;
	stackval *one = top - 1;

	if (one->type == T_STRING || two->type == T_STRING)
	{
		// one or both strings - [compare them as strings] strcmp
		QString sone, stwo;
		int i;
		stwo = popstring();
		sone = popstring();
		i = sone.compare(stwo);
		return i;
	} else {
		// anything else - compare them as floats
		double fone, ftwo;
		ftwo = popfloat();
		fone = popfloat();
		if (fone == ftwo) return 0;
		else if (fone < ftwo) return -1;
		else return 1;
	}
	// default equal
	return 0;
}
