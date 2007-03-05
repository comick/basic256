/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/


using namespace std;
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <QPainter>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include "LEX/basicParse.tab.h"
#include "ByteCodes.h"
#include "Interpreter.h"

extern QMutex mutex;
extern QMutex keymutex;
extern QMutex debugmutex;
extern int currentKey;
extern QWaitCondition waitCond;
extern QWaitCondition waitDebugCond;
extern QWaitCondition waitInput;

#define POP2  stackval *one = stack.pop(); stackval *two = stack.pop(); 

extern "C" {
  extern int basicParse(char *);
  extern int labeltable[];
  extern int linenumber;
  extern int newByteCode(int size);
  extern unsigned char *byteCode;
  extern unsigned int byteOffset;
  extern unsigned int maxbyteoffset;
  extern char *symtable[];
}


static int 
compareNum(stackval *one, stackval *two)
{
  double oneval = 0;
  double twoval = 0;
  if (one->type == T_INT && two->type == T_INT)
    {
      if (one->value.intval == two->value.intval) return 0;
      else if (one->value.intval < two->value.intval) return -1;
      else if (one->value.intval > two->value.intval) return 1;
      return 42;
    }

  if (one->type == T_INT)
    {
      oneval = (double) one->value.intval;
    }
  else 
    {
      oneval = one->value.floatval;
    }

  if (two->type == T_INT)
    {
      twoval = (double) two->value.intval;
    }
  else 
    {
      twoval = two->value.floatval;
    }
  
  if (oneval == twoval) return 0;
  else if (oneval < twoval) return -1;
  else if (oneval > twoval) return 1;
  return 42;
}


stackval::stackval()
{
  next = NULL;
  type = T_UNUSED;
  value.floatval = 0;
}

stackval::~stackval()
{
  if (type == T_STRING && value.string != NULL)
    {
      free(value.string);
      value.string = NULL;
    }
}


Stack::Stack()
{
  top = NULL;
}

void
Stack::push(char *c)
{
  stackval *temp = new stackval;
  
  temp->next = NULL;
  temp->type = T_STRING;
  temp->value.string = strdup(c);
  if (top)
    {
      temp->next = top;
    }
  top = temp;
}

void
Stack::push(int i)
{
  stackval *temp = new stackval;
  
  temp->next = NULL;
  temp->type = T_INT;
  temp->value.intval = i;
  if (top)
    {
      temp->next = top;
    }
  top = temp;
}

void
Stack::push(double d)
{
  stackval *temp = new stackval;
  
  temp->next = NULL;
  temp->type = T_FLOAT;
  temp->value.floatval = d;
  if (top)
    {
      temp->next = top;
    }
  top = temp;
}

stackval *
Stack::pop()
{
  stackval *temp = top;
  if (top)
    {
      top = top->next;
    }
  return temp;
}

void
Interpreter::printError(QString message)
{
  emit(outputReady(tr("ERROR on line ") + QString::number(currentLine) + ": " + message + "\n"));
  emit(goToLine(currentLine));
}

void
Interpreter::setInputReady()
{
  status = R_INPUTREADY;
}

bool
Interpreter::isAwaitingInput()
{
  if (status == R_INPUT)
    {
      return true;
    }
  return false;
}

bool
Interpreter::isRunning()
{
  if (status != R_STOPPED)
    {
      return true;
    }
  return false;
}


bool
Interpreter::isStopped()
{
  if (status == R_STOPPED)
    {
      return true;
    }
  return false;
}


Interpreter::Interpreter(QImage *i, QImage *m)
{
  image = i;
  imask = m;
  fastgraphics = false;
  status = R_STOPPED;
  for (int i = 0; i < NUMVARS; i++)
    {
      vars[i].type = T_UNUSED;
      vars[i].value.floatval = 0;
    }
}


void
Interpreter::clearvars()
{
  for (int i = 0; i < NUMVARS; i++)
    {
      if (vars[i].type == T_STRING && vars[i].value.string != NULL)
	{
	  free(vars[i].value.string);
	  vars[i].value.string = NULL;
	}
      else if (vars[i].type == T_ARRAY && vars[i].value.arr != NULL)
	{
	  delete(vars[i].value.arr->data.fdata);
	  delete(vars[i].value.arr);
	}
      else if (vars[i].type == T_STRARRAY && vars[i].value.arr != NULL)
	{
	  for (int j = 0; j < vars[i].value.arr->size; j++)
	    {
	      if (vars[i].value.arr->data.sdata[j])
		{
		  free(vars[i].value.arr->data.sdata[j]);
		  vars[i].value.arr->data.sdata[j] = NULL;
		}
	    }
	  delete(vars[i].value.arr->data.sdata);
	  delete(vars[i].value.arr);
	}
      vars[i].type = T_UNUSED;
      vars[i].value.floatval = 0;
      vars[i].value.string = NULL;
    }
}


int
Interpreter::compileProgram(char *code)
{
  clearvars();
  if (newByteCode(strlen(code)) < 0)
    {
      return -1;
    }
    
  
  int result = basicParse(code); 
  if (result < 0)
    {
      emit(outputReady(tr("Syntax error on line ") + QString::number(linenumber) + "\n"));
      emit(goToLine(linenumber));
      return -1;
    }

  op = byteCode;
  currentLine = 1;
  while (op <= byteCode + byteOffset)
    {
      if (*op == OP_CURRLINE)
	{
	  op++;
	  int *i = (int *) op;
	  currentLine = *i;
	  op += sizeof(int);
	}
      
      if (*op == OP_GOTO || *op == OP_GOSUB)
	{
	  op += sizeof(unsigned char);
	  int *i = (int *) op;
	  op += sizeof(int);
	  if (labeltable[*i] >=0)
	    {
	      int tbloff = *i;
	      *i = labeltable[tbloff];
	    }
	  else
	    {
	      printError(tr("No such label"));
	      return -1;
	    }
	}
      else if (*op < 0x40)  //no args
	{
	  op += sizeof(unsigned char);
	}
      else if (*op < 0x60) //Int arg
	{
	  op += sizeof(unsigned char) + sizeof(int);
	}
      else if (*op < 0x70) //2 Int arg
	{
	  op += sizeof(unsigned char) + 2 * sizeof(int);
	}
      else if (*op < 0x80) //Float arg
	{
	  op += sizeof(unsigned char) + sizeof(double);
	}
      else //String arg
	{
	  op += sizeof(unsigned char);
	  int len = strlen((char *) op) + 1;
	  op += len;
	}
    }

  currentLine = 1;
  return 0;
}


byteCodeData *
Interpreter::getByteCode(char *code)
{
  if (compileProgram(code) < 0)
    {
      return NULL;
    }
  byteCodeData *temp = new byteCodeData;
  temp->size = byteOffset;
  temp->data = byteCode;

  return temp;
}


void
Interpreter::initialize()
{
  op = byteCode;
  callstack = NULL;
  forstack = NULL;
  fastgraphics = false;
  pencolor = Qt::black;
  status = R_RUNNING;
  once = true;
  currentLine = 1;
  stream = NULL;
}


void
Interpreter::cleanup()
{
  status = R_STOPPED;
  stackval *temp = stack.pop();
  //Clean up stack
  while (temp != NULL) 
    {
      delete temp;
      temp = stack.pop();
    }
  //Clean up variables
  clearvars();
  //Clean up, for frames, etc.

  if (byteCode)
    {
      free(byteCode);
      byteCode = NULL;
    }
}

void
Interpreter::pauseResume()
{
  if (status == R_PAUSED)
    {
      status = oldstatus;
    }
  else 
    {
      oldstatus = status;
      status = R_PAUSED;
    }
}

void
Interpreter::stop()
{
  status = R_STOPPED;
}


void
Interpreter::run()
{
  while (status != R_STOPPED && execByteCode() >= 0); //continue
  status = R_STOPPED;
  emit(runFinished());
}


void
Interpreter::receiveInput(QString text)
{
  inputString = text;
  status = R_INPUTREADY;
}


int
Interpreter::execByteCode()
{
  if (status == R_INPUTREADY)
    {
      stack.push(inputString.toUtf8().data());
      status = R_RUNNING;
      return 0;
    }
  else if (status == R_PAUSED)
    {
      sleep(1);
      return 0;
    }
  else if (status == R_INPUT)
    {
      return 0;
    }

  while (*op == OP_CURRLINE)
    {
      op++;
      int *i = (int *) op;
      currentLine = *i;
      op += sizeof(int);
      if (debugMode && *op != OP_CURRLINE) 
	{
	  emit(highlightLine(currentLine));
	  debugmutex.lock();
	  waitDebugCond.wait(&debugmutex);
	  debugmutex.unlock();
	}
    }
  
  switch(*op)
    {
    case OP_NOP:
      op++;
      break;

    case OP_END:
      {
	return -1;
      }
      break;

    case OP_BRANCH:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	stackval *temp = stack.pop();
	
	if (temp->value.intval == 0) // go to next line on false, otherwise execute rest of line.
	  {
	    op = byteCode + *i;
	  }
	delete temp;
      }
      break;

    case OP_GOSUB:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	frame *temp = new frame;

	temp->returnAddr = op;
	temp->next = callstack;
	callstack = temp;
	op = byteCode + *i;
      }
      break;

    case OP_RETURN:
      {
	frame *temp = callstack;
	if (temp)
	  {
	    op = temp->returnAddr;
	    callstack = temp->next;
	  }
	else
	  {
	    return -1;
	  }
	delete temp;
      }
      break;

      
    case OP_GOTO:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	op = byteCode + *i;
      }
      break;


    case OP_FOR:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	forframe *temp = new forframe;
	stackval *step = stack.pop();
	stackval *endnum = stack.pop();
	stackval *startnum = stack.pop();

	temp->next = forstack;
	temp->prev = NULL;
	temp->variable = *i;

	vars[*i].type = T_FLOAT;
	if (startnum->type == T_INT)
	  {
	    vars[*i].value.floatval = (double) startnum->value.intval;
	  }
	else 
	  {
	    vars[*i].value.floatval = startnum->value.floatval;
	  }

	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*i]), QString::number(vars[*i].value.floatval), -1));
	  }
	
	if (endnum->type == T_INT)
	  {
	    temp->endNum = (double) endnum->value.intval;
	  }
	else 
	  {
	    temp->endNum = endnum->value.floatval;
	  }

	if (step->type == T_INT)
	  {
	    temp->step = (double) step->value.intval;
	  }
	else 
	  {
	    temp->step = step->value.floatval;
	  }
	
	temp->returnAddr = op;
	if (forstack)
	  {
	    forstack->prev = temp;
	  }
	forstack = temp;
	if (temp->step > 0 && vars[*i].value.floatval >= temp->endNum)
	  {
	    while(*op != OP_NEXT)
	      {
		op++;
	      }
	    op += 1 + sizeof(int);
	  }
	else if (temp->step < 0 && vars[*i].value.floatval <= temp->endNum)
	  {
	    while(*op != OP_NEXT)
	      {
		op++;
	      }
	    op += 1 + sizeof(int);
	  }
	  
	delete step;
	delete endnum;
	delete startnum;
      }
      break;


    case OP_NEXT:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	forframe *temp = forstack;
	    
	while (temp && temp->variable != (unsigned int ) *i)
	  {
	    temp = temp->next;
	  }
	if (!temp)
	  {
	    printError(tr("Next without FOR"));
	    return -1;
	  }
	    
	double val = vars[*i].value.floatval;
	val += temp->step;
	vars[*i].value.floatval = val;

	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*i]), QString::number(vars[*i].value.floatval), -1));
	  }

	if (temp->step > 0 && val <= temp->endNum)
	  {
	    op = temp->returnAddr;
	  }
	else if (temp->step < 0 && val >= temp->endNum)
	  {
	    op = temp->returnAddr;
	  }
	else
	  {
	    if (temp->next)
	      {
		temp->next->prev = temp->prev;
	      }
	    if (temp->prev)
	      {
		temp->prev->next = temp->next;
	      }
	    if (forstack == temp)
	      {
		forstack = temp->next;
	      }
	    delete temp;
	  }
      }
      break;


    case OP_OPEN:
      {
	op++;
	stackval *name = stack.pop();

	if (name->type == T_STRING)
	  {
	    if (stream != NULL)
	      {
		stream->close();
		stream = NULL;
	      }

	    stream = new QFile(QString::fromUtf8(name->value.string));


	    if (stream == NULL || !stream->open(QIODevice::ReadWrite | QIODevice::Text))
	      {
		printError(tr("Unable to open file"));
		return -1;
	      }
	  }
	else
	  {
	    printError(tr("Illegal argument to open()"));
	    return -1;
	  }
	delete name;
      }
      break;


    case OP_READ:
      {
	op++;
	char c = ' ';

	if (stream == NULL)
	  {
	    printError(tr("Can't read -- no open file."));
	    return -1;
	  }

	//Remove leading whitespace
	while (c == ' ' || c == '\t' || c == '\n')
	  {
	    if (!stream->getChar(&c))
	      {
		stack.push("");
		return 0;
	      }
	  }
	
	//put back non-whitespace character
	stream->ungetChar(c);

	//read token
	int maxsize = 256;
	int offset = 0; 
	char * strarray = (char *) malloc(maxsize);
	memset(strarray, 0, maxsize);

	while (stream->getChar(strarray + offset) && 
	       *(strarray + offset) != ' ' && 
	       *(strarray + offset) != '\t' && 
	       *(strarray + offset) != '\n' && 
	       *(strarray + offset) != 0)
	  {
	    offset++;
	    if (offset >= maxsize)
	      {
		maxsize *= 2;
		strarray = (char *) realloc(strarray, maxsize);
		memset(strarray + offset, 0, maxsize - offset);
	      }
	  }
	strarray[offset] = 0;

	stack.push(strarray);
	free(strarray);
      }
      break;


    case OP_WRITE:
      {
	op++;
	stackval *temp = stack.pop();

	if (temp->type == T_STRING)
	  {
	     int error = 0;

	     if (stream != NULL)
	       {
		 quint64 oldPos = stream->pos();
		 stream->flush();
		 stream->seek(stream->size());
		 error = stream->write(temp->value.string, strlen(temp->value.string));
		 stream->seek(oldPos);
		 stream->flush();
	       }

	     if (error == -1)
	       {
		 printError(tr("Unable to write to file"));
	       }
	  }
	else
	  {
	    printError(tr("Illegal argument to write()"));
	    return -1;
	  }
	delete temp;
      }
      break;


    case OP_CLOSE:
      {
        op++;

	if (stream != NULL)
	  {
	    stream->close();
	    stream = NULL;
	  }
      }
      break;


    case OP_RESET:
      {
	op++;

	if (stream == NULL)
	  {
	    printError(tr("reset() called when no file is open"));
	    return -1;
	  }
	else
	  {
	    stream->close();

	    if (!stream->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
	      {
		printError(tr("Unable to reset file"));
		return -1;
	      }
	  }
      }
      break;


    case OP_DIM:
    case OP_DIMSTR:
      {
	char whichdim = *op;
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	int var = i[0];
	int size = 0;
	stackval *one = stack.pop();

	if (one->type == T_INT) size = one->value.intval; else size = (int) one->value.floatval;
	
	if (size > 100000)
	  {
	    printError(tr("Array dimension too large"));
	    return -1;
	  }
	
	array *temp = new array;
	if (whichdim == OP_DIM)
	  {
	    double *d = new double[size];
	    for (int j = 0; j < size; j++)
	      {
		d[j] = 0;
	      }
	    vars[var].type = T_ARRAY;
	    temp->data.fdata = d;
	    temp->size = size;
	    vars[var].value.arr = temp;
	  }
	else 
	  {
	    char **c = new char*[size];
	    for (int j = 0; j < size; j++)
	      {
		c[j] = strdup("");
	      }
	    vars[var].type = T_STRARRAY;
	    temp->data.sdata = c;
	    temp->size = size;
	    vars[var].value.arr = temp;
	  }
	delete one;

	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[var]), NULL, size));
	  }
      }
      break;


    case OP_STRARRAYASSIGN:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	POP2; //one = expr, two = index
	int index;
	char **strarray;
	if (two->type == T_INT) index = two->value.intval; else index = (int) two->value.floatval;
	if (one->type != T_STRING) 
	  {
	    printError(tr("Cannot assign non-string to string array"));
	    return -1;
	  }
	if (index >= vars[*i].value.arr->size || index < 0)
	  {
	    printError(tr("Array index out of bounds"));
	    return -1;
	  }

	strarray = vars[*i].value.arr->data.sdata;
	if (strarray[index])
	  {
	    delete(strarray[index]);
	  }
	strarray[index] = strdup(one->value.string);
	delete one;
	delete two;
	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(strarray[index]), index));
	  }
      }
      break;
	  
    case OP_STRARRAYLISTASSIGN:
      {
	op++;
	int *i = (int *) op;
	int items = i[1];
	op += 2 * sizeof(int);
	int index;
	char *str;
	char **strarray;
	
	if (items > vars[*i].value.arr->size || items < 0)
	  {
	    printError(tr("Array dimension too small"));
	    return -1;
	  }
	
	strarray = vars[*i].value.arr->data.sdata;
	for (index = items - 1; index >= 0; index--)
	  {
	    stackval *one = stack.pop();
	    if (one->type == T_STRING)
	      {
		str = strdup(one->value.string); 
	      }
	    else 
	      {
		printError(tr("Array dimension too small"));
		return -1;
	      }
	    if (strarray[index])
	      {
		delete(strarray[index]);
	      }
	    strarray[index] = str;
	    delete one;
	    if(debugMode)
	      {
		emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(strarray[index]), index));
	      }
	  }
      }
      break;
    

    case OP_STRARRAYINPUT:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	POP2; //one = index, two = expr
	int index;
	char **strarray;
	if (one->type == T_INT) index = one->value.intval; else index = (int) one->value.floatval;
	if (two->type != T_STRING) 
	  {
	    printError(tr("Cannot assign non-string to string array"));
	    return -1;
	  }
	if (index >= vars[*i].value.arr->size || index < 0)
	  {
	    printError(tr("Array index out of bounds"));
	    return -1;
	  }

	strarray = vars[*i].value.arr->data.sdata;
	strarray[index] = strdup(two->value.string);
	delete one;
	delete two;
      }
      break;
	  

    case OP_ARRAYASSIGN:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	POP2; //one = expr, two = index
	int index;
	double val;
	double *array;
	if (two->type == T_INT) index = two->value.intval; else index = (int) two->value.floatval;
	if (one->type == T_INT) val = (double) one->value.intval; else val = one->value.floatval;

	if (index >= vars[*i].value.arr->size || index < 0)
	  {
	    printError(tr("Array index out of bounds"));
	    return -1;
	  }
	array = vars[*i].value.arr->data.fdata;
	array[index] = val;
	delete one;
	delete two;
	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*i]), QString::number(val), index));
	  }
      }
      break;


    case OP_ARRAYLISTASSIGN:
      {
	op++;
	int *i = (int *) op;
	int items = i[1];
	op += 2 * sizeof(int);
	int index;
	double val;
	double *array;
	
	if (items > vars[*i].value.arr->size || items < 0)
	  {
	    printError(tr("Array dimension too small"));
	    return -1;
	  }
	
	array = vars[*i].value.arr->data.fdata;
	for (index = items - 1; index >= 0; index--)
	  {
	    stackval *one = stack.pop();
	    if (one->type == T_INT) val = (double) one->value.intval; else val = one->value.floatval;
	    array[index] = val;
	    delete one;
	    if(debugMode)
	      {
		emit(varAssignment(QString(symtable[*i]), QString::number(val), index));
	      }
	  }
      }
      break;
    

    case OP_DEREF:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	stackval *temp = stack.pop();
	int index;
	if (temp->type == T_INT) index = temp->value.intval; else index = (int) temp->value.floatval;
	
	if (vars[*i].type != T_ARRAY && vars[*i].type != T_STRARRAY)
	  {
	    printError(tr("Cannot access non-array variable"));
	    return -1;
	  }

	if (index >= vars[*i].value.arr->size || index < 0)
	  {
	    printError(tr("Array index out of bounds"));
	    return -1;
	  }
	if (vars[*i].type == T_ARRAY)
	  {
	    double *array = vars[*i].value.arr->data.fdata;
	    stack.push(array[index]);
	  }
	else
	  {
	    char **array = vars[*i].value.arr->data.sdata;
	    stack.push(array[index]);
	  }
	delete temp;
      }
      break;

    case OP_PUSHVAR:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	
	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }

	if (vars[*i].type == T_STRING)
	  {
	    stack.push(vars[*i].value.string);
	  }
	else if (vars[*i].type == T_ARRAY)
	  {
	    char buffer[32];
	    sprintf(buffer, "array(0x%p)", vars[*i].value.arr);
	    stack.push(buffer);
	  }
	else if (vars[*i].type == T_STRARRAY)
	  {
	    char buffer[32];
	    sprintf(buffer, "string array(0x%p)", vars[*i].value.arr);
	    stack.push(buffer);
	  }
	else 
	  {
	    stack.push(vars[*i].value.floatval);
	  }
      }
      break;

    case OP_PUSHINT:
      {
	op++;
	int *i = (int *) op;
	stack.push(*i);
	op += sizeof(int);
      }
      break;

    case OP_PUSHFLOAT:
      {
	op++;
	double *d = (double *) op;
	stack.push(*d);
	op += sizeof(double);
      }
      break;

    case OP_PUSHSTRING:
      {
	op++;
	stack.push((char *) op);
	op += strlen((char *) op) + 1;
      }
      break;


    case OP_INT:
      {
	op++;
	stackval *temp = stack.pop();
	if (temp->type == T_INT)
	  {
	    stack.push(temp->value.intval);
	  }
	else if (temp->type == T_FLOAT)
	  {
	    stack.push((int) temp->value.floatval);
	  }
	else if (temp->type == T_STRING)
	  {
	    stack.push((int) atoi(temp->value.string));
	  }
	else 
	  {
	    printError(tr("Illegal argument to int()"));
	    return -1;
	  }
	delete temp;
      }
      break;

    case OP_STRING:
      {
	char buffer[64];
	op++;
	stackval *temp = stack.pop();
	if (temp->type == T_INT)
	  {
	    sprintf(buffer, "%d", temp->value.intval);
	    stack.push(buffer);
	  }
	else if (temp->type == T_FLOAT)
	  {
	    sprintf(buffer, "%g", temp->value.floatval);
	    stack.push(buffer);
	  }
	else if (temp->type == T_STRING)
	  {
	    stack.push(temp->value.string);
	  }
	else 
	  {
	    printError(tr("Illegal argument to string()"));
	    return -1;
	  }
	delete temp;
      }
      break;

    case OP_RAND:
      {
	if (once)
	  {
	    int ms = 999 + QTime::currentTime().msec();
	    once = false;
	    srand(time(NULL) * ms);
	  }
	op += sizeof(unsigned char);
	stack.push((double) rand() / ((double) RAND_MAX));
      }
      break;

    case OP_PAUSE:
      {
	op++;
	double val = 0;
	stackval *temp = stack.pop();
	if (temp->type == T_INT) 
	  {
	    val = (double) temp->value.intval; 
	  }
	else if (temp->type == T_FLOAT)
	  {
	    val = temp->value.floatval;
	  }
	int stime = (int) (val * 1000);
	msleep(stime);
      }
      break;

    case OP_LENGTH:
      {
	op++;
	stackval *temp = stack.pop();
	if (temp->type == T_STRING)
	  {
	    stack.push((int) QString::fromUtf8(temp->value.string).length());
	  }
	else
	  {
	    printError(tr("Illegal argument to length()"));
	    return -1;
	  }
	delete temp;
      }
      break;


    case OP_MID:
      {
	op++;
	stackval *len = stack.pop();
	stackval *pos = stack.pop();
	stackval *str = stack.pop();

	if ((pos->type != T_INT) || (len->type != T_INT) || (str->type != T_STRING))
	  {
	    printError(tr("Illegal argument to mid()"));
	    return -1;
	  }

	if ((pos->value.intval <= 0) || (len->value.intval < 0))
	  {
	    printError(tr("Illegal argument to mid()"));
	    return -1;
	  }
	QString temp = QString::fromUtf8(str->value.string);

	if (pos->value.intval > (int) temp.length())
	  {
	    printError(tr("String not long enough for given starting character"));
	    return -1;
	  }
	QString res = temp.mid(pos->value.intval - 1, len->value.intval);

	stack.push(res.toUtf8().data());

	delete str;
	delete pos;
	delete len;
      }
      break;


    case OP_INSTR:
      {
	op++;
	stackval *needle = stack.pop();
	stackval *haystk = stack.pop();

	if ((needle->type != T_STRING) || (haystk->type != T_STRING))
	  {
	    printError(tr("Illegal argument to instr()"));
	    return -1;
	  }

	int pos = 0;

	QString hay = QString::fromUtf8(haystk->value.string);
	QString str = QString::fromUtf8(needle->value.string);
	pos = hay.indexOf(str);
	pos++;

	stack.push((int) pos);

	delete haystk;
	delete needle;
      }
      break;

    case OP_SIN:
    case OP_COS:
    case OP_TAN:
    case OP_CEIL:
    case OP_FLOOR:
    case OP_ABS:
      {
	unsigned char whichop = *op;
	op += sizeof(unsigned char);
	stackval *temp = stack.pop();
	double val;
	if (temp->type == T_INT) 
	  {
	    val = (double) temp->value.intval; 
	  }
	else if (temp->type == T_FLOAT)
	  {
	    val = temp->value.floatval;
	  }
	else 
	  {
	    switch (whichop)
	      {
	      case OP_SIN:
		printError(tr("Illegal argument to sin()"));
		break;
	      case OP_COS:
		printError(tr("Illegal argument to cos()"));
		break;
	      case OP_TAN:
		printError(tr("Illegal argument to tan()"));
		break;
	      case OP_CEIL:
		printError(tr("Illegal argument to ceil()"));
		break;
	      case OP_FLOOR:
		printError(tr("Illegal argument to floor()"));
		break;
	      case OP_ABS:
		printError(tr("Illegal argument to abs()"));
		break;
	      }
	    return -1;
	  }
	switch (whichop)
	  {
	  case OP_SIN:
	    stack.push(sin(val));
	    break;
	  case OP_COS:
	    stack.push(cos(val));
	    break;
	  case OP_TAN:
	    stack.push(tan(val));
	    break;
	  case OP_CEIL:
	    stack.push((int) ceil(val));
	    break;
	  case OP_FLOOR:
	    stack.push((int) floor(val));
	    break;
	  case OP_ABS:
	    if (val < 0) 
	      {
		val = -val;
	      }
	    stack.push(val);
	    break;
	  }
	delete temp;
      }
      break;


    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_EXP:
      {
	POP2;
	double oneval, twoval;
	if (one->type == T_STRING || two->type == T_STRING)
	  {
	    printError(tr("String in numeric expression"));
	    return -1;
	  }
	if (one->type == two->type && one->type == T_INT)
	  {
	    switch (*op)
	      {
	      case OP_ADD:
		stack.push(one->value.intval + two->value.intval);
		break;
	      case OP_SUB:
		stack.push(two->value.intval - one->value.intval);
		break;
	      case OP_MUL:
		stack.push(two->value.intval * one->value.intval);
		break;
	      case OP_DIV:
		stack.push((double) two->value.intval / (double) one->value.intval);
		break;
	      case OP_EXP:
		stack.push(pow((double) two->value.intval, (double) one->value.intval));
		break;
	      }
	  }

	else 
	  {
	    if (one->type == T_INT) 
	      oneval = (double) one->value.intval; 
	    else 
	      oneval = one->value.floatval;
	    if (two->type == T_INT) 
	      twoval = (double) two->value.intval; 
	    else 
	      twoval = two->value.floatval;

	    switch(*op)
	      {
	      case OP_ADD:
		stack.push(twoval + oneval);
		break;
	      case OP_SUB:
		stack.push(twoval - oneval);
		break;
	      case OP_MUL:
		stack.push(twoval * oneval);
		break;
	      case OP_DIV:
		stack.push(twoval / oneval);
		break;
	      case OP_EXP:
		stack.push(pow((double) twoval, (double) oneval));
		break;
	      }
	  }
	op++;
	delete one;
	delete two;
      }
      break;

    case OP_AND:
      {
	op++;
	POP2;
	if (one->value.intval && two->value.intval)
	  {
	    stack.push(1);
	  }
	else
	  {
	    stack.push(0);
	  }
	delete one;
	delete two;
      }
      break;

    case OP_OR:
      {
	op++;
	POP2;
	if (one->value.intval || two->value.intval)
	  {
	    stack.push(1);
	  }
	else
	  {
	    stack.push(0);
	  }
	delete one;
	delete two;
      }
      break;

    case OP_XOR:
      {
	op++;
	POP2;
	if (!(one->value.intval && two->value.intval) && (one->value.intval || two->value.intval)) 
	  {
	    stack.push(1);
	  }
	else
	  {
	    stack.push(0);
	  }
	delete one;
	delete two;
      }
      break;

    case OP_NOT:
      {
	op++;
	stackval *temp = stack.pop();
	if (temp->value.intval)
	  {
	    stack.push(0);
	  }
	else
	  {
	    stack.push(1);
	  }
	delete temp;
      }
      break;

    case OP_NEGATE:
      {
	op++;
	stackval *temp = stack.pop();
	if (temp->type == T_INT)
	  {
	    stack.push(-temp->value.intval);
	  }
	else
	  {
	    stack.push(-temp->value.floatval);
	  }
	delete temp;
      }
      break;

    case OP_EQUAL:
    case OP_NEQUAL:
      {
	int val;
	if (*op == OP_EQUAL) val = 0; else val = 1;
	op++;
	POP2;

	if (one->type == T_STRING && two->type == T_STRING && strcmp(one->value.string, two->value.string) != 0)
	  {
	    stack.push(val);
	  }
	else if (one->type != T_STRING && two->type != T_STRING && compareNum(one, two) != 0)
	  {
	    stack.push(val);
	  }
	else 
	  {
	    stack.push(val ^ 1);
	  }
	delete one;
	delete two;
      }
      break;
    case OP_GT:
    case OP_LTE:
      {
	int val = 1;
	if (*op == OP_LTE) val = 0;
	op++;
	POP2;

	if (one->type == T_STRING || two->type == T_STRING)
	  {
	    printError(tr("Cannot compare strings with > or <="));
	    return -1;
	  }
	if (compareNum(two, one) == 1)
	  {
	    stack.push(val);
	  }
	else 
	  {
	    stack.push(val ^ 1);
	  }
	delete one;
	delete two;
      }
      break;

    case OP_LT:
    case OP_GTE:
      {
	int val = 1;
	if (*op == OP_GTE) val = 0;
	op++;
	POP2;

	if (one->type == T_STRING || two->type == T_STRING)
	  {
	    printError(tr("Cannot compare strings with < or >="));
	    return -1;
	  }
	if (compareNum(two, one) == -1)
	  {
	    stack.push(val);
	  }
	else 
	  {
	    stack.push(val ^ 1);
	  }
	delete one;
	delete two;
      }
      break;

    case OP_SOUND:
      {
	op++;
	POP2;
	int oneval;
	int twoval;
	
	if (one->type == T_STRING || two->type == T_STRING)
	  {
	    printError(tr("Sound must have a frequency and duration."));
	    return -1;
	  }

	if (one->type == T_INT) oneval = one->value.intval; else oneval = (int) one->value.floatval;
	if (two->type == T_INT) twoval = two->value.intval; else twoval = (int) two->value.floatval;
	
	emit(soundReady(oneval, twoval));
	delete one;
	delete two;
      }
      break;
	
	


    case OP_SETCOLOR:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	switch(*i)
	  {
	  case 0:
	    pencolor = Qt::color0;
	    break;
	  case 1:
	    pencolor = QColor("#f8f8f8");
	    break;
	  case 2:
	    pencolor = Qt::black;
	    break;
	  case 3:
	    pencolor = Qt::red;
	    break;
	  case 4:
	    pencolor = Qt::darkRed;
	    break;
	  case 5:
	    pencolor = Qt::green;
	    break;
	  case 6:
	    pencolor = Qt::darkGreen;
	    break;
	  case 7:
	    pencolor = Qt::blue;
	    break;
	  case 8:
	    pencolor = Qt::darkBlue;
	    break;
	  case 9:
	    pencolor = Qt::cyan;
	    break;
	  case 10:
	    pencolor = Qt::darkCyan;
	    break;
	  case 11:
	    pencolor = Qt::magenta;
	    break;
	  case 12:
	    pencolor = Qt::darkMagenta;
	    break;
	  case 13:
	    pencolor = Qt::yellow;
	    break;
	  case 14:
	    pencolor = Qt::darkYellow;
	    break;
	  case 15: //orange
	    pencolor = QColor("#ff6600");
	    break;
	  case 16: //dark orange
	    pencolor = QColor("#aa3300");
	    break;
	  case 17: //dark orange
	    pencolor = Qt::gray;
	    break;
	  case 18: //dark orange
	    pencolor = Qt::darkGray;
	    break;

	  default:
	    pencolor = Qt::black;
	    break;
	  }
      }
      break;


    case OP_LINE:
      {
	op++;
	stackval *y1 = stack.pop();
	stackval *x1 = stack.pop();
	stackval *y0 = stack.pop();
	stackval *x0 = stack.pop();
	int x0val, y0val, x1val, y1val;
	
	if (x0->type == T_INT) x0val = x0->value.intval; else x0val = (int) x0->value.floatval;
	if (y0->type == T_INT) y0val = y0->value.intval; else y0val = (int) y0->value.floatval;
	if (x1->type == T_INT) x1val = x1->value.intval; else x1val = (int) x1->value.floatval;
	if (y1->type == T_INT) y1val = y1->value.intval; else y1val = (int) y1->value.floatval;
	
	QPainter ian(image);
	QPainter ian2(imask);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if (pencolor == Qt::color0)
	  {
	    ian2.setPen(Qt::color0);
	    ian2.setBrush(Qt::color0);
	  }
	else 
	  {
	    ian2.setPen(Qt::color1);
	    ian2.setBrush(Qt::color1);
	  }
	if (x1val >= 0 && y1val >= 0)
	  {
	    ian.drawLine(x0val, y0val, x1val, y1val);
	    ian2.drawLine(x0val, y0val, x1val, y1val);
	  }
	ian.end();
	ian2.end();
	delete x0;
	delete y0;
	delete x1;
	delete y1;

	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;


    case OP_RECT:
      {
	op++;
	stackval *y1 = stack.pop();
	stackval *x1 = stack.pop();
	stackval *y0 = stack.pop();
	stackval *x0 = stack.pop();
	int x0val, y0val, x1val, y1val;
	
	if (x0->type == T_INT) x0val = x0->value.intval; else x0val = (int) x0->value.floatval;
	if (y0->type == T_INT) y0val = y0->value.intval; else y0val = (int) y0->value.floatval;
	if (x1->type == T_INT) x1val = x1->value.intval; else x1val = (int) x1->value.floatval;
	if (y1->type == T_INT) y1val = y1->value.intval; else y1val = (int) y1->value.floatval;
	
	QPainter ian(image);
	QPainter ian2(imask);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if (pencolor == Qt::color0)
	  {
	    ian2.setPen(Qt::color0);
	    ian2.setBrush(Qt::color0);
	  }
	else 
	  {
	    ian2.setPen(Qt::color1);
	    ian2.setBrush(Qt::color1);
	  }
	if (x1val > 0 && y1val > 0)
	  {
	    ian.drawRect(x0val, y0val, x1val - 1, y1val - 1);
	    ian2.drawRect(x0val, y0val, x1val - 1, y1val - 1);
	  }
	ian.end();
	ian2.end();
	delete x0;
	delete y0;
	delete x1;
	delete y1;

	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;


    case OP_POLY:
      {
	op++;
	int *i = (int *) op;
	int items = *i;
	op += sizeof(int);
	int pairs = 0;

	QPainter poly(image);
	QPainter poly2(imask);
        poly.setPen(pencolor);
        poly.setBrush(pencolor);

	if (vars[*i].type == T_ARRAY)
	  {
	    pairs = vars[*i].value.arr->size / 2;
	    if (pairs < 3)
	      {
		printError(tr("Not enough points in array for poly()"));
		return -1;
	      }
	    
	    double *array = vars[*i].value.arr->data.fdata;
	    QPointF points[pairs];
	    
	    for (int j = 0; j < pairs; j++)
	      {
		points[j].setX(array[j*2]);
		points[j].setY(array[(j*2)+1]);
	      }
	    poly.drawPolygon(points, pairs);
	    poly2.drawPolygon(points, pairs);
	  }
	else //used immediate list
	  {
	    pairs = items / 2;
	    if (pairs < 3)
	      {
		printError(tr("Not enough points in array for poly()"));
		return -1;
	      }
	    QPointF points[pairs];
	    for (int j = 0; j < pairs; j++)
	      {
		int xval, yval;
		POP2;
		if (one->type == T_INT) xval = one->value.intval; else xval = (int) one->value.floatval;
		if (two->type == T_INT) yval = two->value.intval; else yval = (int) two->value.floatval;
		points[j].setX(xval);
		points[j].setY(yval);
		delete one;
		delete two;
	      }
	    poly.drawPolygon(points, pairs);
	    poly2.drawPolygon(points, pairs);
	  }

	poly.end();
	poly2.end();

	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;

    case OP_CIRCLE:
      {
	op++;
	stackval *r = stack.pop();
	stackval *y = stack.pop();
	stackval *x = stack.pop();
	int xval, yval, rval;
	
	if (r->type == T_INT) rval = r->value.intval; else rval = (int) r->value.floatval;
	if (y->type == T_INT) yval = y->value.intval; else yval = (int) y->value.floatval;
	if (x->type == T_INT) xval = x->value.intval; else xval = (int) x->value.floatval;
	
	QPainter ian(image);
	QPainter ian2(imask);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if (pencolor == Qt::color0) //transparent color
	  {
	    ian2.setPen(Qt::color0);
	    ian2.setBrush(Qt::color0);
	  }
	else 
	  {
	    ian2.setPen(Qt::color1);
	    ian2.setBrush(Qt::color1);
	  }
	ian.drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);
	ian2.drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);
	ian.end();
	ian2.end();
	delete x;
	delete y;
	delete r;

	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;

    case OP_CLS:
      {
	op++;
	mutex.lock();
	emit(clearText());
	waitCond.wait(&mutex);
	mutex.unlock();
      }
      break;

    case OP_CLG:
      {
	op++;
	imask->fill(Qt::color0);
	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;
      
    case OP_PLOT:
      {
	op++;
	POP2;
	double oneval, twoval;
	QPainter ian(image);
	QPainter ian2(imask);
	ian.setPen(pencolor);
	if (pencolor == Qt::color0)
	  ian2.setPen(pencolor);

	if (one->type == T_INT) oneval = (double) one->value.intval; else oneval = one->value.floatval;
	if (two->type == T_INT) twoval = (double) two->value.intval; else twoval = two->value.floatval;
	ian.drawPoint((int) twoval, (int) oneval);
	ian2.drawPoint((int) twoval, (int) oneval);
	ian.end();
	ian2.end();
	delete one;
	delete two;

	if (!fastgraphics)
	  {
	    mutex.lock();
	    emit(goutputReady());
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
      }
      break;

    case OP_FASTGRAPHICS:
      {
	op++;
	fastgraphics = true;
	emit(fastGraphics());
      }
      break;

    case OP_REFRESH:
      {
	op++;
	mutex.lock();
	emit(goutputReady());
	waitCond.wait(&mutex);
	mutex.unlock();
      }
      break;

    case OP_INPUT:
      {
	op++;
	status = R_INPUT;
	mutex.lock();
	emit(inputNeeded());
	waitInput.wait(&mutex);
	mutex.unlock();
      }
      break;

    case OP_KEY:
      {
	op++;
	keymutex.lock();
	stack.push(currentKey);
	currentKey = 0;
	keymutex.unlock();
      }
      break;

    case OP_PRINT:
    case OP_PRINTN:
      {
	stackval *temp = stack.pop();
	if (temp->type == T_STRING)
	  {
	    mutex.lock();
	    emit(outputReady(QString::fromUtf8(temp->value.string)));
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
	else if (temp->type == T_INT)
	  {
	    mutex.lock();
	    emit(outputReady(QString::number(temp->value.intval)));
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
	else if (temp->type == T_FLOAT)
	  {
	    mutex.lock();
	    if (floor(temp->value.floatval) == temp->value.floatval)
	      {
		emit(outputReady(QString::number((int) temp->value.floatval)));
	      }
	    else
	      {
		emit(outputReady(QString::number(temp->value.floatval)));
	      }
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
	if (*op == OP_PRINTN)
	  {
	    mutex.lock();
	    emit(outputReady(QString("\n")));
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
	delete temp;
	op++;
      }
      break;

    case OP_CONCAT:
      {
	op++;
	POP2;
	QString result = QString::fromUtf8(one->value.string) + QString::fromUtf8(two->value.string);
	stack.push(result.toUtf8().data());
	delete one;
	delete two;
      }
      break;

    case OP_NUMASSIGN:
      {
	op++;
	int *num = (int *) op;
	op += sizeof(int);
	stackval *temp = stack.pop();


	if (vars[*num].type == T_ARRAY)
	  {
	    delete(vars[*num].value.arr->data.fdata);
	    delete(vars[*num].value.arr);
	  }

	if (temp->type == T_STRING)
	  {
	    printError(tr("String in numeric expression"));
	    return -1;
	  }
	else if (temp->type == T_INT)
	  {
	    vars[*num].type = T_FLOAT;
	    vars[*num].value.floatval = (double) temp->value.intval;
	  } 
	else if (temp->type == T_FLOAT)
	  {
	    vars[*num].type = T_FLOAT;
	    vars[*num].value.floatval = temp->value.floatval;
	  } 
	delete temp;
	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*num]), QString::number(vars[*num].value.floatval), -1));
	  }
      }
      break;

    case OP_STRINGASSIGN:
      {
	op++;
	int *num = (int *) op;
	op += sizeof(int);

	stackval *temp = stack.pop();
	if (temp->type == T_STRING)
	  {
	    vars[*num].type = T_STRING;
	    vars[*num].value.string = strdup(temp->value.string);
	  }
	else
	  {
	    printError(tr("String in numeric expression"));
	    return -1;
	  }
	delete temp;
	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*num]), QString::fromUtf8(vars[*num].value.string), -1));
	  }
      }
      break;

    default:
      status = R_STOPPED; 
      return -1;
      break;
    }

  return 0;
}

