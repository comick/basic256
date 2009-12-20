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

void Stack::swap()
{
	// swap top two elements
	stackval *one = top;
	top = top->next;
	stackval *two = top;
	top = top->next;
	one->next = top;
	two->next = one;
	top = two;
}

int Stack::popint()
{
	stackval *temp = top;
	int i=0;
	if (top) {
		top = top->next;
	}
	if (temp->type == T_INT) {
		i = temp->value.intval;
	}
	else if (temp->type == T_FLOAT) {
		i = (int) temp->value.floatval;
	}
	else if (temp->type == T_STRING)
	{
		i = (int) atoi(temp->value.string);
	}
	delete temp;
	return i;
}

double Stack::popfloat()
{
	stackval *temp = top;
	double f;
	if (top)
	{
		top = top->next;
	}
	if (temp->type == T_FLOAT) {
		f = temp->value.floatval;
	}
	else if (temp->type == T_INT)
	{
		f = (double) temp->value.intval;
	}
	else if (temp->type == T_STRING)
	{
		f = (double) atof(temp->value.string);
	}
	delete temp;
	return f;
}

char* Stack::popstring()
{
	// don't forget to free() the string returned by this function when you are dome with it
	char *s;
	stackval *temp = top;
	if (top)
	{
		top = top->next;
	}
	if (temp->type == T_STRING) {
		s = temp->value.string;
		temp->value.string = NULL;		// set to null so we don't destroy string when stack value is destructed
	}
	else if (temp->type == T_INT)
	{
		char buffer[64];
		sprintf(buffer, "%d", temp->value.intval);
		s = strdup(buffer);
	}
	else if (temp->type == T_FLOAT)
	{
		char buffer[64];
		sprintf(buffer, "%#lf", temp->value.floatval);
		// strip trailing zeros and decimal point
		while(buffer[strlen(buffer)-1]=='0') buffer[strlen(buffer)-1] = 0x00;
		if(buffer[strlen(buffer)-1]=='.') buffer[strlen(buffer)-1] = 0x00;
		// return
		s = strdup(buffer);
	}
	delete temp;
	return s;
}

void Interpreter::printError(QString message)
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
			int val = stack.popint();

			if (val == 0) // go to next line on false, otherwise execute rest of line.
			{
				op = byteCode + *i;
			}
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
			double step = stack.popfloat();
			double endnum = stack.popfloat();
			double startnum = stack.popfloat();

			temp->next = forstack;
			temp->prev = NULL;
			temp->variable = *i;

			vars[*i].type = T_FLOAT;
			vars[*i].value.floatval = startnum;

			if(debugMode)
			{
				emit(varAssignment(QString(symtable[*i]), QString::number(vars[*i].value.floatval), -1));
			}

			temp->endNum = endnum;
			temp->step = step;
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
			char *name = stack.popstring();

			if (stream != NULL)
			{
				stream->close();
				stream = NULL;
			}

			stream = new QFile(name);

			free(name);

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
			char *temp = stack.popstring();

			int error = 0;

			if (stream != NULL)
			{
				quint64 oldPos = stream->pos();
				stream->flush();
				stream->seek(stream->size());
				error = stream->write(temp, strlen(temp));
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

			free(temp);
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

	case OP_SIZE:
		{
			// push the current open file size on the stack
			op++;
			int size = 0;
			if (stream == NULL)
			{
				printError(tr("Can't size -- no open file."));
			} else {
				size = stream->size();
			}
			stack.push(size);
		}
		break;

	case OP_EXISTS:
		{
			// push a 1 if file exists else zero
			op++;
			char *filename = stack.popstring();
			if (QFile::exists(QString(filename)))
			{
				stack.push((int) 1);
			} else {
				stack.push((int) 0);
			}
			free(filename);
		}
		break;

	case OP_SEEK:
		{
			// move file pointer to a specific loaction in file
			op++;
			long pos = stack.popint();

			if (stream == NULL)
			{
				printError(tr("seek() called when no file is open"));
				return -1;
			}
			else
			{
				stream->seek(pos);
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
			int size = stack.popint();

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

			char *val = stack.popstring(); // dont free - assigning to a string variable
			int index = stack.popint();

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
			strarray[index] = val;

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
				char *str = stack.popstring(); // dont free we are assigning this to a variable
				if (strarray[index])
				{
					delete(strarray[index]);
				}
				strarray[index] = str;
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString(strarray[index]), index));
				}
			}
		}
		break;

	case OP_ARRAYASSIGN:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			double val = stack.popfloat();
			int index = stack.popint();
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
				double one = stack.popfloat();
				array[index] = one;
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
			int index = stack.popint();

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
			int val = stack.popint();
			stack.push(val);
		}
		break;


	case OP_FLOAT:
		{
			op++;
			double val = stack.popfloat();
			stack.push(val);
		}
		break;

	case OP_STRING:
		{
			op++;
			char *temp = stack.popstring();
			stack.push(temp);
			free(temp);
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
			double val = stack.popfloat();
			int stime = (int) (val * 1000);
			msleep(stime);
		}
		break;

	case OP_LENGTH:
		{
			op++;
			char *temp = stack.popstring();
			stack.push((int) strlen(temp));
			free(temp);
		}
		break;


	case OP_MID:
		{
			op++;
			int len = stack.popint();
			int pos = stack.popint();
			char *temp = stack.popstring();

			if ((pos < 0) || (len < 0))
			{
				printError(tr("Illegal argument to mid()"));
				return -1;
			}

			if (pos > (int) strlen(temp))
			{
				printError(tr("String not long enough for given starting character"));
				return -1;
			}

			temp += (pos - 1);

			if (len < (int) strlen(temp))
			{
				temp[len] = '\0';
			}

			stack.push(strdup(temp));

			free(temp);
		}
		break;


	case OP_ASC:
		{
			op++;
			char *str = stack.popstring();
			stack.push((int) str[0]);
			free(str);
		}
		break;


	case OP_CHR:
		{
			op++;
			int code = stack.popint();
			char temp[2];
			memset(temp, 0, 2);
			temp[0] = (char) code;
			stack.push(temp);
		}
		break;


	case OP_INSTR:
		{
			op++;
			char *str = stack.popstring();
			char *hay = stack.popstring();

			int pos = 0;

			char *ptr = strstr(hay, str);

			if (ptr != NULL)
			{
				pos = (ptr - hay) + 1;
			}

			stack.push((int) pos);

			free(str);
			free(hay);
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
			double val = stack.popfloat();
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
			int one = stack.popint();
			int two = stack.popint();
			if (one && two)
			{
				stack.push(1);
			}
			else
			{
				stack.push(0);
			}
		}
		break;

	case OP_OR:
		{
			op++;
			int one = stack.popint();
			int two = stack.popint();
			if (one || two)
			{
				stack.push(1);
			}
			else
			{
				stack.push(0);
			}
		}
		break;

	case OP_XOR:
		{
			op++;
			int one = stack.popint();
			int two = stack.popint();
			if (!(one && two) && (one || two))
			{
				stack.push(1);
			}
			else
			{
				stack.push(0);
			}
		}
		break;

	case OP_NOT:
		{
			op++;
			int temp = stack.popint();
			if (temp)
			{
				stack.push(0);
			}
			else
			{
				stack.push(1);
			}
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
			int oneval = stack.popint();
			int twoval = stack.popint();
			emit(soundReady(oneval, twoval));
		}
		break;

	case OP_SAY:
		{
			op++;
			char *temp = stack.popstring();
			//mutex.lock();
			emit(speakWords(QString(temp)));
			//waitCond.wait(&mutex);
			//mutex.unlock();
			free(temp);
		}
		break;

	case OP_WAVPLAY:
		{
			op++;
			char *file = stack.popstring();
			emit(playWAV(QString(file)));
			free(file);
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
			int bval = stack.popint();
			int gval = stack.popint();
			int rval = stack.popint();
			pencolor = QColor(rval, gval, bval);
		}
		break;


	case OP_LINE:
		{
			op++;
			int y1val = stack.popint();
			int x1val = stack.popint();
			int y0val = stack.popint();
			int x0val = stack.popint();

			QPainter ian(image);
			ian.setPen(pencolor);
			ian.setBrush(pencolor);
			if (x1val >= 0 && y1val >= 0)
			{
				ian.drawLine(x0val, y0val, x1val, y1val);
			}
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_RECT:
		{
			op++;
			int y1val = stack.popint();
			int x1val = stack.popint();
			int y0val = stack.popint();
			int x0val = stack.popint();

			QPainter ian(image);
			ian.setPen(pencolor);
			ian.setBrush(pencolor);
			if (x1val > 0 && y1val > 0)
			{
				ian.drawRect(x0val, y0val, x1val - 1, y1val - 1);
			}
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_POLY:
		{
			// doing a polygon from an array
			// i is a pointer to the variable number (array)
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			QPainter poly(image);
			poly.setPen(pencolor);
			poly.setBrush(pencolor);

			if (vars[*i].type == T_ARRAY)
			{
				int pairs = vars[*i].value.arr->size / 2;
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
			else
			{
				printError(tr("Argument not an array for poly()"));
				return -1;
			}

			poly.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_POLY_LIST:
		{
			// doing a polygon from an immediate list
			// i is a pointer to the length of the list
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			QPainter poly(image);
			poly.setPen(pencolor);
			poly.setBrush(pencolor);

			int pairs = *i / 2;
			if (pairs < 3)
			{
				printError(tr("Not enough points in immediate list for poly()"));
				return -1;
			}
			QPointF points[pairs];
			for (int j = 0; j < pairs; j++)
			{
				int ypoint = stack.popint();
				int xpoint = stack.popint();
				points[j].setX(xpoint);
				points[j].setY(ypoint);
			}
			poly.drawPolygon(points, pairs);
				
			poly.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_STAMP:
		{
			// special type of poly where x,y,scale, are given first and
			// the ploy is sized and loacted - so we can move them easy
			// doing a stamp from an array
			// i is a pointer to the variable number (array)
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			double rotate = stack.popfloat();
			double scale = stack.popfloat();
			int y = stack.popint();
			int x = stack.popint();

			QPainter poly(image);
			poly.setPen(pencolor);
			poly.setBrush(pencolor);

			if (vars[*i].type == T_ARRAY)
			{
				int pairs = vars[*i].value.arr->size / 2;
				if (pairs < 3)
				{
					printError(tr("Not enough points in array for stamp()"));
					return -1;
				}

				double *array = vars[*i].value.arr->data.fdata;
				QPointF points[pairs];

				for (int j = 0; j < pairs; j++)
				{
					double scalex = scale * array[j*2];
					double scaley = scale * array[(j*2)+1];
					double rotx = cos(rotate) * scalex - sin(rotate) * scaley;
					double roty = cos(rotate) * scaley + sin(rotate) * scalex;
					points[j].setX(rotx + x);
					points[j].setY(roty + y);
				}
				poly.drawPolygon(points, pairs);
			} 
			else
			{
				printError(tr("Argument not an array for stamp()"));
				return -1;
			}

			poly.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_STAMP_LIST:
	case OP_STAMP_S_LIST:
	case OP_STAMP_SR_LIST:
		{
			// special type of poly where x,y,scale, are given first and
			// the ploy is sized and loacted - so we can move them easy
			// doing a polygon from an immediate list
			// i is a pointer to the length of the list
			// pulling from stack so points are reversed 0=y, 1=x...  in list

			double rotate=0;		// defaule rotation to 0 radians
			double scale=1;			// default scale to full size (1x)
			
			unsigned char opcode = *op;
			op++;
			int *i = (int *) op;
			int llist = *i;
			op += sizeof(int);
			
			// pop the immediate list to uncover the location and scale
			int *list = (int *) calloc(llist, sizeof(int));
			for(int j = llist; j>0 ; j--) {
				list[j-1] = stack.popint();
			}
			
			if (opcode==OP_STAMP_SR_LIST) rotate = stack.popfloat();
			if (opcode==OP_STAMP_SR_LIST || opcode==OP_STAMP_S_LIST) scale = stack.popfloat();
			int y = stack.popint();
			int x = stack.popint();
			
			//char message[1024];
			//sprintf(message, "opcode= %d x=%d y=%d scale=%f rotate=%f", opcode,x,y,scale,rotate);
			//printError(message);

			QPainter poly(image);
			poly.setPen(pencolor);
			poly.setBrush(pencolor);

			int pairs = llist / 2;
			if (pairs < 3)
			{
				printError(tr("Not enough points in immediate list for stamp()"));
				return -1;
			}
			QPointF points[pairs];
			for (int j = 0; j < pairs; j++)
			{
				double scalex = scale * list[(j*2)];
				double scaley = scale * list[(j*2)+1];
				double rotx = cos(rotate) * scalex - sin(rotate) * scaley;
				double roty = cos(rotate) * scaley + sin(rotate) * scalex;
				points[j].setX(rotx + x);
				points[j].setY(roty + y);
			}
			poly.drawPolygon(points, pairs);
			
			poly.end();
			
			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_CIRCLE:
		{
			op++;
			int rval = stack.popint();
			int yval = stack.popint();
			int xval = stack.popint();

			QPainter ian(image);
			ian.setPen(pencolor);
			ian.setBrush(pencolor);
			ian.drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_TEXT:
		{
			op++;
			char *txt = stack.popstring();
			int y0val = stack.popint();
			int x0val = stack.popint();

			QPainter ian(image);
			ian.setPen(pencolor);
			ian.setBrush(pencolor);
			if(!fontfamily.isEmpty()) {
				ian.setFont(QFont(fontfamily, fontpoint, fontweight));
			}
			ian.drawText(x0val, y0val+(QFontMetrics(ian.font()).ascent()), QString::fromAscii(txt));
			ian.end();
			free(txt);

			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_FONT:
		{
			op++;
			fontweight = stack.popint();
			fontpoint = stack.popint();
			char *family = stack.popstring();
			fontfamily = QString::fromAscii(family);
			free(family);
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
			int oneval = stack.popint();
			int twoval = stack.popint();

			QPainter ian(image);
			ian.setPen(pencolor);

			ian.drawPoint(twoval, oneval);
			ian.end();

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
			int oneval = stack.popint();
			int twoval = stack.popint();
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
			char *temp = stack.popstring();
			mutex.lock();
			emit(outputReady(QString(temp)));
			waitCond.wait(&mutex);
			mutex.unlock();
			if (*op == OP_PRINTN)
			{
				mutex.lock();
				emit(outputReady(QString("\n")));
				waitCond.wait(&mutex);
				mutex.unlock();
			}
			free(temp);
			op++;
		}
		break;

	case OP_CONCAT:
		{
			op++;
			char *one = stack.popstring();
			char *two = stack.popstring();
			int len = strlen(one) + strlen(two) + 1;
			char *buffer = (char *) malloc(len);
			if (buffer)
			{
				strcpy(buffer, two);
				strcat(buffer, one);
			}
			stack.push(buffer);
			free(one);
			free(two);
		}
		break;

	case OP_NUMASSIGN:
		{
			op++;
			int *num = (int *) op;
			op += sizeof(int);
			double temp = stack.popfloat();


			if (vars[*num].type == T_ARRAY)
			{
				delete(vars[*num].value.arr->data.fdata);
				delete(vars[*num].value.arr);
			}

			vars[*num].type = T_FLOAT;
			vars[*num].value.floatval = temp;

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

			char *temp = stack.popstring();	// don't free - assigned to a variable
			vars[*num].type = T_STRING;
			vars[*num].value.string = temp;

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

	case OP_STACKSWAP:
		{
			op++;
			stack.swap();
		}
		break;


	default:
		status = R_STOPPED;
		return -1;
		break;
	}




	return 0;
}





