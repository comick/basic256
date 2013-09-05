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

#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QMessageBox>
#else
	#include <QMessageBox>
#endif

using namespace std;

#include "LEX/basicParse.tab.h"
#include "ByteCodes.h"
#include "CompileErrors.h"
#include "Interpreter.h"
#include "md5.h"
#include "Settings.h"
#include "Sound.h"

extern QMutex *mutex;
extern QMutex *debugmutex;
extern QWaitCondition *waitCond;
extern QWaitCondition *waitDebugCond;

extern int currentKey;

extern BasicGraph *graphwin;

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
	extern int numsyms;
}

Interpreter::Interpreter()
{
	fastgraphics = false;
	directorypointer=NULL;
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


int Interpreter::optype(int op) {
	// define how big the ocode is and if anything follows it
	// REQUIRED for all new opcodes
	// use constantants found in ByteCodes.h
	if (op==OP_END) return OPTYPE_NONE;
	else if (op==OP_NOP) return OPTYPE_NONE;
	else if (op==OP_RETURN) return OPTYPE_NONE;
	else if (op==OP_CONCAT) return OPTYPE_NONE;
	else if (op==OP_EQUAL) return OPTYPE_NONE;
	else if (op==OP_NEQUAL) return OPTYPE_NONE;
	else if (op==OP_GT) return OPTYPE_NONE;
	else if (op==OP_LT) return OPTYPE_NONE;
	else if (op==OP_GTE) return OPTYPE_NONE;   
	else if (op==OP_LTE) return OPTYPE_NONE;   
	else if (op==OP_AND) return OPTYPE_NONE;   
	else if (op==OP_NOT) return OPTYPE_NONE;   
	else if (op==OP_OR) return OPTYPE_NONE;   
	else if (op==OP_XOR) return OPTYPE_NONE;   
	else if (op==OP_INT) return OPTYPE_NONE;   
	else if (op==OP_STRING) return OPTYPE_NONE;
	else if (op==OP_ADD) return OPTYPE_NONE;
	else if (op==OP_SUB) return OPTYPE_NONE;
	else if (op==OP_MUL) return OPTYPE_NONE;
	else if (op==OP_DIV) return OPTYPE_NONE;
	else if (op==OP_EX) return OPTYPE_NONE;
	else if (op==OP_NEGATE) return OPTYPE_NONE;
	else if (op==OP_PRINT) return OPTYPE_NONE;
	else if (op==OP_PRINTN) return OPTYPE_NONE;
	else if (op==OP_INPUT) return OPTYPE_NONE;
	else if (op==OP_KEY) return OPTYPE_NONE;
	else if (op==OP_PLOT) return OPTYPE_NONE;
	else if (op==OP_RECT) return OPTYPE_NONE;
	else if (op==OP_CIRCLE) return OPTYPE_NONE;
	else if (op==OP_LINE) return OPTYPE_NONE;
	else if (op==OP_REFRESH) return OPTYPE_NONE;
	else if (op==OP_FASTGRAPHICS) return OPTYPE_NONE;
	else if (op==OP_CLS) return OPTYPE_NONE;
	else if (op==OP_CLG) return OPTYPE_NONE;
	else if (op==OP_GRAPHSIZE) return OPTYPE_NONE;
	else if (op==OP_GRAPHWIDTH) return OPTYPE_NONE;
	else if (op==OP_GRAPHHEIGHT) return OPTYPE_NONE;
	else if (op==OP_SIN) return OPTYPE_NONE;
	else if (op==OP_COS) return OPTYPE_NONE;
	else if (op==OP_TAN) return OPTYPE_NONE;
	else if (op==OP_RAND) return OPTYPE_NONE;
	else if (op==OP_CEIL) return OPTYPE_NONE;
	else if (op==OP_FLOOR) return OPTYPE_NONE;
	else if (op==OP_ABS) return OPTYPE_NONE;
	else if (op==OP_PAUSE) return OPTYPE_NONE;
	else if (op==OP_POLY) return OPTYPE_NONE;
	else if (op==OP_LENGTH) return OPTYPE_NONE;
	else if (op==OP_MID) return OPTYPE_NONE;
	else if (op==OP_INSTR) return OPTYPE_NONE;
	else if (op==OP_INSTR_S) return OPTYPE_NONE;
	else if (op==OP_INSTR_SC) return OPTYPE_NONE;
	else if (op==OP_INSTRX) return OPTYPE_NONE;
	else if (op==OP_INSTRX_S) return OPTYPE_NONE;
	else if (op==OP_OPEN) return OPTYPE_NONE;
	else if (op==OP_READ) return OPTYPE_NONE;
	else if (op==OP_WRITE) return OPTYPE_NONE;
	else if (op==OP_CLOSE) return OPTYPE_NONE;
	else if (op==OP_RESET) return OPTYPE_NONE;
	else if (op==OP_SOUND) return OPTYPE_NONE;
	else if (op==OP_INCREASERECURSE) return OPTYPE_NONE;
	else if (op==OP_DECREASERECURSE) return OPTYPE_NONE;
	else if (op==OP_ASC) return OPTYPE_NONE;
	else if (op==OP_CHR) return OPTYPE_NONE;
	else if (op==OP_FLOAT) return OPTYPE_NONE;
	else if (op==OP_READLINE) return OPTYPE_NONE;
	else if (op==OP_EOF) return OPTYPE_NONE;
	else if (op==OP_MOD) return OPTYPE_NONE;
	else if (op==OP_YEAR) return OPTYPE_NONE;
	else if (op==OP_MONTH) return OPTYPE_NONE;
	else if (op==OP_DAY) return OPTYPE_NONE;
	else if (op==OP_HOUR) return OPTYPE_NONE;
	else if (op==OP_MINUTE) return OPTYPE_NONE;
	else if (op==OP_SECOND) return OPTYPE_NONE;
	else if (op==OP_MOUSEX) return OPTYPE_NONE;
	else if (op==OP_MOUSEY) return OPTYPE_NONE;
	else if (op==OP_MOUSEB) return OPTYPE_NONE;
	else if (op==OP_CLICKCLEAR) return OPTYPE_NONE;
	else if (op==OP_CLICKX) return OPTYPE_NONE;
	else if (op==OP_CLICKY) return OPTYPE_NONE;
	else if (op==OP_CLICKB) return OPTYPE_NONE;
	else if (op==OP_TEXT) return OPTYPE_NONE;
	else if (op==OP_FONT) return OPTYPE_NONE;
	else if (op==OP_SAY) return OPTYPE_NONE;
	else if (op==OP_WAVPLAY) return OPTYPE_NONE;
	else if (op==OP_WAVSTOP) return OPTYPE_NONE;
	else if (op==OP_SEEK) return OPTYPE_NONE;
	else if (op==OP_SIZE) return OPTYPE_NONE;
	else if (op==OP_EXISTS) return OPTYPE_NONE;
	else if (op==OP_LEFT) return OPTYPE_NONE;
	else if (op==OP_RIGHT) return OPTYPE_NONE;
	else if (op==OP_UPPER) return OPTYPE_NONE;
	else if (op==OP_LOWER) return OPTYPE_NONE;
	else if (op==OP_SYSTEM) return OPTYPE_NONE;
	else if (op==OP_VOLUME) return OPTYPE_NONE;
	else if (op==OP_SETCOLOR) return OPTYPE_NONE;
	else if (op==OP_RGB) return OPTYPE_NONE;
	else if (op==OP_PIXEL) return OPTYPE_NONE;
	else if (op==OP_GETCOLOR) return OPTYPE_NONE;
	else if (op==OP_ASIN) return OPTYPE_NONE;
	else if (op==OP_ACOS) return OPTYPE_NONE;
	else if (op==OP_ATAN) return OPTYPE_NONE;
	else if (op==OP_DEGREES) return OPTYPE_NONE;
	else if (op==OP_RADIANS) return OPTYPE_NONE;
	else if (op==OP_INTDIV) return OPTYPE_NONE;
	else if (op==OP_LOG) return OPTYPE_NONE;
	else if (op==OP_LOGTEN) return OPTYPE_NONE;
	else if (op==OP_GETSLICE) return OPTYPE_NONE;
	else if (op==OP_PUTSLICE) return OPTYPE_NONE;
	else if (op==OP_PUTSLICEMASK) return OPTYPE_NONE;
	else if (op==OP_IMGLOAD) return OPTYPE_NONE;
	else if (op==OP_SQR) return OPTYPE_NONE;
	else if (op==OP_EXP) return OPTYPE_NONE;
	else if (op==OP_ARGUMENTCOUNTTEST) return OPTYPE_NONE;
	else if (op==OP_THROWERROR) return OPTYPE_NONE;
	else if (op==OP_READBYTE) return OPTYPE_NONE;
	else if (op==OP_WRITEBYTE) return OPTYPE_NONE;
	else if (op==OP_STACKSWAP) return OPTYPE_NONE;
	else if (op==OP_STACKTOPTO2) return OPTYPE_NONE;
	else if (op==OP_STACKDUP) return OPTYPE_NONE;
	else if (op==OP_STACKDUP2) return OPTYPE_NONE;
	else if (op==OP_STACKSWAP2) return OPTYPE_NONE;
	else if (op==OP_GOTO) return OPTYPE_LABEL;
	else if (op==OP_GOSUB) return OPTYPE_LABEL;
	else if (op==OP_BRANCH) return OPTYPE_LABEL;
	else if (op==OP_NUMASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_STRINGASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_ARRAYASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_STRARRAYASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_PUSHVAR) return OPTYPE_VARIABLE;
	else if (op==OP_PUSHINT) return OPTYPE_INT;
	else if (op==OP_DEREF) return OPTYPE_VARIABLE;
	else if (op==OP_FOR) return OPTYPE_VARIABLE;
	else if (op==OP_NEXT) return OPTYPE_VARIABLE;
	else if (op==OP_CURRLINE) return OPTYPE_INT;
	else if (op==OP_DIM) return OPTYPE_VARIABLE;
	else if (op==OP_DIMSTR) return OPTYPE_VARIABLE;
	else if (op==OP_ONERROR) return OPTYPE_LABEL;
	else if (op==OP_EXPLODESTR) return OPTYPE_VARIABLE;
	else if (op==OP_EXPLODESTR_C) return OPTYPE_VARIABLE;
	else if (op==OP_EXPLODE) return OPTYPE_VARIABLE;
	else if (op==OP_EXPLODE_C) return OPTYPE_VARIABLE;
	else if (op==OP_EXPLODEXSTR) return OPTYPE_VARIABLE;
	else if (op==OP_EXPLODEX) return OPTYPE_VARIABLE;
	else if (op==OP_IMPLODE) return OPTYPE_VARIABLE;
	else if (op==OP_GLOBAL) return OPTYPE_VARIABLE;
	else if (op==OP_STAMP) return OPTYPE_INT;
	else if (op==OP_STAMP_LIST) return OPTYPE_INT;
	else if (op==OP_STAMP_S_LIST) return OPTYPE_INT;
	else if (op==OP_STAMP_SR_LIST) return OPTYPE_INT;
	else if (op==OP_POLY_LIST) return OPTYPE_INT;
	else if (op==OP_WRITELINE) return OPTYPE_NONE;
	else if (op==OP_ARRAYASSIGN2D) return OPTYPE_VARIABLE;
	else if (op==OP_STRARRAYASSIGN2D) return OPTYPE_VARIABLE;
	else if (op==OP_SOUND_ARRAY) return OPTYPE_VARIABLE;
	else if (op==OP_SOUND_LIST) return OPTYPE_INT;
	else if (op==OP_DEREF2D) return OPTYPE_VARIABLE;
	else if (op==OP_REDIM) return OPTYPE_VARIABLE;
	else if (op==OP_REDIMSTR) return OPTYPE_VARIABLE;
	else if (op==OP_REDIM2D) return OPTYPE_VARIABLE;
	else if (op==OP_REDIMSTR2D) return OPTYPE_VARIABLE;
	else if (op==OP_ALEN) return OPTYPE_VARIABLE;
	else if (op==OP_ALENX) return OPTYPE_VARIABLE;
	else if (op==OP_ALENY) return OPTYPE_VARIABLE;
	else if (op==OP_PUSHVARREF) return OPTYPE_VARIABLE;
	else if (op==OP_PUSHVARREFSTR) return OPTYPE_VARIABLE;
	else if (op==OP_VARREFASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_VARREFSTRASSIGN) return OPTYPE_VARIABLE;
	else if (op==OP_FUNCRETURN) return OPTYPE_VARIABLE;
	else if (op==OP_ARRAYLISTASSIGN) return OPTYPE_INTINT;
	else if (op==OP_STRARRAYLISTASSIGN) return OPTYPE_INTINT;
	else if (op==OP_PUSHFLOAT) return OPTYPE_FLOAT;
	else if (op==OP_PUSHSTRING) return OPTYPE_STRING;
	else if (op==OP_EXTENDEDNONE) return OPTYPE_EXTENDED;
	else return OPTYPE_NONE;
}

QString Interpreter::opname(int op) {
	// used to convert opcode number in debuginfo to opcode name
	if (op==OP_END) return QString("OP_END");
	else if (op==OP_NOP) return QString("OP_NOP");
	else if (op==OP_RETURN) return QString("OP_RETURN");
	else if (op==OP_CONCAT) return QString("OP_CONCAT");
	else if (op==OP_EQUAL) return QString("OP_EQUAL");
	else if (op==OP_NEQUAL) return QString("OP_NEQUAL");
	else if (op==OP_GT) return QString("OP_GT");
	else if (op==OP_LT) return QString("OP_LT");
	else if (op==OP_GTE) return QString("OP_GTE");
	else if (op==OP_LTE) return QString("OP_LTE");
	else if (op==OP_AND) return QString("OP_AND");
	else if (op==OP_NOT) return QString("OP_NOT");
	else if (op==OP_OR) return QString("OP_OR");
	else if (op==OP_XOR) return QString("OP_XOR");
	else if (op==OP_INT) return QString("OP_INT");
	else if (op==OP_STRING) return QString("OP_STRING");
	else if (op==OP_ADD) return QString("OP_ADD");
	else if (op==OP_SUB) return QString("OP_SUB");
	else if (op==OP_MUL) return QString("OP_MUL");
	else if (op==OP_DIV) return QString("OP_DIV");
	else if (op==OP_EX) return QString("OP_EX");
	else if (op==OP_NEGATE) return QString("OP_NEGATE");
	else if (op==OP_PRINT) return QString("OP_PRINT");
	else if (op==OP_PRINTN) return QString("OP_PRINTN");
	else if (op==OP_INPUT) return QString("OP_INPUT");
	else if (op==OP_KEY) return QString("OP_KEY");
	else if (op==OP_PLOT) return QString("OP_PLOT");
	else if (op==OP_RECT) return QString("OP_RECT");
	else if (op==OP_CIRCLE) return QString("OP_CIRCLE");
	else if (op==OP_LINE) return QString("OP_LINE");
	else if (op==OP_REFRESH) return QString("OP_REFRESH");
	else if (op==OP_FASTGRAPHICS) return QString("OP_FASTGRAPHICS");
	else if (op==OP_CLS) return QString("OP_CLS");
	else if (op==OP_CLG) return QString("OP_CLG");
	else if (op==OP_GRAPHSIZE) return QString("OP_GRAPHSIZE");
	else if (op==OP_GRAPHWIDTH) return QString("OP_GRAPHWIDTH");
	else if (op==OP_GRAPHHEIGHT) return QString("OP_GRAPHHEIGHT");
	else if (op==OP_SIN) return QString("OP_SIN");
	else if (op==OP_COS) return QString("OP_COS");
	else if (op==OP_TAN) return QString("OP_TAN");
	else if (op==OP_RAND) return QString("OP_RAND");
	else if (op==OP_CEIL) return QString("OP_CEIL");
	else if (op==OP_FLOOR) return QString("OP_FLOOR");
	else if (op==OP_ABS) return QString("OP_ABS");
	else if (op==OP_PAUSE) return QString("OP_PAUSE");
	else if (op==OP_POLY) return QString("OP_POLY");
	else if (op==OP_LENGTH) return QString("OP_LENGTH");
	else if (op==OP_MID) return QString("OP_MID");
	else if (op==OP_INSTR) return QString("OP_INSTR");
	else if (op==OP_INSTR_S) return QString("OP_INSTR_S");
	else if (op==OP_INSTR_SC) return QString("OP_INSTR_SC");
	else if (op==OP_INSTRX) return QString("OP_INSTRX");
	else if (op==OP_INSTRX_S) return QString("OP_INSTRX_S");
	else if (op==OP_OPEN) return QString("OP_OPEN");
	else if (op==OP_READ) return QString("OP_READ");
	else if (op==OP_WRITE) return QString("OP_WRITE");
	else if (op==OP_CLOSE) return QString("OP_CLOSE");
	else if (op==OP_RESET) return QString("OP_RESET");
	else if (op==OP_SOUND) return QString("OP_SOUND");
	else if (op==OP_INCREASERECURSE) return QString("OP_INCREASERECURSE");
	else if (op==OP_DECREASERECURSE) return QString("OP_DECREASERECURSE");
	else if (op==OP_ASC) return QString("OP_ASC");
	else if (op==OP_CHR) return QString("OP_CHR");
	else if (op==OP_FLOAT) return QString("OP_FLOAT");
	else if (op==OP_READLINE) return QString("OP_READLINE");
	else if (op==OP_EOF) return QString("OP_EOF");
	else if (op==OP_MOD) return QString("OP_MOD");
	else if (op==OP_YEAR) return QString("OP_YEAR");
	else if (op==OP_MONTH) return QString("OP_MONTH");
	else if (op==OP_DAY) return QString("OP_DAY");
	else if (op==OP_HOUR) return QString("OP_HOUR");
	else if (op==OP_MINUTE) return QString("OP_MINUTE");
	else if (op==OP_SECOND) return QString("OP_SECOND");
	else if (op==OP_MOUSEX) return QString("OP_MOUSEX");
	else if (op==OP_MOUSEY) return QString("OP_MOUSEY");
	else if (op==OP_MOUSEB) return QString("OP_MOUSEB");
	else if (op==OP_CLICKCLEAR) return QString("OP_CLICKCLEAR");
	else if (op==OP_CLICKX) return QString("OP_CLICKX");
	else if (op==OP_CLICKY) return QString("OP_CLICKY");
	else if (op==OP_CLICKB) return QString("OP_CLICKB");
	else if (op==OP_TEXT) return QString("OP_TEXT");
	else if (op==OP_FONT) return QString("OP_FONT");
	else if (op==OP_SAY) return QString("OP_SAY");
	else if (op==OP_WAVPLAY) return QString("OP_WAVPLAY");
	else if (op==OP_WAVSTOP) return QString("OP_WAVSTOP");
	else if (op==OP_SEEK) return QString("OP_SEEK");
	else if (op==OP_SIZE) return QString("OP_SIZE");
	else if (op==OP_EXISTS) return QString("OP_EXISTS");
	else if (op==OP_LEFT) return QString("OP_LEFT");
	else if (op==OP_RIGHT) return QString("OP_RIGHT");
	else if (op==OP_UPPER) return QString("OP_UPPER");
	else if (op==OP_LOWER) return QString("OP_LOWER");
	else if (op==OP_SYSTEM) return QString("OP_SYSTEM");
	else if (op==OP_VOLUME) return QString("OP_VOLUME");
	else if (op==OP_SETCOLOR) return QString("OP_SETCOLOR");
	else if (op==OP_RGB) return QString("OP_RGB");
	else if (op==OP_PIXEL) return QString("OP_PIXEL");
	else if (op==OP_GETCOLOR) return QString("OP_GETCOLOR");
	else if (op==OP_ASIN) return QString("OP_ASIN");
	else if (op==OP_ACOS) return QString("OP_ACOS");
	else if (op==OP_ATAN) return QString("OP_ATAN");
	else if (op==OP_DEGREES) return QString("OP_DEGREES");
	else if (op==OP_RADIANS) return QString("OP_RADIANS");
	else if (op==OP_INTDIV) return QString("OP_INTDIV");
	else if (op==OP_LOG) return QString("OP_LOG");
	else if (op==OP_LOGTEN) return QString("OP_LOGTEN");
	else if (op==OP_GETSLICE) return QString("OP_GETSLICE");
	else if (op==OP_PUTSLICE) return QString("OP_PUTSLICE");
	else if (op==OP_PUTSLICEMASK) return QString("OP_PUTSLICEMASK");
	else if (op==OP_IMGLOAD) return QString("OP_IMGLOAD");
	else if (op==OP_SQR) return QString("OP_SQR");
	else if (op==OP_EXP) return QString("OP_EXP");
	else if (op==OP_ARGUMENTCOUNTTEST) return QString("OP_ARGUMENTCOUNTTEST");
	else if (op==OP_THROWERROR) return QString("OP_THROWERROR");
	else if (op==OP_READBYTE) return QString("OP_READBYTE");
	else if (op==OP_WRITEBYTE) return QString("OP_WRITEBYTE");
	else if (op==OP_STACKSWAP) return QString("OP_STACKSWAP");
	else if (op==OP_STACKTOPTO2) return QString("OP_STACKTOPTO2");
	else if (op==OP_STACKDUP) return QString("OP_STACKDUP");
	else if (op==OP_STACKDUP2) return QString("OP_STACKDUP2");
	else if (op==OP_STACKSWAP2) return QString("OP_STACKSWAP2");
	else if (op==OP_GOTO) return QString("OP_GOTO");
	else if (op==OP_GOSUB) return QString("OP_GOSUB");
	else if (op==OP_BRANCH) return QString("OP_BRANCH");
	else if (op==OP_NUMASSIGN) return QString("OP_NUMASSIGN");
	else if (op==OP_STRINGASSIGN) return QString("OP_STRINGASSIGN");
	else if (op==OP_ARRAYASSIGN) return QString("OP_ARRAYASSIGN");
	else if (op==OP_STRARRAYASSIGN) return QString("OP_STRARRAYASSIGN");
	else if (op==OP_PUSHVAR) return QString("OP_PUSHVAR");
	else if (op==OP_PUSHINT) return QString("OP_PUSHINT");
	else if (op==OP_DEREF) return QString("OP_DEREF");
	else if (op==OP_FOR) return QString("OP_FOR");
	else if (op==OP_NEXT) return QString("OP_NEXT");
	else if (op==OP_CURRLINE) return QString("OP_CURRLINE");
	else if (op==OP_DIM) return QString("OP_DIM");
	else if (op==OP_DIMSTR) return QString("OP_DIMSTR");
	else if (op==OP_ONERROR) return QString("OP_ONERROR");
	else if (op==OP_EXPLODESTR) return QString("OP_EXPLODESTR");
	else if (op==OP_EXPLODESTR_C) return QString("OP_EXPLODESTR_C");
	else if (op==OP_EXPLODE) return QString("OP_EXPLODE");
	else if (op==OP_EXPLODE_C) return QString("OP_EXPLODE_C");
	else if (op==OP_EXPLODEXSTR) return QString("OP_EXPLODEXSTR");
	else if (op==OP_EXPLODEX) return QString("OP_EXPLODEX");
	else if (op==OP_IMPLODE) return QString("OP_IMPLODE");
	else if (op==OP_GLOBAL) return QString("OP_GLOBAL");
	else if (op==OP_STAMP) return QString("OP_STAMP");
	else if (op==OP_STAMP_LIST) return QString("OP_STAMP_LIST");
	else if (op==OP_STAMP_S_LIST) return QString("OP_STAMP_S_LIST");
	else if (op==OP_STAMP_SR_LIST) return QString("OP_STAMP_SR_LIST");
	else if (op==OP_POLY_LIST) return QString("OP_POLY_LIST");
	else if (op==OP_WRITELINE) return QString("OP_WRITELINE");
	else if (op==OP_ARRAYASSIGN2D) return QString("OP_ARRAYASSIGN2D");
	else if (op==OP_STRARRAYASSIGN2D) return QString("OP_STRARRAYASSIGN2D");
	else if (op==OP_SOUND_ARRAY) return QString("OP_SOUND_ARRAY");
	else if (op==OP_SOUND_LIST) return QString("OP_SOUND_LIST");
	else if (op==OP_DEREF2D) return QString("OP_DEREF2D");
	else if (op==OP_REDIM) return QString("OP_REDIM");
	else if (op==OP_REDIMSTR) return QString("OP_REDIMSTR");
	else if (op==OP_REDIM2D) return QString("OP_REDIM2D");
	else if (op==OP_REDIMSTR2D) return QString("OP_REDIMSTR2D");
	else if (op==OP_ALEN) return QString("OP_ALEN");
	else if (op==OP_ALENX) return QString("OP_ALENX");
	else if (op==OP_ALENY) return QString("OP_ALENY");
	else if (op==OP_PUSHVARREF) return QString("OP_PUSHVARREF");
	else if (op==OP_PUSHVARREFSTR) return QString("OP_PUSHVARREFSTR");
	else if (op==OP_VARREFASSIGN) return QString("OP_VARREFASSIGN");
	else if (op==OP_VARREFSTRASSIGN) return QString("OP_VARREFSTRASSIGN");
	else if (op==OP_FUNCRETURN) return QString("OP_FUNCRETURN");
	else if (op==OP_ARRAYLISTASSIGN) return QString("OP_ARRAYLISTASSIGN");
	else if (op==OP_STRARRAYLISTASSIGN) return QString("OP_STRARRAYLISTASSIGN");
	else if (op==OP_PUSHFLOAT) return QString("OP_PUSHFLOAT");
	else if (op==OP_PUSHSTRING) return QString("OP_PUSHSTRING");
	else if (op==OP_EXTENDEDNONE) return QString("OP_EXTENDEDNONE");
	else return QString("OP_UNKNOWN");
}

QString Interpreter::opxname(int op) {
	// used to convert extended opcode number in debuginfo to opcode name
	if (op==OPX_SPRITEDIM) return QString("OPX_SPRITEDIM");
	else if (op==OPX_SPRITELOAD) return QString("OPX_SPRITELOAD");
	else if (op==OPX_SPRITESLICE) return QString("OPX_SPRITESLICE");
	else if (op==OPX_SPRITEMOVE) return QString("OPX_SPRITEMOVE");
	else if (op==OPX_SPRITEHIDE) return QString("OPX_SPRITEHIDE");
	else if (op==OPX_SPRITESHOW) return QString("OPX_SPRITESHOW");
	else if (op==OPX_SPRITECOLLIDE) return QString("OPX_SPRITECOLLIDE");
	else if (op==OPX_SPRITEPLACE) return QString("OPX_SPRITEPLACE");
	else if (op==OPX_SPRITEX) return QString("OPX_SPRITEX");
	else if (op==OPX_SPRITEY) return QString("OPX_SPRITEY");
	else if (op==OPX_SPRITEH) return QString("OPX_SPRITEH");
	else if (op==OPX_SPRITEW) return QString("OPX_SPRITEW");
	else if (op==OPX_SPRITEV) return QString("OPX_SPRITEV");
	else if (op==OPX_CHANGEDIR) return QString("OPX_CHANGEDIR");
	else if (op==OPX_CURRENTDIR) return QString("OPX_CURRENTDIR");
	else if (op==OPX_WAVWAIT) return QString("OPX_WAVWAIT");
	else if (op==OPX_DBOPEN) return QString("OPX_DBOPEN");
	else if (op==OPX_DBCLOSE) return QString("OPX_DBCLOSE");
	else if (op==OPX_DBEXECUTE) return QString("OPX_DBEXECUTE");
	else if (op==OPX_DBOPENSET) return QString("OPX_DBOPENSET");
	else if (op==OPX_DBCLOSESET) return QString("OPX_DBCLOSESET");
	else if (op==OPX_DBROW) return QString("OPX_DBROW");
	else if (op==OPX_DBINT) return QString("OPX_DBINT");
	else if (op==OPX_DBFLOAT) return QString("OPX_DBFLOAT");
	else if (op==OPX_DBSTRING) return QString("OPX_DBSTRING");
	else if (op==OPX_LASTERROR) return QString("OPX_LASTERROR");
	else if (op==OPX_LASTERRORLINE) return QString("OPX_LASTERRORLINE");
	else if (op==OPX_LASTERRORMESSAGE) return QString("OPX_LASTERRORMESSAGE");
	else if (op==OPX_LASTERROREXTRA) return QString("OPX_LASTERROREXTRA");
	else if (op==OPX_OFFERROR) return QString("OPX_OFFERROR");
	else if (op==OPX_NETLISTEN) return QString("OPX_NETLISTEN");
	else if (op==OPX_NETCONNECT) return QString("OPX_NETCONNECT");
	else if (op==OPX_NETREAD) return QString("OPX_NETREAD");
	else if (op==OPX_NETWRITE) return QString("OPX_NETWRITE");
	else if (op==OPX_NETCLOSE) return QString("OPX_NETCLOSE");
	else if (op==OPX_NETDATA) return QString("OPX_NETDATA");
	else if (op==OPX_NETADDRESS) return QString("OPX_NETADDRESS");
	else if (op==OPX_KILL) return QString("OPX_KILL");
	else if (op==OPX_MD5) return QString("OPX_MD5");
	else if (op==OPX_SETSETTING) return QString("OPX_SETSETTING");
	else if (op==OPX_GETSETTING) return QString("OPX_GETSETTING");
	else if (op==OPX_PORTIN) return QString("OPX_PORTIN");
	else if (op==OPX_PORTOUT) return QString("OPX_PORTOUT");
	else if (op==OPX_BINARYOR) return QString("OPX_BINARYOR");
	else if (op==OPX_BINARYAND) return QString("OPX_BINARYAND");
	else if (op==OPX_BINARYNOT) return QString("OPX_BINARYNOT");
	else if (op==OPX_IMGSAVE) return QString("OPX_IMGSAVE");
	else if (op==OPX_DIR) return QString("OPX_DIR");
	else if (op==OPX_REPLACE) return QString("OPX_REPLACE");
	else if (op==OPX_REPLACE_C) return QString("OPX_REPLACE_C");
	else if (op==OPX_REPLACEX) return QString("OPX_REPLACEX");
	else if (op==OPX_COUNT) return QString("OPX_COUNT");
	else if (op==OPX_COUNT_C) return QString("OPX_COUNT_C");
	else if (op==OPX_COUNTX) return QString("OPX_COUNTX");
	else if (op==OPX_OSTYPE) return QString("OPX_OSTYPE");
	else if (op==OPX_MSEC) return QString("OPX_MSEC");
	else if (op==OPX_EDITVISIBLE) return QString("OPX_EDITVISIBLE");
	else if (op==OPX_GRAPHVISIBLE) return QString("OPX_GRAPHVISIBLE");
	else if (op==OPX_OUTPUTVISIBLE) return QString("OPX_OUTPUTVISIBLE");
	else if (op==OPX_TEXTWIDTH) return QString("OPX_TEXTWIDTH");
	else if (op==OPX_SPRITER) return QString("OPX_SPRITER");
	else if (op==OPX_SPRITES) return QString("OPX_SPRITES");
	else if (op==OPX_FREEFILE) return QString("OPX_FREEFILE");
	else if (op==OPX_FREENET) return QString("OPX_FREENET");
	else if (op==OPX_FREEDB) return QString("OPX_FREEDB");
	else if (op==OPX_FREEDBSET) return QString("OPX_FREEDBSET");
	else if (op==OPX_DBINTS) return QString("OPX_DBINTS");
	else if (op==OPX_DBFLOATS) return QString("OPX_DBFLOATS");
	else if (op==OPX_DBSTRINGS) return QString("OPX_DBSTRINGS");
	else if (op==OPX_DBNULL) return QString("OPX_DBNULL");
	else if (op==OPX_DBNULLS) return QString("OPX_DBNULLS");
	else if (op==OPX_ARC) return QString("OPX_ARC");
	else if (op==OPX_CHORD) return QString("OPX_CHORD");
	else if (op==OPX_PIE) return QString("OPX_PIE");
	else if (op==OPX_PENWIDTH) return QString("OPX_PENWIDTH");
	else if (op==OPX_GETPENWIDTH) return QString("OPX_GETPENWIDTH");
	else if (op==OPX_GETBRUSHCOLOR) return QString("OPX_GETBRUSHCOLOR");
	else if (op==OPX_RUNTIMEWARNING) return QString("OPX_RUNTIMEWARNING");
	else if (op==OPX_ALERT) return QString("OPX_ALERT");
	else if (op==OPX_CONFIRM) return QString("OPX_CONFIRM");
	else if (op==OPX_PROMPT) return QString("OPX_PROMPT");
	else if (op==OPX_FROMRADIX) return QString("OPX_FROMRADIX");
	else if (op==OPX_TORADIX) return QString("OPX_TORADIX");
	else if (op==OPX_DEBUGINFO) return QString("OPX_DEBUGINFO");
	else return QString("OPX_UNKNOWN");
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
			errormessage = tr("No such label");
			break;
		case ERROR_FOR1:
			errormessage = tr("Illegal FOR -- start number > end number");
			break;
		case ERROR_FOR2 :
			errormessage = tr("Illegal FOR -- start number < end number");
			break;
		case ERROR_NEXTNOFOR:
			errormessage = tr("Next without FOR");
			break;
		case ERROR_FILENUMBER:
			errormessage = tr("Invalid File Number");
			break;
		case ERROR_FILEOPEN:
			errormessage = tr("Unable to open file");
			break;
		case ERROR_FILENOTOPEN:
			errormessage = tr("File not open.");
			break;
		case ERROR_FILEWRITE:
			errormessage = tr("Unable to write to file");
			break;
		case ERROR_FILERESET:
			errormessage = tr("Unable to reset file");
			break;
		case ERROR_ARRAYSIZELARGE:
			errormessage = tr("Array %VARNAME% dimension too large");
			break;
		case ERROR_ARRAYSIZESMALL:
			errormessage = tr("Array %VARNAME% dimension too small");
			break;
		case ERROR_NOSUCHVARIABLE:
			errormessage = tr("Unknown variable %VARNAME%");
			break;
		case ERROR_NOTARRAY:
			errormessage = tr("Variable %VARNAME% is not an array");
			break;
		case ERROR_NOTSTRINGARRAY:
			errormessage = tr("Variable %VARNAME% is not a string array");
			break;
		case ERROR_ARRAYINDEX:
			errormessage = tr("Array %VARNAME% index out of bounds");
			break;
		case ERROR_STRNEGLEN:
			errormessage = tr("Substring length less that zero");
			break;
		case ERROR_STRSTART:
			errormessage = tr("Starting position less than zero");
			break;
		case ERROR_STREND:
			errormessage = tr("String not long enough for given starting character");
			break;
		case ERROR_NONNUMERIC:
			errormessage = tr("Non-numeric value in numeric expression");
			break;
		case ERROR_RGB:
			errormessage = tr("RGB Color values must be in the range of 0 to 255.");
			break;
		case ERROR_PUTBITFORMAT:
			errormessage = tr("String input to putbit incorrect.");
			break;
		case ERROR_POLYARRAY:
			errormessage = tr("Argument not an array for poly()/stamp()");
			break;
		case ERROR_POLYPOINTS:
			errormessage = tr("Not enough points in array for poly()/stamp()");
			break;
		case ERROR_IMAGEFILE:
			errormessage = tr("Unable to load image file.");
			break;
		case ERROR_SPRITENUMBER:
			errormessage = tr("Sprite number out of range.");
			break;
		case ERROR_SPRITENA:
			errormessage = tr("Sprite has not been assigned.");
			break;
		case ERROR_SPRITESLICE:
			errormessage = tr("Unable to slice image.");
			break;
		case ERROR_FOLDER:
			errormessage = tr("Invalid directory name.");
			break;
		case ERROR_INFINITY:
			errormessage = tr("Operation returned infinity.");
			break;
		case ERROR_DBOPEN:
			errormessage = tr("Unable to open SQLITE database.");
			break;
		case ERROR_DBQUERY:
			errormessage = tr("Database query error (message follows).");
			break;
		case ERROR_DBNOTOPEN:
			errormessage = tr("Database must be opened first.");
			break;
		case ERROR_DBCOLNO:
			errormessage = tr("Column number out of range or column name not in data set.");
			break;
		case ERROR_DBNOTSET:
			errormessage = tr("Record set must be opened first.");
			break;
		case ERROR_EXTOPBAD:
			errormessage = tr("Invalid Extended Op-code.");
			break;
		case ERROR_NETSOCK:
			errormessage = tr("Error opening network socket.");
			break;
		case ERROR_NETHOST:
			errormessage = tr("Error finding network host.");
			break;
		case ERROR_NETCONN:
			errormessage = tr("Unable to connect to network host.");
			break;
		case ERROR_NETREAD:
			errormessage = tr("Unable to read from network connection.");
			break;
		case ERROR_NETNONE:
			errormessage = tr("Network connection has not been opened.");
			break;
		case ERROR_NETWRITE:
			errormessage = tr("Unable to write to network connection.");
			break;
		case ERROR_NETSOCKOPT:
			errormessage = tr("Unable to set network socket options.");
			break;
		case ERROR_NETBIND:
			errormessage = tr("Unable to bind network socket.");
			break;
		case ERROR_NETACCEPT:
			errormessage = tr("Unable to accept network connection.");
			break;
		case ERROR_NETSOCKNUMBER:
			errormessage = tr("Invalid Socket Number");
			break;
		case ERROR_PERMISSION:
			errormessage = tr("You do not have permission to use this statement/function.");
			break;
		case ERROR_IMAGESAVETYPE:
			errormessage = tr("Invalid image save type.");
			break;
		case ERROR_ARGUMENTCOUNT:
			errormessage = tr("Number of arguments passed does not match FUNCTION/SUBROUTINE definition.");
			break;
		case ERROR_MAXRECURSE:
			errormessage = tr("Maximum levels of recursion exceeded.");
			break;
        case ERROR_DIVZERO:
			errormessage = tr("Division by zero.");
            break;
		case ERROR_BYREF:
			errormessage = tr("Function/Subroutine expecting variable reference in call.");
			break;
		case ERROR_BYREFTYPE:
			errormessage = tr("Function/Subroutine variable incorrect reference type in call.");
            break;
		case ERROR_FREEFILE:
			errormessage = tr("There are no free file numbers to allocate.");
			break;
		case ERROR_FREENET:
			errormessage = tr("There are no free network connections to allocate.");
			break;
		case ERROR_FREEDB:
			errormessage = tr("There are no free database connections to allocate.");
			break;
		case ERROR_DBCONNNUMBER:
			errormessage = tr("Invalid Database Connection Number");
			break;
		case ERROR_FREEDBSET:
			errormessage = tr("There are no free data sets to allocate for that database connection.");
			break;
		case ERROR_DBSETNUMBER:
			errormessage = tr("Invalid data set number.");
			break;
		case ERROR_DBNOTSETROW:
			errormessage = tr("You must advance the data set using DBROW before you can read data from it.");
			break;
		case ERROR_PENWIDTH:
			errormessage = tr("Drawing pen width must be a non-negative number.");
			break;
		case ERROR_COLORNUMBER:
			errormessage = tr("Color values must be in the range of -1 to 16,777,215.");
			break;
		case ERROR_ARRAYINDEXMISSING:
			errormessage = tr("Array variable %VARNAME% has no value without an index");
			break;
		case ERROR_IMAGESCALE:
			errormessage = tr("Image scale must be greater than or equal to zero.");
			break;
		case ERROR_FONTSIZE:
			errormessage = tr("Font size, in points, must be greater than or equal to zero.");
			break;
		case ERROR_FONTWEIGHT:
			errormessage = tr("Font weight must be greater than or equal to zero.");
			break;
		case ERROR_RADIXSTRING:
			errormessage = tr("Unable to convert radix string back to a decimal number.");
			break;
		case ERROR_RADIX:
			errormessage = tr("Radix conversion base muse be between 2 and 36.");
			break;
		case ERROR_LOGRANGE:
			errormessage = tr("Unable to calculate the logarithm or root of a negative number.");
			break;
		case ERROR_STRINGMAXLEN:
			errormessage = tr("String exceeds maximum length of 16,777,216 characters.");
			break;
		case ERROR_FUNCRETURN:
			errormessage = tr("Function must return a value.");
			break;
		case ERROR_STACKUNDERFLOW:
			errormessage = tr("Stack Underflow Error.");
			break;
        // put ERROR new messages here
		case ERROR_NOTIMPLEMENTED:
			errormessage = tr("Feature not implemented in this environment.");
			break;
		default:
			errormessage = tr("User thrown error number.");
			
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
			message = tr("The used format of the statement has been deprecated.  It is recommended that you reauthor that statement.");
			break;
        // put WARNING new messages here
		case WARNING_NOTIMPLEMENTED:
			message = tr("Feature not implemented in this environment.");
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
	QPainter ian(graphwin->image);
	i = nsprites-1;
	while(i>=n) {
		if (sprites[i].underimage && sprites[i].visible) {
			x = sprites[i].x - (sprites[i].underimage->width()/2);
			y = sprites[i].y - (sprites[i].underimage->height()/2);
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
			QPainter ian(graphwin->image);
			if (sprites[i].r==0 && sprites[i].s==1) {
				if (sprites[i].underimage) {
					delete sprites[i].underimage;
					sprites[i].underimage = NULL;
				}
				if (sprites[i].image->width()>0 and sprites[i].image->height()>0) {
					x = sprites[i].x - (sprites[i].image->width()/2);
					y = sprites[i].y - (sprites[i].image->height()/2);
					sprites[i].underimage = new QImage(graphwin->image->copy(x, y, sprites[i].image->width(), sprites[i].image->height()));
					ian.drawImage(x, y, *(sprites[i].image));
				}
			} else {
				QTransform transform = QTransform().translate(0,0).rotateRadians(sprites[i].r).scale(sprites[i].s,sprites[i].s);;
				QImage rotated = sprites[i].image->transformed(transform);
				if (sprites[i].underimage) {
					delete sprites[i].underimage;
					sprites[i].underimage = NULL;
				}
				if (rotated.width()>0 and rotated.height()>0) {
					x = sprites[i].x - (rotated.width()/2);
					y = sprites[i].y - (rotated.height()/2);
					sprites[i].underimage = new QImage(graphwin->image->copy(x, y, rotated.width(), rotated.height()));
					ian.drawImage(x, y, rotated);
				}
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
				emit(outputReady(tr("Error assigning a string to a numeric variable on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ASSIGNN2S:
				emit(outputReady(tr("Error assigning a number to a string variable on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONGOTO:
				emit(outputReady(tr("You may not define a label or use a GOTO or GOSUB statement in a FUNCTION/SUBROUTINE declaration on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_GLOBALNOTHERE:
				emit(outputReady(tr("You may not define GLOBAL variable(s) inside an IF, loop, or FUNCTION/SUBROUTINE on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONNOTHERE:
				emit(outputReady(tr("You may not define a FUNCTION/SUBROUTINE inside an IF, loop, or other FUNCTION/SUBROUTINE on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDFUNCTION:
				emit(outputReady(tr("END FUNCTION/SUBROUTINE without matching FUNCTION/SUBROUTINE on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FUNCTIONNOEND:
				emit(outputReady(tr("FUNCTION/SUBROUTINE without matching END FUNCTION/SUBROUTINE statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_FORNOEND:
				emit(outputReady(tr("FOR without matching NEXT statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_WHILENOEND:
				emit(outputReady(tr("WHILE without matching END WHILE statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_DONOEND:
				emit(outputReady(tr("DO without matching UNTIL statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ELSENOEND:
				emit(outputReady(tr("ELSE without matching END IF statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_IFNOEND:
				emit(outputReady(tr("IF without matching END IF or ELSE statement on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_UNTIL:
				emit(outputReady(tr("UNTIL without matching DO on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDWHILE:
				emit(outputReady(tr("END WHILE without matching WHILE on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ELSE:
				emit(outputReady(tr("ELSE without matching IF on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_ENDIF:
				emit(outputReady(tr("END IF without matching IF on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_NEXT:
				emit(outputReady(tr("NEXT without matching FOR on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_RETURNVALUE:
				emit(outputReady(tr("RETURN with a value is only valid inside a FUNCTION on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_RETURNTYPE:
				emit(outputReady(tr("RETURN value type is not the same as FUNCTION on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_CONTINUEDO:
				emit(outputReady(tr("CONTINUE DO without matching DO on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_CONTINUEFOR:
				emit(outputReady(tr("CONTINUE DO without matching DO on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_CONTINUEWHILE:
				emit(outputReady(tr("CONTINUE WHILE without matching WHILE on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_EXITDO:
				emit(outputReady(tr("EXIT DO without matching DO on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_EXITFOR:
				emit(outputReady(tr("EXIT FOR without matching FOR on line ") + QString::number(linenumber) + ".\n"));
				break;
			case COMPERR_EXITWHILE:
				emit(outputReady(tr("EXIT WHILE without matching WHILE on line ") + QString::number(linenumber) + ".\n"));
				break;
			default:
				if(column==0) {
					emit(outputReady(tr("Syntax error on line ") + QString::number(linenumber) + tr(" around end of line.") + "\n"));
				} else {
					emit(outputReady(tr("Syntax error on line ") + QString::number(linenumber) + tr(" around column ") + QString::number(column) + ".\n"));
				}
		}
		emit(goToLine(linenumber));
		return -1;
	}

	// this logic goes through the bytecode generated and puts the actual
	// label address from labeltable into the bytecode
	op = byteCode;
	currentLine = 1;
	unsigned char currentop;
	while (op <= byteCode + byteOffset)
	{
		currentop = (unsigned char) *op;
		op += sizeof(unsigned char);
		if (currentop == OP_CURRLINE)
		{
			int *i = (int *) op;
			currentLine = *i;
			op += sizeof(int);
		}
		else if (optype(currentop) == OPTYPE_LABEL)
		{
			// change label number to actual bytecode address
			// before execution
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
		else if (optype(currentop) == OPTYPE_NONE)
		{
			// op has no args - do nothing
		}
		else if (optype(currentop) == OPTYPE_INT || optype(currentop) == OPTYPE_VARIABLE)
		{
			// op has an integer following
			op += sizeof(int);
		}
		else if (optype(currentop) == OPTYPE_INTINT)
		{
			// op has two integers following
			op += sizeof(int) * 2;
		}
		else if (optype(currentop) == OPTYPE_EXTENDED)
		{
			// op has second extended op
			op += sizeof(unsigned char);
		}
		else if (optype(currentop) == OPTYPE_FLOAT)
		{
			// double follows float
			op += sizeof(double);
		}
		else if (optype(currentop) == OPTYPE_STRING)
		{
			// in the group of OP_TYPESTRING
			// op has a single null terminated String arg
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
	fontfamily = QString("");
	fontpoint = 0;
	fontweight = 0;
	nsprites = 0;
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
Interpreter::runPaused()
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
Interpreter::runResumed()
{
	runPaused();
}

void
Interpreter::runHalted()
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
	emit(stopRun());
}


void
Interpreter::inputEntered(QString text)
{
	if (status!=R_STOPPED) {
		inputString = text;
		status = R_INPUTREADY;
	}
}


void
Interpreter::waitForGraphics()
{
	// wait for graphics operation to complete
	mutex->lock();
	emit(goutputReady());
	waitCond->wait(mutex);
	mutex->unlock();
}


int
Interpreter::execByteCode()
{
	if (status == R_INPUTREADY)
	{
		stack.pushstring(inputString);
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
	if (stack.error()!=ERROR_NONE) errornum = stack.error(); 
	if (errornum!=ERROR_NONE) {
		lasterrornum = errornum;
		lasterrormessage = errormessage;
		lasterrorline = currentLine;
		errornum = ERROR_NONE;
		errormessage = "";
		if(onerroraddress!=0 && lasterrornum > 0) {
			// progess call to subroutine for error handling
			frame *temp = new frame;
			temp->returnAddr = op;
			temp->next = callstack;
			callstack = temp;
			op = byteCode + onerroraddress;
			return 0;
		} else {
			// no error handler defined or FATAL error - display message and die
			emit(outputReady(QObject::tr("ERROR on line ") + QString::number(lasterrorline) + ": " + getErrorMessage(lasterrornum) + " " + lasterrormessage + "\n"));
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
			debugmutex->lock();
			waitDebugCond->wait(debugmutex);
			debugmutex->unlock();
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
			// goto if true
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
				emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.getfloat(*i)), -1, -1));
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
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.getfloat(*i)), -1, -1));
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
            QString name = stack.popstring();
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
				stream[fn] = new QFile(name);
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
		}
		break;


	case OP_READ:
		{
			op++;
			int fn = stack.popint();
			if (fn<0||fn>=NUMFILES) {
				errornum = ERROR_FILENUMBER;
				stack.pushint(0);
			} else {
				char c = ' ';
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.pushint(0);
				} else {
					int maxsize = 256;
					char * strarray = (char *) malloc(maxsize);
					memset(strarray, 0, maxsize);
					//Remove leading whitespace
					while (c == ' ' || c == '\t' || c == '\n')
					{
						if (!stream[fn]->getChar(&c))
						{
							stack.pushstring(QString::fromUtf8(strarray));
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
					stack.pushstring(QString::fromUtf8(strarray));
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
				stack.pushint(0);
			} else {

				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.pushint(0);
				} else {
					//read entire line
					stack.pushstring(stream[fn]->readLine());
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
				stack.pushint(0);
			} else {
				char c = ' ';
				if (stream[fn]->getChar(&c)) {
					stack.pushint((int) (unsigned char) c);
				} else {
					stack.pushint((int) -1);
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
				stack.pushint(1);
			} else {
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.pushint(1);
				} else {
					if (stream[fn]->atEnd()) {
						stack.pushint(1);
					} else {
						stack.pushint(0);
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
			QString temp = stack.popstring();
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
					error = stream[fn]->write(temp.toUtf8().data());
					if (whichop == OP_WRITELINE)
					{
						error = stream[fn]->putChar('\n');
					}
					stream[fn]->seek(oldPos);
					stream[fn]->flush();
				}
				if (error == -1)
				{
					errornum = ERROR_FILEWRITE;
				}
			}
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
				stack.pushint(0);
			} else {
				int size = 0;
				if (stream[fn] == NULL)
				{
					errornum = ERROR_FILENOTOPEN;
					stack.pushint(0);
				} else {
					size = stream[fn]->size();
				}
				stack.pushint(size);
			}
		}
		break;

	case OP_EXISTS:
		{
			// push a 1 if file exists else zero
			op++;

			QString filename = stack.popstring();
			if (QFile::exists(filename))
			{
				stack.pushint(1);
			} else {
				stack.pushint(0);
			}
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
			variables.arraydim(T_ARRAY, *i, xdim, ydim, whichdim == OP_REDIM);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					if (ydim==1) {
						emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, xdim, -1));
					} else {
						emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, xdim, ydim));
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
			variables.arraydim(T_STRARRAY, *i, xdim, ydim, whichdim == OP_REDIMSTR);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					if (ydim==1) {
						emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, xdim, -1));
					} else {
						emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, xdim, ydim));
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
					stack.pushint(variables.arraysize(*i));
					break;
				case OP_ALENX:
					stack.pushint(variables.arraysizex(*i));
					break;
				case OP_ALENY:
					stack.pushint(variables.arraysizey(*i));
					break;
			}
			if (variables.error()!=ERROR_NONE) {
				errornum = variables.error();
				errorvarnum = variables.errorvarnum();
				stack.pushint(0);	// unknown
			}
		}
		break;

	case OP_STRARRAYASSIGN:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			QString val = stack.popstring();
			int index = stack.popint();

			variables.arraysetstring(*i, index, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), variables.arraygetstring(*i, index), index, -1));
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

			QString val = stack.popstring(); 
			int yindex = stack.popint();
			int xindex = stack.popint();

			variables.array2dsetstring(*i, xindex, yindex, val);
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), variables.array2dgetstring(*i, xindex, yindex), xindex, yindex));
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
			
			if (variables.arraysize(*i)!=items) variables.arraydim(T_STRARRAY, *i, items, 1, false);

			if(errornum==ERROR_NONE)
			{
				if(debugMode)
				{
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, items, -1));
				}
			
				for (int index = items - 1; index >= 0 && errornum==ERROR_NONE; index--)
				{
					QString q = stack.popstring();
					variables.arraysetstring(*i, index, q);
					if (variables.error()==ERROR_NONE) {
						if(debugMode)
						{
							emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), variables.arraygetstring(*i, index), index, -1));
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
	
			QString qneedle = stack.popstring();
			QString qhaystack = stack.popstring();
					
			QStringList list;
			if(opcode==OP_EXPLODE || opcode==OP_EXPLODE_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODESTR_C) {
				list = qhaystack.split(qneedle, QString::KeepEmptyParts , casesens);
			} else {
				list = qhaystack.split(QRegExp(qneedle), QString::KeepEmptyParts);
			}
			
			if(opcode==OP_EXPLODESTR_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODEXSTR) {
				if (variables.arraysize(*i)!=list.size()) variables.arraydim(T_STRARRAY, *i, list.size(), 1, false);
			} else {
				if (variables.arraysize(*i)!=list.size()) variables.arraydim(T_ARRAY, *i, list.size(), 1, false);
			}
			if (variables.error()==ERROR_NONE) {
				if(debugMode)
				{
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, list.size(), -1));
				}
					
				for(int x=0; x<list.size(); x++) {
					if(opcode==OP_EXPLODESTR_C || opcode==OP_EXPLODESTR || opcode==OP_EXPLODEXSTR) {
						variables.arraysetstring(*i, x, list.at(x));
						if (variables.error()==ERROR_NONE) {
							if(debugMode) {
								emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), variables.arraygetstring(*i, x), x, -1));
							}
						} else {
							errornum = variables.error();
							errorvarnum = variables.errorvarnum();
						}			
					} else {
						variables.arraysetfloat(*i, x, list.at(x).toDouble(&ok));
						if (variables.error()==ERROR_NONE) {
							if(debugMode) {
								emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, x)), x, -1));
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
		}
		break;

	case OP_IMPLODE:
		{

			op++;
			int *i = (int *) op;
			op += sizeof(int);
			
			QString qdelim = stack.popstring();

			QString stuff = "";

			if (variables.type(*i) == T_STRARRAY || variables.type(*i) == T_ARRAY)
			{
				int kount = variables.arraysize(*i);
				for(int n=0;n<kount;n++) {
					if (n>0) stuff.append(qdelim);
					if (variables.type(*i) == T_STRARRAY) {
						stuff.append(variables.arraygetstring(*i, n));
					} else {
						stack.pushfloat(variables.arraygetfloat(*i, n));
						stuff.append(stack.popstring());
					}
				}
			} else {
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
			}
			stack.pushstring(stuff);
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
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, index)), index, -1));
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
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.array2dgetfloat(*i, xindex, yindex)), xindex, yindex));
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
			
			if (variables.arraysize(*i)!=items) variables.arraydim(T_ARRAY, *i, items, 1, false);
			if(errornum==ERROR_NONE) {
				if(debugMode && errornum==ERROR_NONE)
				{
					emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), NULL, items, -1));
				}
			
				for (int index = items - 1; index >= 0 && errornum==ERROR_NONE; index--)
				{
					double one = stack.popfloat();
					variables.arraysetfloat(*i, index, one);
					if (variables.error()==ERROR_NONE) {
						if(debugMode)
						{
							emit(varAssignment(variables.getrecurse(),QString(symtable[*i]), QString::number(variables.arraygetfloat(*i, index)), index, -1));
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
				stack.pushstring(variables.arraygetstring(*i, index));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else if (variables.type(*i) == T_ARRAY)
			{
				stack.pushfloat(variables.arraygetfloat(*i, index));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else
			{
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
				stack.pushint(0);
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
				stack.pushstring(variables.array2dgetstring(*i, xindex, yindex));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else if (variables.type(*i) == T_ARRAY)
			{
				stack.pushfloat(variables.array2dgetfloat(*i, xindex, yindex));
				if (variables.error()!=ERROR_NONE) {
					errornum = variables.error();
					errorvarnum = variables.errorvarnum();
				}			
			}
			else
			{
				errornum = ERROR_NOTARRAY;
				errorvarnum = *i;
				stack.pushint(0);
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
				stack.pushstring(variables.getstring(*i));
			}
			else if (variables.type(*i) == T_FLOAT)
			{
				stack.pushfloat(variables.getfloat(*i));
			}
			else if (variables.type(*i) == T_ARRAY || variables.type(*i) == T_STRARRAY)
			{
				errornum = ERROR_ARRAYINDEXMISSING;
				errorvarnum = *i;
				stack.pushint(0);
			}
			else
			{
				errornum = ERROR_NOSUCHVARIABLE;
				errorvarnum = *i;
				stack.pushint(0);
			}
		}
		break;

	case OP_FUNCRETURN:
		{
			op++;
			int *i = (int *) op;
			op += sizeof(int);

			if (variables.type(*i) == T_STRING)
			{
				stack.pushstring(variables.getstring(*i));
			}
			else if (variables.type(*i) == T_FLOAT)
			{
				stack.pushfloat(variables.getfloat(*i));
			}
			else
			{
				errornum = ERROR_FUNCRETURN;
				errorvarnum = *i;
				stack.pushint(0);
			}
		}
		break;

	case OP_PUSHINT:
		{
			op++;
			int *i = (int *) op;
			stack.pushint(*i);
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
			stack.pushfloat(*d);
			op += sizeof(double);
		}
		break;

	case OP_PUSHSTRING:
		{
			// push string from compiled bytecode
			// no need to free because string is in compiled code
			op++;
			int len = strlen((char *) op) + 1;
			stack.pushstring(QString::fromUtf8((char *) op));
			op += len;
		}
		break;


	case OP_INT:
		{
			// bigger integer safe (trim floating point off of a float)
			op++;
			double val = stack.popfloat();
			double intpart;
			val = modf(val, &intpart);
			stack.pushfloat(intpart);
		}
		break;


	case OP_FLOAT:
		{
			op++;
			double val = stack.popfloat();
			stack.pushfloat(val);
		}
		break;

	case OP_STRING:
		{
			op++;
			stack.pushstring(stack.popstring());
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
			stack.pushfloat(r);
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
			// unicode length
			op++;
			stack.pushint(stack.popstring().length());
		}
		break;


	case OP_MID:
		{
			// unicode safe mid string
			op++;
			int len = stack.popint();
			int pos = stack.popint();
			QString qtemp = stack.popstring();

			if ((len < 0))
			{
				errornum = ERROR_STRNEGLEN;
				stack.pushint(0);
			} else {
				if ((pos < 1))
				{
					errornum = ERROR_STRSTART;
					stack.pushint(0);
				} else {
					if ((pos < 1) || (pos > (int) qtemp.length()))
					{
						errornum = ERROR_STREND;
						stack.pushint(0);
					} else {
						stack.pushstring(qtemp.mid(pos-1,len));
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
			QString qtemp = stack.popstring();
			
			if (len < 0)
			{
				errornum = ERROR_STRNEGLEN;
				stack.pushint(0);
			} else {
				switch(opcode) {
					case OP_LEFT:
						stack.pushstring(qtemp.left(len));
						break;
					case OP_RIGHT:
						stack.pushstring(qtemp.right(len));
						break;
				}
			}
		}
		break;


	case OP_UPPER:
		{
			op++;
			stack.pushstring(stack.popstring().toUpper());
		}
		break;

	case OP_LOWER:
		{
			op++;
			stack.pushstring(stack.popstring().toLower());
		}
		break;

	case OP_ASC:
		{
			// unicode character sequence - return 16 bit number representing character
			op++;
			QString qs = stack.popstring();
			stack.pushint((int) qs[0].unicode());
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
			stack.pushstring(qs);
			qs = QString::null;
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
			
			QString qstr = stack.popstring();
			QString qhay = stack.popstring();

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
			stack.pushint(pos);
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
				stack.pushfloat(sin(val));
				break;
			case OP_COS:
				stack.pushfloat(cos(val));
				break;
			case OP_TAN:
				val = tan(val);
				if (isinf(val)) {
					errornum = ERROR_INFINITY;
					stack.pushint(0);
				} else {
					stack.pushfloat(val);
				}
				break;
			case OP_ASIN:
				stack.pushfloat(asin(val));
				break;
			case OP_ACOS:
				stack.pushfloat(acos(val));
				break;
			case OP_ATAN:
				stack.pushfloat(atan(val));
				break;
			case OP_CEIL:
				stack.pushint(ceil(val));
				break;
			case OP_FLOOR:
				stack.pushint(floor(val));
				break;
			case OP_ABS:
				if (val < 0)
				{
					val = -val;
				}
				stack.pushfloat(val);
				break;
			case OP_DEGREES:
				stack.pushfloat(val * 180 / M_PI);
				break;
			case OP_RADIANS:
				stack.pushfloat(val * M_PI / 180);
				break;
			case OP_LOG:
				if (val<0) {
					errornum = ERROR_LOGRANGE;
					stack.pushint(0);
				} else {
					stack.pushfloat(log(val));
				}
				break;
			case OP_LOGTEN:
				if (val<0) {
					errornum = ERROR_LOGRANGE;
					stack.pushint(0);
				} else {
					stack.pushfloat(log10(val));
				}
				break;
			case OP_SQR:
				if (val<0) {
					errornum = ERROR_LOGRANGE;
					stack.pushint(0);
				} else {
					stack.pushfloat(sqrt(val));
				}
				break;
			case OP_EXP:
				val = exp(val);
				if (isinf(val)) {
					errornum = ERROR_INFINITY;
					stack.pushint(0);
				} else {
					stack.pushfloat(val);
				}
				break;
			}
		}
		break;


	case OP_ADD:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			double ans = twoval + oneval;
			if (isinf(ans)) {
				errornum = ERROR_INFINITY;
				stack.pushint(0);
			} else {
				stack.pushfloat(ans);
			}
			break;
		}
		
	case OP_SUB:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			double ans = twoval - oneval;
			if (isinf(ans)) {
				errornum = ERROR_INFINITY;
				stack.pushint(0);
			} else {
				stack.pushfloat(ans);
			}
			break;
		}
		
	case OP_MUL:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			double ans = twoval * oneval;
			if (isinf(ans)) {
				errornum = ERROR_INFINITY;
				stack.pushint(0);
			} else {
				stack.pushfloat(ans);
			}
			break;
		}
		
	case OP_MOD:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			if (oneval==0) {
				errornum = ERROR_DIVZERO;
				stack.pushint(0);
			} else {
				double ans = fmod(twoval, oneval);
				if (isinf(ans)) {
					errornum = ERROR_INFINITY;
					stack.pushint(0);
				} else {
					stack.pushfloat(ans);
				}
			}
			break;
		}
		
	case OP_DIV:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			if (oneval==0) {
				errornum = ERROR_DIVZERO;
				stack.pushint(0);
			} else {
				double ans = twoval / oneval;
				if (isinf(ans)) {
					errornum = ERROR_INFINITY;
					stack.pushint(0);
				} else {
					stack.pushfloat(ans);
				}
			}
			break;
		}
		
	case OP_INTDIV:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			if (oneval==0) {
				errornum = ERROR_DIVZERO;
				stack.pushint(0);
			} else {
				double intpart;
				modf(twoval /oneval, &intpart);
				if (isinf(intpart)) {
					errornum = ERROR_INFINITY;
					stack.pushint(0);
				} else {
					stack.pushfloat(intpart);
				}
			}
			break;
		}

	case OP_EX:
		{
			op++;
			double oneval = stack.popfloat();
			double twoval = stack.popfloat();
			double ans = pow(twoval, oneval);
			if (isinf(ans)) {
				errornum = ERROR_INFINITY;
				stack.pushint(0);
			} else {
				stack.pushfloat(ans);
			}
			break;
		}
		
	case OP_AND:
		{
			op++;
			int one = stack.popint();
			int two = stack.popint();
			if (one && two) {
				stack.pushint(1);
			} else {
				stack.pushint(0);
			}
		}
		break;

	case OP_OR:
		{
			op++;
			int one = stack.popint();
			int two = stack.popint();
			if (one || two)	{
				stack.pushint(1);
			} else {
				stack.pushint(0);
			}
		}
		break;

	case OP_XOR:
		{
			op++;
			int one = stack.popint();
			int two = stack.popint();
			if (!(one && two) && (one || two)) {
				stack.pushint(1);
			} else {
				stack.pushint(0);
			}
		}
		break;

	case OP_NOT:
		{
			op++;
			int temp = stack.popint();
			if (temp) {
				stack.pushint(0);
			} else {
				stack.pushint(1);
			}
		}
		break;

	case OP_NEGATE:
		{
			op++;
			double n = stack.popfloat();
			stack.pushfloat(n * -1);
		}
		break;

	case OP_EQUAL:
		{
			op++;
			int ans = stack.compareTopTwo()==0;
			stack.pushint(ans);
		}
		break;

	case OP_NEQUAL:
		{
			op++;
			int ans = stack.compareTopTwo()!=0;
			stack.pushint(ans);
		}
		break;

	case OP_GT:
		{
			op++;
			int ans = stack.compareTopTwo()==1;
			stack.pushint(ans);

		}
		break;

	case OP_LTE:
		{
			op++;
			int ans = stack.compareTopTwo()!=1;
			stack.pushint(ans);
		}
		break;

	case OP_LT:
		{
			op++;
			int ans = stack.compareTopTwo()==-1;
			stack.pushint(ans);
		}
		break;

	case OP_GTE:
		{
			op++;
			int ans = stack.compareTopTwo()!=-1;
			stack.pushint(ans);
		}
		break;

	case OP_SOUND:
		{
			op++;
			int duration = stack.popint();
			int frequency = stack.popint();
			int* freqdur;
			freqdur = (int*) malloc(2 * sizeof(int));
			freqdur[0] = frequency;
			freqdur[1] = duration;
			sound.playSounds(1 , freqdur);
			free(freqdur);
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
				free(freqdur);
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
			free(freqdur);
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
			mutex->lock();
 			emit(speakWords(stack.popstring()));
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_SYSTEM:
		{
			op++;
			QString temp = stack.popstring();

            SETTINGS;
			if(settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toBool()) {
				mutex->lock();
                emit(executeSystem(temp));
				waitCond->wait(mutex);
				mutex->unlock();
			} else {
				errornum = ERROR_PERMISSION;
			}
		}
		break;

	case OP_WAVPLAY:
		{
			op++;
			mutex->lock();
			emit(playWAV(stack.popstring()));
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_WAVSTOP:
		{
			op++;
			mutex->lock();
			emit(stopWAV());
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_SETCOLOR:
		{
			op++;
			unsigned int brushval = stack.popfloat();
			unsigned int penval = stack.popfloat();

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
				stack.pushfloat( (unsigned int) QColor(rval,gval,bval,aval).rgba());
			}
		}
		break;
		
	case OP_PIXEL:
		{
			op++;
			int y = stack.popint();
			int x = stack.popint();
			QRgb rgb = graphwin->image->pixel(x,y);
			stack.pushfloat((unsigned int) rgb);
		}
		break;
		
	case OP_GETCOLOR:
		{
			op++;
			stack.pushfloat((unsigned int) drawingpen.color().rgba());
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
			if (h*w > (STRINGMAXLEN -8)/8) {
				errornum = ERROR_STRINGMAXLEN;
				stack.pushint(0);
			} else {
				QString qs;
				QRgb rgb;
				int tw, th;
				qs.append(QString::number(w,16).rightJustified(4,'0'));
				qs.append(QString::number(h,16).rightJustified(4,'0'));
				for(th=0; th<h; th++) {
					for(tw=0; tw<w; tw++) {
						rgb = graphwin->image->pixel(x+tw,y+th);
						qs.append(QString::number(rgb,16).rightJustified(8,'0'));
					}
				}
				stack.pushstring(qs);
			}
		}
		break;
		
	case OP_PUTSLICE:
	case OP_PUTSLICEMASK:
		{
			QRgb mask = 0x00;	// default mask transparent - nothing gets masked
			if (*op == OP_PUTSLICEMASK) mask = stack.popint();
			QString imagedata = stack.popstring();
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

					QPainter ian(graphwin->image);
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

			QPainter ian(graphwin->image);
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

			QPainter ian(graphwin->image);
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
			
			if (variables.type(*i) != T_ARRAY) {
				errornum = ERROR_POLYARRAY;
			} else {
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

					QPainter poly(graphwin->image);
					poly.setPen(drawingpen);
					poly.setBrush(drawingbrush);
					if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
						poly.setCompositionMode(QPainter::CompositionMode_Clear);
					}
					poly.drawPolygon(points, pairs);
					poly.end();
					if (!fastgraphics) waitForGraphics();
					delete points;
				}
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
				
				QPainter poly(graphwin->image);
				poly.setPen(drawingpen);
				poly.setBrush(drawingbrush);
				if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
					poly.setCompositionMode(QPainter::CompositionMode_Clear);
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

			double tx, ty, savetx;		// used in scaling and rotating

			if (variables.type(*i) != T_ARRAY) {
				errornum = ERROR_POLYARRAY;
			} else {
				int pairs = variables.arraysize(*i) / 2;
				if (pairs < 3)
				{
					errornum = ERROR_POLYPOINTS;
				} else {
					if (scale<0) {
						errornum = ERROR_IMAGESCALE;
					} else {
						QPointF *points = new QPointF[pairs];
						for (int j = 0; j < pairs; j++)
						{
							tx = scale * variables.arraygetfloat(*i, j*2);
							ty = scale * variables.arraygetfloat(*i, j*2+1);
							if (rotate!=0) {
								savetx = tx;
								tx = cos(rotate) * tx - sin(rotate) * ty;
								ty = cos(rotate) * ty + sin(rotate) * savetx;
							}
							points[j].setX(tx + x);
							points[j].setY(ty + y);
						}

						QPainter poly(graphwin->image);
						poly.setPen(drawingpen);
						poly.setBrush(drawingbrush);
						if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
							poly.setCompositionMode(QPainter::CompositionMode_Clear);
						}
						poly.drawPolygon(points, pairs);
						poly.end();
						if (!fastgraphics) waitForGraphics();

						delete points;
					}
				}
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

			double rotate=0;			// defaule rotation to 0 radians
			double scale=1;			// default scale to full size (1x)
			
			double tx, ty, savetx;		// used in scaling and rotating
			
			unsigned char opcode = *op;
			op++;
			int *i = (int *) op;
			int llist = *i;
			op += sizeof(int);
			
			// pop the immediate list to uncover the location and scale
			double *list = (double *) calloc(llist, sizeof(double));
			for(int j = llist; j>0 ; j--) {
				list[j-1] = stack.popfloat();
			}
			
			if (opcode==OP_STAMP_SR_LIST) rotate = stack.popfloat();
			if (opcode==OP_STAMP_SR_LIST || opcode==OP_STAMP_S_LIST) scale = stack.popfloat();
			int y = stack.popint();
			int x = stack.popint();
			
			if (scale<0) {
				errornum = ERROR_IMAGESCALE;
			} else {
				int pairs = llist / 2;
				if (pairs < 3)
				{
					errornum = ERROR_POLYPOINTS;
				} else {

					QPointF *points = new QPointF[pairs];
					for (int j = 0; j < pairs; j++)
					{
						tx = scale * list[(j*2)];
						ty = scale * list[(j*2)+1];
						if (rotate!=0) {
							savetx = tx;
							tx = cos(rotate) * tx - sin(rotate) * ty;
							ty = cos(rotate) * ty + sin(rotate) * savetx;
						}
						points[j].setX(tx + x);
						points[j].setY(ty + y);
					}

					QPainter poly(graphwin->image);
					poly.setPen(drawingpen);
					poly.setBrush(drawingbrush);
					if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
						poly.setCompositionMode(QPainter::CompositionMode_Clear);
					}
					poly.drawPolygon(points, pairs);
					poly.end();
				
					if (!fastgraphics) waitForGraphics();
					delete points;
				}
			}
			
			free(list);
		}
		break;


	case OP_CIRCLE:
		{
			op++;
			int rval = stack.popint();
			int yval = stack.popint();
			int xval = stack.popint();

			QPainter ian(graphwin->image);
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
			QString file = stack.popstring();
			
			double rotate = stack.popfloat();
			double scale = stack.popfloat();
			double y = stack.popint();
			double x = stack.popint();
			
			if (scale<0) {
				errornum = ERROR_IMAGESCALE;
			} else {
				QImage i(file);
				if(i.isNull()) {
					errornum = ERROR_IMAGEFILE;
				} else {
					QPainter ian(graphwin->image);
					if (rotate != 0 || scale != 1) {
						QTransform transform = QTransform().translate(0,0).rotateRadians(rotate).scale(scale, scale);
						i = i.transformed(transform);
					}
					if (i.width() != 0 && i.height() != 0) {
						ian.drawImage((int)(x - .5 * i.width()), (int)(y - .5 * i.height()), i);
					}
					ian.end();
					if (!fastgraphics) waitForGraphics();
				}
			}
		}
		break;

	case OP_TEXT:
		{
			op++;
			QString txt = stack.popstring();
			int y0val = stack.popint();
			int x0val = stack.popint();

			QPainter ian(graphwin->image);
			ian.setPen(drawingpen);
			ian.setBrush(drawingbrush);
			if (drawingpen.color()==QColor(0,0,0,0) && drawingbrush.color()==QColor(0,0,0,0) ) {
				ian.setCompositionMode(QPainter::CompositionMode_Clear);
			}
			if(!fontfamily.isEmpty()) {
				ian.setFont(QFont(fontfamily, fontpoint, fontweight));
			}
			ian.drawText(x0val, y0val+(QFontMetrics(ian.font()).ascent()), txt);
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;


	case OP_FONT:
		{
			op++;
			int weight = stack.popint();
			int point = stack.popint();
			QString family = stack.popstring();
			if (point<0) {
				errornum = ERROR_FONTSIZE;
			} else {
				if (weight<0) {
					errornum = ERROR_FONTWEIGHT;
				} else {
					fontpoint = point;
					fontweight = weight;
					fontfamily = family;
				}
			}
		}
		break;

	case OP_CLS:
		{
			op++;
			mutex->lock();
			emit(outputClear());
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_CLG:
		{
			op++;
			
			QPainter ian(graphwin->image);
			ian.setPen(QColor(0,0,0,0));
			ian.setBrush(QColor(0,0,0,0));
			ian.setCompositionMode(QPainter::CompositionMode_Clear);
			ian.drawRect(0, 0, graphwin->image->width(), graphwin->image->height());
			ian.end();

			if (!fastgraphics) waitForGraphics();
		}
		break;

	case OP_PLOT:
		{
			op++;
			int oneval = stack.popint();
			int twoval = stack.popint();

			QPainter ian(graphwin->image);
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
				mutex->lock();
				emit(mainWindowsResize(1, width, height));
				waitCond->wait(mutex);
				mutex->unlock();
			}
		}
		break;

	case OP_GRAPHWIDTH:
		{
			op++;
			stack.pushint((int) graphwin->image->width());
		}
		break;

	case OP_GRAPHHEIGHT:
		{
			op++;
			stack.pushint((int) graphwin->image->height());
		}
		break;

	case OP_REFRESH:
		{
			op++;
			mutex->lock();
			emit(goutputReady());
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_INPUT:
		{
			op++;
			status = R_INPUT;
			mutex->lock();
			emit(getInput());
			waitCond->wait(mutex);
			mutex->unlock();
		}
		break;

	case OP_KEY:
		{
			op++;
			mutex->lock();
			stack.pushint(currentKey);
			currentKey = 0;
			mutex->unlock();
		}
		break;

	case OP_PRINT:
	case OP_PRINTN:
		{
			QString p = stack.popstring();
			if (*op == OP_PRINTN)
			{
				p += "\n";
			}
			mutex->lock();
			emit(outputReady(p));
			waitCond->wait(mutex);
			mutex->unlock();
			op++;
		}
		break;

	case OP_CONCAT:
		{
			op++;
			QString one = stack.popstring();
			QString two = stack.popstring();
			unsigned int l = one.length() + two.length();
			if (l<=STRINGMAXLEN) {
				stack.pushstring(two + one);
			} else {
				errornum = ERROR_STRINGMAXLEN;
				stack.pushint(0);
			}
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
                emit(varAssignment(variables.getrecurse(),QString(symtable[*num]), QString::number(variables.getfloat(*num)), -1, -1));
            }
        }
        break;

     case OP_STRINGASSIGN:
		{
			op++;
			int *num = (int *) op;
			op += sizeof(int);

			QString q = stack.popstring();
			variables.setstring(*num, q);
			
			if(debugMode)
			{
			  emit(varAssignment(variables.getrecurse(),QString(symtable[*num]), variables.getstring(*num), -1, -1));
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
				stack.pushint(timeinfo->tm_year + 1900);
				break;
			case OP_MONTH:
				stack.pushint(timeinfo->tm_mon);
				break;
			case OP_DAY:
				stack.pushint(timeinfo->tm_mday);
				break;
			case OP_HOUR:
				stack.pushint(timeinfo->tm_hour);
				break;
			case OP_MINUTE:
				stack.pushint(timeinfo->tm_min);
				break;
			case OP_SECOND:
				stack.pushint(timeinfo->tm_sec);
				break;
			}
			op++;
		}
		break;

	case OP_MOUSEX:
		{
			op++;
			stack.pushint((int) graphwin->mouseX);
		}
		break;

	case OP_MOUSEY:
		{
			op++;
			stack.pushint((int) graphwin->mouseY);
		}
		break;

	case OP_MOUSEB:
		{
			op++;
			stack.pushint((int) graphwin->mouseB);
		}
		break;

	case OP_CLICKCLEAR:
		{
			op++;
			graphwin->clickX = 0;
			graphwin->clickY = 0;
			graphwin->clickB = 0;
		}
		break;

	case OP_CLICKX:
		{
			op++;
			stack.pushint((int) graphwin->clickX);
		}
		break;

	case OP_CLICKY:
		{
			op++;
			stack.pushint((int) graphwin->clickY);
		}
		break;

	case OP_CLICKB:
		{
			op++;
			stack.pushint((int) graphwin->clickB);
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

			if(debugMode) {
				// remove variables from variable window when we return back
				emit(varAssignment(variables.getrecurse(),QString(""), QString(""), -1, -1));
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
						
						QString file = stack.popstring();
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
							sprites[n].image = new QImage(file);
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
							sprites[n].image = new QImage(graphwin->image->copy(x, y, w, h));
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
							if (s<0) {
								errornum = ERROR_IMAGESCALE;
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
										if (x >= (int) graphwin->image->width()) x = (double) graphwin->image->width();
										if (x < 0) x = 0;
										if (y >= (int) graphwin->image->height()) y = (double) graphwin->image->height();
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
								stack.pushint(spritecollide(n1, n2));
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
						stack.pushint(0);	
					} else {
						if(!sprites[n].image) {
							errornum = ERROR_SPRITENA;
							stack.pushint(0);	
						} else {
							if (opcode==OPX_SPRITEX) stack.pushfloat(sprites[n].x);
							if (opcode==OPX_SPRITEY) stack.pushfloat(sprites[n].y);
							if (opcode==OPX_SPRITEH) stack.pushint(sprites[n].image->height());
							if (opcode==OPX_SPRITEW) stack.pushint(sprites[n].image->width());
							if (opcode==OPX_SPRITEV) stack.pushint(sprites[n].visible?1:0);
							if (opcode==OPX_SPRITER) stack.pushfloat(sprites[n].r);
							if (opcode==OPX_SPRITES) stack.pushfloat(sprites[n].s);
						}
					}
				}
				break;

			
			case OPX_CHANGEDIR:
					{
						op++;
						QString file = stack.popstring();
						if(!QDir::setCurrent(file)) {
							errornum = ERROR_FOLDER;
						}
					}
					break;

			case OPX_CURRENTDIR:
				{
					op++;
					stack.pushstring(QDir::currentPath());
				}
				break;
				
			case OPX_WAVWAIT:
				{
					op++;
					mutex->lock();
					emit(waitWAV());
					waitCond->wait(mutex);
					mutex->unlock();
				}
				break;

			case OPX_DBOPEN:
					{
						op++;
						// open database connection
						QString file = stack.popstring();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							closeDatabase(n);
							int error = sqlite3_open(file.toUtf8().data(), &dbconn[n]);
							if (error) {
								errornum = ERROR_DBOPEN;
							}
						}
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
						QString stmt = stack.popstring();
						int n = stack.popint();
						if (n<0||n>=NUMDBCONN) {
							errornum = ERROR_DBCONNNUMBER;
						} else {
							if(dbconn[n]) {
								int error = sqlite3_exec(dbconn[n], stmt.toUtf8().data(), 0, 0, 0);
								if (error != SQLITE_OK) {
									errornum = ERROR_DBQUERY;
									errormessage = sqlite3_errmsg(dbconn[n]);
								}
							} else {
								errornum = ERROR_DBNOTOPEN;
							}
						}
					}
					break;

			case OPX_DBOPENSET:
					{
						op++;
						// open recordset
						QString stmt = stack.popstring();
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
									int error = sqlite3_prepare_v2(dbconn[n], stmt.toUtf8().data(), -1, &(dbset[n][set]), NULL);
									if (error != SQLITE_OK) {
										errornum = ERROR_DBQUERY;
										errormessage = sqlite3_errmsg(dbconn[n]);
									}
								} else {
									errornum = ERROR_DBNOTOPEN;
								}
							}
						}
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
									stack.pushint((sqlite3_step(dbset[n][set]) == SQLITE_ROW?1:0));
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
						QString colname;
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
												if (strcmp(colname.toUtf8().data(),sqlite3_column_name(dbset[n][set],t))==0) {
													col = t;
												}
											}
										}
										if (col < 0 || col >= sqlite3_column_count(dbset[n][set])) {
											errornum = ERROR_DBCOLNO;
										} else {
											switch(opcode) {
												case OPX_DBINT:
												case OPX_DBINTS:
													stack.pushint(sqlite3_column_int(dbset[n][set], col));
													break;
												case OPX_DBFLOAT:
												case OPX_DBFLOATS:
													stack.pushfloat(sqlite3_column_double(dbset[n][set], col));
													break;
												case OPX_DBNULL:
												case OPX_DBNULLS:
													stack.pushint((char*)sqlite3_column_text(dbset[n][set], col)==NULL);
													break;
												case OPX_DBSTRING:
												case OPX_DBSTRINGS:
													stack.pushstring(QString::fromUtf8((char*)sqlite3_column_text(dbset[n][set], col)));
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
					stack.pushint(lasterrornum);
				}
				break;

			case OPX_LASTERRORLINE:
				{
					op++;
					stack.pushint(lasterrorline);
				}
				break;

			case OPX_LASTERROREXTRA:
				{
					op++;
					stack.pushstring(lasterrormessage);
				}
				break;

			case OPX_LASTERRORMESSAGE:
				{
					op++;
					stack.pushstring(getErrorMessage(lasterrornum));
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
					QString address = stack.popstring();
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

							server = gethostbyname(address.toUtf8().data());
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
						stack.pushint(0);
					} else {
						if (netsockfd[fn] < 0) {
							errornum = ERROR_NETNONE;
							stack.pushint(0);
						} else {
							memset(strarray, 0, MAXSIZE);
							n = recv(netsockfd[fn],strarray,MAXSIZE-1,0);
							if (n < 0) {
								errornum = ERROR_NETREAD;
								errormessage = strerror(errno);
								stack.pushint(0);
							} else {
								stack.pushstring(QString::fromUtf8(strarray));
							}
						}
					}
					free(strarray);
				}
				break;

			case OPX_NETWRITE:
				{
					op++;
					QString data = stack.popstring();
					int fn = stack.popint();
					if (fn<0||fn>=NUMSOCKETS) {
						errornum = ERROR_NETSOCKNUMBER;
					} else {
						if (netsockfd[fn]<0) {
							errornum = ERROR_NETNONE;
						} else {
							int n = send(netsockfd[fn],data.toUtf8().data(),data.length(),0);
							if (n < 0) {
								errornum = ERROR_NETWRITE;
								errormessage = strerror(errno);
							}
						}
					}
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
							stack.pushint(0);
						} else {
							if (n==0L) {
								stack.pushint(0);
							} else {
								stack.pushint(1);
							}
						}
						#else
						struct pollfd p[1];
						p[0].fd = netsockfd[fn];
						p[0].events = POLLIN | POLLPRI;
						if(poll(p, 1, 1)<0) {
							stack.pushint(0);
						} else {
							if (p[0].revents & POLLIN || p[0].revents & POLLPRI) {
								stack.pushint(1);
							} else {
								stack.pushint(0);
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
					stack.pushstring(QString::fromUtf8(inet_ntoa(sAddr.sin_addr)));
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
									stack.pushstring(QString::fromUtf8(buf));
									good = true;
								}
							}
						}
						freeifaddrs(myaddrs);
					}
					if (!good) {
						// on error give local loopback
						stack.pushstring(QString("127.0.0.1"));
					}
					#endif
				}
				break;

			case OPX_KILL:
				{
					op++;
					QString name = stack.popstring();
					if(!QFile::remove(name)) {
						errornum = ERROR_FILEOPEN;
					}
				}
				break;

			case OPX_MD5:
				{
					op++;
					QString stuff = stack.popstring();
					stack.pushstring(MD5(stuff.toUtf8().data()).hexdigest());
				}
				break;

			case OPX_SETSETTING:
				{
					op++;
					QString stuff = stack.popstring();
					QString key = stack.popstring();
					QString app = stack.popstring();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						settings.beginGroup(SETTINGSGROUPUSER);
						settings.beginGroup(app);
						settings.setValue(key, stuff);
						settings.endGroup();
						settings.endGroup();
					} else {
						errornum = ERROR_PERMISSION;
					}
				}
				break;

			case OPX_GETSETTING:
				{
					op++;
					QString key = stack.popstring();
					QString app = stack.popstring();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						settings.beginGroup(SETTINGSGROUPUSER);
						settings.beginGroup(app);
						stack.pushstring(settings.value(key, "").toString());
						settings.endGroup();
						settings.endGroup();
					} else {
						errornum = ERROR_PERMISSION;
						stack.pushint(0);
					}
				}
				break;


			case OPX_PORTOUT:
				{
					op++;
					int data = stack.popint();
					int port = stack.popint();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
						#ifdef WIN32
							#ifdef WIN32PORTABLE
									errornum = ERROR_NOTIMPLEMENTED;
							# else
								if (Out32==NULL) {
									errornum = ERROR_NOTIMPLEMENTED;
								} else {
									Out32(port, data);
								}
							#endif
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
                    SETTINGS;
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
						#ifdef WIN32
							#ifdef WIN32PORTABLE
									errornum = ERROR_NOTIMPLEMENTED;
							# else
								if (Inp32==NULL) {
									errornum = ERROR_NOTIMPLEMENTED;
								} else {
									data = Inp32(port);
								}
							#endif
						#else
							errornum = ERROR_NOTIMPLEMENTED;
						#endif
					} else {
						errornum = ERROR_PERMISSION;
					}
					stack.pushint(data);
				}
				break;

			case OPX_BINARYOR:
				{
					op++;
					unsigned long a = stack.popint();
					unsigned long b = stack.popint();
					stack.pushfloat(a|b);
				}
				break;

			case OPX_BINARYAND:
				{
					op++;
					unsigned long a = stack.popint();
					unsigned long b = stack.popint();
					stack.pushfloat(a&b);
				}
				break;

			case OPX_BINARYNOT:
				{
					op++;
					unsigned long a = stack.popint();
					stack.pushfloat(~a);
				}
				break;

			case OPX_IMGSAVE:
				{
					// Image Save - Save image
					op++;
					QString type = stack.popstring();
         			QString file = stack.popstring();
					QStringList validtypes;
					validtypes << "BMP" << "bmp" << "JPG" << "jpg" << "JPEG" << "jpeg" << "PNG" << "png";
					if (validtypes.indexOf(type)!=-1) {
         					graphwin->image->save(file, type.toUtf8().data());
					} else {
						errornum = ERROR_IMAGESAVETYPE;
					}
				}
				break;

			case OPX_DIR:
				{
					// Get next directory entry - id path send start a new folder else get next file name
					// return "" if we have no names on list - skippimg . and ..
					op++;
					QString folder = stack.popstring();
					if (folder.length()>0) {
						if(directorypointer != NULL) {
							closedir(directorypointer);
							directorypointer = NULL;
						}
						directorypointer = opendir( folder.toUtf8().data() );
					}
					if (directorypointer != NULL) {
						struct dirent *dirp;
						dirp = readdir(directorypointer);
						while(dirp != NULL && dirp->d_name[0]=='.') dirp = readdir(directorypointer);
						if (dirp) {
							stack.pushstring(QString::fromUtf8(dirp->d_name));
						} else {
							stack.pushstring(QString(""));
							closedir(directorypointer);
							directorypointer = NULL;
						}
					} else {
						errornum = ERROR_FOLDER;
						stack.pushint(0);
					}
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
			
					QString qto = stack.popstring();
					QString qfrom = stack.popstring();
					QString qhaystack = stack.popstring();

					if(opcode==OPX_REPLACE || opcode==OPX_REPLACE_C) {
						stack.pushstring(qhaystack.replace(qfrom, qto, casesens));
					} else {
						stack.pushstring(qhaystack.replace(QRegExp(qfrom), qto));
					}
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
			
					QString qneedle = stack.popstring();
					QString qhaystack = stack.popstring();
			
					if(opcode==OPX_COUNT || opcode==OPX_COUNT_C) {
						stack.pushint((int) (qhaystack.count(qneedle, casesens)));
					} else {
						stack.pushint((int) (qhaystack.count(QRegExp(qneedle))));
					}
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
					stack.pushint(os);
				}
				break;

			case OPX_MSEC:
				{
					// Return number of milliseconds the BASIC256 program has been running
					op++;
					stack.pushint((int) (runtimer.elapsed()));
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
					QString txt = stack.popstring();
					int w = 0;
					QPainter ian(graphwin->image);
					if(!fontfamily.isEmpty()) {
						ian.setFont(QFont(fontfamily, fontpoint, fontweight));
					}
					w = QFontMetrics(ian.font()).width(txt);
					ian.end();
					stack.pushint((int) (w));
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
						stack.pushint(0);
					} else {
						stack.pushint(f);
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
						stack.pushint(0);
					} else {
						stack.pushint(f);
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
						stack.pushint(0);
					} else {
						stack.pushint(f);
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
						stack.pushint(0);
					} else {
						for (int t=0;(t<NUMDBSET)&&(f==-1);t++) {
							if (!dbset[n][t]) f = t;
						}
						if (f==-1) {
							errornum = ERROR_FREEDBSET;
							stack.pushint(0);
						} else {
							stack.pushint(f);
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

					QPainter ian(graphwin->image);
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
					stack.pushfloat((double) (drawingpen.widthF()));
				}
				break;

			case OPX_GETBRUSHCOLOR:
				{
					op++;
					stack.pushfloat((unsigned int) drawingbrush.color().rgba());
				}
				break;
		
			case OPX_RUNTIMEWARNING:
				{
					// display a runtime warning message
					// added in yacc/bison for deprecated and other warnings
					op++;
					int w = stack.popint();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWWARNINGS, SETTINGSALLOWWARNINGSDEFAULT).toBool()) {
						emit(outputReady(QObject::tr("WARNING on line ") + QString::number(currentLine) + ": " + getWarningMessage(w) + "\n"));
					}
				}
				break;

			case OPX_ALERT:
				{
					op++;
					QString temp = stack.popstring();
					mutex->lock();
					emit(dialogAlert(temp));
					waitCond->wait(mutex);
					mutex->unlock();
					waitForGraphics();
				}
				break;

			case OPX_CONFIRM:
				{
					op++;
					int dflt = stack.popint();
					QString temp = stack.popstring();
					mutex->lock();
					emit(dialogConfirm(temp,dflt));
					waitCond->wait(mutex);
					mutex->unlock();
					stack.pushint(returnInt);
					waitForGraphics();
				}
				break;

			case OPX_PROMPT:
				{
					op++;
					QString dflt = stack.popstring();
					QString msg = stack.popstring();
					mutex->lock();
					emit(dialogPrompt(msg,dflt));
					waitCond->wait(mutex);
					mutex->unlock();
					stack.pushstring(returnString);
					waitForGraphics();
				}
				break;

			case OPX_FROMRADIX:
				{
					op++;
					bool ok;
					unsigned long dec;
					int base = stack.popint();
					QString n = stack.popstring();
					if (base>=2 && base <=36) {
						dec = n.toULong(&ok, base);
						if (ok) {
							stack.pushfloat(dec);
						} else {
							errornum = ERROR_RADIXSTRING;
							stack.pushfloat(0);
						}
					} else {
						errornum = ERROR_RADIX;
						stack.pushfloat(0);
					}
				}
				break;

			case OPX_TORADIX:
				{
					op++;
					int base = stack.popint();
					unsigned long n = stack.popfloat();
					if (base>=2 && base <=36) {
						QString out;
						out.setNum(n, base);
						stack.pushstring(out);
					} else {
						errornum = ERROR_RADIX;
						stack.pushfloat(0);
					}
				}
				break;


			case OPX_DEBUGINFO:
				{
					// get info about BASIC256 runtime and return as a string
					// put totally undocumented stuff HERE
					// NOT FOR HUMANS TO USE
					op++;
					int what = stack.popint();
					switch (what) {
						case 1:
							// stack height
							stack.pushint(stack.height());
							break;
						case 2:
							// type of top stack element
							stack.pushint(stack.peekType());
							break;
						case 3:
							// number of symbols - display them to output area
							{
								for(int i=0;i<numsyms;i++) {
									mutex->lock();
									emit(outputReady(QString("SYM %1 LOC %2\n").arg(symtable[i],-32).arg(labeltable[i],8,16,QChar('0'))));
									waitCond->wait(mutex);
									mutex->unlock();
								}
								stack.pushint(numsyms);
							}
							break;
						case 4:
							// dump the program object code
							{
							unsigned char *o = byteCode;
							while (o <= byteCode + byteOffset) {
								mutex->lock();
								unsigned int offset = o-byteCode;
								unsigned char currentop = (unsigned char) *o;
								o += sizeof(unsigned char);
								if (optype(currentop) == OPTYPE_NONE)	{
									emit(outputReady(QString("%1 %2\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20)));
								} else if (optype(currentop) == OPTYPE_EXTENDED) {	
									unsigned char currentopx = (unsigned char) *o;
									emit(outputReady(QString("%1 %2\n").arg(offset,8,16,QChar('0')).arg(opxname(currentopx),-20)));
									o += sizeof(unsigned char);
								} else if (optype(currentop) == OPTYPE_INT) {
									//op has one Int arg
									emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o)));
									o += sizeof(int);
								} else if (optype(currentop) == OPTYPE_VARIABLE) {
									//op has one Int arg
									emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg(symtable[(int) *o])));
									o += sizeof(int);
								} else if (optype(currentop) == OPTYPE_LABEL) {
									//op has one Int arg
									emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o,8,16,QChar('0'))));
									o += sizeof(int);
								} else if (optype(currentop) == OPTYPE_INTINT) {
									// op has 2 Int arg
									emit(outputReady(QString("%1 %2 %3 %4\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o).arg((int) *(o+sizeof(int)))));
									o += 2 * sizeof(int);
								} else if (optype(currentop) == OPTYPE_FLOAT) {
									// op has a single double arg
									emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((double) *o)));
									o += sizeof(double);
								} else if (optype(currentop) == OPTYPE_STRING) {
									// op has a single null terminated String arg
									emit(outputReady(QString("%1 %2 \"%3\"\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((char*) o)));
									int len = strlen((char*) o) + 1;
									o += len;
								}
								waitCond->wait(mutex);
								mutex->unlock();
							}
							}
							stack.pushint(0);
							break;

						default:
							stack.pushstring("");
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
