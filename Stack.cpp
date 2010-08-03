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
	top->value.string = strdup(c);
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
Stack::push(double d)
{
	checkLimit();
	top++;
	top->type = T_FLOAT;
	top->value.floatval = d;
}

stackval *
Stack::pop()
{
	stackval *temp = top;
	top--;
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

void 
Stack::swap()
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

int 
Stack::popint()
{
	int i=0;
	if (top->type == T_INT) {
		i = top->value.intval;
	}
	else if (top->type == T_FLOAT) {
		i = (int) top->value.floatval;
	}
	else if (top->type == T_STRING)
	{
		i = (int) atoi(top->value.string);
		free(top->value.string);
		top->value.string = NULL;
	}
	top->type = T_UNUSED;
	if (top > bottom) top--;
	return i;
}

double 
Stack::popfloat()
{
	double f=0;
	if (top->type == T_FLOAT) {
		f = top->value.floatval;
	}
	else if (top->type == T_INT)
	{
		f = (double) top->value.intval;
	}
	else if (top->type == T_STRING)
	{
		f = (double) atof(top->value.string);
		free(top->value.string);
		top->value.string = NULL;
	}
	top->type = T_UNUSED;
	if (top > bottom) top--;
	return f;
}

char* 
Stack::popstring()
{
	// don't forget to free() the string returned by this function when you are done with it
	char *s=NULL;
	if (top->type == T_STRING) {
		s = top->value.string;
		top->value.string = NULL;
	}
	else if (top->type == T_INT)
	{
		char buffer[64];
		sprintf(buffer, "%d", top->value.intval);
		s = strdup(buffer);
	}
	else if (top->type == T_FLOAT)
	{
		char buffer[64];
		sprintf(buffer, "%#.*lf", fToAMask, top->value.floatval);
		// strip trailing zeros and decimal point
		while(buffer[strlen(buffer)-1]=='0') buffer[strlen(buffer)-1] = 0x00;
		if(buffer[strlen(buffer)-1]=='.') buffer[strlen(buffer)-1] = 0x00;
		// return
		s = strdup(buffer);
	}
	top->type = T_UNUSED;
	if (top > bottom) top--;
	return s;
}
