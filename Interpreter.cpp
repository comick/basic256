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



#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <cmath>
#include <string>
#include <QString>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
using namespace std;

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


void Interpreter::clearsprites() {
	// meed to implement to cleanup garbage
}

void Interpreter::spriteundraw(int n) {
	// undraw all visible sprites >= n
	int x, y, i;
	QPainter ian(image);
	i = nsprites-1;
	while(i>=n) {
		if (sprites[i].active && sprites[i].visible) {
			x = sprites[i].x- sprites[i].image->width()/2;
			y = sprites[i].y - sprites[i].image->height()/2;
			ian.drawImage(x, y, *(sprites[i].underimage));
		}
		i--;
	}
	ian.end();
}

void Interpreter::spriteredraw(int n) {
	int x, y, i;
	// redraw all sprites n to nsprites-1
	i = n;
	while(i<nsprites) {
		if (sprites[i].active && sprites[i].visible) {
			x = sprites[i].x - sprites[i].image->width()/2;
			y = sprites[i].y - sprites[i].image->height()/2;
			delete sprites[i].underimage;
			sprites[i].underimage = new QImage(image->copy(x, y, sprites[i].image->width(), sprites[i].image->height()));
			QPainter ian(image);
			ian.drawImage(x, y, *(sprites[i].image));
			ian.end();
		}
		i++;
	}
}

bool Interpreter::spritecollide(int n1, int n2) {
	int top1, bottom1, left1, right1;
	int top2, bottom2, left2, right2;
	
	if (n1==n2) return true;
	
	left1 = sprites[n1].x - sprites[n1].image->width()/2;
	left2 = sprites[n2].x - sprites[n2].image->width()/2;
	right1 = left1 + sprites[n1].image->width();
	right2 = left2 + sprites[n2].image->width();
	top1 = sprites[n1].y - sprites[n1].image->height()/2;
	top2 = sprites[n2].y - sprites[n2].image->height()/2;
	bottom1 = top1 + sprites[n1].image->height();
	bottom2 = top2 + sprites[n2].image->height();

   if (bottom1<top2) return false;
   if (top1>bottom2) return false;
   if (right1<left2) return false;
   if (left1>right2) return false;
   return true;
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
	nsprites = 0;
}


void
Interpreter::cleanup()
{
	status = R_STOPPED;
	//Clean up stack
	stack.clear();
	//Clean up variables
	clearvars();
	//Clean up sprites
	clearsprites();
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
	while (status != R_STOPPED && execByteCode() >= 0) {} //continue
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

			free(temp);

			if (error == -1)
			{
				printError(tr("Unable to write to file"));
			}

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
	case OP_REDIM:
	case OP_REDIMSTR:
		{
			unsigned char whichdim = *op;
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			int var = i[0];
			int ydim = stack.popint();
			int xdim = stack.popint();

			int size = xdim * ydim;

			if (size > 100000)
			{
				printError(tr("Array dimension too large"));
				return -1;
			} else if (size < 1)
			{
				printError(tr("Array dimension too small"));
				return -1;
			}

			if (whichdim == OP_REDIM || whichdim == OP_REDIMSTR) {
				if (vars[var].type == T_UNUSED)
				{
					printError(tr("Unknown variable"));
					return -1;
				}	
			}
			
			array *temp = new array;
			
			if (whichdim == OP_DIM || whichdim == OP_REDIM)
			{
				double *d = new double[size];
				for (int j = 0; j < size; j++)
				{
					if(whichdim == OP_REDIM && j < vars[var].value.arr->size) {
						d[j] = vars[var].value.arr->data.fdata[j];						
					} else {
						d[j] = 0;
					}
				}
				vars[var].type = T_ARRAY;
				temp->data.fdata = d;
				temp->size = size;
				temp->xdim = xdim;
				temp->ydim = ydim;
				vars[var].value.arr = temp;
			}
			else
			{
				char **c = new char*[size];
				for (int j = 0; j < size; j++)
				{
					if(whichdim == OP_REDIMSTR && j < vars[var].value.arr->size) {
						c[j] = vars[var].value.arr->data.sdata[j];						
					} else {
						c[j] = strdup("");
					}
				}
				vars[var].type = T_STRARRAY;
				temp->data.sdata = c;
				temp->size = size;
				temp->xdim = xdim;
				temp->ydim = ydim;
				vars[var].value.arr = temp;
			}

			if(debugMode)
			{
				emit(varAssignment(QString(symtable[var]), NULL, size));
			}
		}
		break;


	case OP_ALEN:
		{
			// return array length (total one dimensional length)

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			if (vars[*i].type == T_ARRAY || vars[*i].type == T_STRARRAY)
			{
				stack.push(vars[*i].value.arr->size);
			} else {
				printError(tr("Not an array variable"));
				return -1;
			}
		}
		break;

	case OP_ALENX:
		{
			// return x dimension lengh in 2d array model

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			if (vars[*i].type == T_ARRAY || vars[*i].type == T_STRARRAY)
			{
				stack.push(vars[*i].value.arr->xdim);
			} else {
				printError(tr("Not an array variable"));
				return -1;
			}
		}
		break;

	case OP_ALENY:
		{
			// return y dimension length in 2d array model

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			if (vars[*i].type == T_ARRAY || vars[*i].type == T_STRARRAY)
			{
				stack.push(vars[*i].value.arr->ydim);
			} else {
				printError(tr("Not an array variable"));
				return -1;
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
				free(val);
				return -1;
			}
			else if (vars[*i].type != T_STRARRAY)
			{
				printError(tr("Not a string array variable"));
				free(val);
				return -1;
			}
			else if (index >= vars[*i].value.arr->size || index < 0)
			{
				printError(tr("Array index out of bounds"));
				free(val);
				return -1;
			}
			
			strarray = vars[*i].value.arr->data.sdata;
			if (strarray[index])
			{
				free(strarray[index]);
			}
			strarray[index] = val;

			if(debugMode)
			{
			  emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(strarray[index]), index));
			}
		}
		break;

	case OP_STRARRAYASSIGN2D:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			char *val = stack.popstring(); // dont free - assigning to a string variable
			int yindex = stack.popint();
			int xindex = stack.popint();
			int index;

			char **strarray;

			if (vars[*i].type == T_UNUSED)
			{
				printError(tr("Unknown variable"));
				free(val);
				return -1;
			}
			else if (vars[*i].type != T_STRARRAY)
			{
				printError(tr("Not a string array variable"));
				free(val);
				return -1;
			}
			else if (xindex >= vars[*i].value.arr->xdim || xindex < 0 ||
				yindex >= vars[*i].value.arr->ydim || yindex < 0)
			{
				printError(tr("Array index out of bounds"));
				free(val);
				return -1;
			}

			strarray = vars[*i].value.arr->data.sdata;

			index = xindex * vars[*i].value.arr->ydim + yindex;

			if (strarray[index])
			{
				free(strarray[index]);
			}
			strarray[index] = val;

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
				  emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(strarray[index]), index));
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

	case OP_ARRAYASSIGN2D:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			double val = stack.popfloat();
			int yindex = stack.popint();
			int xindex = stack.popint();
			int index;
			
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
			else if (xindex >= vars[*i].value.arr->xdim || xindex < 0 ||
				yindex >= vars[*i].value.arr->ydim || yindex < 0)
			{
				printError(tr("Array index out of bounds"));
				return -1;
			}

			array = vars[*i].value.arr->data.fdata;
			index = xindex * vars[*i].value.arr->ydim + yindex;
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
					emit(varAssignment(QString(symtable[*i]), QString::number(items), index));
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
			} else if (index >= vars[*i].value.arr->size || index < 0)
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

	case OP_DEREF2D:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			int yindex = stack.popint();
			int xindex = stack.popint();
			int index;

			if (vars[*i].type != T_ARRAY && vars[*i].type != T_STRARRAY)
			{
				printError(tr("Cannot access non-array variable"));
				return -1;
			} else if (xindex >= vars[*i].value.arr->xdim || xindex < 0 ||
				yindex >= vars[*i].value.arr->ydim || yindex < 0)
			{
				printError(tr("Array index out of bounds"));
				return -1;
			}

			index = xindex * vars[*i].value.arr->ydim + yindex;

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
			if (stime > 0) msleep(stime);
		}
		break;

	case OP_LENGTH:
		{
			// unicode length - convert utf8 to unicode and return length
			op++;
			char *temp = stack.popstring();
			QString qs = QString::fromUtf8(temp);
			stack.push(qs.length());
			free(temp);
		}
		break;


	case OP_MID:
		{
			// unicode safe mid string
			op++;
			int len = stack.popint();
			int pos = stack.popint();
			char *temp = stack.popstring();

			QString qtemp = QString::fromUtf8(temp);
			
			if ((pos < 1) || (len < 0))
			{
				printError(tr("Illegal argument"));
				free(temp);
				return -1;
			}

			if (pos > (int) qtemp.length())
			{
				printError(tr("String not long enough for given starting character"));
				free(temp);
				return -1;
			}
			
			stack.push(strdup(qtemp.mid(pos-1,len).toUtf8().data()));
			
			free(temp);
		}
		break;


	case OP_LEFT:
		{
			// unicode save left string
			op++;
			int len = stack.popint();
			char *temp = stack.popstring();
			
			QString qtemp = QString::fromUtf8(temp);
			
			if (len < 0)
			{
				printError(tr("Illegal argument"));
				free(temp);
				return -1;
			}
			
			stack.push(strdup(qtemp.left(len).toUtf8().data()));
			
			free(temp);
		}
		break;


	case OP_RIGHT:
		{
			// unicode save right string
			op++;
			int len = stack.popint();
			char *temp = stack.popstring();
			
			QString qtemp = QString::fromUtf8(temp);
			
			if (len < 0)
			{
				printError(tr("Illegal argument"));
				free(temp);
				return -1;
			}
			
			stack.push(strdup(qtemp.right(len).toUtf8().data()));
			
			free(temp);
		}
		break;


	case OP_UPPER:
		{
			op++;
			char *temp = stack.popstring();

            for(unsigned int p=0;p<strlen(temp);p++) {
				if(isalpha(temp[p])) temp[p] = toupper(temp[p]);
			}

			stack.push(temp);

			free(temp);
		}
		break;

	case OP_LOWER:
		{
			op++;
			char *temp = stack.popstring();

            for(unsigned int p=0;p<strlen(temp);p++) {
				if(isalpha(temp[p])) temp[p] = tolower(temp[p]);
			}

			stack.push(temp);

			free(temp);
		}
		break;


	case OP_ASC:
		{
			// unicode character sequence - return 16 bit number representing character
			op++;
			char *str = stack.popstring();
			QString qs = QString::fromUtf8(str);
			stack.push((int) qs[0].unicode());
			free(str);
		}
		break;


	case OP_CHR:
		{
			// convert a single unicode character sequence to string in utf8
			op++;
			int code = stack.popint();
			QChar temp[2];
			temp[0] = (QChar) code;
			temp[1] = (QChar) 0;
			QString qs = QString::QString(temp,1);
			stack.push(strdup(qs.toUtf8().data()));
		}
		break;


	case OP_INSTR:
		{
			// unicode safe instr function
			op++;
			char *str = stack.popstring();
			char *hay = stack.popstring();

			QString qstr = QString::fromUtf8(str);
			QString qhay = QString::fromUtf8(hay);
			
			stack.push((int) (qhay.indexOf(qstr)+1));

			free(str);
			free(hay);
		}
		break;

	case OP_SIN:
	case OP_COS:
	case OP_TAN:
	case OP_ASIN:
	case OP_ACOS:
	case OP_ATAN:
	case OP_CEIL:
	case OP_FLOOR:
	case OP_ABS:
	case OP_DEGREES:
	case OP_RADIANS:
	case OP_LOG:
	case OP_LOGTEN:
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
			case OP_ASIN:
				stack.push(asin(val));
				break;
			case OP_ACOS:
				stack.push(acos(val));
				break;
			case OP_ATAN:
				stack.push(atan(val));
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
			case OP_DEGREES:
				stack.push(val * 180 / M_PI);
				break;
			case OP_RADIANS:
				stack.push(val * M_PI / 180);
				break;
			case OP_LOG:
				stack.push(log(val));
				break;
			case OP_LOGTEN:
				stack.push(log10(val));
				break;
			}
		}
		break;


	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_MOD:
	case OP_DIV:
	case OP_INTDIV:
	case OP_EXP:
		{
			stackval *one = stack.pop();
			stackval *two = stack.pop();
			double oneval, twoval;
			if (one->type == T_STRING || two->type == T_STRING)
			{
				if (one->type == T_STRING)
					free(one->value.string);
				if (two->type == T_STRING)
					free(two->value.string);
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
				case OP_INTDIV:
					stack.push(two->value.intval / one->value.intval);
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
				case OP_INTDIV:
					stack.push((int) twoval / (int) oneval);
					break;
				case OP_EXP:
					stack.push(pow((double) twoval, (double) oneval));
					break;
				}
			}
			op++;
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
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
			stack.clean(one);
			stack.clean(two);
		}
		break;

	case OP_SOUND:
		{
			op++;
			int duration = stack.popint();
			int frequency = stack.popint();
			int* freqdur;
			freqdur = (int*) malloc(1 * sizeof(int));
			freqdur[0] = frequency;
			freqdur[1] = duration;
			emit(playSounds(1 , freqdur));
		}
		break;
		
	case OP_SOUND_ARRAY:
		{
			// play an array of sounds

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			if (vars[*i].type == T_ARRAY)
			{
				int length = vars[*i].value.arr->size;
				double *array = vars[*i].value.arr->data.fdata;
				
				int* freqdur;
				freqdur = (int*) malloc(length * sizeof(int));
				
				for (int j = 0; j < length; j++)
				{
					freqdur[j] = (int) array[j];
				}
				
				emit(playSounds(length / 2 , freqdur));

			} 
		}
		break;

	case OP_SOUND_LIST:
		{
			// play an immediate list of sounds
			op++;
			int *i = (int *) op;
			int length = *i;
			op += sizeof(int);
			
			int* freqdur;
			freqdur = (int*) malloc(length * sizeof(int));
			
			for (int j = length-1; j >=0; j--)
			{
				freqdur[j] = stack.popint();
			}
			emit(playSounds(length / 2 , freqdur));
		}
		break;
		

	case OP_VOLUME:
		{
			// set the wave output height (volume 0-10)
			op++;
			int volume = stack.popint();
			if (volume<0) volume = 0;
			if (volume>10) volume = 10;
			emit(setVolume(volume));
		}
		break;

	case OP_SAY:
		{
			op++;
			char *temp = stack.popstring();
			//mutex.lock();
			emit(speakWords(QString::fromUtf8(temp)));
			//waitCond.wait(&mutex);
			//mutex.unlock();
			free(temp);
		}
		break;

	case OP_SYSTEM:
		{
			op++;
			char *temp = stack.popstring();
			//mutex.lock();
			emit(system(temp));
			//waitCond.wait(&mutex);
			//mutex.unlock();
			free(temp);
		}
		break;

	case OP_WAVPLAY:
		{
			op++;
			char *file = stack.popstring();
			emit(playWAV(QString::fromUtf8(file)));
			free(file);
		}
		break;

	case OP_WAVSTOP:
		{
			op++;
			emit(stopWAV());
		}
		break;

	case OP_SETCOLORRGB:
		{
			op++;
			int bval = stack.popint();
			int gval = stack.popint();
			int rval = stack.popint();
			if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255)
				{
					printError(tr("RGB Color values must be in the range of 0 to 255."));
					return -1;
				}
			pencolor = QColor(rval, gval, bval);
		}
		break;

	case OP_SETCOLORINT:
		{
			op++;
			QRgb rgbval = stack.popint();
			pencolor = QColor(rgbval);
		}
		break;

	case OP_RGB:
		{
			op++;
			int bval = stack.popint();
			int gval = stack.popint();
			int rval = stack.popint();
			if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255)
				{
					printError(tr("RGB Color values must be in the range of 0 to 255."));
					return -1;
				}
			stack.push((int) qRgb(rval, gval, bval));
		}
		break;
		
	case OP_PIXEL:
		{
			op++;
			int y = stack.popint();
			int x = stack.popint();
			QRgb rgb = (*image).pixel(x,y);
			stack.push((int) rgb % 0x1000000);
		}
		break;
		
	case OP_GETCOLOR:
		{
			op++;
			QRgb rgb = pencolor.rgb();
			stack.push((int) rgb % 0x1000000);
		}
		break;
		
	case OP_GETSLICE:
		{
			// slice format is 4 digit HEX width, 4 digit HEX height,
			// and (w*h)*6 digit HEX RGB for each pixel of slice
			op++;
			int h = stack.popint();
			int w = stack.popint();
			int y = stack.popint();
			int x = stack.popint();
			QString *qs = new QString();
			QRgb rgb;
			int tw, th;
			qs->append(QString::number(w,16).rightJustified(4,'0'));
			qs->append(QString::number(h,16).rightJustified(4,'0'));
			for(th=0; th<h; th++) {
				for(tw=0; tw<w; tw++) {
					rgb = image->pixel(x+tw,y+th);
					qs->append(QString::number(rgb%0x1000000,16).rightJustified(6,'0'));
				}
			}
			stack.push(qs->toUtf8().data());
			delete qs;
		}
		break;
		
	case OP_PUTSLICE:
	case OP_PUTSLICEMASK:
		{
			int mask = 0x1000000;	// mask nothing will equal
			if (*op == OP_PUTSLICEMASK) mask = stack.popint();
			char *txt = stack.popstring();
			QString imagedata = QString::fromUtf8(txt);
			free(txt);
			int y = stack.popint();
			int x = stack.popint();
			bool ok;
			int rgb, lastrgb = 0x1000000;
			int th, tw;
			int offset = 0; // location in qstring to get next hex number

			int w = imagedata.mid(offset,4).toInt(&ok, 16);
			offset+=4;
			if (ok) {
				int h = imagedata.mid(offset,4).toInt(&ok, 16);
				offset+=4;
				if (ok) {

					QPainter ian(image);
					for(th=0; th<h && ok; th++) {
						for(tw=0; tw<w && ok; tw++) {
							rgb = imagedata.mid(offset, 6).toInt(&ok, 16);
							offset+=6;
							if (ok && rgb != mask) {
								if (rgb!=lastrgb) {
									ian.setPen(rgb);
									lastrgb = rgb;
								}
								ian.drawPoint(x + tw, y + th);
							}
						}
					}
					ian.end();
					if (!fastgraphics) waitForGraphics();
				}
			}
			if (!ok) {
					printError(tr("String input to putbit incorrect."));
					return -1;
			}
			op++;
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
				QPointF *points = new QPointF[pairs];

				for (int j = 0; j < pairs; j++)
				{
					points[j].setX(array[j*2]);
					points[j].setY(array[(j*2)+1]);
				}
				poly.drawPolygon(points, pairs);
				poly.end();
				delete points;
			} 
			else
			{
				printError(tr("Argument not an array for poly()"));
				poly.end();
				return -1;
			}

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
			QPointF *points = new QPointF[pairs];
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
			delete points;
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

				if (scale>0) {
					double *array = vars[*i].value.arr->data.fdata;
					QPointF *points = new QPointF[pairs];

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
					poly.end();
					delete points;
				}
			} 
			else
			{
				printError(tr("Argument not an array for stamp()"));
				poly.end();
				return -1;
			}

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
			if (scale>0) {
				QPointF *points = new QPointF[pairs];
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
				delete points;
			}
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

	case OP_IMGLOAD:
	case OP_IMGLOAD_S:
	case OP_IMGLOAD_SR:
		{
			// Image Load - with optional scale and rotate

			double rotate=0;		// defaule rotation to 0 radians
			double scale=1;			// default scale to full size (1x)
			
			unsigned char opcode = *op;
			op++;
			
			// pop the filename to uncover the location and scale
			char *file = stack.popstring();
			
			if (opcode==OP_IMGLOAD_SR) rotate = stack.popfloat();
			if (opcode==OP_IMGLOAD_SR || opcode==OP_IMGLOAD_S) scale = stack.popfloat();
			double y = stack.popint();
			double x = stack.popint();
			
			if (scale>0) {
				QImage i(QString::fromUtf8(file));
				if(i.isNull()) {
					printError(tr("Unable to load image file."));
					free(file);
					return -1;
				} else {
					QPainter ian(image);
					if (rotate != 0 || scale != 1) {
						QMatrix mat = QMatrix().translate(0,0).rotate(rotate * 360 / (2 * M_PI)).scale(scale, scale);
						i = i.transformed(mat);
					}
					ian.drawImage((int)(x - .5 * i.width()), (int)(y - .5 * i.height()), i);
					ian.end();
					if (!fastgraphics) waitForGraphics();
				}
			}
			free(file);
		}
		break;

	case OP_SPRITEDIM:
		{
			int n = stack.popint();
			op++;
			//printError("spritedim - ");
			// deallocate existing sprites
			if (nsprites!=0) {
				free(sprites);
				nsprites = 0;
			}
			//printError("spritedim - af deallocate");
			// create new ones that are not visible, active, and are at origin
			if (n > 0) {
				sprites = (sprite*) malloc(sizeof(sprite) * n);
				nsprites = n;
				while (n>0) {
					n--;
					sprites[n].image = new QImage();
					sprites[n].underimage = new QImage();
					sprites[n].active = false;
					sprites[n].visible = false;
					sprites[n].x = 0;
					sprites[n].y = 0;
				}
			}
			//printError("spritedim - af allocate");
		}
		break;

	case OP_SPRITELOAD:
		{
			
			op++;
			
			char *file = stack.popstring();
			int n = stack.popint();

			if(n < 0 || n >=nsprites) {
				printError(tr("Sprite number out of range."));
				free(file);
				return -1;
			}
			
			spriteundraw(n);
			delete sprites[n].image;
			sprites[n].image = 	new QImage(QString::fromUtf8(file));
			if(sprites[n].image->isNull()) {
				printError(tr("Unable to load image file."));
				free(file);
				return -1;
			}
			delete sprites[n].underimage;
			sprites[n].underimage = new QImage();
			sprites[n].visible = true;
			sprites[n].active = true;
			spriteredraw(n);
			
			free(file);
		}
		break;

	case OP_SPRITEMOVE:
	case OP_SPRITEPLACE:
		{
			
			unsigned char opcode = *op;
			op++;
			
			int y = stack.popint();
			int x = stack.popint();
			int n = stack.popint();
			
			if(n < 0 || n >=nsprites) {
				printError(tr("Sprite number out of range."));
				return -1;
			}
			if(!sprites[n].active) {
				printError(tr("Sprite has not been assigned."));
				return -1;
			}
			
			spriteundraw(n);
			if (opcode==OP_SPRITEMOVE) {
				x += sprites[n].x;
				y += sprites[n].y;
				if (x > (int) graph->image->width()) x = (int) graph->image->width();
				if (x < 0) x = 0;
				if (y > (int) graph->image->height()) y = (int) graph->image->height();
				if (y < 0) y = 0;
			}
			sprites[n].x = x;
			sprites[n].y = y;
			spriteredraw(n);
			
			if (!fastgraphics) waitForGraphics();
			//printError("spritemove - af");
			
		}
		break;

	case OP_SPRITEHIDE:
	case OP_SPRITESHOW:
		{
			
			unsigned char opcode = *op;
			op++;
			
			int n = stack.popint();

			if(n < 0 || n >=nsprites) {
				printError(tr("Sprite number out of range."));
				return -1;
			}
			if(!sprites[n].active) {
				printError(tr("Sprite has not been assigned."));
				return -1;
			}
			
			if (sprites[n].active && sprites[n].visible) {
				spriteundraw(n);
				sprites[n].visible = (opcode==OP_SPRITESHOW);
				spriteredraw(n);
			}
			
			if (!fastgraphics) waitForGraphics();
			
		}
		break;

	case OP_SPRITECOLLIDE:
		{
			
			op++;
			
			int n1 = stack.popint();
			int n2 = stack.popint();

			if(n1 < 0 || n1 >=nsprites || n2 < 0 || n2 >=nsprites) {
				printError(tr("Sprite number out of range."));
				return -1;
			}
			if(!sprites[n1].active || !sprites[n2].active) {
				printError(tr("Sprite has not been assigned."));
				return -1;
			}
			
			stack.push(spritecollide(n1, n2));
			
		}
		break;

	case OP_SPRITEX:
	case OP_SPRITEY:
	case OP_SPRITEH:
	case OP_SPRITEW:
		{
			
			unsigned char opcode = *op;
			op++;
			int n = stack.popint();
			
			if(n < 0 || n >=nsprites) {
				printError(tr("Sprite number out of range."));
				return -1;
			}
			if(!sprites[n].active) {
				printError(tr("Sprite has not been assigned."));
				return -1;
			}
			
			if (opcode==OP_SPRITEX) stack.push(sprites[n].x);
			if (opcode==OP_SPRITEY) stack.push(sprites[n].y);
			if (opcode==OP_SPRITEH) stack.push(sprites[n].image->height());
			if (opcode==OP_SPRITEW) stack.push(sprites[n].image->width());
			
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
			ian.drawText(x0val, y0val+(QFontMetrics(ian.font()).ascent()), QString::fromUtf8(txt));
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
			fontfamily = QString::fromUtf8(family);
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
			QString p = QString::fromUtf8(temp);
			free(temp);
			if (*op == OP_PRINTN)
			{
				p += "\n";
			}
			mutex.lock();
			emit(outputReady(p));
			waitCond.wait(&mutex);
			mutex.unlock();
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
			
			// cleanup old string value if there is one
			if (vars[*num].type == T_STRING && vars[*num].value.string != NULL)
			{
				free(vars[*num].value.string);
				vars[*num].value.string = NULL;
			}

			vars[*num].type = T_STRING;
			vars[*num].value.string = temp;

			if(debugMode)
			{
			  emit(varAssignment(QString(symtable[*num]), QString::fromUtf8(vars[*num].value.string), -1));
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

	case OP_MOUSEX:
		{
			op++;
			stack.push((int) graph->mouseX);
		}
		break;

	case OP_MOUSEY:
		{
			op++;
			stack.push((int) graph->mouseY);
		}
		break;

	case OP_MOUSEB:
		{
			op++;
			stack.push((int) graph->mouseB);
		}
		break;

	case OP_CLICKCLEAR:
		{
			op++;
			graph->clickX = 0;
			graph->clickY = 0;
			graph->clickB = 0;
		}
		break;

	case OP_CLICKX:
		{
			op++;
			stack.push((int) graph->clickX);
		}
		break;

	case OP_CLICKY:
		{
			op++;
			stack.push((int) graph->clickY);
		}
		break;

	case OP_CLICKB:
		{
			op++;
			stack.push((int) graph->clickB);
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





