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
#include <math.h>
#include <string>
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
// unix, mac, android
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <arpa/inet.h>
#include <net/if.h>
#ifndef ANDROID
#include <ifaddrs.h>
#endif
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
#include <QCoreApplication>


#include <QtWidgets/QMessageBox>



#include "LEX/basicParse.tab.h"
#include "WordCodes.h"
#include "CompileErrors.h"
#include "Interpreter.h"
#include "md5.h"
#include "Settings.h"
#include "Sound.h"
#include "Constants.h"


extern QMutex *mymutex;
extern QMutex *mydebugmutex;
extern QWaitCondition *waitCond;
extern QWaitCondition *waitDebugCond;

extern BasicGraph * graphwin;

extern int lastKey;
extern std::list<int> pressedKeys;

extern "C" {
	extern int basicParse(char *);
	extern int linenumber;			// linenumber being LEXd
	extern int column;				// column on line being LEXd
	extern char* lexingfilename;	// current included file name being LEXd

	extern int numparsewarnings;
	extern int newWordCode();
	extern void freeBasicParse();
	extern int bytesToFullWords(int size);
	extern int *wordCode;
	extern unsigned int wordOffset;
	extern unsigned int maxwordoffset;

	extern char *symtable[];		// table of variables and labels (strings)
    extern int symtableaddress[];	// associated label address
    extern int symtableaddresstype[];	// associated address type
    extern int numsyms;				// number of symbols

	// arrays to return warnings from compiler
	// defined in basicParse.y
	extern int parsewarningtable[];
	extern int parsewarningtablelinenumber[];
	extern int parsewarningtablecolumn[];
	extern char *parsewarningtablelexingfilename[];
}

Interpreter::Interpreter(QLocale *applocale) {
	fastgraphics = false;
	directorypointer=NULL;
	status = R_STOPPED;
	printing = false;
    sleeper = new Sleeper();
    error = new Error();
    locale = applocale;
	mediaplayer = NULL;
    downloader = NULL;
    sound = NULL;

#ifdef WIN32
	// WINDOWS
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
		emit(outputError(tr("ERROR - Unable to find inpout32.dll - direct port I/O disabled.\n")));
	} else {
		Inp32 = (InpOut32InpType) GetProcAddress(inpout32dll, "Inp32");
		if (Inp32==NULL) {
			emit(outputError(tr("ERROR - Unable to find Inp32 in inpout32.dll - direct port I/O disabled.\n")));
		}
		Out32 = (InpOut32OutType) GetProcAddress(inpout32dll, "Out32");
		if (Inp32==NULL) {
			emit(outputError(tr("ERROR - Unable to find Out32 in inpout32.dll - direct port I/O disabled.\n")));
		}
	}
#endif
}

Interpreter::~Interpreter() {
	// on a windows box stop winsock
#ifdef WIN32
	WSACleanup();
#endif
    delete sound;
    delete downloader;
    delete sleeper;
    delete error;
}


int Interpreter::optype(int op) {
	// use constantants found in WordCodes.h
	return (OPTYPE_MASK & op) ;
}

QString Interpreter::opname(int op) {
	// used to convert opcode number in debuginfo to opcode name
	if (op==OP_END) return QString("OP_END");
	else if (op==OP_NOP) return QString("OP_NOP");
	else if (op==OP_RETURN) return QString("OP_RETURN");
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
	else if (op==OP_CONCATENATE) return QString("OP_CONCATNATE");
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
	else if (op==OP_LENGTH) return QString("OP_LENGTH");
	else if (op==OP_MID) return QString("OP_MID");
	else if (op==OP_MIDX) return QString("OP_MIDX");
	else if (op==OP_INSTR) return QString("OP_INSTR");
	else if (op==OP_INSTRX) return QString("OP_INSTRX");
	else if (op==OP_OPEN) return QString("OP_OPEN");
	else if (op==OP_READ) return QString("OP_READ");
	else if (op==OP_WRITE) return QString("OP_WRITE");
	else if (op==OP_CLOSE) return QString("OP_CLOSE");
	else if (op==OP_RESET) return QString("OP_RESET");
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
	else if (op==OP_WAVPAUSE) return QString("OP_WAVPAUSE");
	else if (op==OP_WAVPLAY) return QString("OP_WAVPLAY");
	else if (op==OP_WAVSTOP) return QString("OP_WAVSTOP");
	else if (op==OP_SEED) return QString("OP_SEED");
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
	else if (op==OP_ASSIGN) return QString("OP_ASSIGN");
	else if (op==OP_ARRAYASSIGN) return QString("OP_ARRAYASSIGN");
	else if (op==OP_PUSHVAR) return QString("OP_PUSHVAR");
	else if (op==OP_PUSHINT) return QString("OP_PUSHINT");
	else if (op==OP_FOR) return QString("OP_FOR");
	else if (op==OP_NEXT) return QString("OP_NEXT");
	else if (op==OP_CURRLINE) return QString("OP_CURRLINE");
	else if (op==OP_DIM) return QString("OP_DIM");
	else if (op==OP_ONERRORGOSUB) return QString("OP_ONERRORGOSUB");
	else if (op==OP_ONERRORCATCH) return QString("OP_ONERRORCATCH");
	else if (op==OP_EXPLODE) return QString("OP_EXPLODE");
	else if (op==OP_EXPLODEX) return QString("OP_EXPLODEX");
	else if (op==OP_IMPLODE_LIST) return QString("OP_IMPLODE_LIST");
	else if (op==OP_GLOBAL) return QString("OP_GLOBAL");
	else if (op==OP_STAMP_LIST) return QString("OP_STAMP_LIST");
	else if (op==OP_STAMP_S_LIST) return QString("OP_STAMP_S_LIST");
	else if (op==OP_STAMP_SR_LIST) return QString("OP_STAMP_SR_LIST");
	else if (op==OP_POLY_LIST) return QString("OP_POLY_LIST");
	else if (op==OP_WRITELINE) return QString("OP_WRITELINE");
	else if (op==OP_SOUND_LIST) return QString("OP_SOUND_LIST");
	else if (op==OP_DEREF) return QString("OP_DEREF");
	else if (op==OP_REDIM) return QString("OP_REDIM");
	else if (op==OP_ALEN) return QString("OP_ALEN");
	else if (op==OP_ALENROWS) return QString("OP_ALENROWS");
	else if (op==OP_ALENCOLS) return QString("OP_ALENCOLS");
	else if (op==OP_PUSHVARREF) return QString("OP_PUSHVARREF");
	else if (op==OP_VARREFASSIGN) return QString("OP_VARREFASSIGN");
	else if (op==OP_ARRAYLISTASSIGN) return QString("OP_ARRAYLISTASSIGN");
	else if (op==OP_ARRAY2STACK) return QString("OP_ARRAY2STACK");
	else if (op==OP_ARRAYFILL) return QString("OP_ARRAYFILL");
	else if (op==OP_PUSHFLOAT) return QString("OP_PUSHFLOAT");
	else if (op==OP_PUSHSTRING) return QString("OP_PUSHSTRING");
	else if (op==OP_INCLUDEFILE) return QString("OP_INCLUDEFILE");
	else if (op==OP_SPRITEPOLY_LIST) return QString("OP_SPRITEPOLY");
	else if (op==OP_SPRITEDIM) return QString("OP_SPRITEDIM");
	else if (op==OP_SPRITELOAD) return QString("OP_SPRITELOAD");
	else if (op==OP_SPRITESLICE) return QString("OP_SPRITESLICE");
	else if (op==OP_SPRITEMOVE) return QString("OP_SPRITEMOVE");
	else if (op==OP_SPRITEHIDE) return QString("OP_SPRITEHIDE");
	else if (op==OP_SPRITESHOW) return QString("OP_SPRITESHOW");
	else if (op==OP_SPRITECOLLIDE) return QString("OP_SPRITECOLLIDE");
	else if (op==OP_SPRITEPLACE) return QString("OP_SPRITEPLACE");
	else if (op==OP_SPRITEX) return QString("OP_SPRITEX");
	else if (op==OP_SPRITEY) return QString("OP_SPRITEY");
	else if (op==OP_SPRITEH) return QString("OP_SPRITEH");
	else if (op==OP_SPRITEW) return QString("OP_SPRITEW");
	else if (op==OP_SPRITEV) return QString("OP_SPRITEV");
	else if (op==OP_CHANGEDIR) return QString("OP_CHANGEDIR");
	else if (op==OP_CURRENTDIR) return QString("OP_CURRENTDIR");
	else if (op==OP_DBOPEN) return QString("OP_DBOPEN");
	else if (op==OP_DBCLOSE) return QString("OP_DBCLOSE");
	else if (op==OP_DBEXECUTE) return QString("OP_DBEXECUTE");
	else if (op==OP_DBOPENSET) return QString("OP_DBOPENSET");
	else if (op==OP_DBCLOSESET) return QString("OP_DBCLOSESET");
	else if (op==OP_DBROW) return QString("OP_DBROW");
	else if (op==OP_DBINT) return QString("OP_DBINT");
	else if (op==OP_DBFLOAT) return QString("OP_DBFLOAT");
	else if (op==OP_DBSTRING) return QString("OP_DBSTRING");
	else if (op==OP_LASTERROR) return QString("OP_LASTERROR");
	else if (op==OP_LASTERRORLINE) return QString("OP_LASTERRORLINE");
	else if (op==OP_LASTERRORMESSAGE) return QString("OP_LASTERRORMESSAGE");
	else if (op==OP_LASTERROREXTRA) return QString("OP_LASTERROREXTRA");
	else if (op==OP_OFFERROR) return QString("OP_OFFERROR");
	else if (op==OP_NETLISTEN) return QString("OP_NETLISTEN");
	else if (op==OP_NETCONNECT) return QString("OP_NETCONNECT");
	else if (op==OP_NETREAD) return QString("OP_NETREAD");
	else if (op==OP_NETWRITE) return QString("OP_NETWRITE");
	else if (op==OP_NETCLOSE) return QString("OP_NETCLOSE");
	else if (op==OP_NETDATA) return QString("OP_NETDATA");
	else if (op==OP_NETADDRESS) return QString("OP_NETADDRESS");
	else if (op==OP_KILL) return QString("OP_KILL");
	else if (op==OP_MD5) return QString("OP_MD5");
	else if (op==OP_SETSETTING) return QString("OP_SETSETTING");
	else if (op==OP_GETSETTING) return QString("OP_GETSETTING");
	else if (op==OP_PORTIN) return QString("OP_PORTIN");
	else if (op==OP_PORTOUT) return QString("OP_PORTOUT");
	else if (op==OP_BINARYOR) return QString("OP_BINARYOR");
	else if (op==OP_BINARYAND) return QString("OP_BINARYAND");
	else if (op==OP_BINARYNOT) return QString("OP_BINARYNOT");
	else if (op==OP_IMGSAVE) return QString("OP_IMGSAVE");
	else if (op==OP_DIR) return QString("OP_DIR");
	else if (op==OP_REPLACE) return QString("OP_REPLACE");
	else if (op==OP_REPLACEX) return QString("OP_REPLACEX");
	else if (op==OP_COUNT) return QString("OP_COUNT");
	else if (op==OP_COUNTX) return QString("OP_COUNTX");
	else if (op==OP_OSTYPE) return QString("OP_OSTYPE");
	else if (op==OP_MSEC) return QString("OP_MSEC");
	else if (op==OP_EDITVISIBLE) return QString("OP_EDITVISIBLE");
	else if (op==OP_GRAPHVISIBLE) return QString("OP_GRAPHVISIBLE");
	else if (op==OP_OUTPUTVISIBLE) return QString("OP_OUTPUTVISIBLE");
	else if (op==OP_TEXTHEIGHT) return QString("OP_TEXTHEIGHT");
	else if (op==OP_TEXTWIDTH) return QString("OP_TEXTWIDTH");
	else if (op==OP_SPRITER) return QString("OP_SPRITER");
	else if (op==OP_SPRITES) return QString("OP_SPRITES");
	else if (op==OP_SPRITEO) return QString("OP_SPRITEO");
	else if (op==OP_FREEFILE) return QString("OP_FREEFILE");
	else if (op==OP_FREENET) return QString("OP_FREENET");
	else if (op==OP_FREEDB) return QString("OP_FREEDB");
	else if (op==OP_FREEDBSET) return QString("OP_FREEDBSET");
	else if (op==OP_DBNULL) return QString("OP_DBNULL");
	else if (op==OP_DBNULLS) return QString("OP_DBNULLS");
	else if (op==OP_ARC) return QString("OP_ARC");
	else if (op==OP_CHORD) return QString("OP_CHORD");
	else if (op==OP_PIE) return QString("OP_PIE");
	else if (op==OP_PENWIDTH) return QString("OP_PENWIDTH");
	else if (op==OP_GETPENWIDTH) return QString("OP_GETPENWIDTH");
	else if (op==OP_GETBRUSHCOLOR) return QString("OP_GETBRUSHCOLOR");
	else if (op==OP_ALERT) return QString("OP_ALERT");
	else if (op==OP_CONFIRM) return QString("OP_CONFIRM");
	else if (op==OP_PROMPT) return QString("OP_PROMPT");
	else if (op==OP_FROMRADIX) return QString("OP_FROMRADIX");
	else if (op==OP_TORADIX) return QString("OP_TORADIX");
	else if (op==OP_PRINTERPAGE) return QString("OP_PRINTERPAGE");
	else if (op==OP_PRINTEROFF) return QString("OP_PRINTERON");
	else if (op==OP_PRINTERON) return QString("OP_PRINTEROFF");
	else if (op==OP_DEBUGINFO) return QString("OP_DEBUGINFO");
	else if (op==OP_WAVLENGTH) return QString("OP_WAVLENGTH");
	else if (op==OP_WAVPOS) return QString("OP_WAVPOS");
	else if (op==OP_WAVSEEK) return QString("OP_WAVSEEK");
	else if (op==OP_WAVSTATE) return QString("OP_WAVSTATE");
	else if (op==OP_REGEXMINIMAL) return QString("OP_REGEXMINIMAL");
	else if (op==OP_OPENSERIAL) return QString("OP_OPENSERIAL");
	else if (op==OP_TYPEOF) return QString("OP_TYPEOF");
	else if (op==OP_UNASSIGN) return QString("OP_UNASSIGN");
	else if (op==OP_UNASSIGNA) return QString("OP_UNASSIGNA");
	else if (op==OP_ISNUMERIC) return QString("OP_ISNUMERIC");
	else if (op==OP_LTRIM) return QString("OP_LTRIM");
	else if (op==OP_RTRIM) return QString("OP_RTRIM");
	else if (op==OP_TRIM) return QString("OP_TRIM");
	else if (op==OP_EXITFOR) return QString("OP_EXITFOR");
	else if (op==OP_VARIABLEWATCH) return QString("OP_VARIABLEWATCH");
	else if (op==OP_PUSHLABEL) return QString("OP_PUSHLABEL");
	else if (op==OP_SERIALIZE) return QString("OP_SERIALIZE");
	else if (op==OP_UNSERIALIZE) return QString("OP_UNSERIALIZE");
    else if (op==OP_CALLFUNCTION) return QString("OP_CALLFUNCTION");
    else if (op==OP_CALLSUBROUTINE) return QString("OP_CALLSUBROUTINE");

	else return QString("OP_UNKNOWN");
}


void Interpreter::printError() {
	QString msg;
	if (error->isFatal()) {
		msg = tr("ERROR");
	} else {
		msg = tr("WARNING");
	}
	if (currentIncludeFile!="") {
		msg += tr(" in included file ") + currentIncludeFile;
	}
	msg += tr(" on line ") + QString::number(error->line) + tr(": ") + error->getErrorMessage(symtable);
	if (error->extra!="") msg+= " " + error->extra;
	msg += ".\n";
	emit(outputError(msg));
}


int Interpreter::netSockClose(int fd) {
	// tidy up a network socket and return NULL to assign to the
	// fd variable to mark as closed as closed
	// call  f = netSockClose(f);
	if(fd>=0) {
#ifdef WIN32
		shutdown(fd,2);
		closesocket(fd);
#else
		shutdown(fd,2);
		close(fd);
#endif
	}
	return(-1);
}


void Interpreter::netSockCloseAll() {
	// close network connections
	listensockfd = netSockClose(listensockfd);
	for (int t=0; t<NUMSOCKETS; t++) {
		netsockfd[t] = netSockClose(netsockfd[t]);
	}
}

void
Interpreter::setInputReady() {
	status = R_INPUTREADY;
}

bool
Interpreter::isAwaitingInput() {
	if (status == R_INPUT) {
		return true;
	}
	return false;
}

bool
Interpreter::isRunning() {
	if (status != R_STOPPED) {
		return true;
	}
	return false;
}


bool
Interpreter::isStopped() {
	if (status == R_STOPPED) {
		return true;
	}
	return false;
}

void Interpreter::watchvariable(bool doit, int i) {
	// send an event to the variable watch window to display a simple variable's value
	// wait for the event to complete by watching the mutex
    if (doit) {
        mymutex->lock();
        emit(varWinAssign(&variables, i));
        waitCond->wait(mymutex);
        mymutex->unlock();
    }
}
void Interpreter::watchvariable(bool doit, int i, int x, int y) {
	// send an event to the variable watch window to display aan array element's value
	// wait for the event to complete by watching the mutex
    if (doit) {
        mymutex->lock();
        emit(varWinAssign(&variables, i, x ,y));
        waitCond->wait(mymutex);
        mymutex->unlock();
    }
}
void Interpreter::watchdim(bool doit, int i, int x, int y) {
	// send an event to the variable watch window to display an array dim or redim
	// wait for the event to complete by watching the mutex
    if (doit) {
        mymutex->lock();
        emit(varWinDimArray(&variables ,i , x, y));
        waitCond->wait(mymutex);
        mymutex->unlock();
    }
}
void Interpreter::watchdecurse(bool doit) {
	// send an event to the variable watch window to remove a function's variables
	// wait for the event to complete by watching the mutex
    if (doit) {
        mymutex->lock();
        emit(varWinDropLevel(variables->getrecurse()));
        waitCond->wait(mymutex);
        mymutex->unlock();
    }
}

int
Interpreter::compileProgram(char *code) {
	if (newWordCode() < 0) {
		return -1;
	}

	int result = basicParse(code);
	//
	// display warnings from compile and free the lexing file name string
	for(int i=0; i<numparsewarnings; i++) {
		QString msg = tr("COMPILE WARNING");
		if (strlen(parsewarningtablelexingfilename[i])!=0) {
			msg += tr(" in included file ") + QString(parsewarningtablelexingfilename[i]);
		} else {
			emit(goToLine(parsewarningtablelinenumber[i]));
		}
		msg += tr(" on line ") + QString::number(parsewarningtablelinenumber[i]) + tr(": ");
		switch(parsewarningtable[i]) {
			case COMPWARNING_MAXIMUMWARNINGS:
				msg += tr("The maximum number of compiler warnings have been displayed");
				break;
			case COMPWARNING_DEPRECATED_FORM:
				msg += tr("Statement format has been deprecated. It is recommended that you reauthor");
				break;

			default:
				msg += tr("Unknown compiler warning #") + QString::number(parsewarningtable[i]);
		}
		msg += tr(".\n");
		emit(outputError(msg));
		//
	}
	//
	// now display fatal error if there is one
	if (result != COMPERR_NONE)	{
		QString msg = tr("COMPILE ERROR");
		if (strlen(lexingfilename)!=0) {
			msg += tr(" in included file ") + QString(lexingfilename);
		} else {
			emit(goToLine(linenumber));
		}
		msg += tr(" on line ") + QString::number(linenumber) + tr(": ");
		switch(result) {
			case COMPERR_FUNCTIONGOTO:
				msg += tr("You may not define a label or use a GOTO or GOSUB statement in a FUNCTION/SUBROUTINE declaration");
				break;
			case COMPERR_GLOBALNOTHERE:
				msg += tr("You may not define GLOBAL variable(s) inside an IF, loop, TRY, CATCH, or FUNCTION/SUBROUTINE");
				break;
			case COMPERR_FUNCTIONNOTHERE:
				msg += tr("You may not define a FUNCTION/SUBROUTINE inside an IF, loop, TRY, CATCH, or other FUNCTION/SUBROUTINE");
                break;
			case COMPERR_ENDFUNCTION:
                msg += tr("END FUNCTION without matching FUNCTION");
				break;
            case COMPERR_ENDSUBROUTINE:
                msg += tr("END SUBROUTINE without matching SUBROUTINE");
                break;
            case COMPERR_FUNCTIONNOEND:
                msg += tr("FUNCTION without matching END FUNCTION statement");
                break;
            case COMPERR_SUBROUTINENOEND:
                msg += tr("SUBROUTINE without matching END SUBROUTINE statement");
                break;
            case COMPERR_FORNOEND:
				msg += tr("FOR without matching NEXT statement");
				break;
			case COMPERR_WHILENOEND:
				msg += tr("WHILE without matching END WHILE statement");
				break;
			case COMPERR_DONOEND:
				msg += tr("DO without matching UNTIL statement");
				break;
			case COMPERR_ELSENOEND:
				msg += tr("ELSE without matching END IF or END CASE statement");
				break;
			case COMPERR_IFNOEND:
				msg += tr("IF without matching END IF or ELSE statement");
				break;
			case COMPERR_UNTIL:
				msg += tr("UNTIL without matching DO");
				break;
			case COMPERR_ENDWHILE:
				msg += tr("END WHILE without matching WHILE");
				break;
			case COMPERR_ELSE:
				msg += tr("ELSE without matching IF");
				break;
			case COMPERR_ENDIF:
				msg += tr("END IF without matching IF");
				break;
			case COMPERR_NEXT:
				msg += tr("NEXT without matching FOR");
				break;
			case COMPERR_RETURNVALUE:
				msg += tr("RETURN with a value is only valid inside a FUNCTION");
				break;
			case COMPERR_CONTINUEDO:
				msg += tr("CONTINUE DO without matching DO");
				break;
			case COMPERR_CONTINUEFOR:
				msg += tr("CONTINUE DO without matching DO");
				break;
			case COMPERR_CONTINUEWHILE:
				msg += tr("CONTINUE WHILE without matching WHILE");
				break;
			case COMPERR_EXITDO:
				msg += tr("EXIT DO without matching DO");
				break;
			case COMPERR_EXITFOR:
				msg += tr("EXIT FOR without matching FOR");
				break;
			case COMPERR_EXITWHILE:
				msg += tr("EXIT WHILE without matching WHILE");
				break;
			case COMPERR_INCLUDEFILE:
				msg += tr("Unable to open INCLUDE file");
				break;
			case COMPERR_INCLUDEDEPTH:
				msg += tr("Maximum depth of INCLUDE files");
				break;
			case COMPERR_TRYNOEND:
				msg += tr("TRY without matching CATCH statement");
				break;
			case COMPERR_CATCH:
				msg += tr("CATCH whthout matching TRY statement");
				break;
			case COMPERR_CATCHNOEND:
				msg += tr("CATCH whthout matching ENDTRY statement");
				break;
			case COMPERR_ENDTRY:
				msg += tr("ENDTRY whthout matching CATCH statement");
				break;
			case COMPERR_NOTINTRYCATCH:
				msg += tr("You may not define an ONERROR or an OFFERROR in a TRY/CACTCH declaration");
				break;
			case COMPERR_ENDBEGINCASE:
				msg += tr("CASE whthout matching BEGIN CASE statement");
				break;
			case COMPERR_ENDENDCASEBEGIN:
				msg += tr("END CASE without matching BEGIN CASE statement");
				break;
			case COMPERR_ENDENDCASE:
				msg += tr("END CASE whthout matching CASE statement");
				break;
			case COMPERR_BEGINCASENOEND:
				msg += tr("BEGIN CASE whthout matching END CASE statement");
				break;
			case COMPERR_CASENOEND:
				msg += tr("CASE whthout next CASE or matching END CASE statement");
				break;
			case COMPERR_LABELREDEFINED:
                msg += tr("Labels, functions and subroutines must have a unique name");
				break;
			case COMPERR_NEXTWRONGFOR:
				msg += tr("Variable in NEXT does not match FOR");
				break;

			default:
				if(column==0) {
					msg += tr("Syntax error around beginning line");
				} else {
					msg += tr("Syntax error around character ") + QString::number(column);
				}
		}
		msg += tr(".\n");
		emit(outputError(msg));

		freeBasicParse();

		status = R_STOPPED;
		return -1;
	}

	currentLine = 1;
	return 0;
}

void
Interpreter::initialize() {
	error->loadSettings();
	op = wordCode;
	callstack = NULL;
	onerrorstack = NULL;
	forstack = NULL;
	fastgraphics = false;
	drawingpen = QPen(Qt::black);
	drawingbrush = QBrush(Qt::black, Qt::SolidPattern);
    CompositionModeClear = false;
    PenColorIsClear = false;
    status = R_RUNNING;
    //initialize random
    double_random_max = (double) RAND_MAX * (double) RAND_MAX + (double) RAND_MAX + 1.0;
	currentLine = 1;
    currentIncludeFile = QString("");
    emit(mainWindowsResize(1, 300, 300));
	fontfamily = QString("");
	fontpoint = 0;
	fontweight = 0;
	nsprites = 0;
	printing = false;
	regexMinimal = false;
	lastKey = 0;
	pressedKeys.clear();
	runtimer.start();
	// clickclear mouse status
	graphwin->clickX = 0;
	graphwin->clickY = 0;
	graphwin->clickB = 0;
	
	// create the convert and comparer object
    convert = new Convert(error, locale);
	
	// now build the new stack object
	stack = new Stack(error, convert, locale);

	// now create the variable storage
    variables = new Variables(numsyms, error);

	// initialize the sockets to nothing
	listensockfd = -1;
	for (int t=0; t<NUMSOCKETS; t++) netsockfd[t]=-1;
	
	// initialize pointers used for database recordsets (querries)
	for (int t=0; t<NUMDBCONN; t++) {
		for (int u=0; u<NUMDBSET; u++) {
			dbSet[t][u] = NULL;
		}
	}

	// initialize files to NULL (closed)
	filehandle = (QIODevice**) malloc(NUMFILES * sizeof(QIODevice*));
	filehandletype = (int*) malloc(NUMFILES * sizeof(int));
	for (int t=0; t<NUMFILES; t++) {
		filehandle[t] = NULL;
		filehandletype[t] = 0;
	}

	// initialize databse connections
	// by closing any that were previously open
	for (int t=0; t<NUMDBCONN; t++) {
		closeDatabase(t);
	}
}


void
Interpreter::cleanup() {
	// cleanup that MUST happen for run to early terminate is in runHalted
	// called by run() once the run is terminated
	//
	// Clean up run time objects

    if(mediaplayer!=NULL){
        mediaplayer->stop(); //stop asynchronous sounds if program is ended normally
        delete (mediaplayer);
        mediaplayer = NULL;
    }
    delete (downloader);
    downloader = NULL;
    delete (sound);
    sound = NULL;
    delete(stack);
    delete(variables);
    variables=NULL;
    delete(convert);
    // Clean up sprites
    clearsprites();
	// Clean up, for frames, etc.
    freeBasicParse();
	// close open files (set to NULL if closed)
	for (int t=0; t<NUMFILES; t++) {
		if (filehandle[t]) {
			filehandle[t]->close();
			filehandle[t] = NULL;
			filehandletype[t] = 0;
		}
    }
    free(filehandle);
    free(filehandletype);

	// close open database connections and record sets
	for (int t=0; t<NUMDBCONN; t++) {
		closeDatabase(t);
	}
	
	// close the currently open folder
	if(directorypointer != NULL) {
		closedir(directorypointer);
		directorypointer = NULL;
	}
	
	// close a print document if it is open
	if (printing) {
		printing = false;
		printdocumentpainter->end();
		delete printdocumentpainter;
	}   
	
	// close network connections
	netSockCloseAll(); 

	// remove any queued errors
	error->deq();

    //delete stack used by function calls, subroutine calls, and gosubs for return location
    frame *temp_callstack;
    while (callstack!=NULL) {
        temp_callstack = callstack;
        callstack = temp_callstack->next;
        delete(temp_callstack);
        }

    //delete stack used to track nested on-error and try/catch definitions
    onerrorframe *temp_onerrorstack;
    while (onerrorstack!=NULL) {
        temp_onerrorstack = onerrorstack;
        onerrorstack = temp_onerrorstack->next;
        delete(temp_onerrorstack);
        }

    //delete stack used by nested for statements
    forframe *temp_forstack;
    while (forstack!=NULL) {
        temp_forstack = forstack;
        forstack = temp_forstack->next;
        delete(temp_forstack);
    }
}

void Interpreter::closeDatabase(int t) {
	// cleanup database and all of its sets
	QString dbconnection = "DBCONNECTION" + QString::number(t);
	QSqlDatabase db = QSqlDatabase::database(dbconnection);
	if (db.isValid()) {
		for (int u=0; u<NUMDBSET; u++) {
			if (dbSet[t][u]) {
				dbSet[t][u]->clear();
				delete dbSet[t][u];
				dbSet[t][u] = NULL;
			}
		}
		db.close();
		QSqlDatabase::removeDatabase(dbconnection);
	}
}

void
Interpreter::runHalted() {
	// event fires from runcoltroller to tell program to signal user stop
	status = R_STOPING;
	//
	// force the interperter ops that block to go ahead and quit
	
	// stop timers
    sleeper->wake();

    // stop downloading
    if(downloader!=NULL) downloader->stop();
	
    // stop playing any wav files
    if(mediaplayer) mediaplayer->stop();

    // stop playing any sound
    if(sound!=NULL) sound->stop();


	// close network connections
	netSockCloseAll(); 

}


void
Interpreter::run() {
    // main run loop
    isError=false;
    downloader = new BasicDownloader();
    sound = new Sound();
    srand(time(NULL)+QTime::currentTime().msec()*911L); rand(); rand(); 	// initialize the random number generator for this thread
	onerrorstack = NULL;
	if (debugMode!=0) {			// highlight first line correctly in debugging mode
		emit(seekLine(2));
		emit(seekLine(1));
	}
    while (status != R_STOPING && execByteCode() >= 0) {} //continue
    status = R_STOPPED;
	debugMode = 0;
    cleanup(); // cleanup the variables, databases, files, stack and everything
    emit(stopRunFinalized(!isError));
}


void
Interpreter::inputEntered(QString text) {
	if (status!=R_STOPPED) {
		inputString = text;
		status = R_INPUTREADY;
	}
}


void Interpreter::clearsprites() {
	// cleanup sprites - release images and deallocate the space
	graphwin->spritesimage->fill(Qt::transparent);

	int i;
	if (nsprites>0) {
		for(i=0; i<nsprites; i++) {
			if (sprites[i].image) {
				delete sprites[i].image;
				sprites[i].image = NULL;
			}
			if (sprites[i].transformed_image) {
				delete sprites[i].transformed_image;
				sprites[i].transformed_image = NULL;
			}
		}
		free(sprites);
		sprites = NULL;
		nsprites = 0;
		graphwin->draw_sprites_flag = false;
	}
}

void Interpreter::sprite_prepare_for_new_content(int n) {
	if (sprites[n].image) {
		delete sprites[n].image;
		sprites[n].image = NULL;
	}
	if (sprites[n].transformed_image) {
		delete sprites[n].transformed_image;
		sprites[n].transformed_image = NULL;
	}
	sprites[n].x=0;
	sprites[n].y=0;
	sprites[n].r=0;	// rotate
	sprites[n].s=1;	// scale
	sprites[n].o=1;	// opacity
	sprites[n].visible=false;
	sprites[n].changed=true;
	sprites[n].position.setRect(0,0,0,0);
	//last_position and was_printed remains the same in case we need to clear last position
}

bool Interpreter::sprite_collide(int n1, int n2, bool deep) {
	QPolygon p1, p2, result;
	QPoint center;
	QRect rect;
	if (n1==n2) return true;											// cant collide with itself
	if (!sprites[n1].visible || !sprites[n2].visible) return false; 	// cant collide if invisible
	if(!sprites[n1].position.intersects(sprites[n2].position))
		return false;

	if(sprites[n1].r==0 && sprites[n2].r==0){
		if(!deep) return true;
		rect=sprites[n1].position.intersected(sprites[n2].position);
	}else{
		if(sprites[n1].r==0){
			p1=QPolygon(sprites[n1].position);
		}else{
			p1 = QTransform().translate(0,0).rotateRadians(sprites[n1].r).scale(sprites[n1].s,sprites[n1].s).mapToPolygon(QRect(0, 0, sprites[n1].image->width(), sprites[n1].image->height()));
			center = p1.boundingRect().center();
			p1.translate(sprites[n1].x-center.x(), sprites[n1].y-center.y());
		}
		if(sprites[n2].r==0){
			p2=QPolygon(sprites[n2].position);
		}else{
			p2 = QTransform().translate(0,0).rotateRadians(sprites[n2].r).scale(sprites[n2].s,sprites[n2].s).mapToPolygon(QRect(0, 0, sprites[n2].image->width(), sprites[n2].image->height()));
			center = p2.boundingRect().center();
			p2.translate(sprites[n2].x-center.x(), sprites[n2].y-center.y());
		}

		result=p1.intersected(p2);
		if(result.isEmpty()) return false;
		if(!deep) return true;
		//this line can look stupid, but if we use only rect = result.boundingRect(), then we got from time to time
		//bome black lines on the edge of intersected rectangle after we print the two images
		//draw contact zone
		//rect = result.boundingRect();
		rect = result.boundingRect().intersected(sprites[n1].position).intersected(sprites[n2].position);
	}
	if(rect.isEmpty()) return false;

	// Debug
	//    QPainter *ian2;
	//    ian2 = new QPainter(graphwin->image);
	//    ian2->drawPolygon(p1);
	//    ian2->drawPolygon(p2);
	//    ian2->drawPolygon(result);
	//    ian2->drawRect(result.boundingRect());
	//    ian2->end();
	//    delete ian2;
	//////////////////////////////////


	QImage *scan = new QImage(rect.size(), QImage::Format_ARGB32_Premultiplied);
	scan->fill(Qt::transparent);
	QPainter *painter = new QPainter(scan);
	if(sprites[n1].r==0 && sprites[n1].s==1){
		painter->drawImage(sprites[n1].position.x()-rect.x(),sprites[n1].position.y()-rect.y(), *sprites[n1].image);
	}else{
		painter->drawImage(sprites[n1].position.x()-rect.x(),sprites[n1].position.y()-rect.y(), *sprites[n1].transformed_image);
	}
	painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
	if(sprites[n2].r==0 && sprites[n2].s==1){
		painter->drawImage(sprites[n2].position.x()-rect.x(),sprites[n2].position.y()-rect.y(), *sprites[n2].image);
	}else{
		painter->drawImage(sprites[n2].position.x()-rect.x(),sprites[n2].position.y()-rect.y(), *sprites[n2].transformed_image);
	}
	painter->end();
	delete painter;

	//check collision comparing only alpha channel
	const uchar* scanbits = scan->bits();
	bool flag=false;
	const int max = scan->byteCount();
	for(int f=3;f<max;f+=4){
		if(scanbits[f]){
			flag=true;
			break;
		}
	}

	//debug - print collision zone
	//    QPainter *ian;
	//    ian = new QPainter(graphwin->image);
	//    ian->drawImage(0,0,*scan);
	//    ian->end();
	//    delete ian;

	delete scan;
	return flag;
}

void Interpreter::force_redraw_all_sprites_next_time(){
	for(int n=0;n<nsprites;n++){
		sprites[n].was_printed=false;
	}
}

void Interpreter::update_sprite_screen(){
	if(nsprites<=0){
		graphwin->draw_sprites_flag = false;
		return;
	}

	QPainter *painter;
	QRegion region = QRegion(0,0,0,0);
	painter = new QPainter(graphwin->spritesimage);
	bool flag=false;

	for(int n=0;n<nsprites;n++){
		if(sprites[n].was_printed){
			if(!sprites[n].visible){
				//clear old position if sprite is hidden now
				region+=sprites[n].last_position;
				sprites[n].was_printed=false;
			}else{
				if(sprites[n].changed){
					//prepare area for a moved sprite
					region+=sprites[n].last_position;
					region+=sprites[n].position;
				}
			}
		}else{
			//sprite become visible - clear area
			if(sprites[n].visible){
				region+=sprites[n].position; //Delete new - mark for first draw
			}
		}
	}

	graphwin->sprites_clip_region = region;
	painter->setClipRegion(region);
	painter->setCompositionMode(QPainter::CompositionMode_Clear);
	painter->fillRect(region.boundingRect(),Qt::transparent);
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	double lasto=1.0;
	for(int n=0;n<nsprites;n++){
		if(sprites[n].visible){
				if(lasto!=sprites[n].o){
					lasto=sprites[n].o;
					painter->setOpacity(lasto);
				}
				if(sprites[n].s==1 && sprites[n].r==0){
					if(sprites[n].image){
						if(graphwin->sprites_clip_region.intersects(sprites[n].position)){
							painter->drawImage(sprites[n].position, *sprites[n].image);
							sprites[n].last_position=sprites[n].position;
							sprites[n].was_printed=true;
							sprites[n].changed=false;
						}
						graphwin->sprites_clip_region+=sprites[n].position;
						flag = true;
					}
				}else{
					if(sprites[n].transformed_image){
						if(graphwin->sprites_clip_region.intersects(sprites[n].position)){
							painter->drawImage(sprites[n].position, *sprites[n].transformed_image);
							sprites[n].last_position=sprites[n].position;
							sprites[n].was_printed=true;
							sprites[n].changed=false;
						}
						graphwin->sprites_clip_region+=sprites[n].position;
						flag = true;
					}
				}

		}
	}
	painter->end();
	delete painter;
	graphwin->draw_sprites_flag = flag;

}

void
Interpreter::waitForGraphics() {
	update_sprite_screen();
	// wait for graphics operation to complete
	mymutex->lock();
	emit(goutputReady());
	waitCond->wait(mymutex);
	mymutex->unlock();
}



int
Interpreter::execByteCode() {
	int opcode;
    if (status == R_INPUTREADY) {
		stack->pushvariant(inputString, inputType);
		status = R_RUNNING;
		return 0;
	} else if (status == R_PAUSED) {
		sleep(1);
		return 0;
	} else if (status == R_INPUT) {
		return 0;
	}

	// if errnum is set then handle the last thrown error
	if (error->pending()) {
		error->process(currentLine);
		if(onerrorstack && error->e > 0) {
			// progess call to subroutine for error handling
			// or jump to the catch label
			if (onerrorstack->onerrorgosub) {
				frame *temp = new frame;
				temp->returnAddr = op;
				temp->next = callstack;
				callstack = temp;
			}
			op = wordCode + onerrorstack->onerroraddress;
			return 0;
		} else {
            isError=true;
			// no error handler defined or FATAL error - display message
			printError();
			// if error number less than the start of warnings then
			// highlight the current line AND die
			if (error->isFatal()) {
				if (currentIncludeFile!="") emit(goToLine(error->line));
				return -1;
			}
		}
	}

	//emit(outputReady("off=" + QString::number(op-wordCode,16) + " op=" + QString::number(*op,16) + " stack=" + stack->debug() + "\n"));


	opcode = *op;
	op++;

    switch (optype(opcode)) {

		case OPTYPE_VARIABLE: {
			//
			// OPCODES with an variable number following in the wordCcode go in this switch
			// int i is the symbol/variable number
			//
			int i = *op;
            op++;

			switch(opcode) {

				case OP_FOR: {

					forframe *temp = new forframe;

					int exitOffset = stack->popint();
					DataElement *stepE = stack->popelement();
					DataElement *endE = stack->popelement();
					DataElement *startE = stack->popelement();
					
					bool goodloop;	// set to true if we should do the loop atleast once

					variables->setdata(i, startE);	// set variable to initial value
                    watchvariable(debugMode, i);

					if (startE->type==T_INT && stepE->type==T_INT) {
						// an integer start and step (do an integer loop)
						temp->useInt = true;
						temp->intStart = startE->intval;
						temp->intEnd = convert->getLong(endE);	// could b float but cant ever be
						temp->intStep = stepE->intval;
						goodloop = (temp->intStep >=0 && temp->intStart <= temp->intEnd) | (temp->intStep <=0 && temp->intStart >= temp->intEnd);
					} else {
						// start or step not integer - it is a float loop
						temp->useInt = false;
						temp->floatStart = convert->getFloat(startE);
						temp->floatEnd = convert->getFloat(endE);
						temp->floatStep = convert->getFloat(stepE);
						goodloop = (temp->floatStep >=0 && temp->floatStart <= temp->floatEnd) | (temp->floatStep <=0 && temp->floatStart >= temp->floatEnd);
					}

					if (goodloop) {
						// add new forframe to the forframe stack
						temp->next = forstack;
						temp->variable = i;
						temp->recurselevel = variables->getrecurse();
						temp->returnAddr = op;		// first op after FOR statement to loop back to
						forstack = temp;
					} else {
						// bad loop exit straight away - jump to the statement after the next
						delete temp;
						op = wordCode + exitOffset;
						
					}
				}
				break;

				case OP_NEXT: {
					forframe *temp = forstack;

					if (!temp) {
						error->q(ERROR_NEXTNOFOR);
					} else {
						if (temp->variable==i) {
							if (temp->useInt) {
								long val = convert->getLong(variables->getdata(i));
								val += temp->intStep;
								variables->setdata(i, val);

								if (temp->intStep == 0 && temp->intStart <= temp->intEnd && val >= temp->intEnd) {
									forstack = temp->next;
									delete temp;
								} else if (temp->intStep == 0 && temp->intStart >= temp->intEnd && val <= temp->intEnd) {
									forstack = temp->next;
									delete temp;
								} else if (temp->intStep >= 0 && val <= temp->intEnd) {
									op = temp->returnAddr;
								} else if (temp->intStep <= 0 && val >= temp->intEnd) {
									op = temp->returnAddr;
								} else {
									forstack = temp->next;
									delete temp;
								}
							} else {
								double val = convert->getFloat(variables->getdata(i));
								val += temp->floatStep;
								variables->setdata(i, val);

								if (temp->floatStep == 0 && convert->compareFloats(temp->floatStart, temp->floatEnd)!=1 && convert->compareFloats(val, temp->floatEnd)!=-1) {
									forstack = temp->next;
									delete temp;
								} else if (temp->floatStep == 0 && convert->compareFloats(temp->floatStart, temp->floatEnd)!=-1 && convert->compareFloats(val, temp->floatEnd)!=1) {
									forstack = temp->next;
									delete temp;
								} else if (temp->floatStep >= 0 && convert->compareFloats(val, temp->floatEnd)!=1) {
									op = temp->returnAddr;
								} else if (temp->floatStep <= 0 && convert->compareFloats(val, temp->floatEnd)!=-1) {
									op = temp->returnAddr;
								} else {
									forstack = temp->next;
									delete temp;
								}
							}
                            watchvariable(debugMode, i);
						} else {
							error->q(ERROR_NEXTWRONGFOR);
						}
					}

				}
				break;

				case OP_DIM:
				case OP_REDIM: {
					int ydim = stack->popint();
					int xdim = stack->popint();
					if (xdim<=0) xdim=1; // need to dimension as 1d
					variables->arraydim(i, xdim, ydim, opcode == OP_REDIM);
					if(!error->pending()) {
                        watchdim(debugMode, i , xdim, ydim);
					}
				}
				break;

				case OP_ALEN:
				case OP_ALENROWS:
				case OP_ALENCOLS: {
					// return array lengths
					switch(opcode) {
						case OP_ALEN:
							if (variables->arraysizerows(i)==1) {
								stack->pushint(variables->arraysize(i));
							} else {
								stack->pushint(0);
								error->q(ERROR_ARRAYLENGTH2D);
							}
							break;
						case OP_ALENROWS:
							stack->pushint(variables->arraysizerows(i));
							break;
						case OP_ALENCOLS:
							stack->pushint(variables->arraysizecols(i));
							break;
					}
				}
				break;

				case OP_GLOBAL: {
					// make a variable number a global variable
					variables->makeglobal(i);
				}
				break;

				case OP_ARRAYASSIGN: {
					// assign a value to an array element
					// assumes that arrays are always two dimensional (if 1d then y=1)
					DataElement *e = stack->popelement();
					int yindex = stack->popint();
					int xindex = stack->popint();
					if (e->type==T_UNASSIGNED) {
						error->q(ERROR_VARNOTASSIGNED, e->intval);
					} else if (e->type==T_ARRAY) {
						error->q(ERROR_ARRAYINDEXMISSING, e->intval);
					} else {
						variables->arraysetdata(i, xindex, yindex, e);
						if (!error->pending()) {
                            watchvariable(debugMode, i, xindex, yindex);
						}
					}
					// dont delete to an assign of variable
				}
				break;


				case OP_DEREF: {
					// get a value from an array and push it to the stack
					// assumes that arrays are always two dimensional (if 1d then y=0)
					int yindex = stack->popint();
					int xindex = stack->popint();
					DataElement *e = variables->arraygetdata(i, xindex, yindex);
					stack->pushdataelement(e);
				}
				break;

				case OP_PUSHVAR: {
					DataElement *e = variables->getdata(i);
					if (e->type==T_ARRAY) {
						error->q(ERROR_ARRAYINDEXMISSING, e->intval);
						stack->pushint(0);
					} else {
						stack->pushdataelement(e);
					}
				}
				break;

				case OP_PUSHVARREF: {
					stack->pushvarref(i);
				}
				break;

				case OP_ASSIGN: {
					DataElement *e = stack->popelement();
					if (e->type==T_ARRAY) {
						error->q(ERROR_ARRAYINDEXMISSING, e->intval);
					} else {
						if (e->type==T_UNASSIGNED) error->q(ERROR_VARNOTASSIGNED, e->intval);
						variables->setdata(i, e);
                        watchvariable(debugMode, i);
					}
				}
				break;

				case OP_VARREFASSIGN: {
					// assign a variable reference
					DataElement *e = stack->popelement();
					if (e->type==T_REF) {
						 variables->setdata(i, e);
					} else {
						error->q(ERROR_BYREF);
					}
				}
				break;

				case OP_ARRAY2STACK: {
					// Push all of the elements of an array to the stack and then push the length to the stack
					// expects one integer - variable number
					// all arrays are 2 dimensional - push each column, column size, then number of rows
					int columns = variables->arraysizecols(i);
					int rows = variables->arraysizerows(i);
					for(int row = 0; row<rows && !error->pending(); row++) {
						for (int col = 0; col<columns && !error->pending(); col++) {
							DataElement *av = variables->arraygetdata(i, row, col);
							stack->pushdataelement(av);
						}
						stack->pushint(columns);
					}
					stack->pushint(rows);
					
					//mymutex->lock();
					//emit(dialogAlert(stack->debug()));
					//waitCond->wait(mymutex);
					//mymutex->unlock();

					
				}
				break;
				
				case OP_ARRAYFILL: {
					// fill an array with a single value
					int mode = stack->popint();		// 1-fill everything, 0-fill unassigned
					DataElement *e = stack->popelement();	// fill value
					Variable *v = variables->get(i);
                    if (v->data.type==T_ARRAY) {
						int columns = variables->arraysizecols(i);
						int rows = variables->arraysizerows(i);
						for(int row = 0; row<rows && !error->pending(); row++) {
							for (int col = 0; col<columns && !error->pending(); col++) {
								if(mode==1 || variables->arraygetdata(i, row, col)->type == T_UNASSIGNED) {
									variables->arraysetdata(i, row, col, e);
									if (!error->pending()) {
                                        watchvariable(debugMode, i, row, col);
									}
								}
							}
						}
					} else {
						// trying to fill a regular variable - just do an assign
						variables->setdata(i, e);
                        watchvariable(debugMode, i);
					}
				}
				break;

				case OP_UNASSIGN: {
					variables->unassign(i);
                    watchvariable(debugMode, i);
				}
				break;

				case OP_UNASSIGNA: {
					// clear a variable and release resources
					int yindex = stack->popint();
					int xindex = stack->popint();
					variables->arrayunassign(i, xindex, yindex);
                    watchvariable(debugMode, i, xindex, yindex);
				}
				break;
				
				case OP_VARIABLEWATCH: {
                    if (variables->get(i)->data.type == T_ARRAY) {
						// an array - trigger dim and dumping of the elements
						int ydim = variables->arraysizecols(i);
						int xdim = variables->arraysizerows(i);
                        watchdim(true, i , xdim, ydim);
					} else {
						// regular variable - show it
                        watchvariable(true, i);
					}
				}
				break;




			}
		}
		break;

		case OPTYPE_INT: {
			//
			// OPCODES with an integer following in the wordCcode go in this switch
			// int i is the number extracted from the wordCode
			//
			int i = *op;  // integer for opcodes of this type
			op++;
			switch(opcode) {

				case OP_CURRLINE: {
					// opcode currentline is compound and includes level and line number
					int includelevel = i / 0x1000000;
					currentLine = i % 0x1000000;

					if (debugMode != 0) {
						if (includelevel==0) {
							// do debug for the main program not included parts
							// edit needs to eventually have tabs for includes and tracing and debugging
							// would go three dimensional - but not right now
							emit(seekLine(currentLine));
							if ((debugMode==1) || (debugMode==2 && debugBreakPoints->contains(currentLine-1))) {
								// show step and runto options
								emit(debugNextStep());
								// wait for button if we are stepping or if we are at a break point
								mydebugmutex->lock();
								waitDebugCond->wait(mydebugmutex);
								mydebugmutex->unlock();
							} else {
								// when debugging to breakpoint slow execution down so that the
                                // trace on the screen keeps caught up
                                SETTINGS;
								sleeper->sleepMS(settings.value(SETTINGSDEBUGSPEED, SETTINGSDEBUGSPEEDDEFAULT).toInt());
							}
						}
					}
				}
				break;

				case OP_PUSHINT: {
					stack->pushint(i);
				}
				break;

				case OP_ARRAYLISTASSIGN: {
					int rows = stack->popint();
					int columns = stack->popint(); //pop the first row length - the following rows must have the same length

					// create array if we need to (wrong dimensions)
                    if (variables->get(i)->data.type != T_ARRAY || variables->arraysizecols(i)!=columns || variables->arraysizecols(i)!=rows) {
						variables->arraydim(i, rows, columns, false);
						if(error->pending()) break;
                        watchdim(debugMode, i, rows, columns);
					}

					for(int row = rows-1; row>=0 && !error->pending(); row--) {
						//pop row length only if is not first row - already popped
						if(row != rows-1)
							if(stack->popint()!=columns){
								error->q(ERROR_ARRAYNITEMS, i);
							}
						for (int col = columns - 1; col >= 0 && !error->pending(); col--) {
							DataElement *e = stack->popelement();
							if (e->type==T_ARRAY) {
								error->q(ERROR_ARRAYINDEXMISSING, e->intval);
							} else if (e->type==T_UNASSIGNED) {
								variables->arrayunassign(i, row, col);
							} else {
								variables->arraysetdata(i, row, col, e);
							}
							if(!error->pending()) {
                                watchvariable(debugMode, i, row, col);
							}
						}
					}
				}
                break;

                case OP_ARGUMENTCOUNTTEST: {
                     // Throw error if stack does not have enough values
                     // used to check if functions and subroutines have the proper number
                     // of datas on the stack to fill the parameters
                     int a = stack->popint();
                     if (a!=i) error->q(ERROR_ARGUMENTCOUNT);
                 }
                 break;




				}


			}
			break;

		case OPTYPE_FLOAT: {
			//
			// OPCODES with an double following in the wordCcode go in this switch
			// double d is the number extracted from the wordCode
			//
			double *d = (double *) op;
			op += bytesToFullWords(sizeof(double));
			switch(opcode) {

				case OP_PUSHFLOAT: {
					stack->pushfloat(*d);
				}
				break;

			}
		}
		break;

		case OPTYPE_STRING: {
			//
			// OPCODES with a string in the wordCcode go in this switch
			// int len will be set to the length and
			// QString s will be the string extracted from the wordCode
			//
			int len = strlen((char *) op) + 1;
			QString s = QString::fromUtf8((char *) op);
			op += bytesToFullWords(len);

			switch(opcode) {
				case OP_PUSHSTRING: {
					// push string from compiled bytecode
					stack->pushstring(s);
				}
				break;

				case OP_INCLUDEFILE: {
					// current include file name save for runtime error codes
					// set to "" in the byte code when returning to main program
					currentIncludeFile = s;
				}
				break;
			}
		}
		break;

		case OPTYPE_LABEL: {
			//
			// OPCODES with an integer wordCode label/symbol number go here
			// the symbol is looked up in the symtableaddress and changed to an array location
			// and stored in i BEFORE the OPCODES are executed
			//
			// int i is the new execution location offset within the array wordCode
			//
            int l = *op;    // label/symbol number
            int i; // address
            op++;

			// lookup address for the label
            if (symtableaddress[l] >=0) {
                i = symtableaddress[l];
            } else {
                error->q(ERROR_NOSUCHLABEL, l);
				break;
			}

			switch(opcode) {
				case OP_GOTO: {
                    if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL && symtableaddresstype[l]!=ADDRESSTYPE_SYSTEMCALL){
                        error->q(ERROR_NOSUCHLABEL, l);
                    }else{
                        op = wordCode + i;
                    }
				}
				break;

				case OP_BRANCH: {
					// goto if true
					int val = stack->popbool();

					if (val == 0) { // jump on false
						op = wordCode + i;
					}
				}
				break;

                case OP_GOSUB: {
                    if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL){
                        error->q(ERROR_NOSUCHLABEL, l);
                    }else{
                        // setup return
                        frame *temp = new frame;
                        temp->returnAddr = op;
                        temp->next = callstack;
                        callstack = temp;
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;

                case OP_CALLFUNCTION: {
                    //OP_CALLFUNCTION is used when program expect the result to be pushed on stack
                    if(symtableaddresstype[l]!=ADDRESSTYPE_FUNCTION){
                        error->q(ERROR_NOSUCHFUNCTION, l);
                    }else{
                        // setup return
                        frame *temp = new frame;
                        temp->returnAddr = op;
                        temp->next = callstack;
                        callstack = temp;
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;


                case OP_CALLSUBROUTINE: {
                    //OP_CALLSUBROUTINE is used to call subroutines
                    if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
                        error->q(ERROR_NOSUCHSUBROUTINE, l);
                    }else{
                        // setup return
                        frame *temp = new frame;
                        temp->returnAddr = op;
                        temp->next = callstack;
                        callstack = temp;
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;

                case OP_ONERRORGOSUB: {
                    if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL){
                        error->q(ERROR_NOSUCHLABEL, l);
                    }else{
                        // setup onerror frame and put on top of onerrorstack
                        onerrorframe *temp = new onerrorframe;
                        temp->onerroraddress = i;
                        temp->onerrorgosub = true;
                        temp->next = onerrorstack;
                        onerrorstack = temp;
                    }
				}
                break;

				case OP_ONERRORCATCH: {
					// setup onerror frame and put on top of onerrorstack
					onerrorframe *temp = new onerrorframe;
					temp->onerroraddress = i;
					temp->onerrorgosub = false;
					temp->next = onerrorstack;
					onerrorstack = temp;
				}
				break;
				
				case OP_EXITFOR: {
					forframe *temp = forstack;
					if (!temp) {
						error->q(ERROR_NEXTNOFOR);
					} else {
						forstack = temp->next;
						delete temp;
						op = wordCode + i;
					}

				}
				break;

				case OP_PUSHLABEL: {
					// get a label fom the wordcode and push the offset address
					stack->pushint(i);
				}
				break;


				
			}
		}

		case OPTYPE_NONE: {
			switch(opcode) {
				case OP_NOP:
					break;

				case OP_END: {
					return -1;
				}
				break;


				case OP_RETURN: {
					frame *temp = callstack;
					if (temp) {
						op = temp->returnAddr;
						callstack = temp->next;
                        delete temp;
					} else {
						return -1;
					}
				}
				break;



				case OP_OPEN: {
					int type = stack->popint();	// 0 text 1 binary
					QString name = stack->popstring();
					int fn = stack->popint();

					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						// close file number if open
						if (filehandle[fn] != NULL) {
							filehandle[fn]->close();
							filehandle[fn] = NULL;
						}
						// create filehandle
						filehandle[fn] = new QFile(name);
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILEOPEN);
						} else {
							filehandletype[fn] = type;
							if (type==0) {
								// text file (type 0)
								if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Text)) {
									error->q(ERROR_FILEOPEN);
								}
							} else {
								// binary file (type 1)
								if (!filehandle[fn]->open(QIODevice::ReadWrite)) {
									error->q(ERROR_FILEOPEN);
								}
							}
						}
					}
				}
				break;

			   case OP_OPENSERIAL: {
					int flow = stack->popint();
					int parity = stack->popint();
					int stop = stack->popint();
					int data = stack->popint();
					int baud = stack->popint();
					QString name = stack->popstring();
					int fn = stack->popint();
#ifdef ANDROID
					(void)flow;
					(void)parity;
					(void)stop;
					(void)data;
					(void)baud;
					(void)name;
					(void)fn;
					error->q(ERROR_NOTIMPLEMENTED);
# else
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						// close file number if open
						if (filehandle[fn] != NULL) {
							filehandle[fn]->close();
							filehandle[fn] = NULL;
						}
						// create file filehandle
						QSerialPort *p = new QSerialPort();
						if (p == NULL) {
							error->q(ERROR_FILEOPEN);
						} else {
							p->setPortName(name);
							p->setReadBufferSize(SERIALREADBUFFERSIZE);
							if (!error->pending()) {
								if (!p->open(QIODevice::ReadWrite)) {
									error->q(ERROR_FILEOPEN);
								} else {
									// successful open
									filehandle[fn] = p;
									filehandletype[fn] = 2;
									// set the parameters
									if (!p->setBaudRate(baud)) error->q(ERROR_SERIALPARAMETER);
									switch (data) {
										case 5:
											if(!p->setDataBits(QSerialPort::Data5)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 6:
											if(!p->setDataBits(QSerialPort::Data6)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 7:
											if(!p->setDataBits(QSerialPort::Data7)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 8:
											if(!p->setDataBits(QSerialPort::Data8)) error->q(ERROR_SERIALPARAMETER);
											break;
										default: error->q(ERROR_SERIALPARAMETER);
									}
									switch (stop) {
										case 1:
											if(!p->setStopBits(QSerialPort::OneStop)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setStopBits(QSerialPort::TwoStop)) error->q(ERROR_SERIALPARAMETER);
											break;
										default: error->q(ERROR_SERIALPARAMETER);
									}
									switch (parity) {
										case 0:
											if(!p->setParity(QSerialPort::NoParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 1:
											if(!p->setParity(QSerialPort::OddParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setParity(QSerialPort::EvenParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 3:
											if(!p->setParity(QSerialPort::SpaceParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 4:
											if(!p->setParity(QSerialPort::MarkParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										default:
											error->q(ERROR_SERIALPARAMETER);
									}
									switch (flow) {
										case 0:
											if(!p->setFlowControl(QSerialPort::NoFlowControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 1:
											if(!p->setFlowControl(QSerialPort::HardwareControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setFlowControl(QSerialPort::SoftwareControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										default:
											error->q(ERROR_SERIALPARAMETER);
									}
								}
							}
						}
					}
#endif
				}
				break;


				case OP_READ: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
						stack->pushint(0);
					} else {
						char c = ' ';
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
							stack->pushint(0);
						} else {
							int maxsize = 256;
							char * strarray = (char *) malloc(maxsize);
							memset(strarray, 0, maxsize);
							// get the first char - Remove leading whitespace
							do {
								filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
								if (!filehandle[fn]->getChar(&c)) {
									stack->pushstring(QString::fromUtf8(strarray));
									free(strarray);
									return 0;
								}
							} while (c == ' ' || c == '\t' || c == '\n');
							// read token - we already have the first char
							int offset = 0;
							// get next letter until we crap-out or get white space
							do {
								strarray[offset] = c;
								offset++;
								// grow the buffer if we need to
								if (offset+2 >= maxsize) {
									maxsize *= 2;
									strarray = (char *) realloc(strarray, maxsize);
									memset(strarray + offset, 0, maxsize - offset);
								}
								// get next char
								filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
								if (!filehandle[fn]->getChar(&c)) {
									// no more to get - finish the string and push to stack
									strarray[offset] = 0;
									stack->pushstring(QString::fromUtf8(strarray));
									free(strarray);
									return 0;  //nextop
								}
							} while (c != ' ' && c != '\t' && c != '\n');
							// found a delimiter - finish the string and push to stack
							strarray[offset] = 0;
							stack->pushstring(QString::fromUtf8(strarray));
							free(strarray);
							return 0;	// nextop
						}
					}
				}
				break;


				case OP_READLINE: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
						stack->pushint(0);
					} else {

						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
							stack->pushint(0);
						} else {
							//read entire line
							filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
							stack->pushstring(filehandle[fn]->readLine());
						}
					}
				}
				break;

				case OP_READBYTE: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
						stack->pushint(0);
					} else {
						char c = ' ';
						filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
						if (filehandle[fn]->getChar(&c)) {
							stack->pushint((int) (unsigned char) c);
						} else {
							stack->pushint((int) -1);
						}
					}
				}
				break;

				case OP_EOF: {
					//return true to eof if error is returned
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
						stack->pushint(1);
					} else {
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
							stack->pushint(1);
						} else {
							switch (filehandletype[fn]) {
								case 0:
								case 1:
									// normal file eof
									if (filehandle[fn]->atEnd()) {
										stack->pushint(1);
									} else {
										stack->pushint(0);
									}
									break;
								case 2:
									// serial
									QCoreApplication::processEvents();
									if (filehandle[fn]->bytesAvailable()==0) {
										stack->pushint(1);
									} else {
										stack->pushint(0);
									}
									break;
							}
						}
					}
				}
				break;


				case OP_WRITE:
				case OP_WRITELINE: {
					QString temp = stack->popstring();
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						int fileerror = 0;
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							fileerror = filehandle[fn]->write(temp.toUtf8().data());
							if (opcode == OP_WRITELINE) {
								fileerror = filehandle[fn]->putChar('\n');
							}
						   if (filehandle[fn]->isSequential()) {
								filehandle[fn]->waitForBytesWritten(FILEWRITETIMEOUT);
							}
						}
						if (fileerror == -1) {
							error->q(ERROR_FILEWRITE);
						}
					}
				}
				break;

				case OP_WRITEBYTE: {
					int n = stack->popint();
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						int fileerror = 0;
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							fileerror = filehandle[fn]->putChar((unsigned char) n);
							if (filehandle[fn]->isSequential()) filehandle[fn]->waitForBytesWritten(FILEWRITETIMEOUT);
							if (fileerror == -1) {
								error->q(ERROR_FILEWRITE);
							}
						}
					}
				}
				break;


				case OP_CLOSE: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							filehandle[fn]->close();
							filehandle[fn] = NULL;
						}
					}
				}
				break;


				case OP_RESET: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							if (filehandle[fn]->isSequential()) {
								error->q(ERROR_FILEOPERATION);
							} else {
								switch (filehandletype[fn]) {
									case 0:
										// text mode file (close and reopen)
										filehandle[fn]->close();
										if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
											error->q(ERROR_FILERESET);
										}
										break;
									case 1:
										// binary mode file
										filehandle[fn]->close();
										if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
											error->q(ERROR_FILERESET);
										}
										break;
									case 2:
										// serial pot
										error->q(ERROR_FILEOPERATION);
										break;
								}
							}
						}
					}
				}
				break;

				case OP_SIZE: {
					// push the current open file size on the stack
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
						stack->pushint(0);
					} else {
						int size = 0;
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							switch(filehandletype[fn]) {
								case 0:
								case 1:
									// normal file
									size = filehandle[fn]->size();
									break;
								case 2:
									// serial
									QCoreApplication::processEvents();
									size = filehandle[fn]->bytesAvailable();
									break;
							}
						}
						stack->pushint(size);
					}
				}
				break;

				case OP_EXISTS: {
					// push a 1 if file exists else zero

					QString filename = stack->popstring();
					if (QFile::exists(filename)) {
						stack->pushint(1);
					} else {
						stack->pushint(0);
					}
				}
				break;

				case OP_SEEK: {
					// move file pointer to a specific loaction in file
					long pos = stack->popint();
					int fn = stack->popint();
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						if (filehandle[fn] == NULL) {
							error->q(ERROR_FILENOTOPEN);
						} else {
							switch(filehandletype[fn]) {
								case 0:
								case 1:
									// normal file
									filehandle[fn]->seek(pos);
									break;
								case 2:
									// serial
									error->q(ERROR_FILEOPERATION);
									break;
							}
						}
					}
				}
				break;

				case OP_INT: {
					// bigger integer safe (trim floating point off of a float)
					double val = stack->popfloat();
					double intpart;
					val = modf(val + (val>0?EPSILON:-EPSILON), &intpart);
					if (intpart >= LONG_MIN && intpart <= LONG_MAX) {
						stack->pushlong(intpart);
					} else {
						stack->pushfloat(intpart);
					}
				}
				break;


				case OP_FLOAT: {
					double val = stack->popfloat();
					stack->pushfloat(val);
				}
				break;

				case OP_STRING: {
					stack->pushstring(stack->popstring());
				}
				break;

				case OP_RAND: {
                    double r = ((double) rand() * (double) RAND_MAX + (double) rand()) / double_random_max;
                    stack->pushfloat(r);
				}
				break;
				
				case OP_SEED: {
					unsigned int seed = stack->poplong();
					srand(seed);
				}
				break;

				case OP_PAUSE: {
					double val = stack->popfloat();
					if (val > 0) {
						sleeper->sleepSeconds(val);
					}
				}
				break;

				case OP_LENGTH: {
					// unicode length
					stack->pushint(stack->popstring().length());
				}
				break;


				case OP_MID: {
				// unicode safe mid string
				//       MID(string, pos, len)
				// pos - position. String indices begin at 1. If negative pos is given,
				//       then position is starting from the end of the string,
				//       where -1 is the last character position, -2 the second character from the end... and so on
				// len - number of characters to return. If negative number is given,
				//       then the number of characters are removed and the result is returned

					int len = stack->popint();
					int pos = stack->popint();
					QString qtemp = stack->popstring();

					if (pos == 0){
						error->q(ERROR_STRSTART);
						stack->pushint(0);
					}else{
						if(pos<0)
							pos=qtemp.length()+pos;
						else
							pos--;

						if(len==0){
							stack->pushstring(QString(""));
						}else if(len>0){
							stack->pushstring(qtemp.mid(pos,len));
						}else{
							len=-len;
							//QString::remove is not acting like QString::mid when position is negative
							if(pos>=0)
								stack->pushstring(qtemp.remove(pos,len));
							else if(len+pos>=0) //pos is negative - there is something left to return
								stack->pushstring(qtemp.remove(0,len+pos));
							else
								stack->pushstring(qtemp);
						}
					}
				}
				break;


				case OP_MIDX: {
				// regex section string (MID regeX)
				//         midx (expr, qtemp, start)
				// start - start position. String indices begin at 1. If negative start is given,
				//         then position is starting from the end of the string,
				//         where -1 is the last character position, -2 the second character from the end... and so on

					int start = stack->popint();
					QRegExp expr = QRegExp(stack->popstring());
					expr.setMinimal(regexMinimal);
					QString qtemp = stack->popstring();

					if(start == 0) {
						error->q(ERROR_STRSTART);
						stack->pushstring(QString(""));
					} else {
						int pos;
						if (start==1) {
							pos = expr.indexIn(qtemp);
						} else if (start>1){
							pos = expr.indexIn(qtemp.mid(start-1));
						}else{
							pos = expr.indexIn(qtemp.mid(qtemp.length()+start));
						}

						if (pos==-1) {
							// did not find it - return ""
							stack->pushstring(QString(""));
						} else {
							QStringList stuff = expr.capturedTexts();
							stack->pushstring(stuff[0]);
						}
					}
				}
				break;


				case OP_LEFT: {
				// unicode safe left string
				//       LEFT(string, len)
				// len - number of characters to return. If negative number is given,
				//       then the number of characters are removed and the result is returned
				// Eg.
				// LEFT("abcdefg", 3)  - returns "abc"  - translate: Return 3 characters starting from left
				// LEFT("abcdefg", -3) - returns "defg" - translate: Remove 3 characters starting from left

					int len = stack->popint();
					QString qtemp = stack->popstring();

					if (len == 0) {
						stack->pushstring(QString(""));
					} else if (len > 0) {
						stack->pushstring(qtemp.left(len));
					} else {
						stack->pushstring(qtemp.remove(0,-len));
					}
				}
				break;


				case OP_RIGHT: {
				// unicode safe right string
				//       RIGHT(string, len)
				// len - number of characters to return. If negative number is given,
				//       then the number of characters are removed and the result is returned
				// Eg.
				// RIGHT("abcdefg", 3)  - returns "efg"  - translate: Return 3 characters starting from right
				// RIGHT("abcdefg", -3) - returns "abcd" - translate: Remove 3 characters starting from right

					int len = stack->popint();
					QString qtemp = stack->popstring();

					if (len == 0) {
						stack->pushstring(QString(""));
					} else if (len > 0) {
						stack->pushstring(qtemp.right(len));
					} else {
						qtemp.chop(-len);
						stack->pushstring(qtemp);
					}
				}
				break;


				case OP_UPPER: {
					stack->pushstring(stack->popstring().toUpper());
				}
				break;

				case OP_LOWER: {
					stack->pushstring(stack->popstring().toLower());
				}
				break;

				case OP_ASC: {
					// unicode character sequence - return 16 bit number representing character
					QString qs = stack->popstring();
					stack->pushint((int) qs[0].unicode());
				}
				break;


				case OP_CHR: {
					// convert a single unicode character sequence to string in utf8
					int code = stack->popint();
					QChar temp[2];
					temp[0] = (QChar) code;
					temp[1] = (QChar) 0;
					QString qs = QString(temp,1);
					stack->pushstring(qs);
					qs = QString::null;
				}
				break;


				case OP_INSTR: {
				// unicode safe instr function
				//         instr ( qhay , qstr , start , casesens)
				// start - start position. String indices begin at 1. If negative start is given,
				//         then position is starting from the end of the string,
				//         where -1 is the last character position, -2 the second character from the end... and so on
				// 0 sensitive (default) - opposite of QT
					Qt::CaseSensitivity casesens = (stack->popint()==0?Qt::CaseSensitive:Qt::CaseInsensitive);
					int start = stack->popint();
					QString qstr = stack->popstring();
					QString qhay = stack->popstring();

					int pos = 0;
					if(start == 0) {
						error->q(ERROR_STRSTART);
					} else if (start>0){
						pos = qhay.indexOf(qstr, start-1, casesens)+1;
					}else{
						int p = qhay.length()+start;
						if(p<0)
							p=0;
						pos = qhay.indexOf(qstr, p, casesens)+1;
					}
					stack->pushint(pos);
				}
				break;


				case OP_INSTRX: {
				// regex instr
				//         instrx (expr, qtemp, start)
				// start - start position. String indices begin at 1. If negative start is given,
				//         then position is starting from the end of the string,
				//         where -1 is the last character position, -2 the second character from the end... and so on
					int start = stack->popint();
					QRegExp expr = QRegExp(stack->popstring());
					expr.setMinimal(regexMinimal);
					QString qtemp = stack->popstring();

					int pos=0;
					if(start == 0) {
						error->q(ERROR_STRSTART);
					} else if (start>0){
						pos = expr.indexIn(qtemp,start-1)+1;
					}else{
						int p = qtemp.length()+start;
						if(p<0)
							p=0;
						pos = expr.indexIn(qtemp, p)+1;
					}
					stack->pushint(pos);
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
				case OP_DEGREES:
				case OP_RADIANS:
				case OP_LOG:
				case OP_LOGTEN:
				case OP_SQR:
				case OP_EXP: {

					double val = stack->popfloat();
					switch (opcode) {
						case OP_SIN:
							stack->pushfloat(sin(val));
							break;
						case OP_COS:
							stack->pushfloat(cos(val));
							break;
						case OP_TAN:
							val = tan(val);
							if (std::isinf(val)) {
								error->q(ERROR_INFINITY);
								stack->pushint(0);
							} else {
								stack->pushfloat(val);
							}
							break;
						case OP_ASIN:
							stack->pushfloat(asin(val));
							break;
						case OP_ACOS:
							stack->pushfloat(acos(val));
							break;
						case OP_ATAN:
							stack->pushfloat(atan(val));
							break;
						case OP_CEIL:
							stack->pushint(ceil(val));
							break;
						case OP_FLOOR:
							stack->pushint(floor(val));
							break;
						case OP_DEGREES:
							stack->pushfloat(val * 180 / M_PI);
							break;
						case OP_RADIANS:
							stack->pushfloat(val * M_PI / 180);
							break;
						case OP_LOG:
							if (val<0) {
								error->q(ERROR_LOGRANGE);
								stack->pushint(0);
							} else {
								stack->pushfloat(log(val));
							}
							break;
						case OP_LOGTEN:
							if (val<0) {
								error->q(ERROR_LOGRANGE);
								stack->pushint(0);
							} else {
								stack->pushfloat(log10(val));
							}
							break;
						case OP_SQR:
							if (val<0) {
								error->q(ERROR_LOGRANGE);
								stack->pushint(0);
							} else {
								stack->pushfloat(sqrt(val));
							}
							break;
						case OP_EXP:
							val = exp(val);
							if (std::isinf(val)) {
								error->q(ERROR_INFINITY);
								stack->pushint(0);
							} else {
								stack->pushfloat(val);
							}
							break;
					}
					break;
				}
				
				case OP_CONCATENATE:
					// concatenate ";" operator - all types
					{
						QString sone = stack->popstring();
						QString stwo = stack->popstring();
						if (stwo.length() + sone.length()>STRINGMAXLEN) {
							error->q(ERROR_STRINGMAXLEN);
							stack->pushint(0);
						} else {
							stack->pushstring(stwo + sone);
						}
					}
					break;


				case OP_ADD:
				case OP_SUB:
				case OP_MUL: {
					// integer and float safe operations

					DataElement *one = stack->popelement();
					DataElement *two = stack->popelement();
					switch (opcode) {
						case OP_ADD:
							{
								// add - if both integer then add as integers (if no ovverflow)
								// else if both are numbers then convert and add as floats
								// otherwise concatenate (string & number or strings)
								if (one->type==T_INT && two->type==T_INT) {
									long a = two->intval + one->intval;
									if((two->intval<=0||one->intval<=0||a>=0)&&(two->intval>=0||one->intval>=0||a<=0)) {
										// integer add - without overflow
										stack->pushlong(a);
										break;
									}
								}
								if ((one->type==T_INT || one->type==T_FLOAT) && (two->type==T_INT || two->type==T_FLOAT)) {
									// float add (if floats, numeric strings, or overflow
									double fone = convert->getFloat(one);
									double ftwo = convert->getFloat(two);
									double ans = ftwo + fone;
									if (std::isinf(ans)) {
										error->q(ERROR_INFINITY);
										stack->pushint(0);
									} else {
										stack->pushfloat(ans);
									}
								} else {
									// concatenate (if one or both ar not numbers)
									QString sone = convert->getString(one);
									QString stwo = convert->getString(two);
									if (stwo.length() + sone.length()>STRINGMAXLEN) {
										error->q(ERROR_STRINGMAXLEN);
										stack->pushint(0);
									} else {
										stack->pushstring(stwo + sone);
									}
								}
							}
							break;

						case OP_SUB:
							{
								if (one->type==T_INT && two->type==T_INT) {
									long a = two->intval - one->intval;
									if((two->intval<=0||one->intval>0||a>=0)&&(two->intval>=0||one->intval<0||a<=0)) {
										// integer subtract - without overflow
										stack->pushlong(a);
										break;
									}
								}
								// float subtract
								double fone = convert->getFloat(one);
								double ftwo = convert->getFloat(two);
								double ans = ftwo - fone;
								if (std::isinf(ans)) {
									error->q(ERROR_INFINITY);
									stack->pushint(0);
								} else {
									stack->pushfloat(ans);
								}
							}
							break;

						case OP_MUL:
							{
								if (one->type==T_INT && two->type==T_INT) {
									if(two->intval==0||one->intval==0) {
										stack->pushlong(0);
										break;
									}
									if (labs(one->intval)<=LONG_MAX/labs(two->intval)) {
										long a = two->intval * one->intval;
										// integer multiply - without overflow
										stack->pushlong(a);
										break;
									}
								}
								// float multiply
								double fone = convert->getFloat(one);
								double ftwo = convert->getFloat(two);
								double ans = ftwo * fone;
								if (std::isinf(ans)) {
									error->q(ERROR_INFINITY);
									stack->pushint(0);
								} else {
									stack->pushfloat(ans);
								}
							}
							break;


						}
						break;
					}

				case OP_ABS:
				{
					DataElement *one = stack->popelement();
					if (one->type==T_INT) {
						stack->pushlong(labs(one->intval));

					} else {
						stack->pushfloat(fabs(convert->getFloat(one)));
					}
				}
				break;

				case OP_EX: {
					// always return a float value with power "^"
					double oneval = stack->popfloat();
					double twoval = stack->popfloat();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushfloat(0);
					} else {
						double ans = pow(twoval, oneval);
						if (std::isinf(ans)) {
							error->q(ERROR_INFINITY);
							stack->pushfloat(0);
						} else {
							stack->pushfloat(ans);
						}
					}
				}
				break;

				case OP_DIV: {
					// always return a float value with division "/"
					double oneval = stack->popfloat();
					double twoval = stack->popfloat();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushfloat(0);
					} else {
						double ans = twoval / oneval;
						if (std::isinf(ans)) {
							error->q(ERROR_INFINITY);
							stack->pushfloat(0);
						} else {
							stack->pushfloat(ans);
						}
					}
				}
				break;

				case OP_INTDIV: {
					long oneval = stack->poplong();
					long twoval = stack->poplong();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushlong(0);
					} else {
						stack->pushlong(twoval / oneval);
					}
				}
				break;
						
				case OP_MOD: {
					long oneval = stack->poplong();
					long twoval = stack->poplong();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushint(0);
					} else {
						stack->pushlong(twoval % oneval);
					}
				}
				break;

				case OP_AND: {
					int one = stack->popbool();
					int two = stack->popbool();
					stack->pushbool(one && two);
				}
				break;

				case OP_OR: {
					int one = stack->popbool();
					int two = stack->popbool();
					stack->pushbool(one || two);
				}
				break;

				case OP_XOR: {
					int one = stack->popbool();
					int two = stack->popbool();
					stack->pushbool(!(one && two) && (one || two));
				}
				break;

				case OP_NOT: {
					int temp = stack->popbool();
					stack->pushbool(!temp);
				}
				break;

				case OP_NEGATE: {
					// integer save negate
					DataElement *e = stack->popelement();
					if (e->type==T_INT) {
						stack->pushlong(e->intval * -1);
					} else {
						stack->pushfloat(convert->getFloat(e) * -1);
					}
				}
				break;

				case OP_EQUAL:
				case OP_NEQUAL:
				case OP_GT:
				case OP_LTE:
				case OP_LT:
				case OP_GTE:
				{
					DataElement *two = stack->popelement();
					DataElement *one = stack->popelement();
					int ans = convert->compare(one,two);
					switch(opcode) {
						case OP_EQUAL:
							stack->pushbool(ans==0);
							break;
						case OP_NEQUAL:
							stack->pushbool(ans!=0);
							break;
						case OP_GT:
							stack->pushbool(ans==1);
							break;
						case OP_LTE:
							stack->pushbool(ans!=1);
							break;
						case OP_LT:
							stack->pushbool(ans==-1);
							break;
						case OP_GTE:
							stack->pushbool(ans!=-1);
							break;
					}
				}
				break;



				case OP_SOUND_LIST: {
					// play an immediate list of sounds
                    // from a array pushed on the stack or a list of lists
					// fill the sound array backwrds because of pulling from the stack
                    // Multiple rows = multiple voices mixed

                    //const int rows = stack->popint();
                    //int columns;
                    //int j=0, total=0;

                    //int* freqdur=NULL;

                    //for(int r=0;r<rows;r++){
                    //    columns = stack->popint();
                    //    if(columns%2!=0){
                   //         error->q(ERROR_ARRAYEVEN);
                    //        break;
                    //    }
                    //    total=total+columns+1;
                    //    j=total;
                    //    freqdur = (int*) realloc(freqdur, total * sizeof(int));
                    //    for (int col = 0; col < columns && !error->pending(); col++) {
                    //        freqdur[--j] = stack->popint();
                    //    }
                    //    freqdur[--j] = columns;
                    //}
                    //if (!error->pending()) sound->playSounds(rows, freqdur);
                    //if(freqdur!=NULL) free(freqdur);
                    
					// play an immediate list of sounds
					// from a 2d array pushed on the stack or a list of lists
					// fill the sound array backwrds because of pulling from the stack
					// make a list containing voices
					// each voice is a list of freq/dir pairs

					int voices = stack->popint();
					std::vector < std::vector<int> > sounddata;

					for(int voice=0; voice<voices; voice++) {
						std::vector < int > v;	// vector containing duration

						int rows = stack->popint();
						int columns = stack->popint();
						
						if ((rows==1&&columns%2!=0)||(rows>1&&columns!=2)) error->q(ERROR_ARRAYEVEN);
							
						for(int row = 0; row < rows ; row++) {
							//pop columns only if is not first row - already popped
							if(row != 0) {
								if(stack->popint()!=2){
									error->q(ERROR_ARRAYEVEN);
								}
							}
							for (int col = 0; col < columns ; col++) {
								int i = stack->popint();
								//printf(">>%i\n",i);
								v.insert(v.begin(),i);
							}
						}
						sounddata.push_back( v );
					}
					
					if (!error->pending()) sound->playSounds(sounddata);

                }
				break;


				case OP_VOLUME: {
					// set the wave output height (volume 0-10)
					double volume = stack->popfloat();
                    sound->setVolume(volume);
				}
				break;

				case OP_SAY: {
					mymutex->lock();
					emit(speakWords(stack->popstring()));
					waitCond->wait(mymutex);
					mymutex->unlock();
				}
				break;

				case OP_SYSTEM: {
					QString temp = stack->popstring();
                    SETTINGS;

					if(settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toBool()) {
						mymutex->lock();
						emit(executeSystem(temp));
						waitCond->wait(mymutex);
						mymutex->unlock();
					} else {
						error->q(ERROR_PERMISSION);
					}
				}
				break;

				case OP_WAVPLAY: {
					QString file = stack->popstring();
					if(file.compare("")!=0) {
						// load new file and start playback
						if (mediaplayer) {
							delete(mediaplayer);
							mediaplayer = NULL;
						}
						mediaplayer = new BasicMediaPlayer();
						mediaplayer->loadFile(file);
						mediaplayer->play();
						if (mediaplayer->error()!=0) {
							error->q(ERROR_WAVFILEFORMAT);
							delete(mediaplayer);
							mediaplayer = NULL;
						}
					} else {
						// start playing an existing mediaplayer
						if (mediaplayer) {
							mediaplayer->play();
						} else {
							error->q(ERROR_WAVNOTOPEN);
						}
					}
				}
				break;

				case OP_WAVSTOP: {
					if(mediaplayer) {
						mediaplayer->stop();
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
				}
				break;

				case OP_SETCOLOR: {
					unsigned long brushval = stack->poplong();
					unsigned long penval = stack->poplong();

                    //set PenColorIsClear and CompositionModeClear flags here for speed
                    if (penval == COLOR_CLEAR){
                        PenColorIsClear = true;
                        if (brushval == COLOR_CLEAR){
                            CompositionModeClear = true;
                        }else{
                            CompositionModeClear = false;
                        }
                    }else{
                        PenColorIsClear = false;
                        CompositionModeClear = false;
                    }

					drawingpen.setColor(QColor::fromRgba((QRgb) penval));
					drawingbrush.setColor(QColor::fromRgba((QRgb) brushval));
				}
				break;

				case OP_RGB: {
					int aval = stack->popint();
					int bval = stack->popint();
					int gval = stack->popint();
					int rval = stack->popint();
					if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255 || aval < 0 || aval > 255) {
						error->q(ERROR_RGB);
                    } else {
						stack->pushlong( (unsigned long) QColor(rval,gval,bval,aval).rgba());
					}
				}
				break;

				case OP_PIXEL: {
					int y = stack->popint();
					int x = stack->popint();
					QRgb rgb = graphwin->image->pixel(x,y);
					stack->pushlong((unsigned long) rgb);
				}
				break;

				case OP_GETCOLOR: {
					stack->pushlong((unsigned long) drawingpen.color().rgba());
				}
				break;

				case OP_GETSLICE: {
					// slice format was a hex stringnow it is a 2d array list on the stack
					int layer = stack->popint();
					int h = stack->popint();
					int w = stack->popint();
					int y = stack->popint();
					int x = stack->popint();
					QImage *layerimage;
					switch(layer) {
						case SLICE_PAINT:
							layerimage = graphwin->image;
							break;
						case SLICE_SPRITE:
							layerimage = graphwin->spritesimage;
							break;
						default:
							layerimage = graphwin->displayedimage;
							break;
					}
					if (x<0||y<0||x>layerimage->width()||y>layerimage->height()||x+w-1>layerimage->width()||y+h-1>layerimage->height()) {
						error->q(ERROR_SLICESIZE);
						stack->pushint(0);
						stack->pushint(0);
					} else {
						QRgb rgb;
						int tw, th;
						for(th=0; th<h; th++) {
							for(tw=0; tw<w; tw++) {
								rgb = layerimage->pixel(x+tw,y+th);
								stack->pushlong(rgb);
							}
							stack->pushint(w);
						}
						stack->pushint(h);
					}
				}
				break;

				case OP_PUTSLICE: {
					// get image array
					int th, tw;
					int h = stack->popint();
					int w = stack->popint();
					stack->pushint(w);		// put back on stack to make pull off easier
					int data[h][w];
					for(th=h-1; th>=0; th--) {
						stack->popint();	// get extra cols
						for(tw=w-1; tw>=0; tw--) {
							data[th][tw] = stack->popint();
						}
					}
					// get where
					int y = stack->popint();
					int x = stack->popint();
					// draw
					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					for(th=0; th<h; th++) {
						for(tw=0; tw<w; tw++) {
							if(data[th][tw]!=COLOR_CLEAR) {
								ian->setPen(data[th][tw]);
								ian->drawPoint(x + tw, y + th);
							}
						}
					}
					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;

				case OP_LINE: {
					int y1val = stack->popint();
					int x1val = stack->popint();
					int y0val = stack->popint();
					int x0val = stack->popint();

					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

                    ian->setPen(drawingpen);
//					ian->setBrush(drawingbrush);
                    if (CompositionModeClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}
					if (x1val >= 0 && y1val >= 0) {
						ian->drawLine(x0val, y0val, x1val, y1val);
					 }

					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;


				case OP_RECT: {
					int y1val = stack->popint();
					int x1val = stack->popint();
					int y0val = stack->popint();
					int x0val = stack->popint();

					QPainter *ian;
                    if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					if(x1val<0) {
						x0val+=x1val;
						x1val*=-1;
					}
					if(y1val<0) {
						y0val+=y1val;
						y1val*=-1;
					}

                    ian->setBrush(drawingbrush);
                    ian->setPen(drawingpen);
                    if (CompositionModeClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}

					if (x1val > 1 && y1val > 1) {
						ian->drawRect(x0val, y0val, x1val-1, y1val-1);
					} else if (x1val==1 && y1val==1) {
						// rect 1x1 is actually a point
						ian->drawPoint(x0val, y0val);
					} else if (x1val==1 && y1val!=0) {
						// rect 1xn is actually a line
						ian->drawLine(x0val, y0val, x0val, y0val+y1val);
					} else if (x1val!=0 && y1val==1) {
						// rect nx1 is actually a line
						ian->drawLine(x0val, y0val, x0val + x1val, y0val);
					}

					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;


				case OP_POLY_LIST: {
					// doing a polygon from an immediate list or an array's data

					int rows = stack->popint();
					int columns = stack->popint();

					if ((rows==1&&columns%2==0)||columns==2) {
						int pairs = rows*columns/2;

						if (pairs >= 3) {
							// allocate points array and fill from array data on stack
							QPointF *points = new QPointF[pairs];
							int j = 0;
							for(int row = 0; row < rows && !error->pending(); row++) {
								//pop row length only if is not first row - already popped
								if(row != 0)
									if(stack->popint()!=2){
										error->q(ERROR_ARRAYEVEN);
									}
								for (int col = 0; col < columns && !error->pending(); col+=2) {
									points[j].setY(stack->popfloat());
									points[j].setX(stack->popfloat());
									j++;
								}
							}

							if (!error->pending()) {
								QPainter *poly;
								if (printing) {
									poly = printdocumentpainter;
								} else {
									poly = new QPainter(graphwin->image);
								}

								poly->setPen(drawingpen);
								poly->setBrush(drawingbrush);
                                if (CompositionModeClear) {
									poly->setCompositionMode(QPainter::CompositionMode_Clear);
								}
								poly->drawPolygon(points, pairs);

								if(!printing) {
									poly->end();
									delete poly;
									if (!fastgraphics) waitForGraphics();
								}
							}
							delete points;
						} else {
							error->q(ERROR_POLYPOINTS);
						}
					} else {
						error->q(ERROR_ARRAYEVEN);
					}
				}
				break;

				case OP_STAMP_LIST:
				case OP_STAMP_S_LIST:
				case OP_STAMP_SR_LIST: {
					// special type of poly where x,y,scale, are given first and
					// the ploy is sized and loacted - so we can move them easy
					double rotate=0;			// defaule rotation to 0 radians
					double scale=1;			// default scale to full size (1x)
					double tx, ty, savetx;		// used in scaling and rotating
					// create an array of points from an immediate list on the stack
					int rows = stack->popint();
					int columns = stack->popint();
					if ((rows==1&&columns%2==0)||columns==2) {
						int pairs = rows*columns/2;
						if (pairs >= 3) {
							// allocate points array and fill from array data on stack
							QPointF *points = new QPointF[pairs];
							int j = 0;
							for(int row = 0; row < rows && !error->pending(); row++) {
								//pop row length only if is not first row - already popped
								if(row != 0)
									if(stack->popint()!=2){
										error->q(ERROR_ARRAYEVEN);
									}
								for (int col = 0; col < columns && !error->pending(); col+=2) {
									points[j].setY(stack->popfloat());
									points[j].setX(stack->popfloat());
									j++;
								}
							}
							if (!error->pending()) {
								//
								// now get scaling, rotation, and position to stamp the poly
								if (opcode==OP_STAMP_SR_LIST) rotate = stack->popfloat();
								if (opcode==OP_STAMP_SR_LIST || opcode==OP_STAMP_S_LIST) scale = stack->popfloat();
								int y = stack->popint();
								int x = stack->popint();
								if (scale<0) {
									error->q(ERROR_IMAGESCALE);
								} else {
									// scale, rotate, and position the points
									for (int j = 0; j < pairs; j++) {
										tx = scale * points[j].x();
										ty = scale * points[j].y();
										if (rotate!=0) {
											savetx = tx;
											tx = cos(rotate) * tx - sin(rotate) * ty;
											ty = cos(rotate) * ty + sin(rotate) * savetx;
										}
										points[j].setX(tx + x);
										points[j].setY(ty + y);
									}
									// draw on screen or printer
									QPainter *poly;
									if (printing) {
										poly = printdocumentpainter;
									} else {
										poly = new QPainter(graphwin->image);
									}
									poly->setPen(drawingpen);
									poly->setBrush(drawingbrush);
                                    if (CompositionModeClear) {
										poly->setCompositionMode(QPainter::CompositionMode_Clear);
									}
									poly->drawPolygon(points, pairs);
									if (!printing) {
										poly->end();
										delete poly;
										if (!fastgraphics) waitForGraphics();
									}
								}
							}
							delete points;
						} else {
							error->q(ERROR_POLYPOINTS);
						}
					} else {
						error->q(ERROR_ARRAYEVEN);
					}
				}
				break;


				case OP_CIRCLE: {
					int rval = stack->popint();
					int yval = stack->popint();
					int xval = stack->popint();

					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					ian->setPen(drawingpen);
					ian->setBrush(drawingbrush);
                    if (CompositionModeClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}
					ian->drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);

					if(!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;

				case OP_IMGLOAD: {
					// Image Load - with scale and rotate

					// pop the filename to uncover the location and scale
					QString file = stack->popstring();

					double rotate = stack->popfloat();
					double scale = stack->popfloat();
					double y = stack->popint();
					double x = stack->popint();

                    QImage i;
                    if(QFileInfo(file).exists()){
                        i = QImage(file);
                    }else{
                        downloader->download(QUrl::fromUserInput(file));
                        i.loadFromData(downloader->data());
                    }

                    if(i.isNull()) {
                        error->q(ERROR_IMAGEFILE);
                    } else {
                        QPainter *ian;
                        if (printing) {
                            ian = printdocumentpainter;
                        } else {
                            ian = new QPainter(graphwin->image);
                        }

                        if (rotate != 0 || scale != 1) {
                            QTransform transform = QTransform().translate(0,0).rotateRadians(rotate).scale(scale, scale);
                            i = i.transformed(transform);
                        }
                        if (i.width() != 0 && i.height() != 0) {
                            ian->drawImage((int)(x - .5 * i.width()), (int)(y - .5 * i.height()), i);
                        }
                        if (!printing) {
                            ian->end();
                            delete ian;
                            if (!fastgraphics) waitForGraphics();
                        }
                    }
				}
				break;

				case OP_TEXT: {
					QString txt = stack->popstring();
					int y0val = stack->popint();
					int x0val = stack->popint();

					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}
						
					ian->setPen(QPen(drawingpen.color()));
					
                    if (PenColorIsClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}
					if(!fontfamily.isEmpty()) {
						ian->setFont(QFont(fontfamily, fontpoint, fontweight));
					}
					ian->drawText(x0val, y0val+(QFontMetrics(ian->font()).ascent()), txt);

					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;


				case OP_FONT: {
					int weight = stack->popint();
					int point = stack->popint();
					QString family = stack->popstring();
					if (point<0) {
						error->q(ERROR_FONTSIZE);
					} else {
						if (weight<0) {
							error->q(ERROR_FONTWEIGHT);
						} else {
							fontpoint = point;
							fontweight = weight;
							fontfamily = family;
						}
					}
				}
				break;

				case OP_CLS: {
					mymutex->lock();
					emit(outputClear());
					waitCond->wait(mymutex);
					mymutex->unlock();
				}
				break;

				case OP_CLG: {
					
					unsigned long clearcolor = stack->poplong();
					graphwin->image->fill(QColor::fromRgba((QRgb) clearcolor));
					if (!fastgraphics) waitForGraphics();
				}
				break;

				case OP_PLOT: {
					int oneval = stack->popint();
					int twoval = stack->popint();

					QPainter *ian;

					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					ian->setPen(drawingpen);
                    if (PenColorIsClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}
					ian->drawPoint(twoval, oneval);

					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;

				case OP_FASTGRAPHICS: {
					fastgraphics = true;
					emit(fastGraphics());
				}
				break;

				case OP_GRAPHSIZE: {
					int width = 300, height = 300;
					int oneval = stack->popint();
					int twoval = stack->popint();
					if (oneval>0) height = oneval;
					if (twoval>0) width = twoval;
					if (width > 0 && height > 0) {
						mymutex->lock();
						emit(mainWindowsResize(1, width, height));
						waitCond->wait(mymutex);
						mymutex->unlock();
					}
					force_redraw_all_sprites_next_time();
					waitForGraphics();
				}
				break;

				case OP_GRAPHWIDTH: {
					if (printing) {
						stack->pushint(printdocument->width());
					} else {
						stack->pushint((int) graphwin->image->width());
					}
				}
				break;

				case OP_GRAPHHEIGHT: {
					if (printing) {
						stack->pushint(printdocument->height());
					} else {
						stack->pushint((int) graphwin->image->height());
					}
				}
				break;

				case OP_REFRESH: {
					waitForGraphics();
				}
				break;

				case OP_INPUT: {
					inputType = stack->popint();
					QString prompt = stack->popstring();
					if (prompt.length()>0) {
						mymutex->lock();
						emit(outputReady(prompt));
						waitCond->wait(mymutex);
						mymutex->unlock();
					}
#ifdef ANDROID
					// input statement on android with popup keyboard
					// problmatic so use a popup dialog box but display
					// to output like it was really done there

					mymutex->lock();
					if (prompt.length()==0) prompt = "?";
					emit(dialogPrompt(prompt,""));
					waitCond->wait(mymutex);
					mymutex->unlock();
					//
					mymutex->lock();
					emit(outputReady(returnString+"\n"));
					waitCond->wait(mymutex);
					mymutex->unlock();
					waitForGraphics();
					//
					stack->pushvariant(returnString, intType);
#else
					// use the input status of interperter and get
					// input from BasicOutput
					// input is pushed by the emit back to the interperter
					status = R_INPUT;
					mymutex->lock();
					emit(getInput());
					waitCond->wait(mymutex);
					mymutex->unlock();
#endif
				}
				break;

				case OP_KEY: {
#ifdef ANDROID
					error->q(ERROR_NOTIMPLEMENTED);
					stack->pushint(0);
#else
					mymutex->lock();
					stack->pushint(lastKey);
					lastKey = 0;
					mymutex->unlock();
#endif
				}
				break;

				case OP_KEYPRESSED: {
#ifdef ANDROID
					error->q(ERROR_NOTIMPLEMENTED);
					stack->pushint(0);
#else
					mymutex->lock();
					int keyCode = stack->popint();
					if (keyCode==0) {
						stack->pushint(pressedKeys.size());
					} else {
						if (std::find(pressedKeys.begin(), pressedKeys.end(), keyCode) != pressedKeys.end()) {
							stack->pushint(1);
						} else {
							stack->pushint(0);
						}
					}
					mymutex->unlock();
#endif
				}
				break;

				case OP_PRINT:
				case OP_PRINTN: {
					QString p = stack->popstring();
					if (opcode == OP_PRINTN) {
						p += "\n";
					}
					mymutex->lock();
					emit(outputReady(p));
					waitCond->wait(mymutex);
					mymutex->unlock();
				}
				break;


				case OP_YEAR:
				case OP_MONTH:
				case OP_DAY:
				case OP_HOUR:
				case OP_MINUTE:
				case OP_SECOND: {
					time_t rawtime;
					struct tm * timeinfo;

					time ( &rawtime );
					timeinfo = localtime ( &rawtime );

					switch (opcode) {
						case OP_YEAR:
							stack->pushint(timeinfo->tm_year + 1900);
							break;
						case OP_MONTH:
							stack->pushint(timeinfo->tm_mon);
							break;
						case OP_DAY:
							stack->pushint(timeinfo->tm_mday);
							break;
						case OP_HOUR:
							stack->pushint(timeinfo->tm_hour);
							break;
						case OP_MINUTE:
							stack->pushint(timeinfo->tm_min);
							break;
						case OP_SECOND:
							stack->pushint(timeinfo->tm_sec);
							break;
					}
				}
				break;

				case OP_MOUSEX: {
					stack->pushint((int) graphwin->mouseX);
				}
				break;

				case OP_MOUSEY: {
					stack->pushint((int) graphwin->mouseY);
				}
				break;

				case OP_MOUSEB: {
					stack->pushint((int) graphwin->mouseB);
				}
				break;

				case OP_CLICKCLEAR: {
					graphwin->clickX = 0;
					graphwin->clickY = 0;
					graphwin->clickB = 0;
				}
				break;

				case OP_CLICKX: {
					stack->pushint((int) graphwin->clickX);
				}
				break;

				case OP_CLICKY: {
					stack->pushint((int) graphwin->clickY);
				}
				break;

				case OP_CLICKB: {
					stack->pushint((int) graphwin->clickB);
				}
				break;

				case OP_INCREASERECURSE:
					// increase recursion level in variable hash
				{
					variables->increaserecurse();
				}
				break;

				case OP_DECREASERECURSE:
					// decrease recursion level in variable hash
					// and pop any unfinished for statements off of forstack
				{
					while (forstack&&forstack->recurselevel == variables->getrecurse()) {
                        forframe *temp = forstack;
						forstack = temp->next;
						delete temp;
					}

                    watchdecurse(debugMode);
					variables->decreaserecurse();
				}
				break;

				case OP_SPRITEPOLY_LIST: {
					// create a sprite from a polygon from an immediate list on the stack
					// first stack element is the number of points*2 to pull from the stack
					int maxx=0, maxy=0;
					int minx=0, miny=0;
					int rows = stack->popint();
					int columns = stack->popint();

					if ((rows==1&&columns%2==0)||columns==2) {
						int pairs = rows*columns/2;

						if (pairs >= 3) {
							// allocate points array and fill from array data on stack
							QPointF *points = new QPointF[pairs];
							int j = 0;
							for(int row = 0; row < rows && !error->pending(); row++) {
								//pop row length only if is not first row - already popped
								if(row != 0)
									if(stack->popint()!=2){
										error->q(ERROR_ARRAYEVEN);
									}
								for (int col = 0; col < columns && !error->pending(); col+=2) {
									double y = stack->popfloat();
									double x = stack->popfloat();
									if(x<minx) minx=x;
									if(y<miny) miny=y;
									if(x>maxx) maxx=x;
									if(y>maxy) maxy=y;
									points[j].setY(y);
									points[j].setX(x);
									j++;
								}
							}
							if (!error->pending()) {
								//
								// now move points to the top left (if they are not there)
								for(j=0;j<pairs;j++) {
									points[j].rx()-=minx;
									points[j].ry()-=miny;
								}
								//
								// now build sprite
								int n = stack->popint(); // sprite number
								if(n >= 0 && n < nsprites) {
									// free old, draw, and capture sprite
									sprite_prepare_for_new_content(n);
									sprites[n].image = new QImage(maxx+1,maxy+1,QImage::Format_ARGB32_Premultiplied);
									if(!sprites[n].image->isNull()){
										sprites[n].image->fill(Qt::transparent);
										if (!CompositionModeClear) {
											QPainter *poly = new QPainter(sprites[n].image);
											poly->setPen(drawingpen);
											poly->setBrush(drawingbrush);
											poly->drawPolygon(points, pairs);
											poly->end();
											delete poly;
											double img_w=maxx+1;
											double img_h=maxy+1;
											sprites[n].position.setRect(-(img_w/2),-(img_h/2),img_w,img_h);
										}
									}
								} else {
									error->q(ERROR_SPRITENUMBER);
								}
							}
							delete points;
						} else {
							error->q(ERROR_POLYPOINTS);
						}
					} else {
						error->q(ERROR_ARRAYEVEN);
					}
				}
				break;

				case OP_SPRITEDIM: {
					int n = stack->popint();
					// deallocate existing sprites
					clearsprites();
					// create new ones that are not visible, active, and are at origin
					if (n > 0) {
						sprites = (sprite*) malloc(sizeof(sprite) * n);
						nsprites = n;
						while (n>0) {
							n--;
							sprites[n].image = NULL;
							sprites[n].transformed_image = NULL;
							sprites[n].visible = false;
							sprites[n].x = 0;
							sprites[n].y = 0;
							sprites[n].r = 0;
							sprites[n].s = 1;
							sprites[n].position.setRect(0,0,0,0);
							sprites[n].changed=false;
							sprites[n].was_printed = false;
							sprites[n].last_position.setRect(0,0,0,0);
						}
					}
				}
				break;

				case OP_SPRITELOAD: {

					QString file = stack->popstring();
					int n = stack->popint();

					if(n < 0 || n >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
                    } else {
						sprite_prepare_for_new_content(n);


                        QImage *tmp;
                        if(QFileInfo(file).exists()){
                            tmp = new QImage(file);
                        }else{
                            tmp = new QImage();
                            downloader->download(QUrl::fromUserInput(file));
                            tmp->loadFromData(downloader->data());
                        }


                        if(tmp->isNull()) {
							delete tmp;
							error->q(ERROR_IMAGEFILE);
						}else{
							sprites[n].image = new QImage(tmp->convertToFormat(QImage::Format_ARGB32_Premultiplied));
							delete tmp;
							double img_w=sprites[n].image->width();
							double img_h=sprites[n].image->height();
							sprites[n].position.setRect(-(img_w/2),-(img_h/2),img_w,img_h);
						}
					}
				}
				break;

				case OP_SPRITESLICE: {

					int h = stack->popint();
					int w = stack->popint();
					int y = stack->popint();
					int x = stack->popint();
					int n = stack->popint();

					if(n < 0 || n >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
					} else {
						sprite_prepare_for_new_content(n);
						sprites[n].image = new QImage(graphwin->image->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32_Premultiplied));
						if(sprites[n].image->isNull()) {
							error->q(ERROR_SPRITESLICE);
						}else{
							double img_w=sprites[n].image->width();
							double img_h=sprites[n].image->height();
							sprites[n].position.setRect(-(img_w/2),-(img_h/2),img_w,img_h);
						}
					}
				}
				break;

				case OP_SPRITEMOVE:
				case OP_SPRITEPLACE: {
					double o=0, r=0, s=0, y=0, x=0;
					int nr = stack->popint(); // number of arguments (3-6)
					switch(nr){
						case 6  :
						   o = stack->popfloat();
						case 5  :
						   r = stack->popfloat();
						case 4  :
						   s = stack->popfloat();
						default :
						   y = stack->popfloat();
						   x = stack->popfloat();
					}
					int n = stack->popint();

					double img_w, img_h;

					if(n < 0 || n >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
					} else {
							if(!sprites[n].image) {
								error->q(ERROR_SPRITENA);
							} else {

								if (opcode==OP_SPRITEMOVE) {
									x += sprites[n].x;
									y += sprites[n].y;
									s += sprites[n].s;
									r += sprites[n].r;
									o += sprites[n].o;
								}else{
									//OP_SPRITEPLACE - populate missing arguments
									if(nr<6) o = sprites[n].o;
									if(nr<5) r = sprites[n].r;
									if(nr<4) s = sprites[n].s;
								}

									if(sprites[n].s != s || sprites[n].r != r){
										//there is a transformation from the last time
										if (sprites[n].transformed_image) {
											delete sprites[n].transformed_image;
											sprites[n].transformed_image = NULL;
										}
										if(s!=1 || r!=0){
											QTransform transform = QTransform().translate(sprites[n].image->width()/2, sprites[n].image->height()/2).rotateRadians(r).scale(s,s);;
											sprites[n].transformed_image = new QImage(sprites[n].image->transformed(transform).convertToFormat(QImage::Format_ARGB32_Premultiplied));
											img_w=sprites[n].transformed_image->width();
											img_h=sprites[n].transformed_image->height();
											sprites[n].position.setRect(x-(img_w/2),y-(img_h/2),img_w,img_h);
										}else{
											img_w=sprites[n].image->width();
											img_h=sprites[n].image->height();
											sprites[n].position.setRect(x-(img_w/2),y-(img_h/2),img_w,img_h);
										}
										sprites[n].changed=true;
									}else if(sprites[n].x != x || sprites[n].y != y){
										//there is no transformation from last time but is just a movement
										if(s!=1 || r!=0){
											img_w=sprites[n].transformed_image->width();
											img_h=sprites[n].transformed_image->height();
										}else{
											img_w=sprites[n].image->width();
											img_h=sprites[n].image->height();
										}
										sprites[n].position.moveTo(x-(img_w/2),y-(img_h/2));
										sprites[n].changed=true;
									}
									if(sprites[n].o != o){
										if(o<0) o=0;
										if(o>1) o=1;
										if(sprites[n].o != o)
											sprites[n].changed=true;
									}

									sprites[n].x = x;
									sprites[n].y = y;
									sprites[n].s = s;
									sprites[n].r = r;
									sprites[n].o = o;

									if (!fastgraphics) waitForGraphics();
							}
					}
				}
				break;

				case OP_SPRITEHIDE:
				case OP_SPRITESHOW: {

					int n = stack->popint();
					bool vis = opcode==OP_SPRITESHOW;

					if(n < 0 || n >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
					} else {
						if(!sprites[n].image && vis) {
							error->q(ERROR_SPRITENA);
						} else if (sprites[n].visible != vis){
							sprites[n].visible = vis;
							if (!fastgraphics) waitForGraphics();
						}
					}
				}
				break;

				case OP_SPRITECOLLIDE: {
					int val = stack->popbool();
					int n1 = stack->popint();
					int n2 = stack->popint();

					if(n1 < 0 || n1 >=nsprites || n2 < 0 || n2 >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
					} else {
						if(!sprites[n1].image || !sprites[n2].image) {
							error->q(ERROR_SPRITENA);
						} else {
							stack->pushint(sprite_collide(n1, n2, val!=0));
						}
					}
				}
				break;

				case OP_SPRITEX:
				case OP_SPRITEY:
				case OP_SPRITEH:
				case OP_SPRITEW:
				case OP_SPRITEV:
				case OP_SPRITER:
				case OP_SPRITES:
				case OP_SPRITEO: {

					int n = stack->popint();

					if(n < 0 || n >=nsprites) {
						error->q(ERROR_SPRITENUMBER);
						stack->pushint(0);
					} else {
						if (opcode==OP_SPRITEX) stack->pushfloat(sprites[n].x);
						if (opcode==OP_SPRITEY) stack->pushfloat(sprites[n].y);
						if (opcode==OP_SPRITEH) stack->pushint(sprites[n].image?sprites[n].image->height():0);
						if (opcode==OP_SPRITEW) stack->pushint(sprites[n].image?sprites[n].image->width():0);
						if (opcode==OP_SPRITEV) stack->pushint(sprites[n].visible?1:0);
						if (opcode==OP_SPRITER) stack->pushfloat(sprites[n].r);
						if (opcode==OP_SPRITES) stack->pushfloat(sprites[n].s);
						if (opcode==OP_SPRITEO) stack->pushfloat(sprites[n].o);
					}
				}
				break;

				case OP_CHANGEDIR: {
					QString file = stack->popstring();
					if(!QDir::setCurrent(file)) {
						error->q(ERROR_FOLDER);
					}
				}
				break;

				case OP_CURRENTDIR: {
					stack->pushstring(QDir::currentPath());
				}
				break;

				case OP_WAVWAIT: {
					if(mediaplayer) {
						mediaplayer->wait();
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
				}
				break;

				case OP_DBOPEN: {
					// open database connection
					QString file = stack->popstring();
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						closeDatabase(n);
						QString dbconnection = "DBCONNECTION" + QString::number(n);
						QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",dbconnection);
						db.setDatabaseName(file);
						bool ok = db.open();
						if (!ok) {
							error->q(ERROR_DBOPEN);
							closeDatabase(n);
						}
					}
				}
				break;

				case OP_DBCLOSE: {
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						closeDatabase(n);
					}
				}
				break;

				case OP_DBEXECUTE: {
					// execute a statement on the database
					QString stmt = stack->popstring();
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						QString dbconnection = "DBCONNECTION" + QString::number(n);
						QSqlDatabase db = QSqlDatabase::database(dbconnection);
						if(db.isValid()) {
							QSqlQuery *q = new QSqlQuery(db);
							bool ok = q->exec(stmt);
							if (!ok) {
								error->q(ERROR_DBQUERY, 0, q->lastError().databaseText());
							}
							delete q;
						} else {
							error->q(ERROR_DBNOTOPEN);
						}
					}
				}
				break;

				case OP_DBOPENSET: {
					// open recordset
					QString stmt = stack->popstring();
					int set = stack->popint();
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						if (set<0||set>=NUMDBSET) {
							error->q(ERROR_DBSETNUMBER);
						} else {
							QString dbconnection = "DBCONNECTION" + QString::number(n);
							QSqlDatabase db = QSqlDatabase::database(dbconnection);
							if(db.isValid()) {
								if (dbSet[n][set]) {
									dbSet[n][set]->clear();
									delete dbSet[n][set];
									dbSet[n][set] = NULL;
								}
								dbSet[n][set] = new QSqlQuery(db);
								bool ok = dbSet[n][set]->exec(stmt);
								if (!ok) {
									error->q(ERROR_DBQUERY, 0, dbSet[n][set]->lastError().databaseText());
								}
							} else {
								error->q(ERROR_DBNOTOPEN);
							}
						}
					}
				}
				break;

				case OP_DBCLOSESET: {
					int set = stack->popint();
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						if (set<0||set>=NUMDBSET) {
							error->q(ERROR_DBSETNUMBER);
						} else {
							if (dbSet[n][set]) {
								dbSet[n][set]->clear();
								delete dbSet[n][set];
								dbSet[n][set] = NULL;
							} else {
								error->q(ERROR_DBNOTSET);
							}
						}
					}
				}
				break;

				case OP_DBROW: {
					int set = stack->popint();
					int n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						if (set<0||set>=NUMDBSET) {
							error->q(ERROR_DBSETNUMBER);
						} else {
							if (dbSet[n][set]) {
								// return true if we move to a new row else false
								stack->pushint(dbSet[n][set]->next());
							} else {
								error->q(ERROR_DBNOTSET);
							}
						}
					}
				}
				break;

				case OP_DBINT:
				case OP_DBFLOAT:
				case OP_DBNULL:
				case OP_DBSTRING: {

					int col = -1, set, n;
					QString colname;
					bool usename;
					if (stack->peekType()==T_STRING) {
						usename = true;
						colname = stack->popstring();
					} else {
						usename = false;
						col = stack->popint();
					}
					set = stack->popint();
					n = stack->popint();
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
					} else {
						if (set<0||set>=NUMDBSET) {
							error->q(ERROR_DBSETNUMBER);
						} else {
							if (!dbSet[n][set]->isActive()) {
								error->q(ERROR_DBNOTSET);
							} else {
								if (!dbSet[n][set]->isValid()) {
									error->q(ERROR_DBNOTSETROW);
								} else {
									if (usename) {
										col = dbSet[n][set]->record().indexOf(colname);
									}
									if (col < 0 || col >= dbSet[n][set]->record().count()) {
										error->q(ERROR_DBCOLNO);
									} else {
										switch(opcode) {
											case OP_DBINT:
												stack->pushint(dbSet[n][set]->record().value(col).toInt());
												break;
											case OP_DBFLOAT:
												// potential issue with locale and database
												// it seems like locale->toDouble does not support a QVariant
												stack->pushfloat(dbSet[n][set]->record().value(col).toDouble());
												break;
											case OP_DBNULL:
												stack->pushint(dbSet[n][set]->record().value(col).isNull());
												break;
											case OP_DBSTRING:
												// potential issue with locale and database
												// it seems like locale->toString does not support a QVariant
												stack->pushstring(dbSet[n][set]->record().value(col).toString());
												break;
										}
									}
								}
							}
						}
					}
				}
				break;

				case OP_LASTERROR: {
					stack->pushint(error->e);
				}
				break;

				case OP_LASTERRORLINE: {
					stack->pushint(error->line);
				}
				break;

				case OP_LASTERROREXTRA: {
					stack->pushstring(error->extra);
				}
				break;

				case OP_LASTERRORMESSAGE: {
					stack->pushstring(error->getErrorMessage(symtable));
				}
				break;

				case OP_OFFERROR: {
					// pop a trap off of the trap stack
					onerrorframe *temp = onerrorstack;
					if (temp) {
						onerrorstack = temp->next;
					}
				}
				break;

				case OP_NETLISTEN: {
					struct sockaddr_in serv_addr, cli_addr;
					socklen_t clilen;

					int port = stack->popint();
					int fn = stack->popint();
					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
					} else {
						if (netsockfd[fn] >= 0) {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}

						// SOCK_DGRAM = UDP  SOCK_STREAM = TCP
						listensockfd = socket(AF_INET, SOCK_STREAM, 0);
						if (listensockfd < 0) {
							error->q(ERROR_NETSOCK, 0, strerror(errno));
						} else {
							int optval = 1;
							if (setsockopt(listensockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(int))) {
								error->q(ERROR_NETSOCKOPT, 0, strerror(errno));
								listensockfd = netSockClose(listensockfd);
							} else {
								memset((char *) &serv_addr, 0, sizeof(serv_addr));
								serv_addr.sin_family = AF_INET;
								serv_addr.sin_addr.s_addr = INADDR_ANY;
								serv_addr.sin_port = htons(port);
								if (bind(listensockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
									error->q(ERROR_NETBIND, 0, strerror(errno));
									listensockfd = netSockClose(listensockfd);
								} else {
									listen(listensockfd,5);
									clilen = sizeof(cli_addr);
									netsockfd[fn] = accept(listensockfd, (struct sockaddr *) &cli_addr, &clilen);
									if (netsockfd[fn] < 0) {
										error->q(ERROR_NETACCEPT, 0, strerror(errno));
									}
									listensockfd = netSockClose(listensockfd);
								}
							}
						}
					}
				}
				break;

				case OP_NETCONNECT: {

					struct sockaddr_in serv_addr;
					struct hostent *server;

					int port = stack->popint();
					QString address = stack->popstring();
					int fn = stack->popint();

					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
					} else {

						if (netsockfd[fn] >= 0) {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}

						netsockfd[fn] = socket(AF_INET, SOCK_STREAM, 0);
						if (netsockfd[fn] < 0) {
							error->q(ERROR_NETSOCK, 0, strerror(errno));
						} else {

							server = gethostbyname(address.toUtf8().data());
							if (server == NULL) {
								error->q(ERROR_NETHOST, 0, strerror(errno));
								netsockfd[fn] = netSockClose(netsockfd[fn]);
							} else {
								memset((char *) &serv_addr, 0, sizeof(serv_addr));
								serv_addr.sin_family = AF_INET;
								memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
								serv_addr.sin_port = htons(port);
								if (::connect(netsockfd[fn],(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
									error->q(ERROR_NETCONN, 0, strerror(errno));
									netsockfd[fn] = netSockClose(netsockfd[fn]);
								}
							}
						}
					}
				}
				break;

				case OP_NETREAD: {
					int MAXSIZE = 2048;
					int n;
					char * strarray = (char *) malloc(MAXSIZE);

					int fn = stack->popint();
					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
						stack->pushint(0);
					} else {
						if (netsockfd[fn] < 0) {
							error->q(ERROR_NETNONE);
							stack->pushint(0);
                        } else {
							memset(strarray, 0, MAXSIZE);
							n = recv(netsockfd[fn],strarray,MAXSIZE-1,0);
							if (n < 0) {
								error->q(ERROR_NETREAD, 0, strerror(errno));
								stack->pushint(0);
							} else {
								stack->pushstring(QString::fromUtf8(strarray));
							}
						}
					}
					free(strarray);
				}
				break;

				case OP_NETWRITE: {
					QString data = stack->popstring();
					int fn = stack->popint();
					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
					} else {
						if (netsockfd[fn]<0) {
							error->q(ERROR_NETNONE);
						} else {
							int n = send(netsockfd[fn],data.toUtf8().data(),data.length(),0);
							if (n < 0) {
								error->q(ERROR_NETWRITE, 0, strerror(errno));
							}
						}
					}
				}
				break;

				case OP_NETCLOSE: {
					int fn = stack->popint();
					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
					} else {
						if (netsockfd[fn]<0) {
							error->q(ERROR_NETNONE);
						} else {
							netsockfd[fn] = netSockClose(netsockfd[fn]);
						}
					}
				}
				break;

				case OP_NETDATA: {
					// push 1 if there is data to read on network connection
					// wait 1 ms for each poll
					int fn = stack->popint();
					if (fn<0||fn>=NUMSOCKETS) {
						error->q(ERROR_NETSOCKNUMBER);
					} else {
#ifdef WIN32
						unsigned long n;
						if (ioctlsocket(netsockfd[fn], FIONREAD, &n)!=0) {
							stack->pushint(0);
						} else {
							if (n==0L) {
								stack->pushint(0);
							} else {
								stack->pushint(1);
							}
						}
#else
						struct pollfd p[1];
						p[0].fd = netsockfd[fn];
						p[0].events = POLLIN | POLLPRI;
						if(poll(p, 1, 1)<0) {
							stack->pushint(0);
						} else {
							if (p[0].revents & POLLIN || p[0].revents & POLLPRI) {
								stack->pushint(1);
							} else {
								stack->pushint(0);
							}
						}
#endif
					}
				}
				break;

				case OP_NETADDRESS: {
					// get first non "lo" ip4 address
#ifdef WIN32
					char szHostname[100];
					HOSTENT *pHostEnt;
					int nAdapter = 0;
					struct sockaddr_in sAddr;
					gethostname( szHostname, sizeof( szHostname ));
					pHostEnt = gethostbyname( szHostname );
					memcpy ( &sAddr.sin_addr.s_addr, pHostEnt->h_addr_list[nAdapter], pHostEnt->h_length);
					stack->pushstring(QString::fromUtf8(inet_ntoa(sAddr.sin_addr)));
#else
#ifdef ANDROID
					error->q(ERROR_NOTIMPLEMENTED);
#else
					bool good = false;
					struct ifaddrs *myaddrs, *ifa;
					void *in_addr;
					char buf[64];
					if(getifaddrs(&myaddrs) != 0) {
						error->q(ERROR_NETNONE);
					} else {
						for (ifa = myaddrs; ifa != NULL && !good; ifa = ifa->ifa_next) {
							if (ifa->ifa_addr == NULL) continue;
							if (!(ifa->ifa_flags & IFF_UP)) continue;
							if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") !=0 ) {
								struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
								in_addr = &s4->sin_addr;
								if (inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))) {
									stack->pushstring(QString::fromUtf8(buf));
									good = true;
								}
							}
						}
						freeifaddrs(myaddrs);
					}
					if (!good) {
						// on error give local loopback
						stack->pushstring(QString("127.0.0.1"));
					}
#endif
#endif
				}
				break;

				case OP_KILL: {
					QString name = stack->popstring();
					if(!QFile::remove(name)) {
						error->q(ERROR_FILEOPEN);
					}
				}
				break;

				case OP_MD5: {
					QString stuff = stack->popstring();
					stack->pushstring(MD5(stuff.toUtf8().data()).hexdigest());
				}
				break;

				case OP_SETSETTING: {
					QString stuff = stack->popstring();
					QString key = stack->popstring();
					QString app = stack->popstring();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						settings.beginGroup(SETTINGSGROUPUSER);
						settings.beginGroup(app);
						settings.setValue(key, stuff);
						settings.endGroup();
						settings.endGroup();
					} else {
						error->q(ERROR_PERMISSION);
					}
				}
				break;

				case OP_GETSETTING: {
					QString key = stack->popstring();
					QString app = stack->popstring();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool()) {
						if(app==QString("SYSTEM")) {
							stack->pushstring(settings.value(key, "").toString());
						} else {
							settings.beginGroup(SETTINGSGROUPUSER);
							settings.beginGroup(app);
							stack->pushstring(settings.value(key, "").toString());
							settings.endGroup();
							settings.endGroup();
						}
					} else {
						error->q(ERROR_PERMISSION);
						stack->pushint(0);
					}
				}
				break;


				case OP_PORTOUT: {
					int data = stack->popint();
					int port = stack->popint();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
#ifdef WIN32
#ifdef WIN32PORTABLE
						(void) data;
						(void) port;
						error->q(ERROR_NOTIMPLEMENTED);
# else
						if (Out32==NULL) {
							(void) data;
							(void) port;
							error->q(ERROR_NOTIMPLEMENTED);
						} else {
							Out32(port, data);
						}
#endif
#else
						(void) data;
						(void) port;
						error->q(ERROR_NOTIMPLEMENTED);
#endif
					} else {
						error->q(ERROR_PERMISSION);
					}
				}
				break;

				case OP_PORTIN: {
					int data=0;
					int port = stack->popint();
                    SETTINGS;
					if(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool()) {
#ifdef WIN32
#ifdef WIN32PORTABLE
						(void) port;
						error->q(ERROR_NOTIMPLEMENTED);
# else
						if (Inp32==NULL) {
							(void) port;
							error->q(ERROR_NOTIMPLEMENTED);
						} else {
							data = Inp32(port);
						}
#endif
#else
						(void) port;
						error->q(ERROR_NOTIMPLEMENTED);
#endif
					} else {
						error->q(ERROR_PERMISSION);
					}
					stack->pushint(data);
				}
				break;

				case OP_BINARYOR: {
					unsigned long a = stack->poplong();
					unsigned long b = stack->poplong();
					a = a | b;
					stack->pushlong(a);
				}
				break;

				case OP_BINARYAND: {
					DataElement *one = stack->popelement();
					DataElement *two = stack->popelement();
					// if both are numbers then convert to long and bitwise and
					// otherwise concatenate (string & number or strings)
					if ((one->type==T_INT || one->type==T_FLOAT) && (two->type==T_INT || two->type==T_FLOAT)) {
						unsigned long a = convert->getLong(one);
						unsigned long b = convert->getLong(two);
						a = a&b;
						stack->pushlong(a);
					} else {
						// concatenate (if one or both at not numbers or cant be converted to numbers)
						QString sone = convert->getString(one);
						QString stwo = convert->getString(two);
						if (stwo.length() + sone.length()>STRINGMAXLEN) {
							error->q(ERROR_STRINGMAXLEN);
							stack->pushint(0);
						} else {
							stack->pushstring(stwo + sone);
						}
					}
				}
				break;

				case OP_BINARYNOT: {
					unsigned long a = stack->poplong();
					a = ~a;
					stack->pushlong(a);
				}
				break;

				case OP_IMGSAVE: {
					// Image Save - Save image
					QString type = stack->popstring();
					QString file = stack->popstring();
					QStringList validtypes;
					validtypes << IMAGETYPE_BMP << IMAGETYPE_JPG << IMAGETYPE_JPEG << IMAGETYPE_PNG ;
					if (validtypes.contains(type, Qt::CaseInsensitive)) {
						graphwin->image->save(file, type.toUpper().toUtf8().data());
					} else {
						error->q(ERROR_IMAGESAVETYPE);
					}
				}
				break;

				case OP_DIR: {
					// Get next directory entry - id path send start a new folder else get next file name
					// return "" if we have no names on list - skippimg . and ..
					QString folder = stack->popstring();
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
							stack->pushstring(QString::fromUtf8(dirp->d_name));
						} else {
							stack->pushstring(QString(""));
							closedir(directorypointer);
							directorypointer = NULL;
						}
					} else {
						error->q(ERROR_FOLDER);
						stack->pushint(0);
					}
				}
				break;

				case OP_REPLACE: {
					// unicode safe replace function

				   // 0 sensitive (default) - opposite of QT
					Qt::CaseSensitivity casesens = (stack->popint()==0?Qt::CaseSensitive:Qt::CaseInsensitive);

					QString qto = stack->popstring();
					QString qfrom = stack->popstring();
					QString qhaystack = stack->popstring();

					stack->pushstring(qhaystack.replace(qfrom, qto, casesens));
				}
				break;

				case OP_REPLACEX: {
					// regex replace function

					QString qto = stack->popstring();
					QRegExp expr = QRegExp(stack->popstring());
					expr.setMinimal(regexMinimal);
					QString qhaystack = stack->popstring();

					stack->pushstring(qhaystack.replace(expr, qto));
				}
				break;

				case OP_COUNT: {
					// unicode safe count function

					// 0 sensitive (default) - opposite of QT
					Qt::CaseSensitivity casesens = (stack->popint()==0?Qt::CaseSensitive:Qt::CaseInsensitive);

					QString qneedle = stack->popstring();
					QString qhaystack = stack->popstring();

					stack->pushint((int) (qhaystack.count(qneedle, casesens)));
				}
				break;

				case OP_COUNTX: {
					// regex count function

					QRegExp expr = QRegExp(stack->popstring());
					expr.setMinimal(regexMinimal);
					QString qhaystack = stack->popstring();

					stack->pushint((int) (qhaystack.count(expr)));
				}
				break;

				case OP_OSTYPE: {
					// Return type of OS this compile was for
					int os = -1;
#ifdef WIN32
					os = OSTYPE_WINDOWS;
#endif
#ifdef LINUX
					os = OSTYPE_LINUX;
#endif
#ifdef MACX
					os = OSTYPE_MACINTOSH;
#endif
#ifdef ANDROID
					os = OSTYPE_ANDROID;
#endif
					stack->pushint(os);
				}
				break;

				case OP_MSEC: {
					// Return number of milliseconds the BASIC256 program has been running
					stack->pushint((int) (runtimer.elapsed()));
				}
				break;

				case OP_EDITVISIBLE:
				case OP_GRAPHVISIBLE:
				case OP_OUTPUTVISIBLE: {
					int show = stack->popint();
					if (opcode==OP_EDITVISIBLE) emit(mainWindowsVisible(0,show!=0));
					if (opcode==OP_GRAPHVISIBLE) emit(mainWindowsVisible(1,show!=0));
					if (opcode==OP_OUTPUTVISIBLE) emit(mainWindowsVisible(2,show!=0));
				}
				break;

				case OP_REGEXMINIMAL: {
					// set the regular expression minimal flag (true = not greedy)
					regexMinimal = stack->popint()!=0;
				}
				break;

				case OP_TEXTHEIGHT:
				case OP_TEXTWIDTH: {
					// return the number of pixels the font requires for diaplay
					// a string is required for width but not for height
					int v = 0;

					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					if(!fontfamily.isEmpty()) {
						ian->setFont(QFont(fontfamily, fontpoint, fontweight));
					}

					if (opcode==OP_TEXTWIDTH) {
						QString txt = stack->popstring();
						v = QFontMetrics(ian->font()).width(txt);
					}
					if (opcode==OP_TEXTHEIGHT) v = QFontMetrics(ian->font()).height();

					if (!printing) {
						ian->end();
						delete ian;
					}
					stack->pushint((int) (v));
				}
				break;

				case OP_FREEFILE: {
					// return the next free file number - throw error if not free files
					int f=-1;
					for (int t=0; (t<NUMFILES)&&(f==-1); t++) {
						if (!filehandle[t]) f = t;
					}
					if (f==-1) {
						error->q(ERROR_FREEFILE);
						stack->pushint(0);
					} else {
						stack->pushint(f);
					}
				}
				break;

				case OP_FREENET: {
					// return the next free network socket number - throw error if not free sockets
					int f=-1;
					for (int t=0; (t<NUMSOCKETS)&&(f==-1); t++) {
						if (netsockfd[t]==-1) f = t;
					}
					if (f==-1) {
						error->q(ERROR_FREENET);
						stack->pushint(0);
					} else {
						stack->pushint(f);
					}
				}
				break;

				case OP_FREEDB: {
					// return the next free databsae number - throw error if none free
					int f=-1;
					for (int t=0; (t<NUMDBCONN)&&(f==-1); t++) {
						QString dbconnection = "DBCONNECTION" + QString::number(t);
						QSqlDatabase db = QSqlDatabase::database(dbconnection);
						if (!db.isValid()) f = t;
					}
					if (f==-1) {
						error->q(ERROR_FREEDB);
						stack->pushint(0);
					} else {
						stack->pushint(f);
					}
				}
				break;

				case OP_FREEDBSET: {
					// return the next free set for a database - throw error if none free
					int n = stack->popint();
					int f=-1;
					if (n<0||n>=NUMDBCONN) {
						error->q(ERROR_DBCONNNUMBER);
						stack->pushint(0);
					} else {
						for (int t=0; (t<NUMDBSET)&&(f==-1); t++) {
							if (!dbSet[n][t]) f = t;
						}
						if (f==-1) {
							error->q(ERROR_FREEDBSET);
							stack->pushint(0);
						} else {
							stack->pushint(f);
						}
					}
				}
				break;

				case OP_ARC:
				case OP_CHORD:
				case OP_PIE: {
					int yval, xval, hval, wval;
					int arg = stack->popint(); // number of arguments
					double angwval = stack->popfloat();
					double startval = stack->popfloat();

					if(arg==5){
						int rval = stack->popint();
						yval = stack->popint() - rval;
						xval = stack->popint() - rval;
						hval = rval * 2;
						wval = rval * 2;
					}else{
						hval = stack->popint();
						wval = stack->popint();
						yval = stack->popint();
						xval = stack->popint();
					}

					// degrees * 16
					int s = (int) (startval * 360 * 16 / 2 / M_PI);
					int aw = (int) (angwval * 360 * 16 / 2 / M_PI);
					// transform to clockwise from 12'oclock
					s = 1440-s-aw;

					QPainter *ian;
					if (printing) {
						ian = printdocumentpainter;
					} else {
						ian = new QPainter(graphwin->image);
					}

					ian->setPen(drawingpen);
					ian->setBrush(drawingbrush);
                    if (CompositionModeClear) {
						ian->setCompositionMode(QPainter::CompositionMode_Clear);
					}
					if(opcode==OP_ARC) {
						ian->drawArc(xval, yval, wval, hval, s, aw);
					}
					if(opcode==OP_CHORD) {
						ian->drawChord(xval, yval, wval, hval, s, aw);
					}
					if(opcode==OP_PIE) {
						ian->drawPie(xval, yval, wval, hval, s, aw);
					}

					if (!printing) {
						ian->end();
						delete ian;
						if (!fastgraphics) waitForGraphics();
					}
				}
				break;

				case OP_PENWIDTH: {
					double a = stack->popint();
					if (a<0) {
						error->q(ERROR_PENWIDTH);
					} else {
						drawingpen.setWidth(a);
						if (a==0) {
							drawingpen.setStyle(Qt::NoPen);
						} else {
							drawingpen.setStyle(Qt::SolidLine);
						}
					}
				}
				break;

				case OP_GETPENWIDTH: {
					stack->pushint((double) (drawingpen.width()));
				}
				break;

				case OP_GETBRUSHCOLOR: {
					stack->pushlong((unsigned long) drawingbrush.color().rgba());
				}
				break;

				case OP_ALERT: {
					QString temp = stack->popstring();
					mymutex->lock();
					emit(dialogAlert(temp));
					waitCond->wait(mymutex);
					mymutex->unlock();
					waitForGraphics();
				}
				break;

				case OP_CONFIRM: {
					int dflt = stack->popint();
					QString temp = stack->popstring();
					mymutex->lock();
					emit(dialogConfirm(temp,dflt));
					waitCond->wait(mymutex);
					mymutex->unlock();
					stack->pushint(returnInt);
					waitForGraphics();
				}
				break;

				case OP_PROMPT: {
					QString dflt = stack->popstring();
					QString msg = stack->popstring();
					mymutex->lock();
					emit(dialogPrompt(msg,dflt));
					waitCond->wait(mymutex);
					mymutex->unlock();
					stack->pushstring(returnString);
					waitForGraphics();
				}
				break;

				case OP_FROMRADIX: {
					bool ok;
					unsigned long dec;
					int base = stack->popint();
					QString n = stack->popstring();
					if (base>=2 && base <=36) {
						dec = n.toULong(&ok, base);
						if (ok) {
							stack->pushlong(dec);
						} else {
							error->q(ERROR_RADIXSTRING);
							stack->pushlong(0);
						}
					} else {
						error->q(ERROR_RADIX);
						stack->pushlong(0);
					}
				}
				break;

				case OP_TORADIX: {
					int base = stack->popint();
					unsigned long n = stack->poplong();
					if (base>=2 && base <=36) {
						QString out;
						out.setNum(n, base);
						stack->pushstring(out);
					} else {
						error->q(ERROR_RADIX);
						stack->pushint(0);
					}
				}
				break;

				case OP_PRINTEROFF: {
					if (printing) {
						printing = false;
						printdocumentpainter->end();
						delete printdocumentpainter;
						delete printdocument;
					} else {
						error->q(ERROR_PRINTERNOTON);
					}
				}
				break;

				case OP_PRINTERON: {
					if (printing) {
						error->q(ERROR_PRINTERNOTOFF);
					} else {
                        SETTINGS;
						int resolution = settings.value(SETTINGSPRINTERRESOLUTION, SETTINGSPRINTERRESOLUTIONDEFAULT).toInt();
						int printer = settings.value(SETTINGSPRINTERPRINTER, 0).toInt();
						int paper = settings.value(SETTINGSPRINTERPAPER, SETTINGSPRINTERPAPERDEFAULT).toInt();
						if (printer==-1) {
							// pdf printer
							printdocument = new QPrinter((QPrinter::PrinterMode) resolution);
							printdocument->setOutputFormat(QPrinter::PdfFormat);
							printdocument->setOutputFileName(settings.value(SETTINGSPRINTERPDFFILE, "./output.pdf").toString());

						} else {
							// system printer
							QList<QPrinterInfo> printerList=QPrinterInfo::availablePrinters();
							if (printer>=printerList.count()) printer = 0;
							printdocument = new QPrinter(printerList[printer], (QPrinter::PrinterMode) resolution);
						}
						if (printdocument) {
							printdocument->setPaperSize((QPrinter::PaperSize) paper);
							printdocument->setOrientation((QPrinter::Orientation)settings.value(SETTINGSPRINTERORIENT, SETTINGSPRINTERORIENTDEFAULT).toInt());
							printdocumentpainter = new QPainter();
							if (!printdocumentpainter->begin(printdocument)) {
								error->q(ERROR_PRINTEROPEN);
							} else {
								printing = true;
							}
						} else {
							error->q(99999);
						}
					}
				}
				break;

				case OP_PRINTERPAGE: {
					if (printing) {
						printdocument->newPage();
					} else {
						error->q(ERROR_PRINTERNOTON);
					}
				}
				break;

				case OP_PRINTERCANCEL: {
					if (printing) {
						printing = false;
						printdocumentpainter->end();
						delete printdocumentpainter;
						printdocument->abort();
						delete printdocument;
					} else {
						error->q(ERROR_PRINTERNOTON);
					}
				}
				break;

				case OP_DEBUGINFO: {
					// get info about BASIC256 runtime and return as a string
					// put totally undocumented stuff HERE
					// NOT FOR HUMANS TO USE
					int what = stack->popint();
					switch (what) {
						case 1:
							// stack height and content
							mymutex->lock();
							emit(outputReady(stack->debug()));
							waitCond->wait(mymutex);
							mymutex->unlock();
							stack->pushint(stack->height());
							break;
						case 2:
							// type of top stack element
							stack->pushint(stack->peekType());
							break;
						case 3:
							// number of symbols - display them to output area
							{
								for(int i=0; i<numsyms; i++) {
									mymutex->lock();
									emit(outputReady(QString("SYM %1 %2 LOC %3\n").arg(i).arg(symtable[i],-32).arg(symtableaddress[i],8,16,QChar('0'))));
									waitCond->wait(mymutex);
									mymutex->unlock();
								}
								stack->pushint(numsyms);
							}
							break;
						case 4:
							// dump the program object code
							{
								int *o = wordCode;
								while (o <= wordCode + wordOffset) {
									mymutex->lock();
									unsigned int offset = o-wordCode;
									int currentop = *o;
									o++;
									if (optype(currentop) == OPTYPE_NONE)	{
										emit(outputReady(QString("%1 %2\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20)));
									} else if (optype(currentop) == OPTYPE_INT) {
										//op has one Int arg
										emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o)));
										o++;
									} else if (optype(currentop) == OPTYPE_VARIABLE) {
										//op has one Int arg
										emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg(symtable[(int) *o])));
										o++;
									} else if (optype(currentop) == OPTYPE_LABEL) {
										//op has one Int arg (label - lookup the address from the symtableaddress
										emit(outputReady(QString("%1 %2 %3 %4\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg(symtableaddress[(int) *o],8,16,QChar('0')).arg(symtable[(int) *o])));
										o++;
									} else if (optype(currentop) == OPTYPE_FLOAT) {
										// op has a single double arg
										emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((double) *o)));
										o += bytesToFullWords(sizeof(double));
									} else if (optype(currentop) == OPTYPE_STRING) {
										// op has a single null terminated String arg
										emit(outputReady(QString("%1 %2 \"%3\"\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((char*) o)));
										int len = bytesToFullWords(strlen((char*) o) + 1);
										o += len;
									}
									waitCond->wait(mymutex);
									mymutex->unlock();
								}
							}
							stack->pushint(0);
							break;
						case 5:
							// dump the variables
							{
								mymutex->lock();
								emit(outputReady(variables->debug()));
								waitCond->wait(mymutex);
								mymutex->unlock();
							}
							stack->pushint(0);
							break;
						case 6:
							// size of int
							stack->pushint(sizeof(int));
							break;
						case 7:
							// size of long
							stack->pushint(sizeof(long));
							break;
						case 8:
							// size of long long
							stack->pushint(sizeof(long long));
							break;

						default:
							stack->pushstring("");
					}
				}
				break;

				case OP_STACKSWAP: {
					// swap the top of the stack
					// 0, 1, 2, 3...  becomes 1, 0, 2, 3...
					stack->swap();
				}
				break;

				case OP_STACKSWAP2: {
					// swap the top two pairs of the stack
					// 0, 1, 2, 3...  becomes 2,3, 0,1...
					stack->swap2();
                }
				break;

				case OP_STACKDUP: {
					// duplicate top stack entry
					stack->dup();
                }
				break;

				case OP_STACKDUP2: {
					// duplicate top 2 stack entries
					stack->dup2();
                }
				break;

				case OP_STACKTOPTO2: {
					// move the top of the stack under the next two
					// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
					stack->topto2();
                }
				break;

				case OP_THROWERROR: {
					// Throw a user defined error number
					int fn = stack->popint();
					error->q(fn);
				}
				break;

				case OP_WAVLENGTH: {
					double d = 0.0;
					if (mediaplayer) {
						if ((d = mediaplayer->length())==0)
							error->q(WARNING_WAVNODURATION);
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
					stack->pushfloat(d);
				}
				break;

				case OP_WAVPAUSE: {
					if (mediaplayer) {
						mediaplayer->pause();
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
				}
				break;

				case OP_WAVPOS: {
					double p = 0.0;
					if (mediaplayer) {
						p = mediaplayer->position();
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
					stack->pushfloat(p);
				}
				break;

				case OP_WAVSEEK: {
					double pos = stack->popfloat();
					if (mediaplayer) {
						if (!mediaplayer->seek(pos)) {
							error->q(WARNING_WAVNOTSEEKABLE);
						}
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
				}
				break;

				case OP_WAVSTATE: {
					int s = 0;
					if (mediaplayer) {
						s = mediaplayer->state();
					} else {
						error->q(ERROR_WAVNOTOPEN);
					}
					stack->pushint(s);
				}
				break;

				 case OP_TYPEOF: {
					// return type of expression (top of the stack)
					DataElement *e = stack->popelement();
					stack->pushint(e->type);
				}
				break;

				case OP_ISNUMERIC: {
					// return if data element is numeric
					DataElement *e = stack->popelement();
					stack->pushint(convert->isNumeric(e));
				}
				break;

				case OP_LTRIM: {
					QString s = stack->popstring();
					int l = s.length();
					int p = 0;
					while(p<l && s.at(p).isSpace()) {
						p++;
					}
					stack->pushstring(s.mid(p));
				}
				break;

				case OP_RTRIM: {
					QString s = stack->popstring();
					int l = s.length();
					int p = l;
					while(p>0 && s.at(p-1).isSpace()) {
						p--;
					}
					stack->pushstring(s.mid(0,p));
				}
				break;

				case OP_TRIM: {
					QString s = stack->popstring();
					stack->pushstring(s.trimmed());
				}
				break;

				case OP_IMPLODE_LIST: {
					QString coldelim = stack->popstring();
					QString rowdelim = stack->popstring();
					QString stuff = "";
					int rows = stack->popint();
					for (int row=0; row<rows; row++) {
						if (row!=0) stuff.prepend(rowdelim);
						int cols = stack->popint();
						for (int col=0; col<cols; col++) {
								if (col!=0) stuff.prepend(coldelim);
								stuff.prepend(stack->popstring());
						}
					}
					stack->pushstring(stuff);
				}
				break;
				
				case OP_SERIALIZE: {
					// rows,columns,typedata
					DataElement *e;
					QString stuff = "";
					int rows = stack->popint();
					int cols;
					for (int row=0; row<rows; row++) {
						if (row!=0) stuff.prepend(SERALIZE_DELIMITER);
						cols = stack->popint();
						for (int col=0; col<cols; col++) {
								if (col!=0) stuff.prepend(SERALIZE_DELIMITER);
								e = stack->popelement();
								switch (e->type) {
									case T_STRING:
										stuff.prepend(SERALIZE_STRING + QString::fromUtf8(e->stringval.toUtf8().toHex()) );
										break;
									case T_FLOAT:
										stuff.prepend(SERALIZE_FLOAT + QString::number(e->floatval));
										break;
									case T_INT:
										stuff.prepend(SERALIZE_INT + QString::number(e->intval));
										break;
									default:
										stuff.prepend(SERALIZE_UNASSIGNED);
										break;
								}
						}
					}
					stack->pushstring(QString::number(rows) + SERALIZE_DELIMITER + QString::number(cols) + SERALIZE_DELIMITER + stuff);
				}
				break;


				case OP_EXPLODE:
				case OP_EXPLODEX: {
					// unicode safe explode a string to a listoflists
					// pushed on the stack for use in assign and other places
					Qt::CaseSensitivity casesens = Qt::CaseSensitive;

					if (opcode!=OP_EXPLODEX) {
						// 0 sensitive (default) - opposite of QT
						casesens = (stack->popfloat()==0?Qt::CaseSensitive:Qt::CaseInsensitive);
					}
					QString qneedle = stack->popstring();
					QString qhaystack = stack->popstring();

					QStringList list;
					if(opcode==OP_EXPLODE) {
						list = qhaystack.split(qneedle, QString::KeepEmptyParts , casesens);
					} else {
						QRegExp expr = QRegExp(qneedle);
						expr.setMinimal(regexMinimal);
						if (expr.captureCount()>0) {
							// if we have captures in our regex then return them
							int pos = expr.indexIn(qhaystack);
							list = expr.capturedTexts();
						} else {
							// if it is a simple regex without captures then split
							list = qhaystack.split(expr, QString::KeepEmptyParts);
						}
					}
					// push 1d array to stack as a listoflists
					if (!error->pending()) {

						for(int y=0; y<list.size(); y++) {
							// fill the string array
							stack->pushstring(list.at(y));
						}
						stack->pushint(list.size());		// columns
						stack->pushint(1);				// rows
					}
				}
				break;

				case OP_UNSERIALIZE: {
					bool goodrows, goodcols;
					QString data = stack->popstring();
					QStringList list = data.split(SERALIZE_DELIMITER);
					if (list.count()>=3) {
						int rows = list[0].toInt(&goodrows);
						int cols = list[1].toInt(&goodcols);
						if (goodrows&&goodcols) {
							if (list.count()==rows*cols+2) {
								for (int row=0; row<rows&&!error->pending(); row++) {
									for (int col=0; col<cols&&!error->pending(); col++) {
										int i = row * cols + col + 2;
										switch (list[i].at(0).toLatin1()) {
											case SERALIZE_STRING:
												stack->pushstring(QString::fromUtf8(QByteArray::fromHex(list[i].mid(1).toUtf8()).data()));
												break;
											case SERALIZE_FLOAT:
												stack->pushfloat(list[i].mid(1).toDouble());
												break;
											case SERALIZE_INT:
												stack->pushint(list[i].mid(1).toLong());
												break;
											case SERALIZE_UNASSIGNED:
												stack->pushdataelement(NULL);
												break;
											default:
												error->q(ERROR_UNSERIALIZEFORMAT);
										}
									}
									stack->pushint(cols);
								}
								stack->pushint(rows);
							} else {
								error->q(ERROR_UNSERIALIZEFORMAT);
							}
						} else {
							error->q(ERROR_UNSERIALIZEFORMAT);
						}
					} else {
						error->q(ERROR_UNSERIALIZEFORMAT);
					}
				}
				break;

					// insert additional OPTYPE_NONE operations here



			}
		}
		break;

		default: {
			//emit(outputReady("optype=" + QString::number(optype(currentop)) + " op=" + QString::number(currentop,16) + "\n"));
			emit(outputReady(tr("Error in bytecode during label referencing at line ") + QString::number(currentLine) + ".\n"));
			return -1;
		}
		break;
	}

	return 0;
}
