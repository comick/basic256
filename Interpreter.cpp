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
#include "BasicEdit.h"


extern SoundSystem *sound;
extern QMutex *mymutex;
extern QMutex *mydebugmutex;
extern QWaitCondition *waitCond;
extern QWaitCondition *waitDebugCond;

extern BasicGraph * graphwin;
extern BasicEdit * editwin;


extern int lastKey;
extern std::list<int> pressedKeys;

extern "C" {
//extern int yydebug;
    extern int basicParse(char *);
    extern char* include_filenames[];   // filenames being LEXd
    extern int linenumber;              // linenumber being LEXd
    extern int column;                  // column on line being LEXd
    extern char* lexingfilename;        // current included file name being LEXd

    extern int numparsewarnings;
    extern int initializeBasicParse();
    extern void freeBasicParse();
    extern int bytesToFullWords(int size);
    extern int *wordCode;
    extern unsigned int wordOffset;
    extern unsigned int maxwordoffset;

    extern char **symtable;		// table of variables and labels (strings)
    extern int *symtableaddress;	// associated label address
    extern int *symtableaddresstype;	// associated address type
    extern int *symtableaddressargs;	// number of arguments expected by function/subroutine
    extern int numsyms;				// number of symbols

    // arrays to return warnings from compiler
    // defined in basicParse.y
    extern int parsewarningtable[];
    extern int parsewarningtablelinenumber[];
    extern int parsewarningtablecolumn[];
    extern int parsewarningtablelexingfilenumber[];
}

Interpreter::Interpreter(QLocale *applocale) {
    //yydebug = 1;

    fastgraphics = false;
    directorypointer=NULL;
    status = R_STOPPED;
    printing = false;
    sleeper = new Sleeper();
    error = new Error();
    locale = applocale;
    downloader = NULL;
    sys = NULL;

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
    delete downloader;
    delete sleeper;
    delete error;
}

#define optype(op) (OPTYPE_MASK & op) //faster
/*
int Interpreter::optype(int op) {
    // use constantants found in WordCodes.h
    return (OPTYPE_MASK & op) ;
}
*/

QString Interpreter::opname(int op) {
    // used to convert opcode number in debuginfo to opcode name

    switch (op) {
    case OP_END : return QString("OP_END");
    case OP_NOP : return QString("OP_NOP");
    case OP_RETURN : return QString("OP_RETURN");
    case OP_EQUAL : return QString("OP_EQUAL");
    case OP_NEQUAL : return QString("OP_NEQUAL");
    case OP_GT : return QString("OP_GT");
    case OP_LT : return QString("OP_LT");
    case OP_GTE : return QString("OP_GTE");
    case OP_LTE : return QString("OP_LTE");
    case OP_AND : return QString("OP_AND");
    case OP_NOT : return QString("OP_NOT");
    case OP_OR : return QString("OP_OR");
    case OP_XOR : return QString("OP_XOR");
    case OP_INT : return QString("OP_INT");
    case OP_STRING : return QString("OP_STRING");
    case OP_ADD : return QString("OP_ADD");
    case OP_SUB : return QString("OP_SUB");
    case OP_MUL : return QString("OP_MUL");
    case OP_DIV : return QString("OP_DIV");
    case OP_EX : return QString("OP_EX");
    case OP_NEGATE : return QString("OP_NEGATE");
    case OP_PRINT : return QString("OP_PRINT");
    case OP_PRINTN : return QString("OP_PRINTN");
    case OP_INPUT : return QString("OP_INPUT");
    case OP_KEY : return QString("OP_KEY");
    case OP_PLOT : return QString("OP_PLOT");
    case OP_RECT : return QString("OP_RECT");
    case OP_CIRCLE : return QString("OP_CIRCLE");
    case OP_LINE : return QString("OP_LINE");
    case OP_REFRESH : return QString("OP_REFRESH");
    case OP_FASTGRAPHICS : return QString("OP_FASTGRAPHICS");
    case OP_CLS : return QString("OP_CLS");
    case OP_CLG : return QString("OP_CLG");
    case OP_GRAPHSIZE : return QString("OP_GRAPHSIZE");
    case OP_GRAPHWIDTH : return QString("OP_GRAPHWIDTH");
    case OP_GRAPHHEIGHT : return QString("OP_GRAPHHEIGHT");
    case OP_SIN : return QString("OP_SIN");
    case OP_COS : return QString("OP_COS");
    case OP_TAN : return QString("OP_TAN");
    case OP_RAND : return QString("OP_RAND");
    case OP_CEIL : return QString("OP_CEIL");
    case OP_FLOOR : return QString("OP_FLOOR");
    case OP_ABS : return QString("OP_ABS");
    case OP_PAUSE : return QString("OP_PAUSE");
    case OP_LENGTH : return QString("OP_LENGTH");
    case OP_MID : return QString("OP_MID");
    case OP_INSTR : return QString("OP_INSTR");
    case OP_INSTRX : return QString("OP_INSTRX");
    case OP_OPEN : return QString("OP_OPEN");
    case OP_READ : return QString("OP_READ");
    case OP_WRITE : return QString("OP_WRITE");
    case OP_CLOSE : return QString("OP_CLOSE");
    case OP_RESET : return QString("OP_RESET");
    case OP_INCREASERECURSE : return QString("OP_INCREASERECURSE");
    case OP_DECREASERECURSE : return QString("OP_DECREASERECURSE");
    case OP_ASC : return QString("OP_ASC");
    case OP_CHR : return QString("OP_CHR");
    case OP_FLOAT : return QString("OP_FLOAT");
    case OP_READLINE : return QString("OP_READLINE");
    case OP_EOF : return QString("OP_EOF");
    case OP_MOD : return QString("OP_MOD");
    case OP_YEAR : return QString("OP_YEAR");
    case OP_MONTH : return QString("OP_MONTH");
    case OP_DAY : return QString("OP_DAY");
    case OP_HOUR : return QString("OP_HOUR");
    case OP_MINUTE : return QString("OP_MINUTE");
    case OP_SECOND : return QString("OP_SECOND");
    case OP_MOUSEX : return QString("OP_MOUSEX");
    case OP_MOUSEY : return QString("OP_MOUSEY");
    case OP_MOUSEB : return QString("OP_MOUSEB");
    case OP_CLICKCLEAR : return QString("OP_CLICKCLEAR");
    case OP_CLICKX : return QString("OP_CLICKX");
    case OP_CLICKY : return QString("OP_CLICKY");
    case OP_CLICKB : return QString("OP_CLICKB");
    case OP_TEXT : return QString("OP_TEXT");
    case OP_FONT : return QString("OP_FONT");
    case OP_SAY : return QString("OP_SAY");
    case OP_WAVPLAY : return QString("OP_WAVPLAY");
    case OP_WAVSTOP : return QString("OP_WAVSTOP");
    case OP_SEEK : return QString("OP_SEEK");
    case OP_SIZE : return QString("OP_SIZE");
    case OP_EXISTS : return QString("OP_EXISTS");
    case OP_LEFT : return QString("OP_LEFT");
    case OP_RIGHT : return QString("OP_RIGHT");
    case OP_UPPER : return QString("OP_UPPER");
    case OP_LOWER : return QString("OP_LOWER");
    case OP_SYSTEM : return QString("OP_SYSTEM");
    case OP_VOLUME : return QString("OP_VOLUME");
    case OP_SETCOLOR : return QString("OP_SETCOLOR");
    case OP_RGB : return QString("OP_RGB");
    case OP_PIXEL : return QString("OP_PIXEL");
    case OP_GETCOLOR : return QString("OP_GETCOLOR");
    case OP_ASIN : return QString("OP_ASIN");
    case OP_ACOS : return QString("OP_ACOS");
    case OP_ATAN : return QString("OP_ATAN");
    case OP_DEGREES : return QString("OP_DEGREES");
    case OP_RADIANS : return QString("OP_RADIANS");
    case OP_INTDIV : return QString("OP_INTDIV");
    case OP_LOG : return QString("OP_LOG");
    case OP_LOGTEN : return QString("OP_LOGTEN");
    case OP_GETSLICE : return QString("OP_GETSLICE");
    case OP_PUTSLICE : return QString("OP_PUTSLICE");
    case OP_IMGLOAD : return QString("OP_IMGLOAD");
    case OP_SQR : return QString("OP_SQR");
    case OP_EXP : return QString("OP_EXP");
    case OP_THROWERROR : return QString("OP_THROWERROR");
    case OP_READBYTE : return QString("OP_READBYTE");
    case OP_WRITEBYTE : return QString("OP_WRITEBYTE");
    case OP_STACKSWAP : return QString("OP_STACKSWAP");
    case OP_STACKTOPTO2 : return QString("OP_STACKTOPTO2");
    case OP_STACKDUP : return QString("OP_STACKDUP");
    case OP_STACKDUP2 : return QString("OP_STACKDUP2");
    case OP_STACKSWAP2 : return QString("OP_STACKSWAP2");
    case OP_STAMP_LIST : return QString("OP_STAMP_LIST");
    case OP_STAMP_S_LIST : return QString("OP_STAMP_S_LIST");
    case OP_STAMP_SR_LIST : return QString("OP_STAMP_SR_LIST");
    case OP_POLY_LIST : return QString("OP_POLY_LIST");
    case OP_WRITELINE : return QString("OP_WRITELINE");
    case OP_SOUND_LIST : return QString("OP_SOUND_LIST");
    case OP_SPRITEPOLY_LIST : return QString("OP_SPRITEPOLY_LIST");
    case OP_SPRITEDIM : return QString("OP_SPRITEDIM");
    case OP_SPRITELOAD : return QString("OP_SPRITELOAD");
    case OP_SPRITESLICE : return QString("OP_SPRITESLICE");
    case OP_SPRITEMOVE : return QString("OP_SPRITEMOVE");
    case OP_SPRITEHIDE : return QString("OP_SPRITEHIDE");
    case OP_SPRITESHOW : return QString("OP_SPRITESHOW");
    case OP_SPRITECOLLIDE : return QString("OP_SPRITECOLLIDE");
    case OP_SPRITEPLACE : return QString("OP_SPRITEPLACE");
    case OP_SPRITEX : return QString("OP_SPRITEX");
    case OP_SPRITEY : return QString("OP_SPRITEY");
    case OP_SPRITEH : return QString("OP_SPRITEH");
    case OP_SPRITEW : return QString("OP_SPRITEW");
    case OP_SPRITEV : return QString("OP_SPRITEV");
    case OP_CHANGEDIR : return QString("OP_CHANGEDIR");
    case OP_CURRENTDIR : return QString("OP_CURRENTDIR");
    case OP_WAVWAIT : return QString("OP_WAVWAIT");
    case OP_DBOPEN : return QString("OP_DBOPEN");
    case OP_DBCLOSE : return QString("OP_DBCLOSE");
    case OP_DBEXECUTE : return QString("OP_DBEXECUTE");
    case OP_DBOPENSET : return QString("OP_DBOPENSET");
    case OP_DBCLOSESET : return QString("OP_DBCLOSESET");
    case OP_DBROW : return QString("OP_DBROW");
    case OP_DBINT : return QString("OP_DBINT");
    case OP_DBFLOAT : return QString("OP_DBFLOAT");
    case OP_DBSTRING : return QString("OP_DBSTRING");
    case OP_LASTERROR : return QString("OP_LASTERROR");
    case OP_LASTERRORLINE : return QString("OP_LASTERRORLINE");
    case OP_LASTERRORMESSAGE : return QString("OP_LASTERRORMESSAGE");
    case OP_LASTERROREXTRA : return QString("OP_LASTERROREXTRA");
    case OP_OFFERROR : return QString("OP_OFFERROR");
    case OP_OFFERRORCATCH : return QString("OP_OFFERRORCATCH");
    case OP_NETLISTEN : return QString("OP_NETLISTEN");
    case OP_NETCONNECT : return QString("OP_NETCONNECT");
    case OP_NETREAD : return QString("OP_NETREAD");
    case OP_NETWRITE : return QString("OP_NETWRITE");
    case OP_NETCLOSE : return QString("OP_NETCLOSE");
    case OP_NETDATA : return QString("OP_NETDATA");
    case OP_NETADDRESS : return QString("OP_NETADDRESS");
    case OP_KILL : return QString("OP_KILL");
    case OP_MD5 : return QString("OP_MD5");
    case OP_SETSETTING : return QString("OP_SETSETTING");
    case OP_GETSETTING : return QString("OP_GETSETTING");
    case OP_PORTIN : return QString("OP_PORTIN");
    case OP_PORTOUT : return QString("OP_PORTOUT");
    case OP_BINARYOR : return QString("OP_BINARYOR");
    case OP_BINARYAND : return QString("OP_BINARYAND");
    case OP_BINARYNOT : return QString("OP_BINARYNOT");
    case OP_IMGSAVE : return QString("OP_IMGSAVE");
    case OP_DIR : return QString("OP_DIR");
    case OP_REPLACE : return QString("OP_REPLACE");
    case OP_REPLACEX : return QString("OP_REPLACEX");
    case OP_COUNT : return QString("OP_COUNT");
    case OP_COUNTX : return QString("OP_COUNTX");
    case OP_OSTYPE : return QString("OP_OSTYPE");
    case OP_MSEC : return QString("OP_MSEC");
    case OP_EDITVISIBLE : return QString("OP_EDITVISIBLE");
    case OP_GRAPHVISIBLE : return QString("OP_GRAPHVISIBLE");
    case OP_OUTPUTVISIBLE : return QString("OP_OUTPUTVISIBLE");
    case OP_TEXTWIDTH : return QString("OP_TEXTWIDTH");
    case OP_TEXTHEIGHT : return QString("OP_TEXTHEIGHT");
    case OP_SPRITER : return QString("OP_SPRITER");
    case OP_SPRITES : return QString("OP_SPRITES");
    case OP_FREEFILE : return QString("OP_FREEFILE");
    case OP_FREENET : return QString("OP_FREENET");
    case OP_FREEDB : return QString("OP_FREEDB");
    case OP_FREEDBSET : return QString("OP_FREEDBSET");
    case OP_DBNULL : return QString("OP_DBNULL");
    case OP_DBNULLS : return QString("OP_DBNULLS");
    case OP_ARC : return QString("OP_ARC");
    case OP_CHORD : return QString("OP_CHORD");
    case OP_PIE : return QString("OP_PIE");
    case OP_PENWIDTH : return QString("OP_PENWIDTH");
    case OP_GETPENWIDTH : return QString("OP_GETPENWIDTH");
    case OP_GETBRUSHCOLOR : return QString("OP_GETBRUSHCOLOR");
    case OP_ALERT : return QString("OP_ALERT");
    case OP_CONFIRM : return QString("OP_CONFIRM");
    case OP_PROMPT : return QString("OP_PROMPT");
    case OP_FROMRADIX : return QString("OP_FROMRADIX");
    case OP_TORADIX : return QString("OP_TORADIX");
    case OP_PRINTERON : return QString("OP_PRINTERON");
    case OP_PRINTEROFF : return QString("OP_PRINTEROFF");
    case OP_PRINTERPAGE : return QString("OP_PRINTERPAGE");
    case OP_PRINTERCANCEL : return QString("OP_PRINTERCANCEL");
    case OP_DEBUGINFO : return QString("OP_DEBUGINFO");
    case OP_WAVLENGTH : return QString("OP_WAVLENGTH");
    case OP_WAVPOS : return QString("OP_WAVPOS");
    case OP_WAVPAUSE : return QString("OP_WAVPAUSE");
    case OP_WAVSEEK : return QString("OP_WAVSEEK");
    case OP_WAVSTATE : return QString("OP_WAVSTATE");
    case OP_MIDX : return QString("OP_MIDX");
    case OP_REGEXMINIMAL : return QString("OP_REGEXMINIMAL");
    case OP_OPENSERIAL : return QString("OP_OPENSERIAL");
    case OP_TYPEOF : return QString("OP_TYPEOF");
    case OP_CONCATENATE : return QString("OP_CONCATENATE");
    case OP_ISNUMERIC : return QString("OP_ISNUMERIC");
    case OP_LTRIM : return QString("OP_LTRIM");
    case OP_RTRIM : return QString("OP_RTRIM");
    case OP_TRIM : return QString("OP_TRIM");
    case OP_KEYPRESSED : return QString("OP_KEYPRESSED");
    case OP_IMPLODE_LIST : return QString("OP_IMPLODE_LIST");
    case OP_EXPLODE : return QString("OP_EXPLODE");
    case OP_EXPLODEX : return QString("OP_EXPLODEX");
    case OP_SPRITEO : return QString("OP_SPRITEO");
    case OP_SERIALIZE : return QString("OP_SERIALIZE");
    case OP_UNSERIALIZE : return QString("OP_UNSERIALIZE");
    case OP_SEED : return QString("OP_SEED");
    case OP_SOUND : return QString("OP_SOUND");
    case OP_SOUNDPLAY : return QString("OP_SOUNDPLAY");
    case OP_SOUNDPLAY_LIST : return QString("OP_SOUNDPLAY_LIST");
    case OP_SOUNDLOAD : return QString("OP_SOUNDLOAD");
    case OP_SOUNDLOAD_LIST : return QString("OP_SOUNDLOAD_LIST");
    case OP_SOUNDLOADRAW : return QString("OP_SOUNDLOADRAW");
    case OP_SOUNDPAUSE : return QString("OP_SOUNDPAUSE");
    case OP_SOUNDSEEK : return QString("OP_SOUNDSEEK");
    case OP_SOUNDSTOP : return QString("OP_SOUNDSTOP");
    case OP_SOUNDWAIT : return QString("OP_SOUNDWAIT");
    case OP_SOUNDVOLUME : return QString("OP_SOUNDVOLUME");
    case OP_SOUNDPOSITION : return QString("OP_SOUNDPOSITION");
    case OP_SOUNDID : return QString("OP_SOUNDID");
    case OP_SOUNDSTATE : return QString("OP_SOUNDSTATE");
    case OP_SOUNDLENGTH : return QString("OP_SOUNDLENGTH");
    case OP_IMAGENEW : return QString("OP_IMAGENEW");
    case OP_IMAGELOAD : return QString("OP_IMAGELOAD");
    case OP_IMAGECOPY : return QString("OP_IMAGECOPY");
    case OP_IMAGECROP : return QString("OP_IMAGECROP");
    case OP_IMAGERESIZE : return QString("OP_IMAGERESIZE");
    case OP_IMAGESETPIXEL : return QString("OP_IMAGESETPIXEL");
    case OP_IMAGEDRAW : return QString("OP_IMAGEDRAW");
    case OP_IMAGECENTERED : return QString("OP_IMAGECENTERED");
    case OP_IMAGETRANSFORMED : return QString("OP_IMAGETRANSFORMED");
    case OP_IMAGEWIDTH : return QString("OP_IMAGEWIDTH");
    case OP_IMAGEHEIGHT : return QString("OP_IMAGEHEIGHT");
    case OP_IMAGEPIXEL : return QString("OP_IMAGEPIXEL");
    case OP_IMAGEFLIP : return QString("OP_IMAGEFLIP");
    case OP_IMAGEROTATE : return QString("OP_IMAGEROTATE");
    case OP_IMAGESMOOTH : return QString("OP_IMAGESMOOTH");
    case OP_UNLOAD : return QString("OP_UNLOAD");
    case OP_IMAGEAUTOCROP : return QString("OP_IMAGEAUTOCROP");
    case OP_SOUNDPLAYER : return QString("OP_SOUNDPLAYER");
    case OP_SOUNDPLAYER_LIST : return QString("OP_SOUNDPLAYER_LIST");
    case OP_SOUNDWAVEFORM : return QString("OP_SOUNDWAVEFORM");
    case OP_SOUNDWAVEFORM_LIST : return QString("OP_SOUNDWAVEFORM_LIST");
    case OP_SOUNDFADE : return QString("OP_SOUNDFADE");
    case OP_SOUNDLOOP : return QString("OP_SOUNDLOOP");
    case OP_SOUNDENVELOPE : return QString("OP_SOUNDENVELOPE");
    case OP_SOUNDENVELOPE_LIST : return QString("OP_SOUNDENVELOPE_LIST");
    case OP_SOUNDHARMONICS : return QString("OP_SOUNDHARMONICS");
    case OP_SOUNDNOHARMONICS : return QString("OP_SOUNDNOHARMONICS");
    case OP_SOUNDHARMONICS_LIST : return QString("OP_SOUNDHARMONICS_LIST");
    case OP_SOUNDPLAYEROFF : return QString("OP_SOUNDPLAYEROFF");
    case OP_SOUNDSYSTEM : return QString("OP_SOUNDSYSTEM");
    case OP_SOUNDSAMPLERATE : return QString("OP_SOUNDSAMPLERATE");
    case OP_GOTO : return QString("OP_GOTO");
    case OP_GOSUB : return QString("OP_GOSUB");
    case OP_BRANCH : return QString("OP_BRANCH");
    case OP_ONERRORGOSUB : return QString("OP_ONERRORGOSUB");
    case OP_ONERRORCALL : return QString("OP_ONERRORCALL");
    case OP_ONERRORCATCH : return QString("OP_ONERRORCATCH");
    case OP_EXITFOR : return QString("OP_EXITFOR");
    case OP_PUSHLABEL : return QString("OP_PUSHLABEL");
    case OP_CALLFUNCTION : return QString("OP_CALLFUNCTION");
    case OP_CALLSUBROUTINE : return QString("OP_CALLSUBROUTINE");
    case OP_ASSIGN : return QString("OP_ASSIGN");
    case OP_ARRAYASSIGN : return QString("OP_ARRAYASSIGN");
    case OP_PUSHVAR : return QString("OP_PUSHVAR");
    case OP_FOR : return QString("OP_FOR");
    case OP_NEXT : return QString("OP_NEXT");
    case OP_DIM : return QString("OP_DIM");
    case OP_DEREF : return QString("OP_DEREF");
    case OP_REDIM : return QString("OP_REDIM");
    case OP_ALEN : return QString("OP_ALEN");
    case OP_ALENROWS : return QString("OP_ALENROWS");
    case OP_ALENCOLS : return QString("OP_ALENCOLS");
    case OP_PUSHVARREF : return QString("OP_PUSHVARREF");
    case OP_ARRAY2STACK : return QString("OP_ARRAY2STACK");
    case OP_ARRAYFILL : return QString("OP_ARRAYFILL");
    case OP_GLOBAL : return QString("OP_GLOBAL");
    case OP_UNASSIGN : return QString("OP_UNASSIGN");
    case OP_UNASSIGNA : return QString("OP_UNASSIGNA");
    case OP_VARIABLEWATCH : return QString("OP_VARIABLEWATCH");
    case OP_VARIABLECOPY : return QString("OP_VARIABLECOPY");
    case OP_ASSIGNARRAY : return QString("OP_ASSIGNARRAY");
    case OP_PUSHVARARRAY : return QString("OP_PUSHVARARRAY");
    case OP_PUSHVARARRAYREF : return QString("OP_PUSHVARARRAYREF");
    case OP_PUSHINT : return QString("OP_PUSHINT");
    case OP_CURRLINE : return QString("OP_CURRLINE");
    case OP_ARRAYLISTASSIGN : return QString("OP_ARRAYLISTASSIGN");
    case OP_PUSHFLOAT : return QString("OP_PUSHFLOAT");
    case OP_PUSHSTRING : return QString("OP_PUSHSTRING");
    case OP_LAZYIFTRUE : return QString("OP_LAZYIFTRUE");
    case OP_LAZYIFFALSE : return QString("OP_LAZYIFFALSE");
    case OP_ADD1VAR : return QString("OP_ADD1VAR");
    case OP_ADD1ARRAY : return QString("OP_ADD1ARRAY");
    case OP_SUB1VAR : return QString("OP_SUB1VAR");
    case OP_SUB1ARRAY : return QString("OP_SUB1ARRAY");
    case OP_SETGRAPH : return QString("OP_SETGRAPH");
    case OP_TEXTBOXHEIGHT : return QString("OP_TEXTBOXHEIGHT");
    case OP_TEXTBOXWIDTH : return QString("OP_TEXTBOXWIDTH");
    case OP_ELLIPSE : return QString("OP_ELLIPSE");
    case OP_ROUNDEDRECT : return QString("OP_ROUNDEDRECT");

    default: return QString("OP_UNKNOWN");
    }
}

void Interpreter::printError() {
    QString msg;
    if (error->isFatal()) {
        msg = tr("ERROR");
    } else {
        msg = tr("WARNING");
    }
    if (includeFileNumber!=0) {
        msg += tr(" in included file '") + include_filenames[includeFileNumber] + QStringLiteral("'");
    }
    msg += tr(" on line ") + QString::number(error->line) + QStringLiteral(": ") + error->getErrorMessage(symtable);
    if (error->extra!="") msg+= " (" + error->extra + ")";
    msg += QStringLiteral(".\n");
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

bool Interpreter::isAwaitingInput() {
    return (status == R_INPUT);
}

void Interpreter::setInputString(QString s) {
    inputString = s;
}

bool Interpreter::isRunning() {
    return (status != R_STOPPED);
}

bool Interpreter::isStopped() {
    return (status == R_STOPPED);
}

bool Interpreter::isStopping() {
    // interpreter is stopped or is about to stop
    // to avoid RunController::stopRun() to be triggered while status == R_STOPING too
    return (status == R_STOPPED || status == R_STOPING);
}

void Interpreter::setStatus(run_status s) {
    status = s;
}

void Interpreter::watchvariable(bool doit, int i) {
    // send an event to the variable watch window to display a variable/array content
    if (doit) {
        emit(varWinAssign(&variables, i));
    }
}
void Interpreter::watchvariable(bool doit, int i, int x, int y) {
    // send an event to the variable watch window to display aan array element's value
    if (doit) {
        emit(varWinAssign(&variables, i, x ,y));
    }
}

void Interpreter::watchdecurse(bool doit) {
    // send an event to the variable watch window to remove a function's variables
    if (doit) {
        emit(varWinDropLevel(variables->getrecurse()));
    }
}

void Interpreter::decreaserecurse() {
    //clear current forstack
    while (forstack) {
        forframe *temp = forstack;
        forstack = temp->next;
        delete temp;
    }

    watchdecurse(debugMode);
    variables->decreaserecurse();

    //pop forstack from forstacklevel
    int recurse = variables->getrecurse();
    forstack = forstacklevel[recurse];

    //delete try/catch traps from non-existent recurse level
    while(trycatchstack && trycatchstack->recurseLevel > recurse){
        trycatchframe *temp_trycatchstack = trycatchstack;
        trycatchstack=trycatchstack->next;
        delete temp_trycatchstack;
    }
}


int Interpreter::compileProgram(char *code) {
    if (initializeBasicParse() != 0) {
        emit(outputError(tr("COMPILE ERROR") + QStringLiteral(": ") + tr("Out of memory") + QStringLiteral(".\n")));
        return -1;
    }

    int result = basicParse(code);
    //
    // display warnings from compile and free the lexing file name string
        bool gotowarning = (debugMode==0);
        // go to line only for first warning and only if program is not running in debugMode
        // because in debugMode it already call goToLine(1) at start
        for(int i=0; i<numparsewarnings; i++) {
        QString msg = tr("COMPILE WARNING");
        if (parsewarningtablelexingfilenumber!=0) {
            msg += tr(" in included file '") + QString(include_filenames[parsewarningtablelexingfilenumber[i]]) + QStringLiteral("'");
        } else if(gotowarning){
            emit(goToLine(parsewarningtablelinenumber[i]));
            gotowarning = false;
        }
        msg += tr(" on line ") + QString::number(parsewarningtablelinenumber[i]) + QStringLiteral(": ");
        switch(parsewarningtable[i]) {
            case COMPWARNING_MAXIMUMWARNINGS:
                msg += tr("The maximum number of compiler warnings have been displayed");
                break;
            case COMPWARNING_DEPRECATED_FORM:
                msg += tr("Statement format has been deprecated. It is recommended that you reauthor");
                break;
            case COMPWARNING_DEPRECATED_REF:
                msg += tr("You should use REF() when passing arguments, not in the SUBROUTINE/FUNCTION definition");
                break;

            default:
                msg += tr("Unknown compiler warning #") + QString::number(parsewarningtable[i]);
        }
        msg += QStringLiteral(".\n");
        emit(outputError(msg));
        //
    }
    //
    // now display fatal error if there is one
    bool gotoerror = true; // go to line only for first error in this file
    if (result != COMPERR_NONE)	{
        QString msg = tr("COMPILE ERROR");
        if (strlen(lexingfilename)!=0) {
            msg += tr(" in included file '") + QString(lexingfilename) + QStringLiteral("'");
        } else if(gotoerror){
            emit(goToLine(linenumber));
            gotoerror = false;
        }
        msg += tr(" on line ") + QString::number(linenumber) + QStringLiteral(": ");
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
                msg += tr("CATCH without matching TRY statement");
                break;
            case COMPERR_CATCHNOEND:
                msg += tr("CATCH without matching ENDTRY statement");
                break;
            case COMPERR_ENDTRY:
                msg += tr("ENDTRY without matching CATCH statement");
                break;
            case COMPERR_ENDBEGINCASE:
                msg += tr("CASE without matching BEGIN CASE statement");
                break;
            case COMPERR_ENDENDCASEBEGIN:
                msg += tr("END CASE without matching BEGIN CASE statement");
                break;
            case COMPERR_ENDENDCASE:
                msg += tr("END CASE without matching CASE statement");
                break;
            case COMPERR_BEGINCASENOEND:
                msg += tr("BEGIN CASE without matching END CASE statement");
                break;
            case COMPERR_CASENOEND:
                msg += tr("CASE without next CASE or matching END CASE statement");
                break;
            case COMPERR_LABELREDEFINED:
                msg += tr("Labels, functions and subroutines must have a unique name");
                break;
            case COMPERR_NEXTWRONGFOR:
                msg += tr("Variable in NEXT does not match FOR");
                break;
            case COMPERR_INCLUDEMAX:
                msg += tr("Maximum number of INCLUDE files");
                break;
            case COMPERR_INCLUDENOTALONE:
                msg += tr("INCLUDE must be placed in a separate line");
                break;
            case COMPERR_INCLUDENOFILE:
                msg += tr("No file specified for INCLUDE");
                break;
            case COMPERR_ONERRORCALL:
                msg += tr("Cannot pass arguments to a SUBROUTINE used by ONERROR statement");
                break;
            case COMPERR_NUMBERTOOLARGE:
                msg += tr("Number too large");
                break;

            default:
                if(column==0) {
                    msg += tr("Syntax error around beginning line");
                } else {
                    msg += tr("Syntax error around character ") + QString::number(column);
                }
        }
        msg += QStringLiteral(".\n");
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
    imageSmooth = false;
    op = wordCode;
    callstack = new addrStack();
    onerrorstack = new addrStack();
    trycatchstack = NULL;
    forstack = NULL;
    forstacklevelsize = 0;
    status = R_RUNNING;
    //initialize random
    double_random_max = (double) RAND_MAX * (double) RAND_MAX + (double) RAND_MAX + 1.0;
    currentLine = 1;
    includeFileNumber = 0;
    emit(mainWindowsResize(1, 300, 300));

    painter = new QPainter();
    painter_custom_font_flag = false;
    setGraph(""); //after mainWindowsResize
    defaultfontfamily = painter->font().family();
    defaultfontpointsize = painter->font().pointSize();
    defaultfontweight = painter->font().weight();
    defaultfontitalic = painter->font().italic();
    drawingpen = QPen(Qt::black); // default pen color
    drawingbrush = QBrush(Qt::black, Qt::SolidPattern); // default brush color
    painter_pen_color = Qt::black; //last color
    painter_brush_color = Qt::black; //last color
    CompositionModeClear = false;
    PenColorIsClear = false;
    fastgraphics = false;

    nsprites = 0;
    printing = false;
    regexMinimal = false;
    lastKey = 0;
    pressedKeys.clear();
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

    delete (downloader);
    downloader = NULL;
    delete(stack);

    //delete stack used by nested for statements
    int level = variables->getrecurse();
    forframe *temp_forstack;
    while(level>=0){
        while (forstack!=NULL) {
            temp_forstack = forstack;
            forstack = temp_forstack->next;
            delete(temp_forstack);
        }
        level--;
        if(level>0) forstack=forstacklevel[level];
    }

    variables->deleteLater();
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

    // close and delete painter
    if (painter) {
        if(painter->isActive()) painter->end();
        delete painter;
        painter=NULL;
    }
    if(printing){
        printdocument->abort(); //try to abort printing
        delete printdocument;
    }

    // close network connections
    netSockCloseAll();

    // remove any queued errors
    error->deq();

    //delete stack used by function calls, subroutine calls, and gosubs for return location
    delete callstack;

    //delete stack used to track nested on-error definitions
    delete onerrorstack;

    //delete stack used to track nested try/catch definitions
    trycatchframe *temp_trycatchstack;
    while (trycatchstack!=NULL) {
        temp_trycatchstack = trycatchstack;
        trycatchstack = temp_trycatchstack->next;
        delete(temp_trycatchstack);
    }

    //clear images
    QMap<QString, QImage*>::const_iterator it = images.constBegin();
    while (it != images.constEnd()) {
        delete(it.value());
        ++it;
    }
    images.clear();

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
    // force the interperter ops that block to go ahead and quit

    if(sys) sys->kill();

    // stop timers
    sleeper->wake();

    // stop downloading
    if(downloader!=NULL) downloader->stop();

    // stop playing any sound
    if(sound!=NULL) sound->exit();

    // close network connections
    netSockCloseAll();
}


void
Interpreter::run() {
    //read important settings from start
    SETTINGS;
    settingsDebugSpeed = settings.value(SETTINGSDEBUGSPEED, SETTINGSDEBUGSPEEDDEFAULT).toInt();
    settingsAllowSystem = settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toInt();
    settingsAllowSetting = settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool();
    settingsAllowPort = settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toInt();
    settingsSettingsAccess = settings.value(SETTINGSSETTINGSACCESS, SETTINGSSETTINGSACCESSDEFAULT).toInt();
    settingsSettingsMax = settings.value(SETTINGSSETTINGSMAX, SETTINGSSETTINGSMAXDEFAULT).toInt();
    programName.clear();

    settingsPrinterResolution = settings.value(SETTINGSPRINTERRESOLUTION, SETTINGSPRINTERRESOLUTIONDEFAULT).toInt();
    settingsPrinterPrinter = settings.value(SETTINGSPRINTERPRINTER, 0).toInt();
    settingsPrinterPaper = settings.value(SETTINGSPRINTERPAPER, SETTINGSPRINTERPAPERDEFAULT).toInt();
    settingsPrinterPdfFile = settings.value(SETTINGSPRINTERPDFFILE, "./output.pdf").toString();
    settingsPrinterOrient = settings.value(SETTINGSPRINTERORIENT, SETTINGSPRINTERORIENTDEFAULT).toInt();

    // main run loop
    isError=false;
    downloader = new BasicDownloader(error);
    mediaplayer_id_legacy = 0;
    //link sound system to error mechanism
    sound->error = &error;
    srand(time(NULL)+QTime::currentTime().msec()*911L); rand(); rand(); 	// initialize the random number generator for this thread
    runtimer.start(); // used by MSEC function
    while (status != R_STOPING && execByteCode() >= 0) {} //continue
    debugMode = 0;
    cleanup(); // cleanup the variables, databases, files, stack and everything
    emit(stopRunFinalized(!isError));
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
    QPainter *sprite_painter = new QPainter(scan);
    if(sprites[n1].r==0 && sprites[n1].s==1){
        sprite_painter->drawImage(sprites[n1].position.x()-rect.x(),sprites[n1].position.y()-rect.y(), *sprites[n1].image);
    }else{
        sprite_painter->drawImage(sprites[n1].position.x()-rect.x(),sprites[n1].position.y()-rect.y(), *sprites[n1].transformed_image);
    }
    sprite_painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
    if(sprites[n2].r==0 && sprites[n2].s==1){
        sprite_painter->drawImage(sprites[n2].position.x()-rect.x(),sprites[n2].position.y()-rect.y(), *sprites[n2].image);
    }else{
        sprite_painter->drawImage(sprites[n2].position.x()-rect.x(),sprites[n2].position.y()-rect.y(), *sprites[n2].transformed_image);
    }
    sprite_painter->end();
    delete sprite_painter;

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
    //    painter->drawImage(0,0,*scan);
    //    painter->end();

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

    QPainter *sprite_painter;
    QRegion region = QRegion(0,0,0,0);
    sprite_painter = new QPainter(graphwin->spritesimage);
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
    sprite_painter->setClipRegion(region);
    sprite_painter->setCompositionMode(QPainter::CompositionMode_Clear);
    sprite_painter->fillRect(region.boundingRect(),Qt::transparent);
    sprite_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    double lasto=1.0;
    for(int n=0;n<nsprites;n++){
        if(sprites[n].visible){
                if(lasto!=sprites[n].o){
                    lasto=sprites[n].o;
                    sprite_painter->setOpacity(lasto);
                }
                if(sprites[n].s==1 && sprites[n].r==0){
                    if(sprites[n].image){
                        if(graphwin->sprites_clip_region.intersects(sprites[n].position)){
                            sprite_painter->drawImage(sprites[n].position, *sprites[n].image);
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
                            sprite_painter->drawImage(sprites[n].position, *sprites[n].transformed_image);
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
    sprite_painter->end();
    delete sprite_painter;
    graphwin->draw_sprites_flag = flag;

}

void Interpreter::waitForGraphics() {
    update_sprite_screen();
    // wait for graphics operation to complete
    mymutex->lock();
    emit(goutputReady());
    waitCond->wait(mymutex);
    mymutex->unlock();
}

bool Interpreter::setPainterTo(QPaintDevice *destination) {
    drawingOnScreen = (destination == graphwin->image);
    if(painter->isActive()) painter->end();
    painter_pen_need_update=true;
    painter_brush_need_update=true;
    painter_font_need_update=painter_custom_font_flag; //need update only if there is a custom font loaded
    painter_last_compositionModeClear=false;
    return (painter->begin(destination));
}

void Interpreter::setGraph(QString id){
    if(id.isEmpty()){
        setPainterTo(graphwin->image);
        drawto = QString("");
    } else if(id.startsWith("image:")){
        if (images.contains(id)){
            setPainterTo(images[id]);
            drawto = id;
        }else{
            setPainterTo(graphwin->image);
            drawto = QString("");
            error->q(ERROR_IMAGERESOURCE);
        }
    }else{
        error->q(ERROR_INVALIDRESOURCE);
    }
}


int
Interpreter::execByteCode() {
    int opcode;

    // if errnum is set then handle the last thrown error
    if (error->pending()) {
        //change ERROR_VARNOTASSIGNED into ERROR_ARRAYELEMENT if variable implied is in fact an array element
        //still need this for functions like implode
        if(error->newe==ERROR_VARNOTASSIGNED && error->newvar>=0){
            DataElement *e = variables->getdata(error->newvar);
            if(e->type==T_ARRAY) error->newe=ERROR_ARRAYELEMENT;
        }
        if(error->newe==WARNING_VARNOTASSIGNED && error->newvar>=0){
            DataElement *e = variables->getdata(error->newvar);
            if(e->type==T_ARRAY) error->newe=WARNING_ARRAYELEMENT;
        }

        error->process(currentLine);

        if(trycatchstack) {
            // remove try/catch trap and jump to the catch label
            op = trycatchstack->catchAddr;
            //go back to the original recurse level of the try/catch trap
            while(trycatchstack->recurseLevel<variables->getrecurse()){
                decreaserecurse();
            }
            //clear the stack to original size
            if(stack->height()>trycatchstack->stackSize){
                stack->drop(stack->height()-trycatchstack->stackSize);
            }
            //delete trap
            trycatchframe *temp_trycatchstack = trycatchstack;
            trycatchstack=trycatchstack->next;
            delete temp_trycatchstack;
            return 0;
        }else if(onerrorstack->count() > 0) {
            //there is on-error defined
            // progess call to subroutine for error handling
            callstack->push(op);
            op = onerrorstack->peek();
            return 0;
        } else {
            isError=true;
            // no error handler defined or FATAL error - display message
            printError();
            // if error number less than the start of warnings then
            // highlight the current line AND die
            if (error->isFatal()) {
                if (includeFileNumber==0) emit(goToLine(error->line));
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
                    int *nextAddr = wordCode + stack->popint();
                    DataElement *stepE = stack->popelement();
                    DataElement *endE = stack->popelement();
                    DataElement *startE = stack->popelement();

                    // search for previous FOR in stack
                    // this is done because of anti-spaghetti code

                    forframe *temp = forstack;
                    forframe *prev = NULL;
                    while(temp && nextAddr!=temp->nextAddr){
                        prev=temp;
                        temp=temp->next;
                    }
                    if(temp){
                        //if there is a previous FOR in stack pull it to use the allocated memory
                        if(prev){
                            //node is in the middle of stack
                            prev->next=temp->next;
                        }else{
                            //node is last
                            forstack=temp->next;
                        }
                    }else{
                        //or create a new node
                        temp = new forframe;
                    }

                    bool goodloop;	// set to true if we should do the loop atleast once

                    variables->setdata(i, startE);	// set variable to initial value
                    watchvariable(debugMode, i);

                    if (startE->type==T_INT && stepE->type==T_INT) {
                        // an integer start and step (do an integer loop)
                        temp->useInt = true;
                        temp->intStart = startE->intval;
                        temp->intEnd = convert->getLong(endE);	// could b float but cant ever be
                        temp->intStep = stepE->intval;
                        temp->positive = temp->intStep >=0 && temp->intStart < temp->intEnd;
                        temp->negative = temp->intStep <=0 && temp->intStart > temp->intEnd;

                        goodloop = (temp->positive || temp->negative || temp->intStart == temp->intEnd);
                    } else {
                        // start or step not integer - it is a float loop
                        temp->useInt = false;
                        temp->floatStart = convert->getFloat(startE);
                        temp->floatEnd = convert->getFloat(endE);
                        temp->floatStep = convert->getFloat(stepE);
                        temp->positive = temp->floatStep >=0.0 && temp->floatStart < temp->floatEnd;
                        temp->negative = temp->floatStep <=0.0 && temp->floatStart > temp->floatEnd;

                        goodloop = (temp->positive || temp->negative || temp->floatStart == temp->floatEnd);
                    }

                    if (goodloop) {
                        // add new forframe to the forframe stack
                        temp->next = forstack;
                        temp->forAddr = op;		// first op after FOR statement to loop back to
                        temp->nextAddr = nextAddr;
                        forstack = temp;
                    } else {
                        // bad loop exit straight away - jump to the statement after the next
                        delete temp;
                        op = nextAddr;
                    }
                }
                break;

                case OP_NEXT: {
                    forframe *temp = forstack;
                    forframe *prev = NULL;

                    // search for FOR in stack
                    // this is done because of anti-spaghetti code
                    while(temp && op!=temp->nextAddr){
                        prev=temp;
                        temp=temp->next;
                    }

                    if (!temp) {
                        error->q(ERROR_NEXTNOFOR);
                    } else {
                            if (temp->useInt) {
                                const long val = convert->getLong(variables->getdata(i))+temp->intStep;
                                variables->setdata(i, val);
                                watchvariable(debugMode, i);

                                if (temp->positive) {
                                    if(val <= temp->intEnd){
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                } else if (temp->negative) {
                                    if(val >= temp->intEnd){
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                }else{
                                    if (val == temp->intEnd) {
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                }
                            } else {
                                const double val = convert->getFloat(variables->getdata(i))+temp->floatStep;
                                variables->setdata(i, val);


                                if (temp->positive) {
                                    if(convert->compareFloats(val, temp->floatEnd)!=1){
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                } else if (temp->negative) {
                                    if(convert->compareFloats(val, temp->floatEnd)!=-1){
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                }else{
                                    if (convert->compareFloats(val, temp->floatEnd)==0) {
                                        op = temp->forAddr;
                                    } else {
                                        if(prev){
                                            prev->next=temp->next;
                                        }else{
                                            forstack = temp->next;
                                        }
                                        delete temp;
                                    }
                                }
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
                    watchvariable(debugMode, i);
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
                        watchvariable(debugMode, i, xindex, yindex);
                    }
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
                    stack->pushdataelement(e);
                }
                break;

                case OP_PUSHVARARRAY: {
                    //just like OP_PUSHVAR but array is expected
                    //1) if [] are used incorrect
                    //2) when passing to subbroutine/function
                    DataElement *e = variables->getdata(i);
                    if (e->type==T_ARRAY) {
                        stack->pushdataelement(e);
                    }else {
                        error->q(ERROR_NOTARRAY, i);
                        stack->pushint(0);
                    }
                }
                break;

                case OP_PUSHVARREF: {
                    stack->pushvarref(i, variables->getrecurse());
                }
                break;

                case OP_PUSHVARARRAYREF: {
                    //just like OP_PUSHVARREF but array is expected
                    DataElement *e = variables->getdata(i);
                    if (e->type==T_ARRAY) {
                        stack->pushvarref(i, variables->getrecurse());
                    } else {
                        error->q(ERROR_NOTARRAY, i);
                        stack->pushint(0);
                    }
                }
                break;

                case OP_ASSIGN: {
                    DataElement *e = stack->popelement();
                    if (e->type==T_ARRAY) {
                        variables->copyarray(i, e); //copy entire array
                    } else {
                        variables->setdata(i, e);
                    }
                    watchvariable(debugMode, i);
                }
                break;

                case OP_ASSIGNARRAY: {
                    //just like OP_ASSIGN but array is expected
                    // assign an array (array expected when [] are used)
                    DataElement *e = stack->popelement();
                    if (e->type==T_ARRAY) {
                        variables->copyarray(i, e);
                        watchvariable(debugMode, i);
                    }else if (e->type==T_REF) {
                        //follow T_REF in case of passing array as reference to a function/subroutine
                        DataElement *ee = variables->getdata(e); //get the final content to ensure that this is an array
                        if(ee->type==T_ARRAY){
                            variables->setdata(i, e);
                            watchvariable(debugMode, i);
                        }else{
                            error->q(ERROR_EXPECTEDARRAY, i);//ERROR: Array expected
                        }
                    }else{
                        error->q(ERROR_EXPECTEDARRAY, i);//ERROR: Array expected
                    }
                }
                break;

                case OP_ARRAY2STACK: {
                    // Push all of the elements of an array to the stack and then push the length to the stack
                    // expects one integer - variable number
                    // all arrays are 2 dimensional - push each column, column size, then number of rows
                    int columns = variables->arraysizecols(i);
                    int rows = variables->arraysizerows(i);
                    Variable *v = variables->get(i);
                    if(!error->pending()){
                        for(int row = 0; row<rows; row++) {
                            for (int col = 0; col<columns; col++) {
                                int index=row*columns+col;
                                //direct access array's data
                                stack->pushdataelement(&v->arr->datavector[index]);
                            }
                            stack->pushint(columns);
                        }
                        stack->pushint(rows);
                    }else{
                        //0 rows, 0 columns if error
                        stack->pushint(0);
                        stack->pushint(0);
                    }
                }
                break;

                case OP_ARRAYFILL: {
                    // fill an array with a single value
                    int mode = stack->popint();		// 1-fill everything, 0-fill unassigned
                    DataElement *e = stack->popelement();	// fill value

                    if (e->type==T_UNASSIGNED) {
                        error->q(ERROR_VARNOTASSIGNED, e->intval);
                    } else if (e->type==T_ARRAY) {
                        error->q(ERROR_ARRAYINDEXMISSING, e->intval);
                    } else {
                        Variable *v = variables->get(i);
                        if (!error->pending() && v->data.type==T_ARRAY) {
                            int index=v->arr->xdim*v->arr->ydim;
                            //direct access to array's data
                            if(mode==1) {
                                while(index-- > 0) v->arr->datavector[index].copy(e, i);
                            }else{
                                while(index-- > 0) {
                                    if(v->arr->datavector[index].type == T_UNASSIGNED) {
                                        v->arr->datavector[index].copy(e, i);
                                    }
                                }
                            }
                         } else {
                            // trying to fill a regular variable - just do an assign
                            variables->setdata(i, e);
                        }
                        watchvariable(debugMode, i);
                    }
                }
                break;

                case OP_ARRAYLISTASSIGN: {
                    const int rows = stack->popint();
                    const int columns = stack->popint(); //pop the first row length - the following rows must have the same length
                    int columns2 = columns;
                    Variable *v = variables->get(i);

                    // create array if we need to (wrong dimensions or not array)
                    if (v->data.type != T_ARRAY || !v->arr || v->arr->xdim!=columns || v->arr->ydim!=rows) {
                        variables->arraydim(i, rows, columns, false);
                    }

                    int index=rows*columns;
                    for(int row = rows-1; row>=0; row--) {
                        //pop row length only if is not first row - already popped
                        if(row != rows-1)
                            columns2=stack->popint();
                            if(columns2!=columns){
                                error->q(ERROR_ARRAYNITEMS, i);
                                // empty stack to successfully pass over an OnError situation
                                stack->drop(columns2);
                                for(row--; row>=0 ; row--) stack->drop(stack->popint());
                                break;
                            }
                            int col=columns;
                            while(col-- > 0) {
                                DataElement *e = stack->popelement(); //continue to pull from stack even if error occured
                                if (e->type!=T_ARRAY) {
                                    //direct access to array's data
                                    v->arr->datavector[--index].copy(e, i);
                                } else {
                                    index--;
                                    error->q(ERROR_ARRAYINDEXMISSING, e->intval);
                                    variables->arrayunassign(i, col, row);
                               }
                         }
                    }
                    watchvariable(debugMode, i);
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
                    watchvariable(true, i);
                }
                break;

                case OP_ADD1VAR: {
                    //safe increment of variable
                    DataElement *e = variables->getdata(i);
                    if (e->type==T_INT){
                        if(e->intval==LONG_MAX){
                            e->type=T_FLOAT;
                            e->floatval=(double)e->intval + 1.0;
                        }else{
                            e->intval++;
                        }
                        watchvariable(debugMode, i);
                        break;
                    }else if(e->type==T_FLOAT){
                        e->floatval += 1.0;
                        if (std::isinf(e->floatval)) {
                            error->q(ERROR_INFINITY);
                            e->floatval=0.0;
                        }
                        watchvariable(debugMode, i);
                        break;
                    }
                    double f = convert->getFloat(e);
                    f += 1.0;
                    if (std::isinf(f)) {
                        error->q(ERROR_INFINITY);
                    }else{
                        e->type=T_FLOAT;
                        e->floatval=f;
                    }
                    watchvariable(debugMode, i);
                }
                break;

                case OP_ADD1ARRAY:{
                    //safe increment of array's element
                    int yindex = stack->popint();
                    int xindex = stack->popint();
                    DataElement *e = variables->arraygetdata(i, xindex, yindex);
                    if (e->type==T_INT){
                        if(e->intval==LONG_MAX){
                            e->type=T_FLOAT;
                            e->floatval=(double)e->intval + 1.0;
                        }else{
                            e->intval++;
                        }
                        watchvariable(debugMode, i, xindex, yindex);
                        break;
                    }else if(e->type==T_FLOAT){
                        e->floatval += 1.0;
                        if (std::isinf(e->floatval)) {
                            error->q(ERROR_INFINITY);
                            e->floatval=0.0;
                        }
                        watchvariable(debugMode, i, xindex, yindex);
                        break;
                    }
                    double f = convert->getFloat(e);
                    f += 1.0;
                    if (std::isinf(f)) {
                        error->q(ERROR_INFINITY);
                    }else{
                        e->type=T_FLOAT;
                        e->floatval=f;
                    }
                    watchvariable(debugMode, i, xindex, yindex);
                }
                break;

                case OP_SUB1VAR: {
                    //safe decrement of variable
                    DataElement *e = variables->getdata(i);
                   if (e->type==T_INT){
                        if(e->intval==LONG_MIN){
                            e->type=T_FLOAT;
                            e->floatval=(double)e->intval - 1.0;
                        }else{
                            e->intval--;
                        }
                        watchvariable(debugMode, i);
                        break;
                    }else if(e->type==T_FLOAT){
                        e->floatval -= 1.0;
                        if (std::isinf(e->floatval)) {
                            error->q(ERROR_INFINITY);
                            e->floatval=0.0;
                        }
                        watchvariable(debugMode, i);
                        break;
                    }
                    double f = convert->getFloat(e);
                    f -= 1.0;
                    if (std::isinf(f)) {
                        error->q(ERROR_INFINITY);
                    }else{
                        e->type=T_FLOAT;
                        e->floatval=f;
                    }
                    watchvariable(debugMode, i);
                }
                break;

                case OP_SUB1ARRAY: {
                    //safe decrement of array's element
                    int yindex = stack->popint();
                    int xindex = stack->popint();
                    DataElement *e = variables->arraygetdata(i, xindex, yindex);
                    if (e->type==T_INT){
                        if(e->intval==LONG_MIN){
                            e->type=T_FLOAT;
                            e->floatval=(double)e->intval - 1.0;
                        }else{
                            e->intval--;
                        }
                        watchvariable(debugMode, i, xindex, yindex);
                        break;
                    }else if(e->type==T_FLOAT){
                        e->floatval -= 1.0;
                        if (std::isinf(e->floatval)) {
                            error->q(ERROR_INFINITY);
                            e->floatval=0.0;
                        }
                        watchvariable(debugMode, i, xindex, yindex);
                        break;
                    }
                    double f = convert->getFloat(e);
                    f -= 1.0;
                    if (std::isinf(f)) {
                        error->q(ERROR_INFINITY);
                    }else{
                        e->type=T_FLOAT;
                        e->floatval=f;
                    }
                    watchvariable(debugMode, i, xindex, yindex);
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
                    // opcode currentline is compound and includes file number (ID) and line number
                    //includeFileNumber = i / 0x1000000;
                    //currentLine = i % 0x1000000;
                    includeFileNumber = i >> 24;
                    currentLine = i & 0xffffff;

                    if (debugMode != 0) {
                        if (includeFileNumber==0) {
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
                                sleeper->sleepMS(settingsDebugSpeed);
                            }
                        }
                    }
                }
                break;

                case OP_PUSHINT: {
                    stack->pushint(i);
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
            int i = symtableaddress[l]; // address
            op++;

            // if address is -1 then this is not a function, subroutine or label
            if (i < 0) {
                //chose the proper error code if there is no valid address for this label/function/subroutine
                switch (opcode) {
                case OP_CALLFUNCTION:
                    error->q(ERROR_NOSUCHFUNCTION, l);
                    break;
                case OP_CALLSUBROUTINE:
                    error->q(ERROR_NOSUCHSUBROUTINE, l);
                    break;
                default:
                    error->q(ERROR_NOSUCHLABEL, l);
                    break;
                }
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

                case OP_LAZYIFFALSE: {
                    bool val = stack->popbool();
                    if (!val) { // jump on false but push false on stack // Eg. "false AND expr"
                        stack->pushbool(val);
                        op = wordCode + i;
                    }
                }
                break;

                case OP_LAZYIFTRUE: {
                    bool val = stack->popbool();
                    if (val) { // jump on true but push true on stack // Eg. "true OR expr"
                        stack->pushbool(val);
                        op = wordCode + i;
                    }
                }
                break;

                case OP_GOSUB: {
                    if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL){
                        error->q(ERROR_NOSUCHLABEL, l);
                    }else{
                        // setup return
                        callstack->push(op);
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;

                case OP_CALLFUNCTION: {
                    //OP_CALLFUNCTION is used when program expect the result to be pushed on stack
                    int a = stack->popint(); // number of arguments pushed on stack
                    if(symtableaddresstype[l]!=ADDRESSTYPE_FUNCTION){
                        error->q(ERROR_NOSUCHFUNCTION, l);
                    }else if(symtableaddressargs[l]!=a){
                        //the number of arguments passed does not match FUNCTION definition
                        error->q(ERROR_ARGUMENTCOUNT);
                    }else{
                        // setup return
                        callstack->push(op);
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;


                case OP_CALLSUBROUTINE: {
                //OP_CALLSUBROUTINE is used to call subroutines
                    int a = stack->popint(); // number of arguments pushed on stack
                    if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
                        error->q(ERROR_NOSUCHSUBROUTINE, l);
                    }else if(symtableaddressargs[l]!=a){
                        //the number of arguments passed does not match SUBROUTINE definition
                        error->q(ERROR_ARGUMENTCOUNT);
                    }else{
                        // setup return
                        callstack->push(op);
                        // do jump
                        op = wordCode + i;
                    }
                }
                break;

                case OP_ONERRORGOSUB: {
                    if(symtableaddresstype[l]==ADDRESSTYPE_LABEL){
                        // push onerror address
                        onerrorstack->push(wordCode + i);
                    }else{
                        error->q(ERROR_NOSUCHLABEL, l);
                    }
                }
                break;

                case OP_ONERRORCALL: {
                    if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
                        error->q(ERROR_NOSUCHSUBROUTINE, l);
                    }else if(symtableaddressargs[l]!=0){
                        error->q(ERROR_ONERRORSUB, l);
                    }else{
                        // push onerror address
                        onerrorstack->push(wordCode + i);
                    }
                }
                break;

                case OP_ONERRORCATCH: {
                    // setup try/catch trap
                    // label used is an internal generated label (is safe without checking it)
                    trycatchframe *temp = new trycatchframe;
                    temp->catchAddr = wordCode + i;
                    temp->recurseLevel = variables->getrecurse();
                    temp->stackSize = stack->height();
                    temp->next = trycatchstack;
                    trycatchstack = temp;
                }
                break;

                case OP_OFFERRORCATCH: {
                    // no error in try/catch trap
                    // delete the trap from the try/catch stack and jump over the CATCH part

                    //  search if there is a corresponding trap in stack
                    //  (search by catchAddr which must be the same with current *op)
                    int recurse = variables->getrecurse();
                    trycatchframe *temp_trycatchstack = trycatchstack;
                    while (temp_trycatchstack!=NULL){
                        if(temp_trycatchstack->catchAddr==op && recurse==temp_trycatchstack->recurseLevel){
                            //  we found it - delete all nested traps inside it
                            trycatchframe *temp;
                            do{
                                temp = trycatchstack;
                                trycatchstack=trycatchstack->next;
                                delete(temp);
                            }while(temp_trycatchstack!=temp);
                            break;
                        }
                        temp_trycatchstack = temp_trycatchstack->next;
                    }
                    // do jump to the specified address
                    op = wordCode + i;
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
                    int* addr = callstack->pop();
                    if (addr) {
                        op=addr;
                    } else {
                        error->q(ERROR_UNEXPECTEDRETURN);
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
                        stack->pushstring("");
                    } else {

                        if (filehandle[fn] == NULL) {
                            error->q(ERROR_FILENOTOPEN);
                            stack->pushstring("");
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
                    int size = 0;
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
                                    size = filehandle[fn]->size();
                                    break;
                                case 2:
                                    // serial
                                    QCoreApplication::processEvents();
                                    size = filehandle[fn]->bytesAvailable();
                                    break;
                            }
                        }
                    }
                    stack->pushint(size);
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
                        stack->pushstring(QString(""));
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
                            if (val<-1.0 || val>1.0) {
                                error->q(ERROR_ASINACOSRANGE);
                                stack->pushint(0);
                            } else {
                                stack->pushfloat(asin(val));
                            }
                            break;
                        case OP_ACOS:
                            if (val<-1.0 || val>1.0) {
                                error->q(ERROR_ASINACOSRANGE);
                                stack->pushint(0);
                            } else {
                                stack->pushfloat(acos(val));
                            }
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
                            stack->pushfloat(val * 180.0 / M_PI);
                            break;
                        case OP_RADIANS:
                            stack->pushfloat(val * M_PI / 180.0);
                            break;
                        case OP_LOG:
                            if (val<=0.0) {
                                error->q(ERROR_LOGRANGE);
                                stack->pushint(0);
                            } else {
                                stack->pushfloat(log(val));
                            }
                            break;
                        case OP_LOGTEN:
                            if (val<=0.0) {
                                error->q(ERROR_LOGRANGE);
                                stack->pushint(0);
                            } else {
                                stack->pushfloat(log10(val));
                            }
                            break;
                        case OP_SQR:
                            if (val<0.0) {
                                error->q(ERROR_SQRRANGE);
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
                        QString final = stwo + sone;
                        if (final.length()>STRINGMAXLEN) {
                            error->q(ERROR_STRINGMAXLEN);
                            final.truncate(STRINGMAXLEN);
                        }
                        stack->pushstring(final);
                    }
                    break;

                case OP_ADD: {
                        // integer and float safe ADD operation
                        DataElement *one = stack->popelement();
                        DataElement *two = stack->popelement();
                        // add - if both integer then add as integers (if no ovverflow)
                        // else if both are numbers then convert and add as floats
                        // otherwise concatenate (string & number or strings)
                        if (one->type==T_INT){
                            if(two->type==T_INT) {
                                long a = two->intval + one->intval;
                                if((two->intval<=0||one->intval<=0||a>=0)&&(two->intval>=0||one->intval>=0||a<=0)) {
                                    // integer add - without overflow
                                    stack->pushlong(a);
                                }else{
                                    //overflow
                                    stack->pushfloat((double)(two->intval) + (double)(one->intval));
                                }
                                break;
                            }else if(two->type==T_FLOAT){
                                double ans = two->floatval + (double) one->intval;
                                if (std::isinf(ans)) {
                                    error->q(ERROR_INFINITY);
                                    ans = 0.0;
                                }
                                stack->pushfloat(ans);
                                break;
                            }
                        }else if(one->type==T_FLOAT){
                            if(two->type==T_FLOAT){
                                double ans = two->floatval + one->floatval;
                                if (std::isinf(ans)) {
                                    error->q(ERROR_INFINITY);
                                    ans = 0.0;
                                }
                                stack->pushfloat(ans);
                                break;
                            }else if(two->type==T_INT){
                                double ans = (double) two->intval + one->floatval;
                                if (std::isinf(ans)) {
                                    error->q(ERROR_INFINITY);
                                    ans = 0.0;
                                }
                                stack->pushfloat(ans);
                                break;
                            }
                        }

                        // concatenate (if one or both ar not numbers)
                        QString sone = convert->getString(one);
                        QString stwo = convert->getString(two);
                        QString final = stwo + sone;
                        if (final.length()>STRINGMAXLEN) {
                            error->q(ERROR_STRINGMAXLEN);
                            final.truncate(STRINGMAXLEN);
                        }
                        stack->pushstring(final);
                    }
                    break;

                case OP_SUB: {
                        // integer and float safe SUB operation
                        DataElement *one = stack->popelement();
                        DataElement *two = stack->popelement();
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
                            stack->pushfloat(0.0);
                        } else {
                            stack->pushfloat(ans);
                        }
                    }
                    break;

                case OP_MUL: {
                        // integer and float safe MUL operation
                        DataElement *one = stack->popelement();
                        DataElement *two = stack->popelement();
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
                            stack->pushfloat(0.0);
                        } else {
                            stack->pushfloat(ans);
                        }
                    }
                    break;

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
                    double ans = pow(twoval, oneval);
                    if (std::isinf(ans)) {
                        error->q(ERROR_INFINITY);
                        stack->pushfloat(0.0);
                    } else {
                        stack->pushfloat(ans);
                    }
                }
                break;

                case OP_DIV: {
                    // always return a float value with division "/"
                    double oneval = stack->popfloat();
                    double twoval = stack->popfloat();
                    if (oneval==0) {
                        error->q(ERROR_DIVZERO);
                        stack->pushfloat(0.0);
                    } else {
                        double ans = twoval / oneval;
                        if (std::isinf(ans)) {
                            error->q(ERROR_INFINITY);
                            stack->pushfloat(0.0);
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
                //lazy evaluation...
                //if first op is false then the OP_AND is skipped and false will be on stack
                //this is executed only if first expr is true (it is not on stack anymore)
                //this can be excluded but it is still necessary because we can find a non-boolean value on stack
                    //int one = stack->popbool();
                    //int two = stack->popbool();
                    //stack->pushbool(one && two);
                int val = stack->popbool();
                stack->pushbool(val);

                }
                break;

                case OP_OR: {
                //lazy evaluation...
                //if first op is tre then the OP_OR is skipped and true will be on stack
                //this is executed only if first expr is false (it is not on stack anymore)
                //this can be excluded but it is still necessary because we can find a non-boolean value on stack
                    //int one = stack->popbool();
                    //int two = stack->popbool();
                    //stack->pushbool(one || two);
                int val = stack->popbool();
                stack->pushbool(val);
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
                        if(e->intval<=LONG_MIN){
                            stack->pushfloat( (double)e->intval * -1.0);
                        }else{
                            stack->pushlong(e->intval * -1);
                        }
                    } else {
                        stack->pushfloat(convert->getFloat(e) * -1.0);
                    }
                }
                break;

                case OP_EQUAL:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans==0);
                }
                break;

                case OP_NEQUAL:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans!=0);
                }
                break;

                case OP_GT:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans==1);
                }
                break;

                case OP_LTE:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans!=1);
                }
                break;

                case OP_LT:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans==-1);
                }
                break;

                case OP_GTE:{
                    DataElement *two = stack->popelement();
                    DataElement *one = stack->popelement();
                    int ans = convert->compare(one,two);
                    stack->pushbool(ans!=-1);
                }
                break;

                case OP_SOUND:
                case OP_SOUNDPLAY:
                case OP_SOUNDPLAYER:
                case OP_SOUNDLOAD:
                {
                    DataElement *e = stack->popelement();
                    if (e->type == T_STRING) {
                        if(opcode==OP_SOUND || opcode==OP_SOUNDPLAY){
                            mymutex->lock();
                            emit(playSound(e->stringval, false));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            if(opcode==OP_SOUND) sound->wait(id);
                        }else if(opcode==OP_SOUNDPLAYER){
                            mymutex->lock();
                            emit(playSound(e->stringval, true));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            stack->pushint(id);
                        }else{
                            QString s = e->stringval;
                            QUrl url(s);
                            if(QFileInfo(s).exists()){
                                QFile file(s);
                                file.open(QIODevice::ReadOnly);
                                QByteArray arr = file.readAll();
                                file.close();
                                QString id = QString("sound:") + s;
                                mymutex->lock();
                                emit(loadSoundFromArray(id, &arr));
                                waitCond->wait(mymutex);
                                mymutex->unlock();
                                stack->pushstring(id);
                            }else if (url.isValid() && (url.scheme()=="http" || url.scheme()=="https" || url.scheme()=="ftp")){
                                downloader->download(QUrl::fromUserInput(s));
                                QByteArray arr = downloader->data();
                                QString id = QString("sound:") + s;
                                mymutex->lock();
                                emit(loadSoundFromArray(id, &arr));
                                waitCond->wait(mymutex);
                                mymutex->unlock();
                                stack->pushstring(id);
                            }else{
                                stack->pushstring("");
                                error->q(ERROR_SOUNDFILE);
                            }
                        }
                        break;
                     } else if(e->type != T_ARRAY){
                        if(opcode==OP_SOUNDPLAY){
                            mymutex->lock();
                            emit(soundPlay(convert->getInt(e)));
                            waitCond->wait(mymutex);
                            mymutex->unlock();
                            break;
                        }else{
                            //error invalid SOUND syntax
                            error->q(ERROR_EXPECTEDSOUND);
                            if(opcode==OP_SOUNDPLAYER) stack->pushint(0);
                            if(opcode==OP_SOUNDLOAD) stack->pushstring("");
                            break;
                        }
                    }

                    int a = e->intval; //array variable
                    int columns = variables->arraysizecols(a);
                    if(columns%2!=0){
                        error->q(ERROR_ARRAYEVEN);
                        break;
                    }

                    double i;
                    int rows = variables->arraysizerows(a);

                    std::vector < std::vector<double> > sounddata;

                    for(int row = 0; row < rows; row++) {
                        std::vector < double > v;	// vector containing duration
                       for (int col = 0; col < columns; col++) {
                            DataElement *av = variables->arraygetdata(a, row, col);
                            if(col%2==0)
                                i = convert->getMusicalNote(av);
                            else
                                i = convert->getFloat(av);
                            //printf(">>%i\n",i);
                            v.push_back(i);
                        }
                        sounddata.push_back( v );
                    }

                    if (!error->pending()){
                        if(opcode==OP_SOUND || opcode==OP_SOUNDPLAY){
                            mymutex->lock();
                            emit(playSound(sounddata, false));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            if(opcode==OP_SOUND) sound->wait(id);
                        }else if(opcode==OP_SOUNDPLAYER){
                            mymutex->lock();
                            emit(playSound(sounddata, true));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            stack->pushint(id);
                        }else{
                            stack->pushstring(sound->loadSoundFromVector(sounddata));
                        }
                    }
                }
                break;


                case OP_SOUND_LIST:
                case OP_SOUNDPLAY_LIST:
                case OP_SOUNDPLAYER_LIST:
                case OP_SOUNDLOAD_LIST:
                {
                    std::vector < std::vector<double> > sounddata;

                    int rows = stack->popint();
                    double i;
                    for(int row = 0; row < rows ; row++) {
                        std::vector < double > v;	// vector containing duration
                        int columns = stack->popint();
                        if(columns%2!=0){
                            //empty stack from the rest of data
                            stack->drop(columns);
                            for(row++; row < rows ; row++) stack->drop(stack->popint());
                            error->q(ERROR_ARRAYEVEN);
                            break;
                        }
                        for (int col = 0; col < columns ; col++) {
                            if(col%2!=0)
                                i = stack->popnote();
                            else
                                i = stack->popfloat();
                            //printf(">>%i\n",i);
                            v.insert(v.begin(),i);
                        }
                        sounddata.push_back( v );
                    }
                    if (!error->pending()){
                        if(opcode==OP_SOUND_LIST || opcode==OP_SOUNDPLAY_LIST){
                            mymutex->lock();
                            emit(playSound(sounddata, false));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            if(opcode==OP_SOUND_LIST) sound->wait(id);
                        }else if(opcode==OP_SOUNDPLAYER_LIST){
                            mymutex->lock();
                            emit(playSound(sounddata, true));
                            waitCond->wait(mymutex);
                            int id = sound->soundID;
                            mymutex->unlock();
                            stack->pushint(id);
                        }else{
                            stack->pushstring(sound->loadSoundFromVector(sounddata));
                        }
                    }else{
                        if(opcode==OP_SOUNDPLAYER_LIST) stack->pushint(0);
                        if(opcode==OP_SOUNDLOAD_LIST) stack->pushstring("");
                    }
                }
                break;

                case OP_SOUNDLOADRAW: {
                    std::vector<double> sounddata;
                    int rows = stack->popint();
                    if(rows>1){
                        for(int f=0;f<rows;f++) stack->drop(stack->popint()); //clear the rest of array from stack in case of ONERROR or TRY/CATCH
                        error->q(ERROR_ONEDIMENSIONAL);//error 1 dimensional!
                        stack->pushstring("");
                        break;
                    }
                    int columns = stack->popint();
                    sounddata.resize(columns);
                    while(columns>1){
                        columns--;
                        double d = stack->popfloat();
                        sounddata[columns]=d;
                    }
                    if(!error->pending())
                        stack->pushstring(sound->loadRaw(sounddata));
                    else
                        stack->pushstring("");
                }
                break;

                case OP_SOUNDPAUSE: {
                    int i = stack->popint();
                    sound->pause(i);
                }
                break;

                case OP_SOUNDSTOP: {
                    int i = stack->popint();
                    mymutex->lock();
                    emit(soundStop(i));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                }
                break;

                case OP_SOUNDPLAYEROFF: {
                    int i = stack->popint();
                    mymutex->lock();
                    emit(soundPlayerOff(i));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                }
                break;

                case OP_SOUNDSYSTEM: {
                    int i = stack->popint();
                    mymutex->lock();
                    emit(soundSystem(i));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                }
                break;

                case OP_SOUNDSAMPLERATE: {
                    stack->pushint(sound->samplerate());
                }
                break;

                case OP_SOUNDWAIT: {
                    int i = stack->popint();
                    sound->wait(i);
                }
                break;

                case OP_SOUNDNOHARMONICS: {
                    sound->noharmonics();
                }
                break;

                case OP_SOUNDHARMONICS: {
                    double a = stack->popfloat();
                    int h = stack->popint();
                    if(h<1){
                        error->q(ERROR_HARMONICNUMBER);
                    }else{
                        sound->harmonics(h, a);
                    }
                }
                break;

                case OP_SOUNDHARMONICS_LIST: {
                    std::map<int,bool> repeated;
                    int rows = stack->popint();
                    for(int i=0;i<rows;i++){
                        int columns = stack->popint();
                        if((rows>1 && columns!=2) || (rows==1 && columns%2!=0)){
                            //clear stack from the rest of data
                            stack->drop(columns);
                            for(i++;i<rows;i++) stack->drop(stack->popint());
                            error->q(ERROR_HARMONICLIST);
                            break;
                        }
                        while(columns>1){
                            columns-=2;
                            double a = stack->popfloat();
                            int h = stack->popint();
                            if(h<1){
                                //clear stack from the rest of data
                                stack->drop(columns);
                                for(i++;i<rows;i++) stack->drop(stack->popint());
                                error->q(ERROR_HARMONICNUMBER);
                                break;
                            }else{
                                //because we extract in reverse order from stack
                                //eg: soundharmonics {3, 0, 3, 1}
                                //in this case the 3th harmonic will be set to 1, not to 0
                                if(repeated.count(h)==0){
                                    sound->harmonics(h, a);
                                    repeated[h]=true;
                                }
                            }
                        }
                        rows--;
                    }
                }
                break;

                case OP_SOUNDNOENVELOPE: {
                    sound->noenvelope();
                }
                break;

                case OP_SOUNDENVELOPE: {
                    std::vector<double> envelope;
                    double r = stack->popfloat(); //release
                    double s = stack->popfloat(); //sustain
                    double d = stack->popfloat(); //decrease
                    double a = stack->popfloat(); //attack
                    envelope.resize(6);
                    envelope[0] = 0.0;
                    envelope[1] = a;
                    envelope[2] = 1.0;
                    envelope[3] = d;
                    envelope[4] = s;
                    envelope[5] = r;
                    if(!error->pending()) sound->envelope(envelope);
                }
                break;

                case OP_SOUNDENVELOPE_LIST: {
                    std::vector<double> envelope;
                    int rows = stack->popint();
                    int columns = stack->popint();

                    if(rows!=1 || columns%2!=1 || columns<4){
                        //clear stack from the rest of data
                        stack->drop(columns);
                        for(int i=1;i<rows;i++) stack->drop(stack->popint());
                        if(rows!=1)
                            error->q(ERROR_ONEDIMENSIONAL);
                        else
                            error->q(ERROR_ENVELOPEODD);
                        break;
                    }else{
                        envelope.resize(columns);
                        while(columns>0){
                            columns--;
                            double v=stack->popfloat();
                            envelope[columns]=v;
                        }
                        if(!error->pending())
                            sound->envelope(envelope);
                        }
                }
                break;

                case OP_SOUNDWAVEFORM: {
                    DataElement *e = stack->popelement();
                    //this is an array without [] - act like OP_SOUNDWAVEFORM_LIST
                    if (e->type==T_ARRAY){
                        int a = e->intval; //array variable
                        int columns = variables->arraysizecols(a);
                        int rows = variables->arraysizerows(a);
                        std::vector<double> wave;

                        if(rows!=1){
                            //clear stack from the rest of data
                            stack->drop(columns);
                            for(int i=1;i<rows;i++) stack->drop(stack->popint());
                            error->q(ERROR_ONEDIMENSIONAL); //Creating custom waveform request one dimensional array data
                            break;
                        }else{
                            wave.resize(columns,0);
                            while(columns>0){
                                columns--;
                                DataElement *av = variables->arraygetdata(a, 0, columns);
                                wave[columns]=convert->getInt(av);
                            }
                            if(!error->pending())
                                sound->customWaveform(wave, false);
                        }

                    }else{
                        sound->waveform(convert->getInt(e));
                    }
                }
                break;

                case OP_SOUNDWAVEFORM_LIST: {
                    std::vector<double> wave;
                    bool logic = stack->popbool(); //if data is logical, not raw
                    int rows = stack->popint();
                    int columns = stack->popint();

                    if(rows!=1 || (logic && columns<3)){
                        //clear stack from the rest of data
                        stack->drop(columns);
                        for(int i=1;i<rows;i++) stack->drop(stack->popint());
                        if(rows!=1)
                            error->q(ERROR_ONEDIMENSIONAL); //Creating custom waveform request one dimensional array data
                        else
                            error->q(ERROR_WAVEFORMLOGICAL); //Creating custom waveform using logical coordinates it request at least 3 elements
                        break;
                    }
                    wave.resize(columns,0);
                    while(columns>0){
                        columns--;
                        wave[columns]=stack->popfloat();
                    }
                    if(!error->pending())
                        sound->customWaveform(wave, logic);
                }
                break;

                case OP_SOUNDSEEK: {
                    double p = stack->popfloat();
                    int i = stack->popint();
                    sound->seek(i, p);
                }
                break;

                case OP_SOUNDVOLUME: {
                    double v = stack->popfloat();
                    DataElement *e = stack->popelement();
                    if (e->type == T_STRING) {
                        //because it do not stop any timer, it is safe to do it from current thread
                        sound->volume(e->stringval, v);
                    }else{
                        mymutex->lock();
                        emit(soundVolume(convert->getInt(e), v));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                    }
                }
                break;

                case OP_SOUNDLOOP: {
                    int l = stack->popint();
                    DataElement *e = stack->popelement();
                    if (e->type == T_STRING) {
                        sound->loop(e->stringval, l);
                    }else{
                        sound->loop(convert->getInt(e), l);
                    }
                }
                break;

                case OP_SOUNDID: {
                    stack->pushint(sound->soundID);
                }
                break;

                case OP_SOUNDPOSITION: {
                    int i = stack->popint();
                    stack->pushfloat(sound->position(i));
                }
                break;

                case OP_SOUNDFADE: {
                    double delay = stack->popfloat();
                    double ms = stack->popfloat();
                    double v = stack->popfloat();
                    DataElement *e = stack->popelement();
                    if (e->type == T_STRING) {
                        //because it do not stop any timer, it is safe to do it from current thread
                        sound->fade(e->stringval, v, int(ms*1000), int(delay*1000));
                    }else{
                    mymutex->lock();
                    emit(soundFade(convert->getInt(e), v, int(ms*1000), int(delay*1000)));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                    }
                }
                break;

                case OP_SOUNDLENGTH: {
                    int i = stack->popint();
                    stack->pushfloat(sound->length(i));
                }
                break;

                case OP_SOUNDSTATE: {
                    int i = stack->popint();
                    stack->pushint(sound->state(i));
                }
                break;

                case OP_VOLUME: {
                    // set the wave output height (volume 0-10)
                    double volume = stack->popfloat();
                    sound->setMasterVolume(volume);
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
                    int doit = settingsAllowSystem;
                    if(doit==SETTINGSALLOWASK){
                        mymutex->lock();
                        emit(dialogAllowSystem(temp));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                        doit = returnInt;
                    }
                    if(doit==SETTINGSALLOWNO){
                        error->q(ERROR_PERMISSION);
                    }else if(doit==SETTINGSALLOWYES) {
                        sys = new QProcess();
                        sys->start(temp);
                        if (sys->waitForStarted(-1)) {
                            if (!sys->waitForFinished(-1)) {
                                //QByteArray result = sy.readAll();
                            }
                        }
                        delete sys;
                        sys=NULL;
                    }
                }
                break;

                case OP_WAVPLAY: {
                //obsolete
                    QString file = stack->popstring();
                    if(file.compare("")!=0) {
                        //warning
                        error->q(WARNING_WAVOBSOLETE);
                        //stop previous mediaplayer
                        mymutex->lock();
                        emit(soundStop(mediaplayer_id_legacy));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                        //play new file as a mediaplayer
                        mymutex->lock();
                        emit(playSound(file, true));
                        waitCond->wait(mymutex);
                        mediaplayer_id_legacy = sound->soundID;
                        mymutex->unlock();
                    }
                    // start playing mediaplayer
                    mymutex->lock();
                    emit(soundPlay(mediaplayer_id_legacy));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                }
                break;

                case OP_WAVSTOP: {
                //obsolete
                    mymutex->lock();
                    emit(soundStop(mediaplayer_id_legacy));
                    waitCond->wait(mymutex);
                    mymutex->unlock();
                }
                break;

                case OP_SETCOLOR: {
                    const unsigned long brushval = stack->poplong();
                    const unsigned long penval = stack->poplong();

                    if(penval!=painter_pen_color){
                        drawingpen.setColor(QColor::fromRgba((QRgb) penval));
                        painter_pen_need_update=true;
                        painter_pen_color=penval;

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
                    }

                    if(brushval!=painter_brush_color){
                        drawingbrush.setColor(QColor::fromRgba((QRgb) brushval));
                        painter_brush_need_update=true;
                        painter_brush_color=brushval;
                    }
                }
                break;

                case OP_RGB: {
                    int aval = stack->popint();
                    int bval = stack->popint();
                    int gval = stack->popint();
                    int rval = stack->popint();
                    if (((rval | gval | bval | aval)&(~0xff))!=0) {
                    //if (rval < 0 || rval > 255 || gval < 0 || gval > 255 || bval < 0 || bval > 255 || aval < 0 || aval > 255) {
                        error->q(ERROR_RGB);
                        stack->pushlong(0);
                    } else {
                        stack->pushlong( (unsigned long) QColor(rval,gval,bval,aval).rgba());
                    }
                }
                break;

                case OP_PIXEL: {
                    int y = stack->popint();
                    int x = stack->popint();
                    if(drawingOnScreen || drawto.isEmpty()){
                        QRgb rgb = graphwin->image->pixel(x,y);
                        stack->pushlong((unsigned long) rgb);
                    }else{
                        QRgb rgb = images[drawto]->pixel(x,y);
                        stack->pushlong((unsigned long) rgb);
                    }
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
                            if(drawingOnScreen || drawto.isEmpty()){
                                layerimage = graphwin->image;
                            }else{
                                layerimage = images[drawto];
                            }
                            break;
                        case SLICE_SPRITE:
                            layerimage = graphwin->spritesimage;
                            break;
                        default:
                            layerimage = graphwin->displayedimage;
                            break;
                    }
                    if (w<=0 || h<=0) {
                        error->q(ERROR_SLICESIZE);
                        stack->pushint(0);
                        stack->pushint(0);
                    } else {
                        QImage tmp = QImage(layerimage->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32));
                        const uchar* p = tmp.constBits();
                        QRgb *r = (QRgb *) p;
                        int counter = 0;
                        int tw, th;
                        for(th=0; th<h; th++) {
                            for(tw=0; tw<w; tw++) {
                                stack->pushlong(r[counter]);
                                counter++;
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

                    if(h<=0 || w<=0){
                        error->q(ERROR_SLICESIZE);
                        //free stack in case of error catching
                        while(h>0){
                            stack->drop(stack->popint());
                            h--;
                        }
                        stack->drop(2); //y and x
                        break;
                    }

                    QImage tmp = QImage(w, h, QImage::Format_ARGB32);
                    const uchar* p = tmp.constBits();

                    QRgb *r = (QRgb *) p;
                    int counter = w*h;
                    counter--;

                    for(th=h-1; th>=0; th--) {
                        int test = stack->popint();	// get extra cols
                        if(test!=w){
                            //error: number of columns is not the same (case of list of lists {})
                            error->q(ERROR_SLICESIZE);
                            //free stack in case of error catching
                            stack->drop(test);
                            th--;
                            for(; th>=0; th--) stack->drop(stack->popint());
                            stack->drop(2); //y and x
                            break;
                        }
                        for(tw=w-1; tw>=0; tw--) {
                            r[counter] = (QRgb) stack->poplong();
                            counter--;
                        }
                    }
                    //update painter only if needed (faster)
                    if(CompositionModeClear){
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=false;
                    }
                    //end update painter
                    // get where
                    int y = stack->popint();
                    int x = stack->popint();
                    painter->drawImage(x, y, tmp);
                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_LINE: {
                    int y1val = stack->popint();
                    int x1val = stack->popint();
                    int y0val = stack->popint();
                    int x0val = stack->popint();

                    //update painter's attributes only if needed (only pen)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=PenColorIsClear){
                        if(PenColorIsClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=PenColorIsClear;
                    }
                    //end painter update

                    painter->drawLine(x0val, y0val, x1val, y1val);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;


                case OP_ROUNDEDRECT:{
                    double y_rad = stack->popfloat();
                    double x_rad = stack->popfloat();
                    int y1val = stack->popint();
                    int x1val = stack->popint();
                    int y0val = stack->popint();
                    int x0val = stack->popint();

                    if(x1val<0) {
                        x0val+=x1val+1;
                        x1val*=-1;
                    }
                    if(y1val<0) {
                        y0val+=y1val+1;
                        y1val*=-1;
                    }

                    //update painter's attributes only if needed (pen and brush)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_brush_need_update){
                        painter->setBrush(drawingbrush);
                        painter_brush_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=CompositionModeClear){
                        if(CompositionModeClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=CompositionModeClear;
                    }
                    //end painter update

                    if (x1val > 1 && y1val > 1) {
                        painter->drawRoundedRect(x0val, y0val, x1val-1, y1val-1, x_rad, y_rad);
                    } else if (x1val==1 && y1val==1) {
                        // rect 1x1 is actually a point
                        painter->drawPoint(x0val, y0val);
                    } else if (x1val==1 && y1val!=0) {
                        // rect 1xn is actually a line
                        painter->drawLine(x0val, y0val, x0val, y0val+y1val);
                    } else if (x1val!=0 && y1val==1) {
                        // rect nx1 is actually a line
                        painter->drawLine(x0val, y0val, x0val + x1val, y0val);
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_RECT: {
                    int y1val = stack->popint();
                    int x1val = stack->popint();
                    int y0val = stack->popint();
                    int x0val = stack->popint();

                    if(x1val<0) {
                        x0val+=x1val+1;
                        x1val*=-1;
                    }
                    if(y1val<0) {
                        y0val+=y1val+1;
                        y1val*=-1;
                    }

                    //update painter's attributes only if needed (pen and brush)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_brush_need_update){
                        painter->setBrush(drawingbrush);
                        painter_brush_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=CompositionModeClear){
                        if(CompositionModeClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=CompositionModeClear;
                    }
                    //end painter update

                    if (x1val > 1 && y1val > 1) {
                        painter->drawRect(x0val, y0val, x1val-1, y1val-1);
                    } else if (x1val==1 && y1val==1) {
                        // rect 1x1 is actually a point
                        painter->drawPoint(x0val, y0val);
                    } else if (x1val==1 && y1val!=0) {
                        // rect 1xn is actually a line
                        painter->drawLine(x0val, y0val, x0val, y0val+y1val);
                    } else if (x1val!=0 && y1val==1) {
                        // rect nx1 is actually a line
                        painter->drawLine(x0val, y0val, x0val + x1val, y0val);
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;


                case OP_POLY_LIST: {
                    // doing a polygon from an immediate list or an array's data
                    int rows = stack->popint();
                    int columns = stack->popint();
                    if((rows==1&&columns%2!=0)||(rows>1&&columns!=2)){
                        //clear stack from the data
                        stack->drop(columns);
                        for(int row=1;row<rows;row++) stack->drop(stack->popint());
                        error->q(ERROR_ARRAYEVEN);
                        break;
                    }
                    QPointF *points = new QPointF[rows*columns];
                    int pairs = 0;

                    for(int row = 0; row < rows; row++){
                        if(row!=0) columns = stack->popint();
                        if(rows>1&&columns!=2){
                            //clear stack from the data
                            stack->drop(columns);
                            for(row++;row<rows;row++) stack->drop(stack->popint());
                            error->q(ERROR_ARRAYEVEN);
                            break;
                        }
                        for (int col = 0; col < columns; col+=2) {
                            points[pairs].setY(stack->popfloat());
                            points[pairs].setX(stack->popfloat());
                            pairs++;
                        }
                    }


                    if (!error->pending()) {
                        if (pairs < 3){
                            error->q(ERROR_POLYPOINTS);
                        }else{

                            //update painter's attributes only if needed (pen and brush)
                            if(painter_pen_need_update){
                                painter->setPen(drawingpen);
                                painter_pen_need_update=false;
                            }
                            if(painter_brush_need_update){
                                painter->setBrush(drawingbrush);
                                painter_brush_need_update=false;
                            }
                            if(painter_last_compositionModeClear!=CompositionModeClear){
                                if(CompositionModeClear)
                                    painter->setCompositionMode(QPainter::CompositionMode_Clear);
                                else
                                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                                painter_last_compositionModeClear=CompositionModeClear;
                            }
                            //end painter update

                            painter->drawPolygon(points, pairs);

                            if (!fastgraphics && drawingOnScreen) waitForGraphics();
                        }
                    }
                    delete[] points;
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

                    if((rows==1&&columns%2!=0)||(rows>1&&columns!=2)){
                        //clear stack from the data
                        stack->drop(columns);
                        for(int row=1;row<rows;row++) stack->drop(stack->popint());
                        error->q(ERROR_ARRAYEVEN);
                        break;
                    }

                    QPointF *points = new QPointF[rows*columns];
                    int pairs = 0;

                    for(int row = 0; row < rows; row++){
                        if(row!=0) columns = stack->popint();
                        if(rows>1&&columns!=2){
                            //clear stack from the data
                            stack->drop(columns);
                            for(row++;row<rows;row++) stack->drop(stack->popint());
                            error->q(ERROR_ARRAYEVEN);
                            break;
                        }
                        for (int col = 0; col < columns; col+=2) {
                            points[pairs].setY(stack->popfloat());
                            points[pairs].setX(stack->popfloat());
                            pairs++;
                        }
                    }

                    if (!error->pending()) {
                        if (pairs < 3){
                            error->q(ERROR_POLYPOINTS);
                        }else{
                            //
                            // now get scaling, rotation, and position to stamp the poly
                            if (opcode==OP_STAMP_SR_LIST) rotate = stack->popfloat();
                            if (opcode==OP_STAMP_SR_LIST || opcode==OP_STAMP_S_LIST) scale = stack->popfloat();
                            int y = stack->popint();
                            int x = stack->popint();
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

                            //update painter's attributes only if needed (pen and brush)
                            if(painter_pen_need_update){
                                painter->setPen(drawingpen);
                                painter_pen_need_update=false;
                            }
                            if(painter_brush_need_update){
                                painter->setBrush(drawingbrush);
                                painter_brush_need_update=false;
                            }
                            if(painter_last_compositionModeClear!=CompositionModeClear){
                                if(CompositionModeClear)
                                    painter->setCompositionMode(QPainter::CompositionMode_Clear);
                                else
                                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                                painter_last_compositionModeClear=CompositionModeClear;
                            }
                            //end painter update

                            painter->drawPolygon(points, pairs);
                            if (!fastgraphics && drawingOnScreen) waitForGraphics();
                        }
                    }
                    delete[] points;
                }
                break;


                case OP_CIRCLE: {
                    int rval = stack->popint();
                    int yval = stack->popint();
                    int xval = stack->popint();

                    //update painter's attributes only if needed (pen and brush)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_brush_need_update){
                        painter->setBrush(drawingbrush);
                        painter_brush_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=CompositionModeClear){
                        if(CompositionModeClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=CompositionModeClear;
                    }
                    //end painter update

                    painter->drawEllipse(xval - rval, yval - rval, 2 * rval, 2 * rval);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_ELLIPSE: {
                    int hval = stack->popint();
                    int wval = stack->popint();
                    int yval = stack->popint();
                    int xval = stack->popint();

                    //update painter's attributes only if needed (pen and brush)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_brush_need_update){
                        painter->setBrush(drawingbrush);
                        painter_brush_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=CompositionModeClear){
                        if(CompositionModeClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=CompositionModeClear;
                    }
                    //end painter update

                    painter->drawEllipse(xval, yval, wval, hval);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
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


                        if (rotate != 0 || scale != 1) {
                            QTransform transform = QTransform().translate(0,0).rotateRadians(rotate).scale(scale, scale);
                            i = i.transformed(transform);
                        }
                        if (i.width() != 0 && i.height() != 0) {

                            //update painter only if needed (faster)
                            if(CompositionModeClear){
                                painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                                painter_last_compositionModeClear=false;
                            }
                            //end update painter

                            painter->drawImage((int)(x - .5 * i.width()), (int)(y - .5 * i.height()), i);
                        }
                        if (!fastgraphics && drawingOnScreen) waitForGraphics();
                    }
                }
                break;

                case OP_TEXT: {
                    QString txt = stack->popstring();
                    int y0val = stack->popint();
                    int x0val = stack->popint();

                    //update painter's attributes only if needed (only pen)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=PenColorIsClear){
                        if(PenColorIsClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=PenColorIsClear;
                    }
                    //end painter update

                    if(painter_font_need_update){
                        painter->setFont(font);
                        painter_font_need_update=false;
                    }
                    painter->drawText(x0val, y0val+(QFontMetrics(painter->font()).ascent()), txt);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;


                case OP_TEXTBOX: {
                    int flags = stack->popint();
                    QString txt = stack->popstring();
                    int h = stack->popint();
                    int w = stack->popint();
                    int y = stack->popint();
                    int x = stack->popint();

                    if(h<0){
                        y+=h;
                        h=-h;
                    }
                    if(w<0){
                        x+=w;
                        w=-w;
                    }

                    //update painter's attributes only if needed (only pen)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=PenColorIsClear){
                        if(PenColorIsClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=PenColorIsClear;
                    }
                    //end painter update

                    if(painter_font_need_update){
                        painter->setFont(font);
                        painter_font_need_update=false;
                    }
                    painter->drawText(x, y, w, h, flags|Qt::TextWordWrap|Qt::TextExpandTabs, txt);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_TEXTBOXHEIGHT:
                case OP_TEXTBOXWIDTH: {
                    int w = stack->popint();
                    QString txt = stack->popstring();

                    if(w<0) w=-w;

                    //update painter's attributes only if needed (only pen)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    //end painter update

                    if(painter_font_need_update){
                        painter->setFont(font);
                        painter_font_need_update=false;
                    }
                    QRect boundingRect;
                    painter->drawText(-1, -1, w, 0, Qt::TextWordWrap, txt, &boundingRect);
                    if(opcode==OP_TEXTBOXHEIGHT){
                        stack->pushint(boundingRect.height());
                    }else{
                        stack->pushint(boundingRect.width());
                    }
                }
                break;

                case OP_FONT: {
                    bool italic = stack->popbool();
                    int weight = stack->popint();
                    int size = stack->popint();
                    QString family = stack->popstring().trimmed();
                    if(family.isEmpty())
                        font = QFont(defaultfontfamily, size, weight, italic);
                    else
                        font = QFont(family, size, weight, italic);

                    if(defaultfontpointsize == font.pointSize() && defaultfontweight == font.weight() && defaultfontfamily == painter->font().family() && defaultfontitalic == font.italic())
                        painter_custom_font_flag = false;
                    else
                        painter_custom_font_flag = true;

                    painter_font_need_update=true;
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
                    QColor c = QColor::fromRgba((QRgb) clearcolor);

                    if (drawingOnScreen){
                        graphwin->image->fill(c);
                        if (!fastgraphics) waitForGraphics();
                    }else if(printing){
                        if(printdocument->pageRect()==printdocument->paperRect()){
                            //printer is in full page mode already
                            painter->fillRect(printdocument->paperRect(),c);
                        }else{
                            //a good solution is to end painter and begin after setFullPage(true)
                            //this will reset origins for painter to top-left of the page
                            //but swiching back setFullPage(false) and starting again painter (begin) to page
                            //clear the page entirely.
                            QRect r = printdocument->pageRect();
                            printdocument->setFullPage(true);
                            painter->translate(-r.left(),-r.top());
                            painter->fillRect(printdocument->paperRect(),c);
                            printdocument->setFullPage(false);
                            painter->translate(r.topLeft());
                        }
                    }else{
                        images[drawto]->fill(c);
                    }
                }
                break;

                case OP_PLOT: {
                    int oneval = stack->popint();
                    int twoval = stack->popint();

                    //update painter's attributes only if needed (only pen)
                    if(painter_pen_need_update){
                        painter->setPen(drawingpen);
                        painter_pen_need_update=false;
                    }
                    if(painter_last_compositionModeClear!=PenColorIsClear){
                        if(PenColorIsClear)
                            painter->setCompositionMode(QPainter::CompositionMode_Clear);
                        else
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter_last_compositionModeClear=PenColorIsClear;
                    }
                    //end painter update

                    painter->drawPoint(twoval, oneval);

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_FASTGRAPHICS: {
                    fastgraphics = true;
                    emit(fastGraphics());
                }
                break;

                case OP_GRAPHSIZE: {
                    int height = stack->popint();
                    int width = stack->popint();

                    if (height<=0 || width<=0){
                        height = GSIZE_INITIAL_HEIGHT;
                        width = GSIZE_INITIAL_WIDTH;
                    }
                    if(drawingOnScreen || drawto.isEmpty()){
                        //change graph size if current graph is on screen (it may be printing also)
                        if(drawingOnScreen) painter->end();
                        mymutex->lock();
                        emit(mainWindowsResize(1, width, height));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                        if(drawingOnScreen) setPainterTo(graphwin->image);
                        force_redraw_all_sprites_next_time();
                        waitForGraphics();
                    }else{
                        QImage tmp = QImage(width, height, QImage::Format_ARGB32);
                        tmp.fill(Qt::transparent);
                        if(!printing){
                            painter->end();
                            images[drawto]->swap(tmp);
                            setPainterTo(images[drawto]);
                        }else{
                            images[drawto]->swap(tmp);
                        }
                    }
                }
                break;

                case OP_GRAPHWIDTH: {
                    int w = 0;
                    if (drawingOnScreen){
                        w = graphwin->image->width();
                    }else if(printing){
                        w = printdocument->width();
                    }else{
                        w = images[drawto]->width();
                    }
                    stack->pushint(w);
                }
                break;

                case OP_GRAPHHEIGHT: {
                    int h = 0;
                    if (drawingOnScreen) {
                        h = graphwin->image->height();
                    }else if(printing){
                        h = printdocument->height();
                    }else{
                        h = images[drawto]->height();
                    }
                    stack->pushint(h);
                }
                break;

                case OP_REFRESH: {
                    waitForGraphics();
                }
                break;

                case OP_INPUT: {
                    inputType = stack->popint();
                    inputString.clear();
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
                    //we got input from user or program is going to stop
                    if (status==R_INPUT) {//program is not stopped by user in the mean time
                        status = R_RUNNING;
                        stack->pushvariant(inputString, inputType);
                    }

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
                    //push forstack into forstacklevel
                    const int level = variables->getrecurse();
                    if(forstacklevelsize <= level){
                        forstacklevel.resize(level+1);
                        forstacklevelsize++;
                    }
                    forstacklevel[level] = forstack;
                    forstack = NULL;
                    variables->increaserecurse();
                }
                break;

                case OP_DECREASERECURSE:
                    // decrease recursion level in variable hash
                    // and pop any unfinished for statements off of forstack
                {
                    decreaserecurse();
                }
                break;

                case OP_SPRITEPOLY_LIST: {
                    // create a sprite from a polygon from an immediate list on the stack
                    // first stack element is the number of points*2 to pull from the stack
                    int maxx=0, maxy=0;
                    int minx=0, miny=0;

                    // doing a polygon from an immediate list or an array's data
                    int rows = stack->popint();
                    int columns = stack->popint();
                    if((rows==1&&columns%2!=0)||(rows>1&&columns!=2)){
                        //clear stack from the data
                        stack->drop(columns);
                        for(int row=1;row<rows;row++) stack->drop(stack->popint());
                        stack->drop(1); // sprite number
                        error->q(ERROR_ARRAYEVEN);
                        break;
                    }
                    QPointF *points = new QPointF[rows*columns];
                    int pairs = 0;

                    for(int row = 0; row < rows; row++){
                        if(row!=0) columns = stack->popint();
                        if(rows>1&&columns!=2){
                            //clear stack from the data
                            stack->drop(columns);
                            for(row++;row<rows;row++) stack->drop(stack->popint());
                            stack->drop(1); // sprite number
                            error->q(ERROR_ARRAYEVEN);
                            break;
                        }
                        for (int col = 0; col < columns; col+=2) {
                            double y = stack->popfloat();
                            double x = stack->popfloat();
                            if(x<minx) minx=x;
                            if(y<miny) miny=y;
                            if(x>maxx) maxx=x;
                            if(y>maxy) maxy=y;
                            points[pairs].setY(y);
                            points[pairs].setX(x);
                            pairs++;
                        }
                    }


                    if (!error->pending()) {
                        if (pairs < 3){
                            error->q(ERROR_POLYPOINTS);
                        }else{
                            //
                            // now move points to the top left (if they are not there)
                            for(int j=0;j<pairs;j++) {
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
                    }
                    delete points;
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
                        if(drawingOnScreen || drawto.isEmpty()){
                            sprites[n].image = new QImage(graphwin->image->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32_Premultiplied));
                        }else{
                            sprites[n].image = new QImage(images[drawto]->copy(x, y, w, h).convertToFormat(QImage::Format_ARGB32_Premultiplied));
                        }
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
                //obsolete
                    sound->wait(mediaplayer_id_legacy);
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
                                    } else if (!error->pending()){
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
                                    }else{
                                        //in case of an error
                                        if(opcode==OP_DBSTRING)
                                            stack->pushstring("");
                                        else
                                            stack->pushint(0);
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
                    // pop a trap off of the on-error stack
                    onerrorstack->drop();
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
                        stack->pushstring("");
                    } else {
                        if (netsockfd[fn] < 0) {
                            error->q(ERROR_NETNONE);
                            stack->pushstring("");
                        } else {
                            memset(strarray, 0, MAXSIZE);
                            n = recv(netsockfd[fn],strarray,MAXSIZE-1,0);
                            if (n < 0) {
                                error->q(ERROR_NETREAD, 0, strerror(errno));
                                stack->pushstring("");
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
                        stack->pushint(0);
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
                    // on error give local loopback
                    stack->pushstring(QString("127.0.0.1"));
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
                    QString key = stack->popstring().trimmed();
                    QString app = stack->popstring().trimmed();

                    if(app.isEmpty() || app.contains(QChar('\\')) || app.contains(QChar('/')) || app.size()>255 || QString::compare(app, "SYSTEM", Qt::CaseInsensitive)==0){
                        error->q(ERROR_INVALIDPROGNAME);
                        break;
                    }
                    if(key.isEmpty() || key.contains(QChar('\\')) || key.contains(QChar('/')) || key.size()>255){
                        error->q(ERROR_INVALIDKEYNAME);
                        break;
                    }
                    if(stuff.size()>16383){
                        error->q(ERROR_SETTINGMAXLEN);
                        break;
                    }

                    if(settingsSettingsAccess!=2){
                        if(programName.isEmpty()){
                            programName=app;
                        }else if(programName!=app){
                            error->q(ERROR_SETTINGSSETACCESS);
                            break;
                        }
                    }
                    if(settingsAllowSetting) {
                        SETTINGS;
                        settings.beginGroup(SETTINGSGROUPUSER);
                        settings.beginGroup(app);
                        if(stuff.isEmpty()){
                            settings.remove(key);
                        }else{
                            if(settingsSettingsMax>0){
                                QStringList list=settings.childKeys();
                                int s=list.size();
                                if(!(s<settingsSettingsMax || list.contains(key))){
                                    error->q(ERROR_SETTINGMAXKEYS);
                                    break;
                                }
                            }
                            settings.setValue(key, stuff);
                        }
                        settings.endGroup();
                        settings.endGroup();
                    } else {
                        if(stuff.isEmpty()){
                            fakeSettings[app].remove(key);
                        }else{
                            if(settingsSettingsMax>0){
                                int s=fakeSettings[app].size();
                                if(!(s<settingsSettingsMax || fakeSettings[app].contains(key))){
                                    error->q(ERROR_SETTINGMAXKEYS);
                                    break;
                                }
                            }
                            fakeSettings[app][key]=stuff;
                        }
                    }
                }
                break;

                case OP_GETSETTING: {
                    QString key = stack->popstring().trimmed();
                    QString app = stack->popstring().trimmed();
                    if(QString::compare(app, "SYSTEM", Qt::CaseInsensitive)==0) {
                        SETTINGS;
                        QString v = settings.value(key, "").toString();
                        if(v.length()!=32){
                            //not MD5, not the password
                            stack->pushstring(v);
                        }else{
                            //check if user try to get the password
                            //user can use different keys to get the password such "////Pref//Password/" or "PREF/password"
                            //the safest way is to compare the value with saved password to avoid user roundabouts
                            QString p = settings.value(SETTINGSPREFPASSWORD, "").toString();
                            if(QString::compare(v,p)==0){
                                stack->pushstring("****");
                            }else{
                                stack->pushstring(v);
                            }
                        }
                        break;
                    }
                    if(app.isEmpty() || app.contains(QChar('\\')) || app.contains(QChar('/')) || app.size()>255){
                        error->q(ERROR_INVALIDPROGNAME);
                        stack->pushstring("");
                        break;
                    }
                    if(key.isEmpty() || key.contains(QChar('\\')) || key.contains(QChar('/')) || key.size()>255){
                        error->q(ERROR_INVALIDKEYNAME);
                        stack->pushstring("");
                        break;
                    }

                    if(settingsSettingsAccess==0){
                        if(programName.isEmpty()){
                            programName=app;
                        }else if(programName!=app){
                            error->q(ERROR_SETTINGSGETACCESS);
                            stack->pushstring("");
                            break;
                        }
                    }

                    if(settingsAllowSetting){
                        SETTINGS;
                        settings.beginGroup(SETTINGSGROUPUSER);
                        settings.beginGroup(app);
                        stack->pushstring(settings.value(key, "").toString());
                        settings.endGroup();
                        settings.endGroup();
                    } else {
                        QString s("");
                        if(fakeSettings.contains(app)){
                            if(fakeSettings[app].contains(key)) s=fakeSettings[app][key];
                        }
                        stack->pushstring(s);
                    }
                }
                break;


                case OP_PORTOUT: {
                    int data = stack->popint();
                    int port = stack->popint();
#ifdef WIN32
                    int doit = settingsAllowPort;
                    if(doit==SETTINGSALLOWASK){
                        mymutex->lock();
                        emit(dialogAllowPortInOut(QString("PORTOUT ") + QString::number(port) + ", " + QString::number(data)));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                        doit = returnInt;
                    }
                    if(doit>0) {
                        if (Out32==NULL) {
                            error->q(ERROR_NOTIMPLEMENTED);
                        } else {
                            Out32(port, data);
                        }
                    } else if(doit==0){
                        error->q(ERROR_PERMISSION);
                    }
# else
                        error->q(ERROR_NOTIMPLEMENTED);
#endif
                }
                break;

                case OP_PORTIN: {
                    int data=0;
                    int port = stack->popint();
#ifdef WIN32
                    int doit = settingsAllowPort;
                    if(doit==SETTINGSALLOWASK){
                        mymutex->lock();
                        emit(dialogAllowPortInOut(QString("PORTIN ") + QString::number(port)));
                        waitCond->wait(mymutex);
                        mymutex->unlock();
                        doit = returnInt;
                    }

                    if(doit==SETTINGSALLOWNO){
                        error->q(ERROR_PERMISSION);
                    }else if(doit==SETTINGSALLOWYES) {
                        if (Inp32==NULL) {
                            error->q(ERROR_NOTIMPLEMENTED);
                        } else {
                            data = Inp32(port);
                        }
                    }
#else
                        error->q(ERROR_NOTIMPLEMENTED);
#endif
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
                        QString final = stwo + sone;
                        if (final.length()>STRINGMAXLEN) {
                            final.truncate(STRINGMAXLEN);
                            error->q(ERROR_STRINGMAXLEN);
                        }
                        stack->pushstring(stwo + sone);
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
                        if(drawingOnScreen || drawto.isEmpty()){
                            graphwin->image->save(file, type.toUpper().toUtf8().data());
                        }else{
                            images[drawto]->save(file, type.toUpper().toUtf8().data());
                        }
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
                        stack->pushstring(QString(""));
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

                case OP_TEXTHEIGHT: {
                    // returns the height of the font.
                    if(painter_font_need_update){
                        painter->setFont(font);
                        painter_font_need_update=false;
                    }
                    stack->pushint((int) (QFontMetrics(painter->font()).height()));
                }
                break;

                case OP_TEXTWIDTH: {
                    // return the number of pixels the font requires for diaplay
                    // a string is required for width but not for height
                    QString txt = stack->popstring();
                    if(painter_font_need_update){
                        painter->setFont(font);
                        painter_font_need_update=false;
                    }
                    stack->pushint((int) (QFontMetrics(painter->font()).width(txt)));
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


                    if(opcode==OP_ARC) {
                        //update painter's attributes only if needed (only pen)
                        if(painter_pen_need_update){
                            painter->setPen(drawingpen);
                            painter_pen_need_update=false;
                        }
                        if(painter_last_compositionModeClear!=PenColorIsClear){
                            if(PenColorIsClear)
                                painter->setCompositionMode(QPainter::CompositionMode_Clear);
                            else
                                painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                            painter_last_compositionModeClear=PenColorIsClear;
                        }
                        //end painter update
                    }else{
                        //update painter's attributes only if needed (pen and brush)
                        if(painter_pen_need_update){
                            painter->setPen(drawingpen);
                            painter_pen_need_update=false;
                        }
                        if(painter_brush_need_update){
                            painter->setBrush(drawingbrush);
                            painter_brush_need_update=false;
                        }
                                if(painter_last_compositionModeClear!=CompositionModeClear){
                                    if(CompositionModeClear)
                                        painter->setCompositionMode(QPainter::CompositionMode_Clear);
                                    else
                                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                                    painter_last_compositionModeClear=CompositionModeClear;
                                }
                        //end painter update
                    }

                    if(opcode==OP_ARC) {
                        painter->drawArc(xval, yval, wval, hval, s, aw);
                    }
                    if(opcode==OP_CHORD) {
                        painter->drawChord(xval, yval, wval, hval, s, aw);
                    }
                    if(opcode==OP_PIE) {
                        painter->drawPie(xval, yval, wval, hval, s, aw);
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
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
                        painter_pen_need_update=true;
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
                        stack->pushstring(QString("0"));
                    }
                }
                break;

                case OP_PRINTEROFF: {
                    if (printing) {
                        printing = false;
                        setGraph(drawto);
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
                        int printer = settingsPrinterPrinter;
                        if (printer==-1) {
                            // pdf printer
                            printdocument = new QPrinter((QPrinter::PrinterMode) settingsPrinterResolution);
                            printdocument->setOutputFormat(QPrinter::PdfFormat);
                            printdocument->setOutputFileName(settingsPrinterPdfFile);

                        } else {
                            // system printer
                            QList<QPrinterInfo> printerList=QPrinterInfo::availablePrinters();
                            if (printer>=printerList.count()) printer = 0;
                            printdocument = new QPrinter(printerList[printer], (QPrinter::PrinterMode) settingsPrinterResolution);
                        }
                        if (printdocument) {
                            if(printdocument->isValid()){
                                printdocument->setCreator(QString(SETTINGSAPP));
                                printdocument->setDocName(editwin->title);
                                printdocument->setPaperSize((QPrinter::PaperSize) settingsPrinterPaper);
                                printdocument->setOrientation((QPrinter::Orientation) settingsPrinterOrient);
                                if (!setPainterTo(printdocument)) {
                                    error->q(ERROR_PRINTEROPEN);
                                    setGraph(drawto); //if drawing on printer fails, then fall back to graph area
                                } else {
                                    printing = true;
                                }
                            }else{
                                delete printdocument;
                                error->q(ERROR_PRINTEROPEN);
                            }
                        } else {
                            error->q(ERROR_PRINTEROPEN);
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
                        setGraph(drawto);
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
                //obsolete
                    stack->pushfloat(sound->length(mediaplayer_id_legacy));
                }
                break;

                case OP_WAVPAUSE: {
                //obsolete
                    sound->pause(mediaplayer_id_legacy);
                }
                break;

                case OP_WAVPOS: {
                //obsolete
                    stack->pushfloat(sound->position(mediaplayer_id_legacy));
                }
                break;

                case OP_WAVSEEK: {
                //obsolete
                    double pos = stack->popfloat();
                    sound->seek(mediaplayer_id_legacy, pos);
                }
                break;

                case OP_WAVSTATE: {
                //obsolete
                    stack->pushint(sound->state(mediaplayer_id_legacy));
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
                    int cols = 0;
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
                            expr.indexIn(qhaystack);
                            list = expr.capturedTexts();
                        } else {
                            // if it is a simple regex without captures then split
                            list = qhaystack.split(expr, QString::KeepEmptyParts);
                        }
                    }
                    // push 1d array to stack as a listoflists
                    for(int y=0; y<list.size(); y++) {
                        // fill the string array
                        stack->pushstring(list.at(y));
                    }
                    stack->pushint(list.size());		// columns
                    stack->pushint(1);				// rows
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
                                for (int row=0; row<rows; row++) {
                                    for (int col=0; col<cols; col++) {
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
                                                stack->pushdataelement(NULL);
                                                error->q(ERROR_UNSERIALIZEFORMAT);
                                        }
                                    }
                                    stack->pushint(cols);
                                }
                                stack->pushint(rows);
                            } else {
                                error->q(ERROR_UNSERIALIZEFORMAT);
                                stack->pushint(0);//zero columns
                                stack->pushint(1);//one row
                            }
                        } else {
                            error->q(ERROR_UNSERIALIZEFORMAT);
                            stack->pushint(0);//zero columns
                            stack->pushint(1);//one row
                        }
                    } else {
                        error->q(ERROR_UNSERIALIZEFORMAT);
                        stack->pushint(0);//zero columns
                        stack->pushint(1);//one row
                    }
                }
                break;

                case OP_IMAGELOAD: {
                    QString s = stack->popstring();
                    lastImageId++;
                    QString id = QString("image:") + QString::number(lastImageId) + QStringLiteral(":") + s;
                    if(QFileInfo(s).exists()){
                        images[id] = new QImage(QImage(s).convertToFormat(QImage::Format_ARGB32));
                    }else{
                        QImage *temp = new QImage();
                        downloader->download(QUrl::fromUserInput(s));
                        temp->loadFromData(downloader->data());
                        images[id] = new QImage(temp->convertToFormat(QImage::Format_ARGB32));
                        delete temp;
                    }
                    stack->pushstring(id);
                    if(images[id]->isNull())
                        error->q(ERROR_IMAGEFILE);
                }
                break;


                case OP_IMAGENEW: {
                    unsigned long c = stack->poplong();
                    int h = stack->popint();
                    int w = stack->popint();
                    lastImageId++;
                    QString id = QString("image:") + QString::number(lastImageId);
                    images[id] = new QImage(w, h, QImage::Format_ARGB32);
                    images[id]->fill(QColor::fromRgba((QRgb) c));
                    stack->pushstring(id);
                }
                break;


                case OP_IMAGECOPY: {
                    int nr = stack->popint();
                    lastImageId++;
                    QString id = QString("image:") + QString::number(lastImageId);
                    switch (nr){
                    case 0:{
                        if(drawingOnScreen || drawto.isEmpty()){
                            images[id] = new QImage(*graphwin->image);
                        }else{
                            images[id] = new QImage(*images[drawto]);
                        }
                        break;
                    }
                    case 4:{
                        int h = stack->popint();
                        int w = stack->popint();
                        int y = stack->popint();
                        int x = stack->popint();
                        if(drawingOnScreen || drawto.isEmpty()){
                            images[id] = new QImage(graphwin->image->copy(x, y, w, h));
                        }else{
                            images[id] = new QImage(images[drawto]->copy(x, y, w, h));
                        }
                        break;
                    }
                    case 1:{
                        QString id2 = stack->popstring();
                        if (images.contains(id2)){
                            images[id] = new QImage(*images[id2]);
                        }else{
                            error->q(ERROR_IMAGERESOURCE);
                        }
                        break;
                    }
                    case 5:{
                        int h = stack->popint();
                        int w = stack->popint();
                        int y = stack->popint();
                        int x = stack->popint();
                        QString id2 = stack->popstring();
                        if (images.contains(id2)){
                            images[id] = new QImage(images[id2]->copy(x, y, w, h));
                        }else{
                            error->q(ERROR_IMAGERESOURCE);
                        }
                        break;
                    }
                    }
                    stack->pushstring(id);
                }
                break;

                case OP_IMAGECROP: {
                    int h = stack->popint();
                    int w = stack->popint();
                    int y = stack->popint();
                    int x = stack->popint();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        QImage tmp = QImage(images[id]->copy(x, y, w, h));
                        if(id==drawto){
                            painter->end();
                            images[id]->swap(tmp);
                            setPainterTo(images[id]);
                        }else{
                            images[id]->swap(tmp);
                        }
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGEAUTOCROP: {
                    int x,y,w,h,y1,y2,x1=0,x2=0;
                    unsigned long c=0;
                    int nr = stack->popint();
                    if(nr==2) c = stack->poplong();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        w=images[id]->width();
                        h=images[id]->height();
                        if(w>0 && h>0){
                            if(nr==2){
                                for(y=0;y<h;y++){
                                    for(x=0;x<w;x++){
                                        if(images[id]->pixel(x,y)!=c) break;
                                    }
                                if(x<w) break;
                                }
                                y1=y;
                                for(y=h-1;y>y1;y--){
                                    for(x=0;x<w;x++){
                                        if(images[id]->pixel(x,y)!=c) break;
                                    }
                                if(x<w) break;
                                }
                                y2=y;
                                if(y1!=y2){
                                    for(x=0;x<w;x++){
                                        for(y=y1;y<y2;y++){
                                            if(images[id]->pixel(x,y)!=c) break;
                                        }
                                    if(y<y2) break;
                                    }
                                    x1=x;
                                    for(x=w-1;x>x1;x--){
                                        for(y=y1;y<y2;y++){
                                            if(images[id]->pixel(x,y)!=c) break;
                                        }
                                    if(y<y2) break;
                                    }
                                    x2=x;
                                }
                            }else{
                                for(y=0;y<h;y++){
                                    for(x=0;x<w;x++){
                                        if(qAlpha(images[id]->pixel(x,y))!=0) break;
                                    }
                                if(x<w) break;
                                }
                                y1=y;
                                for(y=h-1;y>y1;y--){
                                    for(x=0;x<w;x++){
                                        if(qAlpha(images[id]->pixel(x,y))!=0) break;
                                    }
                                if(x<w) break;
                                }
                                y2=y;
                                if(y1!=y2){
                                    for(x=0;x<w;x++){
                                        for(y=y1;y<y2;y++){
                                            if(qAlpha(images[id]->pixel(x,y))!=0) break;
                                        }
                                    if(y<y2) break;
                                    }
                                    x1=x;
                                    for(x=w-1;x>x1;x--){
                                        for(y=y1;y<y2;y++){
                                            if(qAlpha(images[id]->pixel(x,y))!=0) break;
                                        }
                                    if(y<y2) break;
                                    }
                                    x2=x;
                                }
                            }
                            QImage tmp;
                            if(y2>y1){
                                 tmp = QImage(images[id]->copy(x1, y1, x2-x1+1, y2-y1+1));
                            }else{
                                tmp = QImage();
                            }

                            if(id==drawto){
                                painter->end();
                                images[id]->swap(tmp);
                                setPainterTo(images[id]);
                            }else{
                                images[id]->swap(tmp);
                            }
                        }
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGERESIZE: {
                    int h, w, nr;
                    double s;
                    nr = stack->popint();
                    if(nr==3){
                        h = stack->popint();
                        w = stack->popint();
                    }else{
                        s = stack->popfloat();
                    }
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        QImage tmp;
                        if(nr==3){
                            tmp = QImage(images[id]->scaled(w,h,Qt::IgnoreAspectRatio,imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
                        }else{
                            QTransform transform = QTransform().scale(s,s);
                            tmp = QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
                        }
                        if(id==drawto){
                            painter->end();
                            images[id]->swap(tmp);
                            setPainterTo(images[id]);
                        }else{
                            images[id]->swap(tmp);
                        }
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGESETPIXEL: {
                    unsigned long c = stack->poplong();
                    int y = stack->popint();
                    int x = stack->popint();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        images[id]->setPixel(x,y,c);
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGEWIDTH: {
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        stack->pushint(images[id]->width());
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGEHEIGHT: {
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        stack->pushint(images[id]->height());
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGEPIXEL: {
                    int y = stack->popint();
                    int x = stack->popint();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        unsigned long c = images[id]->pixel(x,y);
                        stack->pushlong(c);
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                        stack->pushint(0);
                    }
                }
                break;


                case OP_IMAGEDRAW: {
                    int nr = stack->popint();
                    double o = 1;
                    int x, y, h, w;
                    QString id;



                    switch (nr){
                    case 4:
                        o = stack->popfloat();
                    case 3:
                        y = stack->popint();
                        x = stack->popint();
                        id = stack->popstring();
                        if (images.contains(id)){
                            if(o!=1.0) painter->setOpacity(o);
                            painter->drawImage(x, y, *images[id]);
                            if(o!=1.0) painter->setOpacity(1.0);
                        }else{
                            error->q(ERROR_IMAGERESOURCE);
                        }
                        break;
                    case 6:
                        o = stack->popfloat();
                    case 5:
                        h = stack->popint();
                        w = stack->popint();
                        y = stack->popint();
                        x = stack->popint();
                        id = stack->popstring();
                        if (images.contains(id)){
                            bool r=false;
                            r = painter->testRenderHint(QPainter::SmoothPixmapTransform);
                            painter->setRenderHint(QPainter::SmoothPixmapTransform,imageSmooth);
                            if(o!=1.0) painter->setOpacity(o);
                            if(w<0 || h<0){
                                painter->drawImage(QRectF(x,y,w<0?w*-1:w,h<0?h*-1:h), images[id]->mirrored(w<0, h<0), QRectF(0,0,images[id]->width(),images[id]->height()));
                            }else{
                                painter->drawImage(QRectF(x,y,w,h), *images[id], QRectF(0,0,images[id]->width(),images[id]->height()));
                            }
                            painter->setRenderHints(QPainter::SmoothPixmapTransform, r);
                            if (o!=1.0) painter->setOpacity(1.0);
                        }else{
                            error->q(ERROR_IMAGERESOURCE);
                        }
                        break;
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;


                case OP_IMAGEFLIP: {
                    bool h = stack->popbool();
                    bool w = stack->popbool();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        QImage tmp = QImage(images[id]->mirrored(w, h));
                        if(id==drawto){
                            painter->end();
                            images[id]->swap(tmp);
                            setPainterTo(images[id]);
                        }else{
                            images[id]->swap(tmp);
                        }
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGEROTATE: {
                    double d = stack->popfloat();
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        QTransform rot;
                        rot.rotateRadians(d);
                        QImage tmp = QImage(images[id]->transformed(rot, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
                        if(id==drawto){
                            painter->end();
                            images[id]->swap(tmp);
                            setPainterTo(images[id]);
                        }else{
                            images[id]->swap(tmp);
                        }
                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }
                }
                break;

                case OP_IMAGESMOOTH: {
                    bool v = stack->popbool();
                    imageSmooth = v;
                }
                break;

                case OP_IMAGECENTERED: {
                    double o=1, r=0, s=1, y=0, x=0;
                    int nr = stack->popint(); // number of arguments (3-6)
                    switch(nr){
                        case 6  :
                           o = stack->popfloat();
                        case 5  :
                           r = stack->popfloat();
                        case 4  :
                           s = stack->popfloat();
                        case 3 :
                           y = stack->popfloat();
                           x = stack->popfloat();
                    }
                    QString id = stack->popstring();
                    if (images.contains(id)){
                        QTransform transform = QTransform().translate(images[id]->width()/2, images[id]->height()/2).rotateRadians(r).scale(s,s);
                        QImage *tmp = new QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));
                        if(o!=1.0) painter->setOpacity(o);
                        painter->drawImage(x-(tmp->width()/2),y-(tmp->height()/2), *tmp);
                        delete tmp;
                        if (o!=1.0) painter->setOpacity(1.0);

                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_UNLOAD: {
                    QString id = stack->popstring();
                    if(id.startsWith("image:")){
                        if (images.contains(id)){
                            if(drawto==id) setGraph("");
                            delete(images[id]);
                            images.remove(id);
                        }else{
                            error->q(ERROR_IMAGERESOURCE);
                        }
                    }else if(id.startsWith("sound:") || id.startsWith("beep:")){
                        if(!sound->unloadSound(id)){
                            error->q(ERROR_SOUNDRESOURCE);
                        }
                    }else{
                        error->q(ERROR_INVALIDRESOURCE);
                    }
                }
                break;

                case OP_IMAGETRANSFORMED: {
                    double o = stack->popfloat();
                    int y4 = stack->popint();
                    int x4 = stack->popint();
                    int y3 = stack->popint();
                    int x3 = stack->popint();
                    int y2 = stack->popint();
                    int x2 = stack->popint();
                    int y1 = stack->popint();
                    int x1 = stack->popint();
                    QString id = stack->popstring();

                    if (images.contains(id)){
                        double w = images[id]->width();
                        double h = images[id]->height();
                        QPolygonF polygon1, polygon2;
                        polygon2 << QPointF(x1, y1) << QPointF(x2, y2) << QPointF(x3, y3) << QPointF(x4, y4);
                        polygon1 << QPointF(0.0, 0.0) << QPointF(w-1.0, 0.0) << QPointF(w-1.0, h-1.0) << QPointF(0.0, h-1.0);
                        QTransform transform;
                        QTransform::quadToQuad(polygon1,polygon2,transform);
                        QImage *tmp = new QImage(images[id]->transformed(transform, imageSmooth?Qt::SmoothTransformation:Qt::FastTransformation));

                        if(x1>x2) x1=x2;
                        if(x1>x3) x1=x3;
                        if(x1>x4) x1=x4;
                        if(y1>y2) y1=y2;
                        if(y1>y3) y1=y3;
                        if(y1>y4) y1=y4;

                        if(o!=1.0) painter->setOpacity(o);
                        painter->drawImage((int) x1, (int) y1, *tmp);
                        delete tmp;
                        if (o!=1.0) painter->setOpacity(1.0);

                    }else{
                        error->q(ERROR_IMAGERESOURCE);
                    }

                    if (!fastgraphics && drawingOnScreen) waitForGraphics();
                }
                break;

                case OP_SETGRAPH: {
                    QString id = stack->popstring();
                    setGraph(id);
                }
                break;









                // insert additional OPTYPE_NONE operations here



            }
        }
        break;

        default: {
            //emit(outputReady("optype=" + QString::number(optype(currentop)) + " op=" + QString::number(currentop,16) + QStringLiteral(".\n"));
            emit(outputReady(tr("Error in bytecode during label referencing at line ") + QString::number(currentLine) + QStringLiteral(".\n")));
            return -1;
        }
        break;
    }

    return 0;
}
