#include "Stack.h"
#include <string>

Stack::Stack()
{
	top = bottom = (stackval *) malloc (sizeof(stackval) * initialSize);
	limit = bottom + initialSize;
	top->type = T_UNUSED;
	top->value.floatval = 0;
	fToAMask = defaultFToAMask;
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
		if (top->type == T_STRING)
		{
			free(top->value.string);
			top->value.string = NULL;
		}
		top->type = T_UNUSED;
		top--;
	}
	top->type = T_UNUSED;
	top->value.floatval = 0;
	fToAMask = defaultFToAMask;
}

void
Stack::debug()
{
	// display the contents of the stack
	stackval *temp=top;
	while (temp >= bottom)
	{
		if (temp->type == T_STRING) printf(">>S.%s",temp->value.string);
		if (temp->type == T_INT) printf(">>I.%d",temp->value.intval);
        if (temp->type == T_VARREF) printf(">>V.%d",temp->value.intval);
        if (temp->type == T_VARREFSTR) printf(">>VS.%d",temp->value.intval);
        if (temp->type == T_FLOAT) printf(">>F.%f",temp->value.floatval);
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
Stack::push(char *c)
{
	checkLimit();
	top++;
	top->type = T_STRING;
	if (c==NULL) {
		c = (char *) malloc(sizeof(char));
		c[0] = 0x00;
		top->value.string = c;
	} else {
		top->value.string = strdup(c);
	}
}

void
Stack::push(int i)
{
	checkLimit();
	top++;
	top->type = T_INT;
	top->value.intval = i;
}

void
Stack::pushvarref(int i)
{
    checkLimit();
    top++;
    top->type = T_VARREF;
    top->value.intval = i;
}

void
Stack::pushvarrefstr(int i)
{
    checkLimit();
    top++;
    top->type = T_VARREFSTR;
    top->value.intval = i;
}

void
Stack::push(double d)
{
	checkLimit();
	top++;
	top->type = T_FLOAT;
	top->value.floatval = d;
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

void
Stack::clean(stackval *sv)
{
	if (sv->type == T_STRING && sv->value.string)
	{
		free(sv->value.string);
		top->value.string = NULL;
		sv->type = T_UNUSED;
	}
}

void Stack::swap2()
{
	// swap top two pairs of elements
	stackval temp;
	stackval *one = top - 1;
	stackval *two = top - 2;
	stackval *three = top - 3;
	
	temp.type = two->type;
	temp.value = two->value;
	two->type = top->type;
	two->value = top->value;
	top->type = temp.type;
	top->value = temp.value;

	temp.type = three->type;
	temp.value = three->value;
	three->type = one->type;
	three->value = one->value;
	one->type = temp.type;
	one->value = temp.value;
}

void Stack::swap()
{
	// swap top two elements
	stackval temp;
	stackval *two = top - 1;
	
	temp.type = two->type;
	temp.value = two->value;
	
	two->type = top->type;
	two->value = top->value;

	top->type = temp.type;
	top->value = temp.value;
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
	temp.value = top->value;
	
	top->type = two->type;
	top->value = two->value;

	two->type = three->type;
	two->value = three->value;

	three->type = temp.type;
	three->value = temp.value;
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
	if (orig->type==T_INT) {
		push(orig->value.intval);
	}
    if (orig->type==T_VARREF) {
        pushvarref(orig->value.intval);
    }
    if (orig->type==T_VARREFSTR) {
        pushvarrefstr(orig->value.intval);
    }
    if (orig->type==T_FLOAT) {
		push(orig->value.floatval);
	}
	if (orig->type==T_STRING) {
		char *t;
		t = (char *) malloc((strlen(orig->value.string)+1) * sizeof(char));
		strcpy(t,orig->value.string);
		push(t);
	}
}

int 
Stack::popint()
{
	int i=0;
	i = toint(top);
	clean(top);
	if (top > bottom) top--;
	return i;
}

int 
Stack::toint(stackval *sv)
{
	int i=0;
    if (sv->type == T_INT || sv->type == T_VARREF || sv->type == T_VARREFSTR) {
		i = sv->value.intval;
	}
	else if (sv->type == T_FLOAT) {
		i = (int) sv->value.floatval;
	}
	else if (sv->type == T_STRING)
	{
		i = (int) atoi(sv->value.string);
	}
	return i;
}

double 
Stack::popfloat()
{
	double f=0;
	f = tofloat(top);
	clean(top);
	if (top > bottom) top--;
	return f;
}

double 
Stack::tofloat(stackval *sv)
{
	double f=0;
	if (sv->type == T_FLOAT) {
		f = sv->value.floatval;
	}
    else if (sv->type == T_INT || sv->type == T_VARREF || sv->type == T_VARREFSTR)
	{
		f = (double) sv->value.intval;
	}
	else if (sv->type == T_STRING)
	{
		f = (double) atof(sv->value.string);
	}
	return f;
}

char* 
Stack::popstring()
{
	char *s=tostring(top);
	top->type = T_UNUSED;
	if (top > bottom) top--;
	return s;
}

char* 
Stack::tostring(stackval *sv)
{
	// don't forget to free() the string returned by this function when you are done with it
	char *s=NULL;
	if (sv->type == T_STRING) {
		s = sv->value.string;
		sv->value.string = NULL;
		sv->type = T_UNUSED;
	}
    else if (sv->type == T_INT || sv->type == T_VARREF || sv->type == T_VARREFSTR)
	{
		char buffer[64];
		sprintf(buffer, "%d", sv->value.intval);
		s = strdup(buffer);
	}
	else if (sv->type == T_FLOAT)
	{
		char buffer[64];
		sprintf(buffer, "%#.*lf", fToAMask, sv->value.floatval);
		// strip trailing zeros and decimal point
		while(buffer[strlen(buffer)-1]=='0') buffer[strlen(buffer)-1] = 0x00;
		if(buffer[strlen(buffer)-1]=='.') buffer[strlen(buffer)-1] = 0x00;
		// return
		s = strdup(buffer);
	}
	return s;
}
