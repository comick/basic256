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
#include <time.h>
#include <cmath>
#include <string>
#include <QString>
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
  extern int column;
  extern int newByteCode(int size);
  extern unsigned char *byteCode;
  extern unsigned int byteOffset;
  extern unsigned int maxbyteoffset;
  extern char *symtable[];
}

static int compareTwoStackVal(stackval *two, stackval *one)
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	// DOES NOT COMPARE ONE STRING WITH ONE NUMBER...
	//
	// both integers - compare
	if (one->type == T_INT && two->type == T_INT)
    {
		if (one->value.intval == two->value.intval) return 0;
		else if (one->value.intval < two->value.intval) return -1;
		else return 1;
    }
	// both floats - compare
	if (one->type == T_FLOAT && two->type == T_FLOAT)
    {
		if (one->value.floatval == two->value.floatval) return 0;
		else if (one->value.floatval < two->value.floatval) return -1;
		else return 1;
    }
	// both strings - strcmp
	if (one->type == T_STRING && two->type == T_STRING)
    {
		return(strcmp(one->value.string, two->value.string));
	}
	// mix of floats and integers - float both and compare	
	if (one->type == T_INT) one->value.floatval = (double) one->value.intval;
	if (two->type == T_INT) two->value.floatval = (double) two->value.intval;
	if (one->value.floatval == two->value.floatval) return 0;
	else if (one->value.floatval < two->value.floatval) return -1;
	else return 1;
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

stackval *
Stack::popint()
{
  stackval *temp = top;
  if (top)
    {
      top = top->next;
    }
    if (temp->type == T_FLOAT)
    {
	temp->value.intval = (int) temp->value.floatval;
	temp->type = T_INT;
    }
    else if (temp->type == T_STRING)
    {
      	temp->value.intval = (int) atoi(temp->value.string);
	temp->type = T_INT;
    }

  return temp;
}

stackval *
Stack::popfloat()
{
  stackval *temp = top;
  if (top)
    {
      top = top->next;
    }
  if (temp->type == T_INT)
    {
	temp->value.floatval = (double) temp->value.intval;
	temp->type = T_FLOAT;
    }
  else if (temp->type == T_STRING)
    {
      	temp->value.floatval = (double) atof(temp->value.string);
	temp->type = T_FLOAT;
    }

  return temp;
}

stackval *
Stack::popstring()
{
  stackval *temp = top;
  if (top)
    {
      top = top->next;
    }
  if (temp->type == T_INT)
    {
	char buffer[64];
        sprintf(buffer, "%d", temp->value.intval);
       temp->value.string = strdup(buffer);
	temp->type = T_STRING;
    }
  else if (temp->type == T_FLOAT)
    {
	char buffer[64];
        sprintf(buffer, "%lf", temp->value.floatval);
        temp->value.string = strdup(buffer);
	temp->type = T_STRING;
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


Interpreter::Interpreter(BasicGraph *bg)
{
  image = bg->image;
  graph = bg;
  fastgraphics = false;
  status = R_STOPPED;
  for (int i = 0; i < NUMVARS; i++)
    {
      vars[i].type = T_UNUSED;
      vars[i].value.floatval = 0;
      vars[i].value.string = NULL;
      vars[i].value.arr = NULL;
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
      vars[i].value.arr = NULL;
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
	    if(column==0) {
		    emit(outputReady(tr("Syntax error on line ") + QString::number(linenumber) + tr(" around end of line.") + "\n"));
	    } else {
		    emit(outputReady(tr("Syntax error on line ") + QString::number(linenumber) + tr(" around column ") + QString::number(column) + ".\n"));
	    }
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
  emit(resizeGraph(300, 300));
  image = graph->image;
  fontfamily = QString::QString();
  fontpoint = 0;
  fontweight = 0;

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


void
Interpreter::waitForGraphics()
  {
    // wait for graphics operation to complete
    mutex.lock();
    emit(goutputReady());
    waitCond.wait(&mutex);
    mutex.unlock();
  }


int
Interpreter::execByteCode()
{
  if (status == R_INPUTREADY)
    {
      stack.push(strdup(inputString.toAscii().data()));
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
	stackval *temp = stack.popint();
	
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
	stackval *step = stack.popfloat();
	stackval *endnum = stack.popfloat();
	stackval *startnum = stack.popfloat();

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
	
       temp->endNum = endnum->value.floatval;
       temp->step = step->value.floatval;
       temp->returnAddr = op;
	if (forstack)
	  {
	    forstack->prev = temp;
	  }
	forstack = temp;
	if (temp->step > 0 && vars[*i].value.floatval > temp->endNum)
	  {
	    printError(tr("Illegal FOR -- start number > end number"));
	    return -1;
	  }
	else if (temp->step < 0 && vars[*i].value.floatval < temp->endNum)
	  {
	    printError(tr("Illegal FOR -- start number < end number"));
	    return -1;
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
	stackval *name = stack.popstring();

        if (stream != NULL)
          {
            stream->close();
	    stream = NULL;
	  }

	stream = new QFile(name->value.string);

        delete name;

	if (stream == NULL || !stream->open(QIODevice::ReadWrite | QIODevice::Text))
	  {
	    printError(tr("Unable to open file"));
	    return -1;
	  }
	
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
		stack.push(strdup(""));
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

	stack.push(strdup(strarray));
	free(strarray);
      }
      break;

      
    case OP_READLINE:
      {
	op++;

	if (stream == NULL)
	  {
	    printError(tr("Can't read -- no open file."));
	    return -1;
	  }

	//read entire line
	int maxsize = 2048;
	char * strarray = (char *) malloc(maxsize);
	memset(strarray, 0, maxsize);
	stream->readLine(strarray, maxsize);
	while((char) strarray[strlen(strarray)-1] == '\n') strarray[strlen(strarray)-1] = (char) 0x00; 
	stack.push(strdup(strarray));
	free(strarray);
      }
      break;
      
    case OP_EOF:
      {
	op++;

	if (stream == NULL)
	  {
	    printError(tr("Can't read -- no open file."));
	    return -1;
	  }
	
	  if (stream->atEnd()) {
            stack.push(1);
	  } else {
            stack.push(0);
	  }
      }
      break;


    case OP_WRITE:
    case OP_WRITELINE:
      {
	unsigned char whichop = *op;
	op++;
	stackval *temp = stack.popstring();

	int error = 0;

	if (stream != NULL)
	  {
	    quint64 oldPos = stream->pos();
	    stream->flush();
	    stream->seek(stream->size());
	    error = stream->write(temp->value.string, strlen(temp->value.string));
	    if (whichop == OP_WRITELINE)
	      {
	        error = stream->write("\n", 1);
	      }
	    stream->seek(oldPos);
	    stream->flush();
	  }

	 if (error == -1)
	   {
	     printError(tr("Unable to write to file"));
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
	stackval *one = stack.popint();

	size = one->value.intval;;
	
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
	//one = expr, two = index
	stackval *one = stack.popstring();
	stackval *two = stack.popint();

	int index = two->value.intval;
	char **strarray;

	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_STRARRAY)
	  {
	    printError(tr("Not a string array variable"));
	    return -1;
	  }
	else if (index >= vars[*i].value.arr->size || index < 0)
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
	    emit(varAssignment(QString(symtable[*i]), QString(strarray[index]), index));
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

	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_STRARRAY)
	  {
	    printError(tr("Not a string array variable"));
	    return -1;
	  }
	else if (items > vars[*i].value.arr->size || items < 0)
	  {
	    printError(tr("Array dimension too small"));
	    return -1;
	  }
	
	strarray = vars[*i].value.arr->data.sdata;
	for (index = items - 1; index >= 0; index--)
	  {
	    stackval *one = stack.popstring();
	    str = strdup(one->value.string); 
	    if (strarray[index])
	      {
		delete(strarray[index]);
	      }
	    strarray[index] = str;
	    delete one;
	    if(debugMode)
	      {
		emit(varAssignment(QString(symtable[*i]), QString(strarray[index]), index));
	      }
	  }
      }
      break;
    

    case OP_STRARRAYINPUT:
      {
		  // this is almost exactly the same  as OP_STRARRAYASSIGN except that the two stack items
		  // come in reversed order.  The OP_INPUT has already put the value to assign on the stack
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	//one = index, two = expr
	stackval *one = stack.popint();
	stackval *two = stack.popstring();
	int index = one->value.intval;;
	char **strarray;

	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_STRARRAY)
	  {
	    printError(tr("Not a string array variable"));
	    return -1;
	  }
	else if (index > vars[*i].value.arr->size || index < 0)
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
	 //one = expr, two = index
	stackval *one = stack.popfloat();
	stackval *two = stack.popint();
	int index = two->value.intval;
	double val = one->value.floatval;
	double *array;

	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_ARRAY)
	  {
	    printError(tr("Not an array variable"));
	    return -1;
	  }
	else if (index >= vars[*i].value.arr->size || index < 0)
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
	
	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_ARRAY)
	  {
	    printError(tr("Not an array variable"));
	    return -1;
	  }
	else if (items > vars[*i].value.arr->size || items < 0)
	  {
	    printError(tr("Array dimension too small"));
	    return -1;
	  }
	
	array = vars[*i].value.arr->data.fdata;
	for (index = items - 1; index >= 0; index--)
	  {
	    stackval *one = stack.popfloat();
	    array[index] = one->value.floatval;
	    delete one;
	    if(debugMode)
	      {
		emit(varAssignment(QString(symtable[*i]), QString::number(val), index));
	      }
	  }
      }
      break;
    
    case OP_ARRAYINPUT:
      {
		  // this is almost exactly the same  as OP_ARRAYASSIGN except that the two stack items
		  // come in reversed order.  The OP_INPUT has already put the value to assign on the stack
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	 //one = expr, two = index
	stackval *two = stack.popfloat();	// value
	stackval *one = stack.popint();	// index
	int index = one->value.intval;
	double val = two->value.floatval;
	double *array;

	if (vars[*i].type == T_UNUSED)
	  {
	    printError(tr("Unknown variable"));
	    return -1;
	  }
	else if (vars[*i].type != T_ARRAY)
	  {
	    printError(tr("Not an array variable"));
	    return -1;
	  }
	else if (index >= vars[*i].value.arr->size || index < 0)
	  {
		  //char b1[64];
		  //sprintf(b1, "%d", vars[*i].value.arr->size);
		  //char b2[64];
		  //sprintf(b2, "%d", index);
		  //char b3[64];
		  //sprintf(b3, "%g", val);
		  printError(tr("Array index out of bounds"));
		  //printError(b1);
		  //printError(b2);
		  //printError(b3);
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

    case OP_DEREF:
      {
	op++;
	int *i = (int *) op;
	op += sizeof(int);
	stackval *temp = stack.popint();
	int index = temp->value.intval;
	
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
	stackval *temp = stack.popint();
        stack.push(temp->value.intval);
	delete temp;
      }
      break;


    case OP_FLOAT:
      {
	op++;
	stackval *temp = stack.popfloat();
        stack.push(temp->value.floatval);
	delete temp;
      }
      break;

    case OP_STRING:
      {
	op++;
	stackval *temp = stack.popstring();
        stack.push(temp->value.string);
	delete temp;
      }
      break;

    case OP_RAND:
      {
	double r = 1.0;
	double ra;
	double rx;
	op++;
	if (once)
	  {
	    int ms = 999 + QTime::currentTime().msec();
	    once = false;
	    srand(time(NULL) * ms);
	  }
	while(r == 1.0) {
	  ra = (double) rand() * (double) RAND_MAX + (double) rand();
	  rx = (double) RAND_MAX * (double) RAND_MAX + (double) RAND_MAX + 1.0;
	  r = ra/rx;
        }
	stack.push(r);
      }
      break;

    case OP_PAUSE:
      {
	op++;
	double val = 0;
	stackval *temp = stack.popfloat();
	val = temp->value.floatval;
	int stime = (int) (val * 1000);
	msleep(stime);
      }
      break;

    case OP_LENGTH:
      {
	op++;
	stackval *temp = stack.popstring();
        stack.push((int) strlen((char *) temp->value.string));
	delete temp;
      }
      break;


    case OP_MID:
      {
	op++;
	stackval *len = stack.popint();
	stackval *pos = stack.popint();
	stackval *str = stack.popstring();

	if ((pos->value.intval < 0) || (len->value.intval < 0))
	  {
	    printError(tr("Illegal argument to mid()"));
	    return -1;
	  }

	char *temp = (char *) str->value.string;

	if (pos->value.intval > (int) strlen(temp))
	  {
	    printError(tr("String not long enough for given starting character"));
	    return -1;
	  }

	temp += (pos->value.intval - 1);

	if (len->value.intval < (int) strlen(temp))
	  {
	    temp[len->value.intval] = '\0';
	  }

	stack.push(strdup(temp));

	delete str;
	delete pos;
	delete len;
      }
      break;


    case OP_ASC:
      {
	op++;
	stackval *str = stack.popstring();
	stack.push((int) str->value.string[0]);
	delete str;
      }
      break;


    case OP_CHR:
      {
	op++;
	stackval *code = stack.popint();
	char temp[2];
	memset(temp, 0, 2);
	temp[0] = (char) code->value.intval;
	stack.push(temp);
	delete code;
      }
      break;


    case OP_INSTR:
      {
	op++;
	stackval *needle = stack.popstring();
	stackval *haystk = stack.popstring();

	int pos = 0;

	char *hay = (char *) haystk->value.string;
	char *str = (char *) needle->value.string;
	char *ptr = strstr(hay, str);

	if (ptr != NULL)
	  {
	    pos = (ptr - hay) + 1;
	  }

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
	stackval *temp = stack.popfloat();
	double val;
	val = temp->value.floatval;
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
    case OP_MOD:
    case OP_DIV:
    case OP_EXP:
      {
	stackval *one = stack.pop();
	stackval *two = stack.pop();
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
	      case OP_MOD:
		stack.push(two->value.intval % one->value.intval);
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
	      case OP_MOD:
		stack.push((int) twoval % (int) oneval);
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
	stackval *one = stack.popint();
	stackval *two = stack.popint();
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
	stackval *one = stack.popint();
	stackval *two = stack.popint();
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
	stackval *one = stack.popint();
	stackval *two = stack.popint();
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
	stackval *temp = stack.popint();
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
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)==0)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_NEQUAL:
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)!=0)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_GT:
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)==1)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_LTE:
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)!=1)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_LT:
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)==-1)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_GTE:
		{
			op++;
			POP2
			if(compareTwoStackVal(one,two)!=-1)
				stack.push(1);
			else
				stack.push(0);
			delete one;
			delete two;
		}
		break;
		
    case OP_SOUND:
      {
	op++;
	stackval *one = stack.popint();
	stackval *two = stack.popint();
	int oneval = one->value.intval;
	int twoval = two->value.intval;
	emit(soundReady(oneval, twoval));
	delete one;
	delete two;
      }
      break;
      
    case OP_SAY:
      {
	op++;
	stackval *temp = stack.popstring();
        //mutex.lock();
	emit(speakWords(QString(temp->value.string)));
	//waitCond.wait(&mutex);
	//mutex.unlock();
	delete temp;
      }
      break;

    case OP_WAVPLAY:
      {
        op++;
        stackval *file = stack.popstring();
        emit(playWAV(QString(file->value.string)));
         delete file;
      }
      break;

    case OP_WAVSTOP:
      {
        op++;
        emit(stopWAV());
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


    case OP_SETCOLORRGB:
      {
	op++;
	stackval *b = stack.popint();
	stackval *g = stack.popint();
	stackval *r = stack.popint();
	int rval = r->value.intval;
	int gval = g->value.intval;
	int bval = b->value.intval;
	pencolor = QColor(rval, gval, bval);
	delete r;
	delete g;
	delete b;
      }
      break;


    case OP_LINE:
      {
	op++;
	stackval *y1 = stack.popint();
	stackval *x1 = stack.popint();
	stackval *y0 = stack.popint();
	stackval *x0 = stack.popint();
	int x0val = x0->value.intval;
	int y0val = y0->value.intval;
	int x1val = x1->value.intval;
	int y1val = y1->value.intval;
	
	QPainter ian(image);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if (x1val >= 0 && y1val >= 0)
	  {
	    ian.drawLine(x0val, y0val, x1val, y1val);
	  }
	ian.end();
	delete x0;
	delete y0;
	delete x1;
	delete y1;

	if (!fastgraphics) waitForGraphics();
      }
      break;


    case OP_RECT:
      {
	op++;
	stackval *y1 = stack.popint();
	stackval *x1 = stack.popint();
	stackval *y0 = stack.popint();
	stackval *x0 = stack.popint();
	int x0val = x0->value.intval;
	int y0val = y0->value.intval;
	int x1val = x1->value.intval;
	int y1val = y1->value.intval;
	
	QPainter ian(image);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if (x1val > 0 && y1val > 0)
	  {
	    ian.drawRect(x0val, y0val, x1val - 1, y1val - 1);
	  }
	ian.end();
	delete x0;
	delete y0;
	delete x1;
	delete y1;

	if (!fastgraphics) waitForGraphics();
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
		stackval *xpoint = stack.popint();
	        stackval *ypoint = stack.popint();
		points[j].setX(xpoint->value.intval);
		points[j].setY(ypoint->value.intval);
		delete xpoint;
		delete ypoint;
	      }
	    poly.drawPolygon(points, pairs);
	  }

	poly.end();

	if (!fastgraphics) waitForGraphics();
      }
      break;

    case OP_CIRCLE:
      {
	op++;
	stackval *r = stack.popint();
	stackval *y = stack.popint();
	stackval *x = stack.popint();
	int xval = x->value.intval;
	int yval = y->value.intval;
	int rval = r->value.intval;
	
	QPainter ian(image);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	ian.drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);
	ian.end();
	delete x;
	delete y;
	delete r;

	if (!fastgraphics) waitForGraphics();
      }
      break;

    case OP_TEXT:
      {
	op++;
	stackval *txt = stack.popstring();
	stackval *y0 = stack.popint();
	stackval *x0 = stack.popint();
	int x0val = x0->value.intval;
	int y0val = y0->value.intval;
	
	QPainter ian(image);
	ian.setPen(pencolor);
	ian.setBrush(pencolor);
	if(!fontfamily.isEmpty()) {
	   ian.setFont(QFont(fontfamily, fontpoint, fontweight));
	}
        ian.drawText(x0val, y0val+(QFontMetrics(ian.font()).ascent()), QString::fromAscii(txt->value.string));
	ian.end();
	delete x0;
	delete y0;
	delete txt;

	if (!fastgraphics) waitForGraphics();
      }
      break;
      
      
    case OP_FONT:
      {
	op++;
	stackval *weight = stack.popint();
	stackval *point = stack.popint();
	stackval *family = stack.popstring();
	fontfamily = QString::fromAscii(family->value.string);
	fontpoint = point->value.intval;
	fontweight = weight->value.intval;
	delete weight;
	delete point;
	delete family;
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
	image->fill(Qt::color0);
	if (!fastgraphics) waitForGraphics();
      }
      break;
      
    case OP_PLOT:
      {
	op++;
	stackval *one = stack.popint();
	stackval *two = stack.popint();
	int oneval = one->value.intval;
	int twoval = two->value.intval;

	QPainter ian(image);
	ian.setPen(pencolor);

	ian.drawPoint(twoval, oneval);
	ian.end();
	delete one;
	delete two;

	if (!fastgraphics) waitForGraphics();
      }
      break;

    case OP_FASTGRAPHICS:
      {
	op++;
	fastgraphics = true;
	emit(fastGraphics());
      }
      break;

    case OP_GRAPHSIZE:
      {
	int width = 300, height = 300;
	op++;
	stackval *one = stack.popint();
	stackval *two = stack.popint();
	int oneval = one->value.intval;
	int twoval = two->value.intval;
	if (oneval>0) height = oneval;
	if (twoval>0) width = twoval;
	if (width > 0 && height > 0)
	  {
	    mutex.lock();
	    emit(resizeGraph(width, height));
	    waitCond.wait(&mutex);
	    mutex.unlock();
	  }
	image = graph->image;
	delete one;
	delete two;
      }
      break;

    case OP_GRAPHWIDTH:
      {
		  op++;
		  stack.push((int) graph->image->width());
      }
      break;

    case OP_GRAPHHEIGHT:
      {
		  op++;
		  stack.push((int) graph->image->height());
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
	stackval *temp = stack.popstring();
        mutex.lock();
	emit(outputReady(QString(temp->value.string)));
	waitCond.wait(&mutex);
	mutex.unlock();
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
	stackval *one = stack.popstring();
	stackval *two = stack.popstring();
	int len = strlen(one->value.string) + strlen(two->value.string) + 1;
	char *buffer = (char *) malloc(len);
	if (buffer)
	  {
	    strcpy(buffer, two->value.string);
	    strcat(buffer, one->value.string);
	  }
	stack.push(buffer);
	delete one;
	delete two;
      }
      break;

    case OP_NUMASSIGN:
      {
	op++;
	int *num = (int *) op;
	op += sizeof(int);
	stackval *temp = stack.popfloat();


	if (vars[*num].type == T_ARRAY)
	  {
	    delete(vars[*num].value.arr->data.fdata);
	    delete(vars[*num].value.arr);
	  }

        vars[*num].type = T_FLOAT;
	vars[*num].value.floatval = temp->value.floatval;
	
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

	stackval *temp = stack.popstring();
        vars[*num].type = T_STRING;
	vars[*num].value.string = strdup(temp->value.string);
	
	delete temp;
	if(debugMode)
	  {
	    emit(varAssignment(QString(symtable[*num]), QString(vars[*num].value.string), -1));
	  }
      }
      break;

    case OP_YEAR:
    case OP_MONTH:
    case OP_DAY:
    case OP_HOUR:
    case OP_MINUTE:
    case OP_SECOND:
      {
        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        switch (*op)
	  {
	    case OP_YEAR:
              stack.push(timeinfo->tm_year + 1900);
	      break;
	    case OP_MONTH:
              stack.push(timeinfo->tm_mon);
	      break;
	    case OP_DAY:
              stack.push(timeinfo->tm_mday);
	      break;
	    case OP_HOUR:
              stack.push(timeinfo->tm_hour);
	      break;
	    case OP_MINUTE:
              stack.push(timeinfo->tm_min);
	      break;
	    case OP_SECOND:
              stack.push(timeinfo->tm_sec);
	      break;
	  }
	op++;
      }
      break;
      
      
    default:
      status = R_STOPPED; 
      return -1;
      break;
    }
    


    
  return 0;
}





