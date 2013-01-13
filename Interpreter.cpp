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
#include <sqlite3.h>
#include <errno.h>

#ifdef WIN32
	#include <winsock.h>
	#include <windows.h>

	typedef int socklen_t;

	// for parallel port operations
	HINSTANCE inpout32dll = NULL;
	typedef unsigned char (CALLBACK* InpOut32InpType)(short int);
	typedef void (CALLBACK* InpOut32OutType)(short int, unsigned char);
	InpOut32InpType Inp32 = NULL;
	InpOut32OutType Out32 = NULL;
#else
	#include <sys/types.h> 
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h> 
	#include <poll.h> 
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <ifaddrs.h>
	#include <unistd.h>
#endif

#include <QString>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
using namespace std;

#include "LEX/basicParse.tab.h"
#include "ByteCodes.h"
#include "CompileErrors.h"
#include "Interpreter.h"
#include "md5.h"
#include "Settings.h"
#include "Sound.h"

QMutex keymutex;
int currentKey;

extern QMutex mutex;
extern QMutex debugmutex;
extern QWaitCondition waitCond;
extern QWaitCondition waitDebugCond;
extern QWaitCondition waitInput;

//#ifdef WIN32
//	extern "C" { 
//		unsigned char Inp32(short int);
//		void Out32(short int, unsigned char);
//	}
//#endif

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

int Interpreter::compareTwoStackVal(stackval *two, stackval *one)
{
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	// one or both strings - [compare them as strings] strcmp
	if (one->type == T_STRING || two->type == T_STRING)
	{
		char *sone, *stwo;
		int i;
		sone = stack.tostring(one);
		stwo = stack.tostring(two);
		i = strcmp(sone, stwo);
		free(sone);
		free(stwo);
		return i;
	}
	// both integers - compare as integers
	if (one->type == T_INT && two->type == T_INT)
	{
		int ione, itwo;
		ione = stack.toint(one);
		itwo = stack.toint(two);
		if (ione == itwo) return 0;
		else if (ione < itwo) return -1;
		else return 1;
	}
	// one or more floats - compare them as floats
	if (one->type == T_FLOAT || two->type == T_FLOAT)
	{
		double fone, ftwo;
		fone = stack.tofloat(one);
		ftwo = stack.tofloat(two);
		if (fone == ftwo) return 0;
		else if (fone < ftwo) return -1;
		else return 1;
	}
	// default equal
	return 0;
}

void Interpreter::printError(int e, QString message)
{
	emit(outputReady(tr("ERROR on line ") + QString::number(currentLine) + ": " + getErrorMessage(e) + " " + message + "\n"));
	emit(goToLine(currentLine));
}

QString Interpreter::getErrorMessage(int e) {
	QString errormessage("");
	switch(e) {
		case ERROR_NOSUCHLABEL:
			errormessage = tr(ERROR_NOSUCHLABEL_MESSAGE);
			break;
		case ERROR_FOR1:
			errormessage = tr(ERROR_FOR1_MESSAGE);
			break;
		case ERROR_FOR2 :
			errormessage = tr(ERROR_FOR2_MESSAGE);
			break;
		case ERROR_NEXTNOFOR:
			errormessage = tr(ERROR_NEXTNOFOR_MESSAGE);
			break;
		case ERROR_FILENUMBER:
			errormessage = tr(ERROR_FILENUMBER_MESSAGE);
			break;
		case ERROR_FILEOPEN:
			errormessage = tr(ERROR_FILEOPEN_MESSAGE);
			break;
		case ERROR_FILENOTOPEN:
			errormessage = tr(ERROR_FILENOTOPEN_MESSAGE);
			break;
		case ERROR_FILEWRITE:
			errormessage = tr(ERROR_FILEWRITE_MESSAGE);
			break;
		case ERROR_FILERESET:
			errormessage = tr(ERROR_FILERESET_MESSAGE);
			break;
		case ERROR_ARRAYSIZELARGE:
			errormessage = tr(ERROR_ARRAYSIZELARGE_MESSAGE);
			break;
		case ERROR_ARRAYSIZESMALL:
			errormessage = tr(ERROR_ARRAYSIZESMALL_MESSAGE);
			break;
		case ERROR_NOSUCHVARIABLE:
			errormessage = tr(ERROR_NOSUCHVARIABLE_MESSAGE);
			break;
		case ERROR_NOTARRAY:
			errormessage = tr(ERROR_NOTARRAY_MESSAGE);
			break;
		case ERROR_NOTSTRINGARRAY:
			errormessage = tr(ERROR_NOTSTRINGARRAY_MESSAGE);
			break;
		case ERROR_ARRAYINDEX:
			errormessage = tr(ERROR_ARRAYINDEX_MESSAGE);
			break;
		case ERROR_STRNEGLEN:
			errormessage = tr(ERROR_STRNEGLEN_MESSAGE);
			break;
		case ERROR_STRSTART:
			errormessage = tr(ERROR_STRSTART_MESSAGE);
			break;
		case ERROR_STREND:
			errormessage = tr(ERROR_STREND_MESSAGE);
			break;
		case ERROR_NONNUMERIC:
			errormessage = tr(ERROR_NONNUMERIC_MESSAGE);
			break;
		case ERROR_RGB:
			errormessage = tr(ERROR_RGB_MESSAGE);
			break;
		case ERROR_PUTBITFORMAT:
			errormessage = tr(ERROR_PUTBITFORMAT_MESSAGE);
			break;
		case ERROR_POLYARRAY:
			errormessage = tr(ERROR_POLYARRAY_MESSAGE);
			break;
		case ERROR_POLYPOINTS:
			errormessage = tr(ERROR_POLYPOINTS_MESSAGE);
			break;
		case ERROR_IMAGEFILE:
			errormessage = tr(ERROR_IMAGEFILE_MESSAGE);
			break;
		case ERROR_SPRITENUMBER:
			errormessage = tr(ERROR_SPRITENUMBER_MESSAGE);
			break;
		case ERROR_SPRITENA:
			errormessage = tr(ERROR_SPRITENA_MESSAGE);
			break;
		case ERROR_SPRITESLICE:
			errormessage = tr(ERROR_SPRITESLICE_MESSAGE);
			break;
		case ERROR_FOLDER:
			errormessage = tr(ERROR_FOLDER_MESSAGE);
			break;
		case ERROR_DECIMALMASK:
			errormessage = tr(ERROR_DECIMALMASK_MESSAGE);
			break;
		case ERROR_DBOPEN:
			errormessage = tr(ERROR_DBOPEN_MESSAGE);
			break;
		case ERROR_DBQUERY:
			errormessage = tr(ERROR_DBQUERY_MESSAGE);
			break;
		case ERROR_DBNOTOPEN:
			errormessage = tr(ERROR_DBNOTOPEN_MESSAGE);
			break;
		case ERROR_DBCOLNO:
			errormessage = tr(ERROR_DBCOLNO_MESSAGE);
			break;
		case ERROR_DBNOTSET:
			errormessage = tr(ERROR_DBNOTSET_MESSAGE);
			break;
		case ERROR_EXTOPBAD:
			errormessage = tr(ERROR_EXTOPBAD_MESSAGE);
			break;
		case ERROR_NETSOCK:
			errormessage = tr(ERROR_NETSOCK_MESSAGE);
			break;
		case ERROR_NETHOST:
			errormessage = tr(ERROR_NETHOST_MESSAGE);
			break;
		case ERROR_NETCONN:
			errormessage = tr(ERROR_NETCONN_MESSAGE);
			break;
		case ERROR_NETREAD:
			errormessage = tr(ERROR_NETREAD_MESSAGE);
			break;
		case ERROR_NETNONE:
			errormessage = tr(ERROR_NETNONE_MESSAGE);
			break;
		case ERROR_NETWRITE:
			errormessage = tr(ERROR_NETWRITE_MESSAGE);
			break;
		case ERROR_NETSOCKOPT:
			errormessage = tr(ERROR_NETSOCKOPT_MESSAGE);
			break;
		case ERROR_NETBIND:
			errormessage = tr(ERROR_NETBIND_MESSAGE);
			break;
		case ERROR_NETACCEPT:
			errormessage = tr(ERROR_NETACCEPT_MESSAGE);
			break;
		case ERROR_NETSOCKNUMBER:
			errormessage = tr(ERROR_NETSOCKNUMBER_MESSAGE);
			break;
		case ERROR_PERMISSION:
			errormessage = tr(ERROR_PERMISSION_MESSAGE);
			break;
		case ERROR_IMAGESAVETYPE:
			errormessage = tr(ERROR_IMAGESAVETYPE_MESSAGE);
			break;
		case ERROR_ARGUMENTCOUNT:
			errormessage = tr(ERROR_ARGUMENTCOUNT_MESSAGE);
			break;
		case ERROR_MAXRECURSE:
			errormessage = tr(ERROR_MAXRECURSE_MESSAGE);
			break;
	        case ERROR_DIVZERO:
			errormessage = tr(ERROR_DIVZERO_MESSAGE);
            break;
		case ERROR_BYREF:
			errormessage = tr(ERROR_BYREF_MESSAGE);
			break;
		case ERROR_BYREFTYPE:
			errormessage = tr(ERROR_BYREFTYPE_MESSAGE);
            break;
		case ERROR_FREEFILE:
			errormessage = tr(ERROR_FREEFILE_MESSAGE);
			break;
		case ERROR_FREENET:
			errormessage = tr(ERROR_FREENET_MESSAGE);
			break;
		case ERROR_FREEDB:
			errormessage = tr(ERROR_FREEDB_MESSAGE);
			break;
		case ERROR_DBCONNNUMBER:
			errormessage = tr(ERROR_DBCONNNUMBER_MESSAGE);
			break;
		case ERROR_FREEDBSET:
			errormessage = tr(ERROR_FREEDBSET_MESSAGE);
			break;
		case ERROR_DBSETNUMBER:
			errormessage = tr(ERROR_DBSETNUMBER_MESSAGE);
			break;
		case ERROR_DBNOTSETROW:
			errormessage = tr(ERROR_DBNOTSETROW_MESSAGE);
			break;
		case ERROR_PENWIDTH:
			errormessage = tr(ERROR_PENWIDTH_MESSAGE);
			break;
		case ERROR_COLORNUMBER:
			errormessage = tr(ERROR_COLORNUMBER_MESSAGE);
			break;
		case ERROR_ARRAYINDEXMISSING:
			errormessage = tr(ERROR_ARRAYINDEXMISSING_MESSAGE);
			break;
        // put ERROR new messages here
		case ERROR_NOTIMPLEMENTED:
			errormessage = tr(ERROR_NOTIMPLEMENTED_MESSAGE);
			break;
		default:
			errormessage = tr(ERROR_USER_MESSAGE);
			
	}
	if (errormessage.contains(QString("%VARNAME%"),Qt::CaseInsensitive)) {
		if (symtable[errorvarnum]) {
			errormessage.replace(QString("%VARNAME%"),QString(symtable[errorvarnum]),Qt::CaseInsensitive);
		}
	}
	return errormessage;
}

QString Interpreter::getWarningMessage(int e) {
	QString message("");
	switch(e) {
		// warnings
		case WARNING_DEPRECATED_FORM :
			message = tr(WARNING_DEPRECATED_FORM_MESSAGE);
			break;
        // put WARNING new messages here
		case WARNING_NOTIMPLEMENTED:
			message = tr(WARNING_NOTIMPLEMENTED_MESSAGE);
			break;
	}
	return message;
}

int Interpreter::netSockClose(int fd)
{
	// tidy up a network socket and return NULL to assign to the
	// fd variable to mark as closed as closed
	// call  f = netSockClose(f);
	if(fd>=0) {
		#ifdef WIN32
			closesocket(fd);
		#else
			close(fd);
		#endif
	}
	return(-1);
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
	directorypointer=NULL;
	stack.fToAMask = stack.defaultFToAMask;
	status = R_STOPPED;
	for (int t=0;t<NUMSOCKETS;t++) netsockfd[t]=-1;
	// on a windows box start winsock
	#ifdef WIN32
		//
		// initialize the winsock network library
		WSAData wsaData;
		int nCode;
		if ((nCode = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0) {
			emit(outputReady(tr("ERROR - Unable to initialize Winsock library.\n")));
		}
		//
		// initialize the inpout32 dll
		inpout32dll  = LoadLibrary(L"inpout32.dll");
		if (inpout32dll==NULL) {
			emit(outputReady(tr("ERROR - Unable to find inpout32.dll - direct port I/O disabled.\n")));
		} else {
			Inp32 = (InpOut32InpType) GetProcAddress(inpout32dll, "Inp32");
			if (Inp32==NULL) {
				emit(outputReady(tr("ERROR - Unable to find Inp32 in inpout32.dll - direct port I/O disabled.\n")));
			}
			Out32 = (InpOut32OutType) GetProcAddress(inpout32dll, "Out32");
			if (Inp32==NULL) {
				emit(outputReady(tr("ERROR - Unable to find Out32 in inpout32.dll - direct port I/O disabled.\n")));
			}
		}
	#endif
}

Interpreter::~Interpreter() {
	// on a windows box stop winsock
	#ifdef WIN32
		WSACleanup();
	#endif
}

void Interpreter::clearsprites() {
	// cleanup sprites - release images and deallocate the space
	int i;
	if (nsprites>0) {
		for(i=0;i<nsprites;i++) {
			if (sprites[i].image) {
				delete sprites[i].image;
				sprites[i].image = NULL;
			}
			if (sprites[i].underimage) {
				delete sprites[i].underimage;
				sprites[i].underimage = NULL;
			}
		}
		free(sprites);
		sprites = NULL;
		nsprites = 0;
	}
}

void Interpreter::spriteundraw(int n) {
	// undraw all visible sprites >= n
	int x, y, i;
	QPainter ian(image);
	i = nsprites-1;
	while(i>=n) {
		if (sprites[i].underimage && sprites[i].visible) {
			x = (int) (sprites[i].x - (double) sprites[i].underimage->width()/2.0);
			y = (int) (sprites[i].y - (double) sprites[i].underimage->height()/2.0);
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
		if (sprites[i].image && sprites[i].visible) {
			QPainter ian(image);
			if (sprites[i].r==0 && sprites[i].s==1) {
				x = (int) (sprites[i].x - (double) sprites[i].image->width()/2.0);
				y = (int) (sprites[i].y - (double) sprites[i].image->height()/2.0);
				if (sprites[i].underimage) {
					delete sprites[i].underimage;
					sprites[i].underimage = NULL;
				}
				sprites[i].underimage = new QImage(image->copy(x, y, sprites[i].image->width(), sprites[i].image->height()));
				ian.drawImage(x, y, *(sprites[i].image));
			} else {
				QTransform transform = QTransform().translate(0,0).rotateRadians(sprites[i].r).scale(sprites[i].s,sprites[i].s);;
				QImage rotated = sprites[i].image->transformed(transform);
				x = (int) (sprites[i].x - (double) rotated.width()/2.0);
				y = (int) (sprites[i].y - (double) rotated.height()/2.0);
				if (sprites[i].underimage) {
					delete sprites[i].underimage;
					sprites[i].underimage = NULL;
				}
				sprites[i].underimage = new QImage(image->copy(x, y, rotated.width(), rotated.height()));
				ian.drawImage(x, y, rotated);
			}
			ian.end();
		}
		i++;
	}
}

bool Interpreter::spritecollide(int n1, int n2) {
	int top1, bottom1, left1, right1;
	int top2, bottom2, left2, right2;
	
	if (n1==n2) return true;
	
	if (!sprites[n1].image || !sprites[n2].image) return false;
	
	left1 = (int) (sprites[n1].x - (double) sprites[n1].image->width()*sprites[n1].s/2.0);
	left2 = (int) (sprites[n2].x - (double) sprites[n2].image->width()*sprites[n2].s/2.0);;
	right1 = left1 + sprites[n1].image->width()*sprites[n1].s;
	right2 = left2 + sprites[n2].image->width()*sprites[n2].s;
	top1 = (int) (sprites[n1].y - (double) sprites[n1].image->height()*sprites[n1].s/2.0);
	top2 = (int) (sprites[n2].y - (double) sprites[n2].image->height()*sprites[n2].s/2.0);
	bottom1 = top1 + sprites[n1].image->height()*sprites[n1].s;
	bottom2 = top2 + sprites[n2].image->height()*sprites[n2].s;

   if (bottom1<top2) return false;
   if (top1>bottom2) return false;
   if (right1<left2) return false;
   if (left1>right2) return false;
   return true;
}

int
Interpreter::compileProgram(char *code)
{
	variables.clear();
	if (newByteCode(strlen(code)) < 0)
	{
		return -1;
	}


	int result = basicParse(code);
	if (result < 0)
	{
		switch(result)
		{
			case COMPERR_ASSIGNS2N:
				emit(outputReady(tr(COMPERR_ASSIGNS2N_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ASSIGNN2S:
				emit(outputReady(tr(COMPERR_ASSIGNN2S_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONGOTO:
				emit(outputReady(tr(COMPERR_FUNCTIONGOTO_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_GLOBALNOTHERE:
				emit(outputReady(tr(COMPERR_GLOBALNOTHERE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONNOTHERE:
				emit(outputReady(tr(COMPERR_FUNCTIONNOTHERE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDFUNCTION:
				emit(outputReady(tr(COMPERR_ENDFUNCTION_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONNOEND:
				emit(outputReady(tr(COMPERR_FUNCTIONNOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FORNOEND:
				emit(outputReady(tr(COMPERR_FORNOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_WHILENOEND:
				emit(outputReady(tr(COMPERR_WHILENOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_DONOEND:
				emit(outputReady(tr(COMPERR_DONOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ELSENOEND:
				emit(outputReady(tr(COMPERR_ELSENOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_IFNOEND:
				emit(outputReady(tr(COMPERR_IFNOEND_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_UNTIL:
				emit(outputReady(tr(COMPERR_UNTIL_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDWHILE:
				emit(outputReady(tr(COMPERR_ENDWHILE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ELSE:
				emit(outputReady(tr(COMPERR_ELSE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDIF:
				emit(outputReady(tr(COMPERR_ENDIF_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_NEXT:
				emit(outputReady(tr(COMPERR_NEXT_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_RETURNVALUE:
				emit(outputReady(tr(COMPERR_RETURNVALUE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_RETURNTYPE:
				emit(outputReady(tr(COMPERR_RETURNTYPE_MESSAGE) + QString::number(linenumber) + ".\n"));
				break;
			default:
				if(column==0) {
					emit(outputReady(tr(COMPERR_SYNTAX_MESSAGE) + QString::number(linenumber) + tr(" around end of line.") + "\n"));
				} else {
					emit(outputReady(tr(COMPERR_SYNTAX_MESSAGE) + QString::number(linenumber) + tr(" around column ") + QString::number(column) + ".\n"));
				}
		}
		emit(goToLine(linenumber));
		return -1;
	}

	// this logic goes through the bytecode generated and puts the actual
	// label address from labeltable into the bytecode
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
		else if (*op == OP_GOTO || *op == OP_GOSUB || *op == OP_ONERROR)
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
				printError(ERROR_NOSUCHLABEL,"");
				return -1;
			}
		}
		else if (*op < (unsigned char) OP_TYPEARGINT)
		{
			// in the group of OP_TYPEARGNONE
			// op has no args - move to next byte
			op += sizeof(unsigned char);
		}
		else if (*op == OP_EXTENDEDNONE)
		{
			// simple one byte op follows the extended
			// op has no args - move to next byte
			op += sizeof(unsigned char) * 2;
		}
		else if (*op < (unsigned char) OP_TYPEARG2INT)
		{
			// in the group of OP_TYPEARGINT
			//op has one Int arg
			op += sizeof(unsigned char) + sizeof(int);
		}
		else if (*op < (unsigned char) OP_TYPEARGFLOAT)
		{
			// in the group of OP_TYPEARG2INT
			// op has 2 Int arg
			op += sizeof(unsigned char) + 2 * sizeof(int);
		}
		else if (*op < (unsigned char) OP_TYPEARGSTRING)
		{
			// in the group of OP_TYPEARGFLOAT
			// op has a single Float arg
			op += sizeof(unsigned char) + sizeof(double);
		}
		else if (*op < (unsigned char) OP_TYPEARGEXT)
		{
			// in the group of OP_TYPESTRING
			// op has a single null terminated String arg
			op += sizeof(unsigned char);
			int len = strlen((char *) op) + 1;
			op += len;
		}
		else
		{
			emit(outputReady(tr("Error in bytecode during label referencing at line ") + QString::number(currentLine) + ".\n"));
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
	drawingpen = QPen(Qt::black);
	drawingbrush = QBrush(Qt::black, Qt::SolidPattern);
	status = R_RUNNING;
	once = true;
	currentLine = 1;
	emit(mainWindowsResize(1, 300, 300));
	image = graph->image;
	fontfamily = QString("");
	fontpoint = 0;
	fontweight = 0;
	nsprites = 0;
	stack.fToAMask = stack.defaultFToAMask;
	runtimer.start();
	// initialize files to NULL (closed)
	stream = new QFile *[NUMFILES];
	for (int t=0;t<NUMFILES;t++) {
		stream[t] = NULL;
	}
	// initialize network sockets to closed (-1)
	for (int t=0;t<NUMSOCKETS;t++) {
		netsockfd[t] = netSockClose(netsockfd[t]);
	}
	// initialize databse connections
	dbconn = new sqlite3 *[NUMDBCONN];
	dbset = new sqlite3_stmt **[NUMDBCONN];
	for (int t=0;t<NUMDBCONN;t++) {
		dbconn[t] = NULL;
		dbset[t] = new sqlite3_stmt *[NUMDBSET];
		for (int u=0;u<NUMDBSET;u++) {
			dbset[t][u] = NULL;
			dbsetrow[t][u] = false;
		}
	}
}


void
Interpreter::cleanup()
{
	// called by run() once the run is terminated
	//
	// Clean up stack
	stack.clear();
	// Clean up variables
	variables.clear();
	// Clean up sprites
	clearsprites();
	// Clean up, for frames, etc.
	if (byteCode)
	{
		free(byteCode);
		byteCode = NULL;
	}
	// close open files (set to NULL if closed)
	for (int t=0;t<NUMFILES;t++) {
		if (stream[t]) {
			stream[t]->close();
			stream[t] = NULL;
		}
	}
	// close open network connections (set to -1 of closed)
	for (int t=0;t<NUMSOCKETS;t++) {
		if (netsockfd[t]) netsockfd[t] = netSockClose(netsockfd[t]);
	}
	// close open database connections and record sets
	for (int t=0;t<NUMDBCONN;t++) {
		closeDatabase(t);
	}
	//
	if(directorypointer != NULL) {
		closedir(directorypointer);
		directorypointer = NULL;
	}
}

void Interpreter::closeDatabase(int n) {
	// cleanup database
	for (int t=0; t<NUMDBSET; t++) {
		if (dbset[n][t]) {
			sqlite3_finalize(dbset[n][t]);
			dbset[n][t] = NULL;
			dbsetrow[n][t] = false;
		}
	}
	if (dbconn[n]) {
		sqlite3_close(dbconn[n]);
		dbconn[n] = NULL;
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
	errornum = ERROR_NONE;
	errormessage = "";
	lasterrornum = ERROR_NONE;
    lasterrormessage = "";
	lasterrorline = 0;
	onerroraddress = 0;
	while (status != R_STOPPED && execByteCode() >= 0) {} //continue
	status = R_STOPPED;
	cleanup(); // cleanup the variables, databases, files, stack and everything
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

	// if errnum is set then handle the last thrown error
	if (errornum!=ERROR_NONE) {
		lasterrornum = errornum;
		lasterrormessage = errormessage;
		lasterrorline = currentLine;
		errornum = ERROR_NONE;
		errormessage = "";
		if(onerroraddress!=0) {
			// progess call to subroutine for error handling
			frame *temp = new frame;
			temp->returnAddr = op;
			temp->next = callstack;
			callstack = temp;
			op = byteCode + onerroraddress;
			return 0;
		} else {
			// no error handler defined - display message and die
			emit(outputReady(tr("ERROR on line ") + QString::number(lasterrorline) + ": " + getErrorMessage(lasterrornum) + " " + lasterrormessage + "\n"));
			emit(goToLine(currentLine));
			return -1;
		}
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

	//printf("%02x %d  -> ",*op, stack.height());stack.debug();

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

	case OP_ONERROR:
		{
			// get the address of the subroutine for error handling
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			onerroraddress = *i;
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
			temp->recurselevel = variables.getrecurse();

			variables.setfloat(*i, startnum);

			if(debugMode)
			{
				emit(varAssignment(QString(symtable[*i]), QString::number(variables.getfloat(*i)), -1, -1));
			}

			temp->endNum = endnum;
			temp->step = step;
			temp->returnAddr = op;
			if (forstack)
			{
				forstack->prev = temp;
			}
			forstack = temp;
			if (temp->step > 0 && variables.getfloat(*i) > temp->endNum)
			{
				errornum = ERROR_FOR1;
			} else if (temp->step < 0 && variables.getfloat(*i) < temp->endNum)
			{
				errornum = ERROR_FOR2;
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
				errornum = ERROR_NEXTNOFOR;
			} else {

				double val = variables.getfloat(*i);
				val += temp->step;
				variables.setfloat(*i, val);

				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString::number(variables.getfloat(*i)), -1, -1));
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
		}
		break;


	case OP_OPEN:
		{
			op++;
			int type = stack.popint();	// 0 text 1 binary
            char *name = stack.popstring();
			int fn = stack.popint();

            if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
                // close file number if open
                if (stream[fn] != NULL) {
					stream[fn]->close();
					stream[fn] = NULL;
				}
                // create file stream
				stream[fn] = new QFile(QString::fromUtf8(name));
                if (stream[fn] == NULL) {
                    errornum = ERROR_FILEOPEN;
                } else {
                    if (type==0) {
                        // text file
                        if (!stream[fn]->open(QIODevice::ReadWrite | QIODevice::Text)) {
                            errornum = ERROR_FILEOPEN;
                        }
                    } else {
                        // binary file
                        if (!stream[fn]->open(QIODevice::ReadWrite)) {
                            errornum = ERROR_FILEOPEN;
                        }
                    }
                }
			}
			free(name);
		}
		break;


	case OP_READ:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.push(0);
			} else {
				char c = ' ';
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.push(0);
				} else {
					int maxsize = 256;
					char * strarray = (char *) malloc(maxsize);
					memset(strarray, 0, maxsize);
					//Remove leading whitespace
					while (c == ' ' || c == '\t' || c == '\n')
					{
						if (!stream[fn]->getChar(&c))
						{
							stack.push(strarray);
							free(strarray);
							return 0;
						}
					}
					//put back non-whitespace character
					stream[fn]->ungetChar(c);
					//read token
					int offset = 0;
					while (stream[fn]->getChar(strarray + offset) &&
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
			}
		}
		break;


	case OP_READLINE:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.push(0);
			} else {

				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.push(0);
				} else {
					//read entire line
					QByteArray l = stream[fn]->readLine();
					stack.push(strdup(l.data()));
				}
			}
		}
		break;

	case OP_READBYTE:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.push(0);
			} else {
				char c = ' ';
				if (stream[fn]->getChar(&c)) {
					stack.push((int) (unsigned char) c);
				} else {
					stack.push((int) -1);
				}
			}
		}
		break;

	case OP_EOF:
		{
			//return true to eof if error is returned
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.push(1);
			} else {
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.push(1);
				} else {
					if (stream[fn]->atEnd()) {
						stack.push(1);
					} else {
						stack.push(0);
					}
				}
			}
		}
		break;


	case OP_WRITE:
	case OP_WRITELINE:
		{
			unsigned char whichop = *op;
			op++;
			char *temp = stack.popstring();
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
				int error = 0;
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
				} else {
					quint64 oldPos = stream[fn]->pos();
					stream[fn]->flush();
					stream[fn]->seek(stream[fn]->size());
					error = stream[fn]->write(temp, strlen(temp));
					if (whichop == OP_WRITELINE)
					{
						error = stream[fn]->write("\n", 1);
					}
					stream[fn]->seek(oldPos);
					stream[fn]->flush();
				}
				if (error == -1)
				{
					errornum = ERROR_FILEWRITE;
				}
			}
			free(temp);
		}
		break;

	case OP_WRITEBYTE:
		{
			op++;
			int n = stack.popint();
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
				int error = 0;
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
				} else {
					error = stream[fn]->putChar((unsigned char) n);
					if (error == -1)
					{
						errornum = ERROR_FILEWRITE;
					}
				}
			}
		}
		break;



	case OP_CLOSE:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
				} else {
					stream[fn]->close();
					stream[fn] = NULL;
				}
			}
		}
		break;


	case OP_RESET:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
				} else {	
					if (stream[fn]->isTextModeEnabled()) {
						// text mode file 
						stream[fn]->close();
						if (!stream[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
						{
							errornum = ERROR_FILERESET;
						}
					} else {
						// binary mode file 
						stream[fn]->close();
						if (!stream[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate))
						{
							errornum = ERROR_FILERESET;
						}
					}
				}
			}
		}
		break;

	case OP_SIZE:
		{
			// push the current open file size on the stack
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.push(0);
			} else {
				int size = 0;
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.push(0);
				} else {
					size = stream[fn]->size();
				}
				stack.push(size);
			}
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
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
			} else {
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
				} else {
					stream[fn]->seek(pos);
				}
			}
		}
		break;

	case OP_DIM:
	case OP_REDIM:
		{
			unsigned char whichdim = *op;
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			int ydim = stack.popint();
			int xdim = stack.popint();
			variables.arraydimfloat(*i, xdim, ydim, whichdim == OP_REDIM);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					if (ydim==1) {
						emit(varAssignment(QString(symtable[*i]), NULL, xdim, -1));
					} else {
						emit(varAssignment(QString(symtable[*i]), NULL, xdim, ydim));
					}
				}
			} else {
				 errornum = variables.error();
				 errorvarnum = variables.errorvarnum();
			}
		}
		break;

	case OP_DIMSTR:
	case OP_REDIMSTR:
		{
			unsigned char whichdim = *op;
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			int ydim = stack.popint();
			int xdim = stack.popint();
			variables.arraydimstring(*i, xdim, ydim, whichdim == OP_REDIMSTR);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					if (ydim==1) {
						emit(varAssignment(QString(symtable[*i]), NULL, xdim, -1));
					} else {
						emit(varAssignment(QString(symtable[*i]), NULL, xdim, ydim));
					}
				}
			} else {
				 errornum = variables.error();
 				 errorvarnum = variables.errorvarnum();
			}
		}
		break;

	case OP_ALEN:
	case OP_ALENX:
	case OP_ALENY:
		{
			// return array lengths
			unsigned char opcode = *op;
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			switch(opcode) {
				case OP_ALEN:
					stack.push(variables.arraysize(*i));
					break;
				case OP_ALENX:
					stack.push(variables.arraysizex(*i));
					break;
				case OP_ALENY:
					stack.push(variables.arraysizey(*i));
					break;
			}
			if (variables.error()!=ERROR_NONE) {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}
		}
		break;

	case OP_STRARRAYASSIGN:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			char *val = stack.popstring(); // dont free if successful - assigning to a string variable
			int index = stack.popint();

			variables.arraysetstring(*i, index, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(variables.arraygetstring(*i, index)), index, -1));
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
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

			variables.array2dsetstring(*i, xindex, yindex, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(variables.array2dgetstring(*i, xindex, yindex)), xindex, yindex));
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}			
		}
		break;

	case OP_STRARRAYLISTASSIGN:
		{
			op++;
			int *i = (int *) op;
			int items = i[1];
			op += 2 * sizeof(int);
			
			if (variables.arraysize(*i)!=items) variables.arraydimstring(*i, items, 1, false);

			if(errornum==ERROR_NONE)
			{
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), NULL, items, -1));
				}
			
				for (int index = items - 1; index >= 0 && errornum==ERROR_NONE; index--)
				{
					char *str = stack.popstring(); // dont free we are assigning this to a variable
					variables.arraysetstring(*i, index, str);
					if (variables.error()==ERROR_NONE) {
						if(debugMode)
						{
							emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(variables.arraygetstring(*i, index)), index, -1));
						}
					} else {
						errornum = variables.error();
						errorvarnum = variables.errorvarnum();
					}			
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}			
		}
		break;
		
	case OP_EXPLODE:
	case OP_EXPLODE_C:
	case OP_EXPLODESTR:
	case OP_EXPLODESTR_C:
	case OP_EXPLODEX:
	case OP_EXPLODEXSTR:
		{
			// unicode safe explode a string to an array function
			bool ok;
			unsigned char opcode = *op;
			op++;
			int *i = (int *) op;		// variable number
			op += sizeof(int);
			
			Qt::CaseSensitivity casesens = Qt::CaseSensitive;
			if(opcode==OP_EXPLODESTR_C || opcode==OP_EXPLODE_C) {
				if(stack.popfloat()!=0) casesens = Qt::CaseInsensitive;
			}
	
			char *needle = stack.popstring();
			char *haystack = stack.popstring();
			QString qneedle = QString::fromUtf8(needle);
			QString qhaystack = QString::fromUtf8(haystack);
					
			QStringList list;
			if(opcode==OP_EXPLODE || opcode==OP_EXPLODE_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODESTR_C) {
				list = qhaystack.split(qneedle, QString::KeepEmptyParts , casesens);
			} else {
				list = qhaystack.split(QRegExp(qneedle), QString::KeepEmptyParts);
			}
			
			if(opcode==OP_EXPLODESTR_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODEXSTR) {
				if (variables.arraysize(*i)!=list.size()) variables.arraydimstring(*i, list.size(), 1, false);
			} else {
				if (variables.arraysize(*i)!=list.size()) variables.arraydimfloat(*i, list.size(), 1, false);
			}
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), NULL, list.size(), -1));
				}
					
				for(int x=0; x<list.size(); x++) {
					if(opcode==OP_EXPLODESTR_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODEXSTR) {
						variables.arraysetstring(*i, x, strdup(list.at(x).toUtf8().data()));
						if (variables.error()==ERROR_NONE) {
							if(debugMode) {
								emit(varAssignment(QString(symtable[*i]), QString::fromUtf8(variables.arraygetstring(*i, x)), x, -1));
							}
						} else {
							errornum = variables.error();
							errorvarnum = variables.errorvarnum();
						}			
					} else {
						variables.arraysetfloat(*i, x, list.at(x).toDouble(&ok));
						if (variables.error()==ERROR_NONE) {
							if(debugMode) {
								emit(varAssignment(QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, x)), x, -1));
							}
						} else {
							errornum = variables.error();
							errorvarnum = variables.errorvarnum();
						}			
					}
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}
			free(needle);
			free(haystack);
		}
		break;

	case OP_IMPLODE:
		{

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			char *delim = stack.popstring();
			QString qdelim = QString::fromUtf8(delim);

			QString stuff = "";

			if (variables.type(*i) == T_STRARRAY || variables.type(*i) == T_ARRAY)
			{
				int kount = variables.arraysize(*i);
				for(int n=0;n<kount;n++) {
					if (n>0) stuff.append(qdelim);
					if (variables.type(*i) == T_STRARRAY) {
						stuff.append(QString::fromUtf8(variables.arraygetstring(*i, n)));
					} else {
						stack.push(variables.arraygetfloat(*i, n));
						stuff.append(stack.popstring());
					}
				}
			} else {
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
			}

			stack.push(strdup(stuff.toUtf8().data()));
		
			free(delim);
		}
		break;

	case OP_GLOBAL:
		{
			// make a variable number a global variable
			op++;
			int *i = (int *) op;
			op += sizeof(int);
			variables.makeglobal(*i);
		}
		break;

	case OP_ARRAYASSIGN:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			double val = stack.popfloat();
			int index = stack.popint();
			
			variables.arraysetfloat(*i, index, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, index)), index, -1));
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
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
			
			variables.array2dsetfloat(*i, xindex, yindex, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(QString(symtable[*i]), QString::number(variables.array2dgetfloat(*i, xindex, yindex)), xindex, yindex));
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}			
		}
		break;


	case OP_ARRAYLISTASSIGN:
		{
			op++;
			int *i = (int *) op;
			int items = i[1];
			op += 2 * sizeof(int);
			
			if (variables.arraysize(*i)!=items) variables.arraydimfloat(*i, items, 1, false);
			if(errornum==ERROR_NONE) {
				if(debugMode && errornum==ERROR_NONE)
				{
					emit(varAssignment(QString(symtable[*i]), NULL, items, -1));
				}
			
				for (int index = items - 1; index >= 0 && errornum==ERROR_NONE; index--)
				{
					double one = stack.popfloat();
					variables.arraysetfloat(*i, index, one);
					if (variables.error()==ERROR_NONE) {
						if(debugMode)
						{
							emit(varAssignment(QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, index)), index, -1));
						}
					} else {
						errornum = variables.error();
						errorvarnum = variables.errorvarnum();
					}			
				}
			} else {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}			

		}
		break;


	case OP_DEREF:
		{

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			int index = stack.popint();

			if (variables.type(*i) == T_STRARRAY)
			{
				stack.push(variables.arraygetstring(*i, index));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else if (variables.type(*i) == T_ARRAY)
			{
				stack.push(variables.arraygetfloat(*i, index));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else
			{
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
				stack.push(0);
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

			if (variables.type(*i) == T_STRARRAY)
			{
				stack.push(variables.array2dgetstring(*i, xindex, yindex));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else if (variables.type(*i) == T_ARRAY)
			{
				stack.push(variables.array2dgetfloat(*i, xindex, yindex));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else
			{
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
				stack.push(0);
			}
		}
		break;

	case OP_PUSHVAR:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			if (variables.type(*i) == T_STRING)
			{
				stack.push(variables.getstring(*i));
			}
			else if (variables.type(*i) == T_FLOAT)
			{
				stack.push(variables.getfloat(*i));
			}
			else if (variables.type(*i) == T_ARRAY || variables.type(*i) == T_STRARRAY)
			{
				errornum = ERROR_ARRAYINDEXMISSING;
				errorvarnum = *i;
				stack.push(0);
			}
			else
			{
				errornum = ERROR_NOSUCHVARIABLE;
				errorvarnum = *i;
				stack.push(0);
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

    case OP_PUSHVARREF:
        {
            op++;
            int *i = (int *) op;
            stack.pushvarref(*i);
            op += sizeof(int);
        }
        break;

    case OP_PUSHVARREFSTR:
        {
            op++;
            int *i = (int *) op;
            stack.pushvarrefstr(*i);
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
			
			if ((len < 0))
			{
				errornum = ERROR_STRNEGLEN;
				stack.push(0);
				free(temp);
			} else {
				if ((pos < 1))
				{
					errornum = ERROR_STRSTART;
					stack.push(0);
					free(temp);
				} else {
					if ((pos < 1) || (pos > (int) qtemp.length()))
					{
						errornum = ERROR_STREND;
						stack.push(0);
						free(temp);
					} else {
						stack.push(strdup(qtemp.mid(pos-1,len).toUtf8().data()));
						free(temp);
					}
				}
			}
		}
		break;


	case OP_LEFT:
	case OP_RIGHT:
		{
			// unicode safe left/right string
			unsigned char opcode = *op;
			op++;
			int len = stack.popint();
			char *temp = stack.popstring();
			
			QString qtemp = QString::fromUtf8(temp);
			
			if (len < 0)
			{
				errornum = ERROR_STRNEGLEN;
				free(temp);
				stack.push(0);
			} else {
				switch(opcode) {
					case OP_LEFT:
						stack.push(strdup(qtemp.left(len).toUtf8().data()));
						break;
					case OP_RIGHT:
						stack.push(strdup(qtemp.right(len).toUtf8().data()));
						break;
				}
				free(temp);
			}
		}
		break;


	case OP_UPPER:
	case OP_LOWER:
		{
			unsigned char opcode = *op;
			op++;
			char *temp = stack.popstring();

			QString qtemp = QString::fromUtf8(temp);
			if(opcode==OP_UPPER) {
				qtemp = qtemp.toUpper();
			} else {
				qtemp = qtemp.toLower();
			}

			stack.push(strdup(qtemp.toUtf8().data()));
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
			QString qs = QString(temp,1);
			stack.push(strdup(qs.toUtf8().data()));
		}
		break;


	case OP_INSTR:
	case OP_INSTR_S:
	case OP_INSTR_SC:
	case OP_INSTRX:
	case OP_INSTRX_S:
		{
			// unicode safe instr function
			unsigned char opcode = *op;
			op++;
			
			Qt::CaseSensitivity casesens = Qt::CaseSensitive;
			if(opcode==OP_INSTR_SC) {
				if(stack.popfloat()!=0) casesens = Qt::CaseInsensitive;
			}

			int start = 1;
			if(opcode==OP_INSTR_S || opcode==OP_INSTR_SC || opcode==OP_INSTRX_S) {
				start = stack.popfloat();
			}
			
			char *str = stack.popstring();
			char *hay = stack.popstring();

			QString qstr = QString::fromUtf8(str);
			QString qhay = QString::fromUtf8(hay);
			
			int pos = 0;
			if(start < 1) {
				errornum = ERROR_STRSTART;
			} else {
				if (start > qhay.length()) {
					errornum = ERROR_STREND;
				} else {
					if(opcode==OP_INSTR || opcode==OP_INSTR_S || opcode==OP_INSTR_SC) {
						pos = qhay.indexOf(qstr, start-1, casesens)+1;
					} else {
						pos = qhay.indexOf(QRegExp(qstr), start-1)+1;
					}
				}
			}
			stack.push(pos);

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
	case OP_SQR:
	case OP_EXP:
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
			case OP_SQR:
				stack.push(sqrt(val));
				break;
			case OP_EXP:
				stack.push(exp(val));
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
	case OP_EX:
		{
			unsigned char whichop = *op;
			op++;
			stackval *one = stack.pop();
			stackval *two = stack.pop();
			if (one->type == T_STRING || two->type == T_STRING || one->type == T_ARRAY || two->type == T_ARRAY || one->type == T_STRARRAY || two->type == T_STRARRAY || one->type == T_UNUSED || two->type == T_UNUSED)
			{
				errornum = ERROR_NONNUMERIC;
				stack.push(0);
			} else {
				if (one->type == two->type && one->type == T_INT)
				{
					int i1, i2;
					i1 = one->value.intval;
					i2 = two->value.intval;
					stack.clean(one);
					stack.clean(two);
					//
					switch (whichop)
					{
					case OP_ADD:
						stack.push(i2 + i1);
						break;
					case OP_SUB:
						stack.push(i2 - i1);
						break;
					case OP_MUL:
						stack.push(i2 * i1);
						break;
					case OP_MOD:
						if (i1==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push(i2 % i1);
						}
						break;
					case OP_DIV:
						if (i1==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push((double) i2/ (double) i1);
						}
						break;
					case OP_INTDIV:
						if (i1==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push(i2 / i1);
						}
						break;
					case OP_EX:
						stack.push(pow((double) i2, (double) i1));
						break;
					}
				} else {
					double oneval, twoval;
					if (one->type == T_INT)
						oneval = (double) one->value.intval;
					else
						oneval = one->value.floatval;
					if (two->type == T_INT)
						twoval = (double) two->value.intval;
					else
						twoval = two->value.floatval;
					stack.clean(one);
					stack.clean(two);
					//
					switch(whichop)
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
						if ((int)oneval==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push((int) twoval % (int) oneval);
						}
						break;
					case OP_DIV:
						if (oneval==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push(twoval / oneval);
						}
						break;
					case OP_INTDIV:
						if ((int)oneval==0) {
							errornum = ERROR_DIVZERO;
						} else {
							stack.push((int) twoval / (int) oneval);
						}
						break;
					case OP_EX:
						stack.push(pow((double) twoval, (double) oneval));
						break;
					}
				}
			}	
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
			int ans = compareTwoStackVal(one,two)==0;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);
		}
		break;

	case OP_NEQUAL:
		{
			op++;
			POP2
			int ans = compareTwoStackVal(one,two)!=0;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);
		}
		break;

	case OP_GT:
		{
			op++;
			POP2
			int ans = compareTwoStackVal(one,two)==1;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);

		}
		break;

	case OP_LTE:
		{
			op++;
			POP2
			int ans = compareTwoStackVal(one,two)!=1;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);
		}
		break;

	case OP_LT:
		{
			op++;
			POP2
			int ans = compareTwoStackVal(one,two)==-1;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);
		}
		break;

	case OP_GTE:
		{
			op++;
			POP2
			int ans = compareTwoStackVal(one,two)!=-1;
			stack.clean(one);
			stack.clean(two);
			stack.push(ans);
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
			sound.playSounds(1 , freqdur);
		}
		break;
		
	case OP_SOUND_ARRAY:
		{
			// play an array of sounds

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			if (variables.type(*i) == T_ARRAY)
			{
				int length = variables.arraysize(*i);
				
				int* freqdur;
				freqdur = (int*) malloc(length * sizeof(int));
				
				for (int j = 0; j < length; j++)
				{
					freqdur[j] = (int) variables.arraygetfloat(*i, j);
				}
				
				sound.playSounds(length / 2 , freqdur);
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
			sound.playSounds(length / 2 , freqdur);
		}
		break;
		

	case OP_VOLUME:
		{
			// set the wave output height (volume 0-10)
			op++;
			int volume = stack.popint();
			if (volume<0) volume = 0;
			if (volume>10) volume = 10;
			sound.setVolume(volume);
		}
		break;

	case OP_SAY:
		{
			op++;
			char *temp = stack.popstring();
			emit(speakWords(QString::fromUtf8(temp)));
			free(temp);
		}
		break;

	case OP_SYSTEM:
		{
			op++;
			char *temp = stack.popstring();

			QSettings settings(SETTINGSORG, SETTINGSAPP);
			if(settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toBool()) {
				mutex.lock();
				emit(executeSystem(temp));
				waitCond.wait(&mutex);
				mutex.unlock();
			} else {
				errornum = ERROR_PERMISSION;
			}
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

	case OP_SETCOLORINT:
		{
			op++;
			int brushval = stack.popint();
			int penval = stack.popint();

			drawingpen.setColor(QColor::fromRgba((QRgb) penval));
			drawingbrush.setColor(QColor::fromRgba((QRgb) brushval));
		}
		break;

	case OP_RGB:
		{
			op++;
			int aval = stack.popint();
			int bval = stack.popint();
			int gval = stack.popint();
			int rval = stack.popint();
			if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255 || aval < 0 || aval > 255)
			{
				errornum = ERROR_RGB;
			} else {
				stack.push((int) QColor(rval,gval,bval,aval).rgba());
			}
		}
		break;
		
	case OP_PIXEL:
		{
			op++;
			int y = stack.popint();
			int x = stack.popint();
			QRgb rgb = (*image).pixel(x,y);
			stack.push((int) rgb);
		}
		break;
		
	case OP_GETCOLOR:
		{
			op++;
			stack.push((int) drawingpen.color().rgba());
		}
		break;
		
	case OP_GETSLICE:
		{
			// slice format is 4 digit HEX width, 4 digit HEX height,
			// and (w*h)*8 digit HEX RGB for each pixel of slice
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
					qs->append(QString::number(rgb,16).rightJustified(8,'0'));
				}
			}
			stack.push(qs->toUtf8().data());
			delete qs;
		}
		break;
		
	case OP_PUTSLICE:
	case OP_PUTSLICEMASK:
		{
			QRgb mask = 0x00;	// default mask transparent - nothing gets masked
			if (*op == OP_PUTSLICEMASK) mask = stack.popint();
			char *txt = stack.popstring();
			QString imagedata = QString::fromUtf8(txt);
			free(txt);
			int y = stack.popint();
			int x = stack.popint();
			bool ok;
			QRgb rgb, lastrgb = 0x00;
			int th, tw;
			int offset = 0; // location in qstring to get next hex number

			int w = imagedata.mid(offset,4).toInt(&ok, 16);
			offset+=4;
			if (ok) {
				int h = imagedata.mid(offset,4).toInt(&ok, 16);
				offset+=4;
				if (ok) {

					QPainter ian(image);
					ian.setPen(lastrgb);
					for(th=0; th<h && ok; th++) {
						for(tw=0; tw<w && ok; tw++) {
							rgb = imagedata.mid(offset, 8).toUInt(&ok, 16);
							offset+=8;
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
				errornum = ERROR_PUTBITFORMAT;
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
			ian.setPen(drawingpen);
			ian.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}
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
			ian.setBrush(drawingbrush);
			ian.setPen(drawingpen);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}
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
			poly.setPen(drawingpen);
			poly.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				poly.setCompositionMode(QPainter::CompositionMode_Clear);
			}
			if (variables.type(*i) == T_ARRAY)
			{
				int pairs = variables.arraysize(*i) / 2;
				if (pairs < 3)
				{
					errornum = ERROR_POLYPOINTS;
				} else {

					QPointF *points = new QPointF[pairs];
	
					for (int j = 0; j < pairs; j++)
					{
						points[j].setX(variables.arraygetfloat(*i, j*2));
						points[j].setY(variables.arraygetfloat(*i, j*2+1));
					}
					poly.drawPolygon(points, pairs);
					poly.end();
					if (!fastgraphics) waitForGraphics();
					delete points;
				}
			} else {
				errornum = ERROR_POLYARRAY;
				poly.end();
			}
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
			poly.setPen(drawingpen);
			poly.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				poly.setCompositionMode(QPainter::CompositionMode_Clear);
			}
			int pairs = *i / 2;
			if (pairs < 3)
			{
				errornum = ERROR_POLYPOINTS;
			} else {
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

			if (variables.type(*i) == T_ARRAY)
			{
				int pairs = variables.arraysize(*i) / 2;
				if (pairs < 3)
				{
					errornum = ERROR_POLYPOINTS;
				} else {
	
					if (scale>0) {
						QPainter poly(image);
						poly.setPen(drawingpen);
						poly.setBrush(drawingbrush);
						if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
							poly.setCompositionMode(QPainter::CompositionMode_Clear);
						}
						QPointF *points = new QPointF[pairs];
	
						for (int j = 0; j < pairs; j++)
						{
							double scalex = scale * variables.arraygetfloat(*i, j*2);
							double scaley = scale * variables.arraygetfloat(*i, j*2+1);
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
			} else {
				errornum = ERROR_POLYARRAY;
			}

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
			
			QPainter poly(image);
			poly.setPen(drawingpen);
			poly.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				poly.setCompositionMode(QPainter::CompositionMode_Clear);
			}

			int pairs = llist / 2;
			if (pairs < 3)
			{
				errornum = ERROR_POLYPOINTS;
			} else {
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
		}
		break;


	case OP_CIRCLE:
		{
			op++;
			int rval = stack.popint();
			int yval = stack.popint();
			int xval = stack.popint();

			QPainter ian(image);
			ian.setPen(drawingpen);
			ian.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}
			ian.drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_IMGLOAD:
		{
			// Image Load - with scale and rotate
			op++;
			
			// pop the filename to uncover the location and scale
			char *file = stack.popstring();
			
			double rotate = stack.popfloat();
			double scale = stack.popfloat();
			double y = stack.popint();
			double x = stack.popint();
			
			if (scale>0) {
				QImage i(QString::fromUtf8(file));
				if(i.isNull()) {
					errornum = ERROR_IMAGEFILE;
				} else {
					QPainter ian(image);
					if (rotate != 0 || scale != 1) {
						QTransform transform = QTransform().translate(0,0).rotateRadians(rotate).scale(scale, scale);
						i = i.transformed(transform);
					}
					ian.drawImage((int)(x - .5 * i.width()), (int)(y - .5 * i.height()), i);
					ian.end();
					if (!fastgraphics) waitForGraphics();
				}
			}
			free(file);
		}
		break;

	case OP_TEXT:
		{
			op++;
			char *txt = stack.popstring();
			int y0val = stack.popint();
			int x0val = stack.popint();

			QPainter ian(image);
			ian.setPen(drawingpen);
			ian.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}
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
			
			QPainter ian(image);
			ian.setPen(QColor(0,0,0,0));
			ian.setBrush(QColor(0,0,0,0));
			ian.setCompositionMode(QPainter::CompositionMode_Clear);
			ian.drawRect(0, 0, graph->image->width(), graph->image->height());
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_PLOT:
		{
			op++;
			int oneval = stack.popint();
			int twoval = stack.popint();

			QPainter ian(image);
			ian.setPen(drawingpen);
			if (drawingpen.color()==QColor(0,0,0,0)) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}

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
				emit(mainWindowsResize(1, width, height));
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


            variables.setfloat(*num, temp);

            if(debugMode)
            {
                emit(varAssignment(QString(symtable[*num]), QString::number(variables.getfloat(*num)), -1, -1));
            }
        }
        break;

     case OP_STRINGASSIGN:
		{
			op++;
			int *num = (int *) op;
			op += sizeof(int);

			char *temp = stack.popstring();	// don't free - assigned to a variable
			
			variables.setstring(*num, temp);

			if(debugMode)
			{
			  emit(varAssignment(QString(symtable[*num]), QString::fromUtf8(variables.getstring(*num)), -1, -1));
			}
		}
		break;

    case OP_VARREFASSIGN:
        {
        // assign a numeric variable reference
        op++;
        int *num = (int *) op;
        op += sizeof(int);
        int type = stack.peekType();
        int value = stack.popint();
        if (type==T_VARREF) {
            variables.setvarref(*num, value);
        } else {
            if (type==T_VARREFSTR) {
                errornum = ERROR_BYREFTYPE;
            } else {
                errornum = ERROR_BYREF;
            }
        }
        }
        break;

    case OP_VARREFSTRASSIGN:
        {
        // assign a string variable reference
        op++;
        int *num = (int *) op;
        op += sizeof(int);
        int type = stack.peekType();
        int value = stack.popint();
        if (type==T_VARREFSTR) {
            variables.setvarref(*num, value);
        } else {
            if (type==T_VARREF) {
                errornum = ERROR_BYREFTYPE;
            } else {
                errornum = ERROR_BYREF;
            }
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

	case OP_INCREASERECURSE:
		// increase recursion level in variable hash
		{
			op++;
			variables.increaserecurse();
			if (variables.error()!=ERROR_NONE) {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}			
		}
		break;

	case OP_DECREASERECURSE:
		// decrease recursion level in variable hash
		// and pop any unfinished for statements off of forstack
		{
			op++;
			while (forstack&&forstack->recurselevel == variables.getrecurse()) {
				forframe *temp = new forframe;
				temp = forstack;
				forstack = temp->next;
				delete temp;
			}

			variables.decreaserecurse();
			if (variables.error()!=ERROR_NONE) {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
			}
		}
		break;

	
	case OP_EXTENDEDNONE:
		{
			// extended none - allow for an extra range of operations
			// ALL MUST BE ONE BYTE op codes in thiis group of extended ops
			op++;
			switch(*op) {
				case OPX_SPRITEDIM:
					{
						int n = stack.popint();
						op++;
						// deallocate existing sprites
						clearsprites();
						// create new ones that are not visible, active, and are at origin
						if (n > 0) {
							sprites = (sprite*) malloc(sizeof(sprite) * n);
							nsprites = n;
							while (n>0) {
								n--;
								sprites[n].image = NULL;
								sprites[n].underimage = NULL;
								sprites[n].visible = false;
								sprites[n].x = 0;
								sprites[n].y = 0;
								sprites[n].r = 0;
								sprites[n].s = 1;
							}
						}
					}
					break;

				case OPX_SPRITELOAD:
					{
						op++;
						
						char *file = stack.popstring();
						int n = stack.popint();
						
						if(n < 0 || n >=nsprites) {
							errornum = ERROR_SPRITENUMBER;
						} else {
							spriteundraw(n);
							if (sprites[n].image) {
								// free previous image before replacing
								delete sprites[n].image;
								sprites[n].image = NULL;
							}
							sprites[n].image = new QImage(QString::fromUtf8(file));
							if(sprites[n].image->isNull()) {
								errornum = ERROR_IMAGEFILE;
								sprites[n].image = NULL;							
							} else {
								if (sprites[n].underimage) {
									// free previous underimage before replacing
									delete sprites[n].underimage;
									sprites[n].underimage = NULL;
								}
								sprites[n].visible = false;
							}
							spriteredraw(n);
						}
			
						free(file);
					}
					break;
					
				case OPX_SPRITESLICE:
					{
						op++;
						
						int h = stack.popint();
						int w = stack.popint();
						int y = stack.popint();
						int x = stack.popint();
						int n = stack.popint();
						
						if(n < 0 || n >=nsprites) {
							errornum = ERROR_SPRITENUMBER;
						} else {
							spriteundraw(n);
							if (sprites[n].image) {
								// free previous image before replacing
								delete sprites[n].image;
								sprites[n].image = NULL;
							}
							sprites[n].image = new QImage(image->copy(x, y, w, h));
							if(sprites[n].image->isNull()) {
								errornum = ERROR_SPRITESLICE;
								sprites[n].image = NULL;
							} else {
								if (sprites[n].underimage) {
									// free previous underimage before replacing
									delete sprites[n].underimage;
									sprites[n].underimage = NULL;
								}
								sprites[n].visible = false;
							}
							spriteredraw(n);
						}
					}
					break;
					
				case OPX_SPRITEMOVE:
				case OPX_SPRITEPLACE:
					{
						
						unsigned char opcode = *op;
						op++;
						
						double r = stack.popfloat();
						double s = stack.popfloat();
						double y = stack.popfloat();
						double x = stack.popfloat();
						int n = stack.popint();
						
						if(n < 0 || n >=nsprites) {
							errornum = ERROR_SPRITENUMBER;
						} else {
							if(!sprites[n].image) {
								errornum = ERROR_SPRITENA;
							} else {
				
								spriteundraw(n);
								if (opcode==OPX_SPRITEMOVE) {
									x += sprites[n].x;
									y += sprites[n].y;
									s += sprites[n].s;
									r += sprites[n].r;
									if (x >= (int) graph->image->width()) x = (double) graph->image->width();
									if (x < 0) x = 0;
									if (y >= (int) graph->image->height()) y = (double) graph->image->height();
									if (y < 0) y = 0;
									if (s < 0) s = 0;
								}
								sprites[n].x = x;
								sprites[n].y = y;
								sprites[n].s = s;
								sprites[n].r = r;
								spriteredraw(n);
							
								if (!fastgraphics) waitForGraphics();
							}
						}
					}
					break;

				case OPX_SPRITEHIDE:
				case OPX_SPRITESHOW:
					{
						
						unsigned char opcode = *op;
						op++;
						
						int n = stack.popint();
						
						if(n < 0 || n >=nsprites) {
							errornum = ERROR_SPRITENUMBER;
						} else {
							if(!sprites[n].image) {
								errornum = ERROR_SPRITENA;
							} else {
						
								spriteundraw(n);
								sprites[n].visible = (opcode==OPX_SPRITESHOW);
								spriteredraw(n);
						
								if (!fastgraphics) waitForGraphics();
							}
						}
					}
					break;

				case OPX_SPRITECOLLIDE:
					{
						op++;
						
						int n1 = stack.popint();
						int n2 = stack.popint();
						
						if(n1 < 0 || n1 >=nsprites || n2 < 0 || n2 >=nsprites) {
							errornum = ERROR_SPRITENUMBER;
						} else {
							if(!sprites[n1].image || !sprites[n2].image) {
								 errornum = ERROR_SPRITENA;
							} else {
								stack.push(spritecollide(n1, n2));
							}
						}
					}
					break;

			case OPX_SPRITEX:
			case OPX_SPRITEY:
			case OPX_SPRITEH:
			case OPX_SPRITEW:
			case OPX_SPRITEV:
			case OPX_SPRITER:
			case OPX_SPRITES:
				{
					
					unsigned char opcode = *op;
					op++;
					int n = stack.popint();
					
					if(n < 0 || n >=nsprites) {
						errornum = ERROR_SPRITENUMBER;
						stack.push(0);	
					} else {
						if(!sprites[n].image) {
							errornum = ERROR_SPRITENA;
							stack.push(0);	
						} else {
							if (opcode==OPX_SPRITEX) stack.push(sprites[n].x);
							if (opcode==OPX_SPRITEY) stack.push(sprites[n].y);
							if (opcode==OPX_SPRITEH) stack.push(sprites[n].image->height());
							if (opcode==OPX_SPRITEW) stack.push(sprites[n].image->width());
							if (opcode==OPX_SPRITEV) stack.push(sprites[n].visible?1:0);
							if (opcode==OPX_SPRITER) stack.push(sprites[n].r);
							if (opcode==OPX_SPRITES) stack.push(sprites[n].s);
						}
					}
				}
				break;

			
			case OPX_CHANGEDIR:
					{
						op++;
						char *file = stack.popstring();
						if(!QDir::setCurrent(QString::fromUtf8(file))) {
							errornum = ERROR_FOLDER;
						}
						free(file);
					}
					break;

			case OPX_CURRENTDIR:
				{
					op++;
					stack.push(QDir::currentPath().toUtf8().data());
				}
				break;
				
			case OPX_WAVWAIT:
				{
					op++;
					emit(waitWAV());
				}
				break;

			case OPX_DECIMAL:
					{
						// set number of digits used in stack.popstring to
						// specify max number of decimal places in float to string
						op++;
						int n = stack.popint();
						if(n<0 || n > 15) {
							errornum = ERROR_DECIMALMASK;
						} else {
							stack.fToAMask = n;
						}
					}
					break;

			case OPX_DBOPEN:
					{
						op++;
						// open database connection
						char *file = stack.popstring();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							closeDatabase(n);
							int error = sqlite3_open(file, &dbconn[n]);
							if (error) {
								errornum = ERROR_DBOPEN;
							}
						}
						free(file);
					}
					break;

			case OPX_DBCLOSE:
					{
						op++;
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							closeDatabase(n);
						}
					}
					break;

			case OPX_DBEXECUTE:
					{
						op++;
						// execute a statement on the database
						char *stmt = stack.popstring();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if(dbconn[n]) {
								int error = sqlite3_exec(dbconn[n], stmt, 0, 0, 0);
								if (error != SQLITE_OK) {
									errornum = ERROR_DBQUERY;
									errormessage = sqlite3_errmsg(dbconn[n]);
								}
							} else {
								errornum = ERROR_DBNOTOPEN;
							}
						}
						free(stmt);
					}
					break;

			case OPX_DBOPENSET:
					{
						op++;
						// open recordset
						char *stmt = stack.popstring();
						int set = stack.popint();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if (set<0||set>=NUMDBSET) {
								errornum = ERROR_DBSETNUMBER;
							} else {
								if(dbconn[n]) {
									if (dbset[n][set]) {
										sqlite3_finalize(dbset[n][set]);
										dbset[n][set] = NULL;
									}
									dbsetrow[n][set] = false;
									int error = sqlite3_prepare_v2(dbconn[n], stmt, -1, &(dbset[n][set]), NULL);
									if (error != SQLITE_OK) {
										errornum = ERROR_DBQUERY;
										errormessage = sqlite3_errmsg(dbconn[n]);
									}
								} else {
									errornum = ERROR_DBNOTOPEN;
								}
							}
						}
						free(stmt);
					}
					break;

			case OPX_DBCLOSESET:
					{
						op++;
						int set = stack.popint();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if (set<0||set>=NUMDBSET) {
								errornum = ERROR_DBSETNUMBER;
							} else {
								if (dbset[n][set]) {
									sqlite3_finalize(dbset[n][set]);
									dbset[n][set] = NULL;
								} else {
									errornum = ERROR_DBNOTSET;
								}
							}
						}
					}
					break;

			case OPX_DBROW:
					{
						op++;
						int set = stack.popint();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if (set<0||set>=NUMDBSET) {
								errornum = ERROR_DBSETNUMBER;
							} else {
								if (dbset[n][set]) {
									// return true if we move to a new row else false
									dbsetrow[n][set] = true;
									stack.push((sqlite3_step(dbset[n][set]) == SQLITE_ROW?1:0));
								} else {
									errornum = ERROR_DBNOTSET;
								}
							}
						}
					}
					break;

			case OPX_DBINT:
			case OPX_DBINTS:
			case OPX_DBFLOAT:
			case OPX_DBFLOATS:
			case OPX_DBNULL:
			case OPX_DBNULLS:
			case OPX_DBSTRING:
			case OPX_DBSTRINGS:
					{
						unsigned char opcode = *op;
						int col = -1, set, n;
						char *colname = NULL;
						op++;
						// get a column data (integer)
						if (opcode == OPX_DBINTS || opcode == OPX_DBFLOATS || opcode == OPX_DBNULLS || opcode == OPX_DBSTRINGS) {
							colname = stack.popstring();
						} else {
							col = stack.popint();
						}
						set = stack.popint();
						n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if (set<0||set>=NUMDBSET) {
								errornum = ERROR_DBSETNUMBER;
							} else {
								if (!dbset[n][set]) {
									errornum = ERROR_DBNOTSET;
								} else {
									if (!dbsetrow[n][set]) {
										errornum = ERROR_DBNOTSETROW;
									} else {
										if (opcode == OPX_DBINTS || opcode == OPX_DBFLOATS || opcode == OPX_DBNULLS || opcode == OPX_DBSTRINGS) {
											// find the column number for name and save in col
											for(int t=0;t<sqlite3_column_count(dbset[n][set])&&col==-1;t++) {
												if (strcmp(colname,sqlite3_column_name(dbset[n][set],t))==0) {
													col = t;
												}
											}
											free(colname);
										}
										if (col < 0 || col >= sqlite3_column_count(dbset[n][set])) {
											errornum = ERROR_DBCOLNO;
										} else {
											switch(opcode) {
												case OPX_DBINT:
												case OPX_DBINTS:
													stack.push(sqlite3_column_int(dbset[n][set], col));
													break;
												case OPX_DBFLOAT:
												case OPX_DBFLOATS:
													stack.push(sqlite3_column_double(dbset[n][set], col));
													break;
												case OPX_DBNULL:
												case OPX_DBNULLS:
													stack.push((char*)sqlite3_column_text(dbset[n][set], col)==NULL);
													break;
												case OPX_DBSTRING:
												case OPX_DBSTRINGS:
													stack.push((char*)sqlite3_column_text(dbset[n][set], col));
													break;
											}
										}
									}
								}
							}
						}
					}
					break;

			case OPX_LASTERROR:
				{
					op++;
					stack.push(lasterrornum);
				}
				break;

			case OPX_LASTERRORLINE:
				{
					op++;
					stack.push(lasterrorline);
				}
				break;

			case OPX_LASTERROREXTRA:
				{
					op++;
					stack.push(lasterrormessage.toUtf8().data());
				}
				break;

			case OPX_LASTERRORMESSAGE:
				{
					op++;
					stack.push(getErrorMessage(lasterrornum).toUtf8().data());
				}
				break;

			case OPX_OFFERROR:
				{
					// turn off error trapping
					op++;
					onerroraddress = 0;
				}
				break;

			case OPX_NETLISTEN:
				{
					op++;
					int tempsockfd;
					struct sockaddr_in serv_addr, cli_addr;
					socklen_t clilen;

					int port = stack.popint();
					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {
						if (netsockfd[fn] >= 0) {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}

						// SOCK_DGRAM = UDP  SOCK_STREAM = TCP
						tempsockfd = socket(AF_INET, SOCK_STREAM, 0);
						if (tempsockfd < 0) {
							errornum = ERROR_NETSOCK;
							errormessage = strerror(errno);
						} else {
							int optval = 1;
							if (setsockopt(tempsockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(int))) {
								errornum = ERROR_NETSOCKOPT;
								errormessage = strerror(errno);
								tempsockfd = netSockClose(tempsockfd);
							} else {
								memset((char *) &serv_addr, 0, sizeof(serv_addr));
								serv_addr.sin_family = AF_INET;
								serv_addr.sin_addr.s_addr = INADDR_ANY;
								serv_addr.sin_port = htons(port);
								if (bind(tempsockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
									errornum = ERROR_NETBIND;
									errormessage = strerror(errno);
									tempsockfd = netSockClose(tempsockfd);
								} else {
									listen(tempsockfd,5);
									clilen = sizeof(cli_addr);
									netsockfd[fn] = accept(tempsockfd, (struct sockaddr *) &cli_addr, &clilen);
									if (netsockfd[fn] < 0) {
										errornum = ERROR_NETACCEPT;
										errormessage = strerror(errno);
									}
									tempsockfd = netSockClose(tempsockfd);
								}
							}
						}
					}
				}
				break;

			case OPX_NETCONNECT:
				{
					op++;

					struct sockaddr_in serv_addr;
					struct hostent *server;

					int port = stack.popint();
					char* address = stack.popstring();
					int fn = stack.popint();
					
					
					
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {

						if (netsockfd[fn] >= 0) {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}
					
						netsockfd[fn] = socket(AF_INET, SOCK_STREAM, 0);
						if (netsockfd[fn] < 0) {
							errornum = ERROR_NETSOCK;
							errormessage = strerror(errno);
						} else {

							server = gethostbyname(address);
							if (server == NULL) {
								errornum = ERROR_NETHOST;
								errormessage = strerror(errno);
								netsockfd[fn] = netSockClose(netsockfd[fn]);
							} else {
								memset((char *) &serv_addr, 0, sizeof(serv_addr));
								serv_addr.sin_family = AF_INET;
								memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
								serv_addr.sin_port = htons(port);
								if (::connect(netsockfd[fn],(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
									errornum = ERROR_NETCONN;
									errormessage = strerror(errno);
									netsockfd[fn] = netSockClose(netsockfd[fn]);
								}
							}
						}
					}
					free(address);
				}
				break;

			case OPX_NETREAD:
				{
					op++;
					int MAXSIZE = 2048;
					int n;
					char * strarray = (char *) malloc(MAXSIZE);

					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
						stack.push(strdup(""));
					} else {
						if (netsockfd[fn] < 0) {
							errornum = ERROR_NETNONE;
							stack.push(strdup(""));
						} else {
							memset(strarray, 0, MAXSIZE);
							n = recv(netsockfd[fn],strarray,MAXSIZE-1,0);
							if (n < 0) {
								errornum = ERROR_NETREAD;
								errormessage = strerror(errno);
							}
							stack.push(strdup(strarray));
							free(strarray);
						}
					}
				}
				break;

			case OPX_NETWRITE:
				{
					op++;
					char* data = stack.popstring();
					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {
						if (netsockfd[fn]<0) {
							errornum = ERROR_NETNONE;
						} else {
							int n = send(netsockfd[fn],data,strlen(data),0);
							if (n < 0) {
								errornum = ERROR_NETWRITE;
								errormessage = strerror(errno);
							}
						}
					}
					free(data);
				}
				break;

			case OPX_NETCLOSE:
				{
					op++;
					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {
						if (netsockfd[fn]<0) {
							errornum = ERROR_NETNONE;
						} else {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}
					}
				}
				break;

			case OPX_NETDATA:
				{
					op++;
					// push 1 if there is data to read on network connection
					// wait 1 ms for each poll
					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {
						#ifdef WIN32
						unsigned long n;
						if (ioctlsocket(netsockfd[fn], FIONREAD, &n)!=0) {
							stack.push(0);
						} else {
							if (n==0L) {
								stack.push(0);
							} else {
								stack.push(1);
							}
						}
						#else
						struct pollfd p[1];
						p[0].fd = netsockfd[fn];
						p[0].events = POLLIN | POLLPRI;
						if(poll(p, 1, 1)<0) {
							stack.push(0);
						} else {
							if (p[0].revents & POLLIN || p[0].revents & POLLPRI) {
								stack.push(1);
							} else {
								stack.push(0);
							}
						}
						#endif
					}
				}
				break;

			case OPX_NETADDRESS:
				{
					op++;
					// get first non "lo" ip4 address
					#ifdef WIN32
					char szHostname[100];
					HOSTENT *pHostEnt;
					int nAdapter = 0;
					struct sockaddr_in sAddr;
					gethostname( szHostname, sizeof( szHostname ));
					pHostEnt = gethostbyname( szHostname );
					memcpy ( &sAddr.sin_addr.s_addr, pHostEnt->h_addr_list[nAdapter], pHostEnt->h_length);
					stack.push(strdup(inet_ntoa(sAddr.sin_addr)));
					#else
					bool good = false;
					struct ifaddrs *myaddrs, *ifa;
					void *in_addr;
					char buf[64];
					if(getifaddrs(&myaddrs) != 0) {
						errornum = ERROR_NETNONE;
					} else {
						for (ifa = myaddrs; ifa != NULL && !good; ifa = ifa->ifa_next) {
							if (ifa->ifa_addr == NULL) continue;
							if (!(ifa->ifa_flags & IFF_UP)) continue;
							if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") !=0 ) {
								struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
								in_addr = &s4->sin_addr;
								if (inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))) {
									stack.push(strdup(buf));
									good = true;
								}
							}
						}
						freeifaddrs(myaddrs);
					}
					if (!good) {
						// on error give local loopback
						stack.push(strdup("127.0.0.1"));
					}
					#endif
				}
				break;

			case OPX_KILL:
				{
					op++;
					char *name = stack.popstring();
					if(!QFile::remove(QString::fromUtf8(name))) {
						errornum = ERROR_FILEOPEN;
					}
					free(name);
				}
				break;

			case OPX_MD5:
				{
					op++;
					char *stuff = stack.popstring();
					stack.push(MD5(stuff).hexdigest());
					free(stuff);
				}
				break;

			case OPX_SETSETTING:
				{
					op++;
					char *stuff = stack.popstring();
					char *key = stack.popstring();
					char *app = stack.popstring();
					QSettings settings(SETTINGSORG, SETTINGSAPP);
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						settings.beginGroup(SETTINGSGROUPUSER);
						settings.beginGroup(app);
						settings.setValue(key, stuff);
						settings.endGroup();
						settings.endGroup();
					} else {
						errornum = ERROR_PERMISSION;
					}
					free(stuff);
					free(key);
					free(app);
				}
				break;

			case OPX_GETSETTING:
				{
					op++;
					char *key = stack.popstring();
					char *app = stack.popstring();
					QSettings settings(SETTINGSORG, SETTINGSAPP);
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						settings.beginGroup(SETTINGSGROUPUSER);
						settings.beginGroup(app);
						stack.push(strdup(settings.value(key, "").toString().toUtf8().data()));
						settings.endGroup();
						settings.endGroup();
					} else {
						errornum = ERROR_PERMISSION;
					}
					free(key);
					free(app);
				}
				break;


			case OPX_PORTOUT:
				{
					op++;
					int data = stack.popint();
					int port = stack.popint();
					QSettings settings(SETTINGSORG, SETTINGSAPP);
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
						#ifdef WIN32
							if (Out32==NULL) {
								errornum = ERROR_NOTIMPLEMENTED;
							} else {
								Out32(port, data);
							}
						#else
							errornum = ERROR_NOTIMPLEMENTED;
						#endif
					} else {
						errornum = ERROR_PERMISSION;
					}
				}
				break;

			case OPX_PORTIN:
				{
					op++;
					int data=0;
					int port = stack.popint();
					QSettings settings(SETTINGSORG, SETTINGSAPP);
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
						#ifdef WIN32
							if (Inp32==NULL) {
								errornum = ERROR_NOTIMPLEMENTED;
							} else {
								data = Inp32(port);
							}
						#else
							errornum = ERROR_NOTIMPLEMENTED;
						#endif
					} else {
						errornum = ERROR_PERMISSION;
					}
					stack.push(data);
				}
				break;

			case OPX_BINARYOR:
				{
					op++;
					int a = stack.popint();
					int b = stack.popint();
					stack.push(a|b);
				}
				break;

			case OPX_BINARYAND:
				{
					op++;
					int a = stack.popint();
					int b = stack.popint();
					stack.push(a&b);
				}
				break;

			case OPX_BINARYNOT:
				{
					op++;
					int a = stack.popint();
					stack.push(~a);
				}
				break;

			case OPX_IMGSAVE:
				{
					// Image Save - Save image
					op++;
					char *type = stack.popstring();
         				char *file = stack.popstring();
					QStringList validtypes;
					validtypes << "BMP" << "bmp" << "JPG" << "jpg" << "JPEG" << "jpeg" << "PNG" << "png";
					if (validtypes.indexOf(QString(type))!=-1) {
         					image->save(QString::fromUtf8(file), type);
					} else {
						errornum = ERROR_IMAGESAVETYPE;
					}
					free(file);
					free(type);
				}
				break;

			case OPX_DIR:
				{
					// Get next directory entry - id path send start a new folder else get next file name
					// return "" if we have no names on list - skippimg . and ..
					op++;
					char *folder = stack.popstring();
					if (strlen(folder)>0) {
						if(directorypointer != NULL) {
							closedir(directorypointer);
							directorypointer = NULL;
						}
						directorypointer = opendir( folder );
					}
					if (directorypointer != NULL) {
						struct dirent *dirp;
						dirp = readdir(directorypointer);
						while(dirp != NULL && dirp->d_name[0]=='.') dirp = readdir(directorypointer);
						if (dirp) {
							stack.push(strdup(dirp->d_name));
						} else {
							stack.push(strdup(""));
							closedir(directorypointer);
							directorypointer = NULL;
						}
					} else {
						errornum = ERROR_FOLDER;
						stack.push(strdup(""));
					}
					free(folder);
				}
				break;

			case OPX_REPLACE:
			case OPX_REPLACE_C:
			case OPX_REPLACEX:
				{
					// unicode safe replace function
					unsigned char opcode = *op;
					op++;
					
					Qt::CaseSensitivity casesens = Qt::CaseSensitive;
					if(opcode==OPX_REPLACE_C) {
						if(stack.popfloat()!=0) casesens = Qt::CaseInsensitive;
					}
			
					char *to = stack.popstring();
					char *from = stack.popstring();
					char *haystack = stack.popstring();

					QString qto = QString::fromUtf8(to);
					QString qfrom = QString::fromUtf8(from);
					QString qhaystack = QString::fromUtf8(haystack);
			
					if(opcode==OPX_REPLACE || opcode==OPX_REPLACE_C) {
						stack.push(strdup(qhaystack.replace(qfrom, qto, casesens).toUtf8().data()));
					} else {
						stack.push(strdup(qhaystack.replace(QRegExp(qfrom), qto).toUtf8().data()));
					}

					free(to);
					free(from);
					free(haystack);
				}
				break;

			case OPX_COUNT:
			case OPX_COUNT_C:
			case OPX_COUNTX:
				{
					// unicode safe count function
					unsigned char opcode = *op;
					op++;
					
					Qt::CaseSensitivity casesens = Qt::CaseSensitive;
					if(opcode==OPX_COUNT_C) {
						if(stack.popfloat()!=0) casesens = Qt::CaseInsensitive;
					}
			
					char *needle = stack.popstring();
					char *haystack = stack.popstring();

					QString qneedle = QString::fromUtf8(needle);
					QString qhaystack = QString::fromUtf8(haystack);
			
					if(opcode==OPX_COUNT || opcode==OPX_COUNT_C) {
						stack.push((int) (qhaystack.count(qneedle, casesens)));
					} else {
						stack.push((int) (qhaystack.count(QRegExp(qneedle))));
					}

					free(needle);
					free(haystack);
				}
				break;

			case OPX_OSTYPE:
				{
					// Return type of OS this compile was for
					op++;
					int os = -1;
					#ifdef WIN32
						os = 0;
					#endif
					#ifdef LINUX
						os = 1;
					#endif
					#ifdef MACX
						os = 2;
					#endif
					stack.push(os);
				}
				break;

			case OPX_MSEC:
				{
					// Return number of milliseconds the BASIC256 program has been running
					op++;
					stack.push((int) (runtimer.elapsed()));
				}
				break;

			case OPX_EDITVISIBLE:
			case OPX_GRAPHVISIBLE:
			case OPX_OUTPUTVISIBLE:
				{
					unsigned char opcode = *op;
					op++;
					int show = stack.popint();
					if (opcode==OPX_EDITVISIBLE) emit(mainWindowsVisible(0,show!=0));
					if (opcode==OPX_GRAPHVISIBLE) emit(mainWindowsVisible(1,show!=0));
					if (opcode==OPX_OUTPUTVISIBLE) emit(mainWindowsVisible(2,show!=0));
				}
				break;

			case OPX_TEXTWIDTH:
				{
					// return the number of pixels the test string will require for diaplay
					op++;
					char *txt = stack.popstring();
					int w = 0;
					QPainter ian(image);
					if(!fontfamily.isEmpty()) {
						ian.setFont(QFont(fontfamily, fontpoint, fontweight));
					}
					w = QFontMetrics(ian.font()).width(QString::fromUtf8(txt));
					ian.end();
					free(txt);
					stack.push((int) (w));

				}
				break;

			case OPX_FREEFILE:
				{
					// return the next free file number - throw error if not free files
					op++;
					int f=-1;
					for (int t=0;(t<NUMFILES)&&(f==-1);t++) {
						if (!stream[t]) f = t;
					}
					if (f==-1) {
						errornum = ERROR_FREEFILE;
					} else {
						stack.push(f);
					}
				}
				break;

			case OPX_FREENET:
				{
					// return the next free network socket number - throw error if not free sockets
					op++;
					int f=-1;
					for (int t=0;(t<NUMSOCKETS)&&(f==-1);t++) {
						if (netsockfd[t]==-1) f = t;
					}
					if (f==-1) {
						errornum = ERROR_FREENET;
					} else {
						stack.push(f);
					}
				}
				break;

			case OPX_FREEDB:
				{
					// return the next free databsae number - throw error if none free
					op++;
					int f=-1;
					for (int t=0;(t<NUMDBCONN)&&(f==-1);t++) {
						if (!dbconn[t]) f = t;
					}
					if (f==-1) {
						errornum = ERROR_FREEDB;
					} else {
						stack.push(f);
					}
				}
				break;

			case OPX_FREEDBSET:
				{
					// return the next free set for a database - throw error if none free
					op++;
					int n = stack.popint();
					int f=-1;
					if (n<0||n>=NUMDBCONN) {
						errornum = ERROR_DBCONNNUMBER;
					} else {
						for (int t=0;(t<NUMDBSET)&&(f==-1);t++) {
							if (!dbset[n][t]) f = t;
						}
						if (f==-1) {
							errornum = ERROR_FREEDBSET;
						} else {
							stack.push(f);
						}
					}
				}
				break;

			case OPX_ARC:
			case OPX_CHORD:
			case OPX_PIE:
				{
					unsigned char opcode = *op;
					op++;
					double angwval = stack.popfloat();
					double startval = stack.popfloat();
					int hval = stack.popint();
					int wval = stack.popint();
					int yval = stack.popint();
					int xval = stack.popint();
					
					// degrees * 16
					int s = (int) (startval * 360 * 16 / 2 / M_PI);
					int aw = (int) (angwval * 360 * 16 / 2 / M_PI);
					// transform to clockwise from 12'oclock
					s = 1440-s-aw;

					QPainter ian(image);
					ian.setPen(drawingpen);
					ian.setBrush(drawingbrush);
					if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
						ian.setCompositionMode(QPainter::CompositionMode_Clear);
					}
					if(opcode==OPX_ARC) {
						ian.drawArc(xval, yval, wval, hval, s, aw);
					}
					if(opcode==OPX_CHORD) {
						ian.drawChord(xval, yval, wval, hval, s, aw);
					}
					if(opcode==OPX_PIE) {
						ian.drawPie(xval, yval, wval, hval, s, aw);
					}
					
					ian.end();

					if (!fastgraphics) waitForGraphics();
				}
				break;

			case OPX_PENWIDTH:
				{
					op++;
					double a = stack.popfloat();
					if (a<0) {
						errornum = ERROR_PENWIDTH;
					} else {
						drawingpen.setWidthF(a);
					}
				}
				break;

			case OPX_GETPENWIDTH:
				{
					op++;
					stack.push((double) (drawingpen.widthF()));
				}
				break;

			case OPX_GETBRUSHCOLOR:
				{
					op++;
					stack.push((int) drawingbrush.color().rgba());
				}
				break;
		
			case OPX_RUNTIMEWARNING:
				{
					// display a runtime warning message
					// added in yacc/bison for deprecated and other warnings
					op++;
					int w = stack.popint();
					QSettings settings(SETTINGSORG, SETTINGSAPP);
					if(settings.value(SETTINGSALLOWWARNINGS, SETTINGSALLOWWARNINGSDEFAULT).toBool()) {
						emit(outputReady(tr("WARNING on line ") + QString::number(currentLine) + ": " + getWarningMessage(w) + "\n"));
					}
				}
				break;







				// insert additional extended operations here
				
			default:
				{
					printError(ERROR_EXTOPBAD, "");
					status = R_STOPPED;
					return -1;
					break;
				}
			}
		}
		break;
		
	case OP_STACKSWAP:
		{
			op++;
			// swap the top of the stack
			// 0, 1, 2, 3...  becomes 1, 0, 2, 3...
			stack.swap();
		}
		break;

	case OP_STACKSWAP2:
		{
			op++;
			// swap the top two pairs of the stack
			// 0, 1, 2, 3...  becomes 2,3, 0,1...
			stack.swap2();
		}
		break;
		
	case OP_STACKDUP:
		{
			op++;
			// duplicate top stack entry
			stack.dup();
		}
		break;
		
	case OP_STACKDUP2:
		{
			op++;
			// duplicate top 2 stack entries
			stack.dup2();
		}
		break;
		
	case OP_STACKTOPTO2:
		{
			// move the top of the stack under the next two
			// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
			op++;
			stack.topto2();
		}
		break;

	case OP_ARGUMENTCOUNTTEST:
		{
			// Throw error if stack does not have enough values
			// used to check if functions and subroutines have the proper number
			// of datas on the stack to fill the parameters
			op++;
			int a = stack.popint();
			if (stack.height()<a) errornum = ERROR_ARGUMENTCOUNT;
			//printf("ac args=%i stack=%i\n",a,stack.height());
		}
		break;
		
	case OP_THROWERROR:
		{
			// Throw a user defined error number
			op++;
			int fn = stack.popint();
			errornum = fn;
		}
		break;

	default:
		status = R_STOPPED;
		return -1;
		break;
	}

	return 0;
}
