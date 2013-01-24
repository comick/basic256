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


%{

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../ByteCodes.h"
#include "../CompileErrors.h"
#include "../WarningCodes.h"
#include "../Version.h"


#define SYMTABLESIZE 2000
#define IFTABLESIZE 1000

extern int yylex();
extern char *yytext;
int yyerror(const char *);
int errorcode;
extern int column;
extern int linenumber;

char *byteCode = NULL;
unsigned int byteOffset = 0;
unsigned int lastLineOffset = 0; // store the byte offset for the end of the last line - use in loops
unsigned int oldByteOffset = 0;
unsigned int listlen = 0;

int functionDefSymbol = -1;	// if in a function definition (what is the symbol number) -1 = not in fundef
int functionType;		// function return type (used in return)
int subroutineDefSymbol = -1;	// if in a subroutine definition (what is the symbol number) -1 = not in fundef

#define FUNCTIONTYPEFLOAT 0
#define FUNCTIONTYPESTRING 1


struct label
{
	char *name;
	int offset;
};

char *EMPTYSTR = "";
char *symtable[SYMTABLESIZE];
int labeltable[SYMTABLESIZE];
int numsyms = 0;
int numlabels = 0;
unsigned int maxbyteoffset = 0;

// array to hold stack of if statement branch locations
// that need to have final jump location added to them
// the iftable is also used by for, subroutine, and function to insure
// that no if,do,while,... is nested incorrectly
unsigned int iftable[IFTABLESIZE];
unsigned int iftablesourceline[IFTABLESIZE];
unsigned int iftabletype[IFTABLESIZE];
int numifs = 0;

#define IFTABLETYPEIF 1
#define IFTABLETYPEELSE 2
#define IFTABLETYPEDO 3
#define IFTABLETYPEWHILE 4
#define IFTABLETYPEFOR 5
#define IFTABLETYPEFUNCTION 6


// store the function variables here during a function definition
unsigned int args[100];
unsigned int argstype[100];
int numargs = 0;

#define ARGSTYPEINT 0
#define ARGSTYPESTR 1
#define ARGSTYPEVARREF 2
#define ARGSTYPEVARREFSTR 3

int
basicParse(char *);

void
clearIfTable() {
	int j;
	for (j = 0; j < IFTABLESIZE; j++) {
		iftable[j] = -1;
		iftablesourceline[j] = -1;
		iftabletype[j] = -1;
	}
	numifs = 0;
}

int testIfOnTable() {
	// return line number if there is an unfinished while.do.if.else
	// or send back -1
	if (numifs >=1 ) {
		return iftablesourceline[numifs-1];
	}	
	return -1;
}

int testIfOnTableError() {
	// return Error number if there is an unfinished while.do.if.else
	// or send back 0
	if (numifs >=1 ) {
		if (iftabletype[numifs-1]==IFTABLETYPEIF) return COMPERR_IFNOEND;
		if (iftabletype[numifs-1]==IFTABLETYPEELSE) return COMPERR_ELSENOEND;
		if (iftabletype[numifs-1]==IFTABLETYPEDO) return COMPERR_DONOEND;
		if (iftabletype[numifs-1]==IFTABLETYPEWHILE) return COMPERR_WHILENOEND;
		if (iftabletype[numifs-1]==IFTABLETYPEFOR) return COMPERR_FORNOEND;
		if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) return COMPERR_FUNCTIONNOEND;
	}	
	return 0;
}

void
clearLabelTable() {
	int j;
	for (j = 0; j < SYMTABLESIZE; j++) {
		labeltable[j] = -1;
	}
	numlabels = 0;
}

void
clearSymbolTable() {
	int j;
	if (numsyms == 0) {
		for (j = 0; j < SYMTABLESIZE; j++) {
			symtable[j] = 0;
		}
	}
	for (j = 0; j < numsyms; j++) {
		if (symtable[j]) {
			free(symtable[j]);
		}
		symtable[j] = 0;
	}
	numsyms = 0;
}

int
getSymbol(char *name) {
	int i;
	for (i = 0; i < numsyms; i++) {
		if (symtable[i] && !strcmp(name, symtable[i]))
			return i;
	}
	return -1;
}

int
newIf(int offset, int sourceline, int type) {
	iftable[numifs] = offset;
	iftablesourceline[numifs] = sourceline;
	iftabletype[numifs] = type;
	numifs++;
	return numifs - 1;
}

int
newSymbol(char *name) {
	symtable[numsyms] = name;
	numsyms++;
	return numsyms - 1;
}

int
newByteCode(unsigned int size) {
	if (byteCode) {
		free(byteCode);
	}
	maxbyteoffset = 1024;
	byteCode = malloc(maxbyteoffset);

	if (byteCode) {
		memset(byteCode, 0, maxbyteoffset);
		byteOffset = 0;
		return 0;
	}
	return -1;
}

void
checkByteMem(unsigned int addedbytes) {
	if (byteOffset + addedbytes + 1 >= maxbyteoffset) {
		maxbyteoffset += maxbyteoffset + addedbytes + 32;
		byteCode = realloc(byteCode, maxbyteoffset);
		memset(byteCode + byteOffset, 0, maxbyteoffset - byteOffset);
	}
}

void
addOp(char op) {
	checkByteMem(sizeof(char));
	byteCode[byteOffset] = op;
	byteOffset++;
}

void
addExtendedOp(char extop) {
	// ad an extended op WITHOUT an argument
	addOp(OP_EXTENDEDNONE);
	addOp(extop);
}

unsigned int
addInt(int data) {
	// add an integer to the bytecode at the current location
	// return starting location of the integer - so we can write to it later
	int *temp;
	unsigned int holdOffset = byteOffset;
	checkByteMem(sizeof(int));
	temp = (int *) (byteCode + byteOffset);
	*temp = data;
	byteOffset += sizeof(int);
	return holdOffset;
}

void
addString(char *data) {
	int len = strlen(data) + 1;
	checkByteMem(len+1);
	strncpy((char *) byteCode + byteOffset, data, len);
	byteOffset += len;
}

void
addIntOp(char op, int data) {
	int *temp = NULL;
	checkByteMem(sizeof(char) + sizeof(int));
	byteCode[byteOffset] = op;
	byteOffset++;

	temp = (int *) (byteCode + byteOffset);
	*temp = data;
	byteOffset += sizeof(int);
}

void
addInt2Op(char op, int data1, int data2) {
	int *temp = NULL;
	checkByteMem(sizeof(char) + 2 * sizeof(int));
	byteCode[byteOffset] = op;
	byteOffset++;

	temp = (int *) (byteCode + byteOffset);
	temp[0] = data1;
	temp[1] = data2;
	byteOffset += 2 * sizeof(int);
}

void
addFloatOp(char op, double data) {
	double *temp = NULL;
	checkByteMem(sizeof(char) + sizeof(double));
	byteCode[byteOffset] = op;
	byteOffset++;

	temp = (double *) (byteCode + byteOffset);
	*temp = data;
	byteOffset += sizeof(double);
}

void
addStringOp(char op, char *data) {
	addOp(op);
	addString(data);
}


#ifdef __cplusplus
}
#endif

%}

%token B256PRINT B256INPUT B256KEY 
%token B256PIXEL B256RGB B256PLOT B256CIRCLE B256RECT B256POLY B256STAMP B256LINE B256FASTGRAPHICS B256GRAPHSIZE B256REFRESH B256CLS B256CLG
%token B256IF B256THEN B256ELSE B256ENDIF B256WHILE B256ENDWHILE B256DO B256UNTIL B256FOR B256TO B256STEP B256NEXT 
%token B256OPEN B256OPENB B256READ B256WRITE B256CLOSE B256RESET
%token B256GOTO B256GOSUB B256RETURN B256REM B256END B256SETCOLOR
%token B256GTE B256LTE B256NE
%token B256DIM B256REDIM B256NOP
%token B256TOINT B256TOSTRING B256LENGTH B256MID B256LEFT B256RIGHT B256UPPER B256LOWER B256INSTR B256INSTRX
%token B256CEIL B256FLOOR B256RAND B256SIN B256COS B256TAN B256ASIN B256ACOS B256ATAN B256ABS B256PI B256DEGREES B256RADIANS B256LOG B256LOGTEN B256SQR B256EXP
%token B256AND B256OR B256XOR B256NOT
%token B256PAUSE B256SOUND
%token B256ASC B256CHR B256TOFLOAT B256READLINE B256WRITELINE B256BOOLEOF B256MOD B256INTDIV
%token B256YEAR B256MONTH B256DAY B256HOUR B256MINUTE B256SECOND B256TEXT B256FONT B256TEXTWIDTH
%token B256SAY B256SYSTEM
%token B256VOLUME
%token B256GRAPHWIDTH B256GRAPHHEIGHT B256GETSLICE B256PUTSLICE B256IMGLOAD
%token B256SPRITEDIM B256SPRITELOAD B256SPRITESLICE B256SPRITEMOVE B256SPRITEHIDE B256SPRITESHOW B256SPRITEPLACE
%token B256SPRITECOLLIDE B256SPRITEX B256SPRITEY B256SPRITEH B256SPRITEW B256SPRITEV B256SPRITER B256SPRITES
%token B256WAVPLAY B256WAVSTOP B256WAVWAIT
%token B256SIZE B256SEEK B256EXISTS
%token B256BOOLTRUE B256BOOLFALSE
%token B256MOUSEX B256MOUSEY B256MOUSEB
%token B256CLICKCLEAR B256CLICKX B256CLICKY B256CLICKB
%token B256GETCOLOR
%token B256CLEAR B256BLACK B256WHITE B256RED B256DARKRED B256GREEN B256DARKGREEN B256BLUE B256DARKBLUE
%token B256CYAN B256DARKCYAN B256PURPLE B256DARKPURPLE B256YELLOW B256DARKYELLOW
%token B256ORANGE B256DARKORANGE B256GREY B256DARKGREY
%token B256CHANGEDIR B256CURRENTDIR B256DIR B256DECIMAL
%token B256DBOPEN B256DBCLOSE B256DBEXECUTE B256DBOPENSET B256DBCLOSESET B256DBROW B256DBINT B256DBFLOAT B256DBSTRING
%token B256ONERROR B256OFFERROR B256LASTERROR B256LASTERRORMESSAGE B256LASTERRORLINE B256LASTERROREXTRA
%token B256NETLISTEN B256NETCONNECT B256NETREAD B256NETWRITE B256NETCLOSE B256NETDATA B256NETADDRESS
%token B256KILL B256MD5 B256SETSETTING B256GETSETTING B256PORTIN B256PORTOUT
%token B256BINARYOR B256BINARYAND B256BINARYNOT
%token B256IMGSAVE
%token B256REPLACE B256COUNT B256EXPLODE B256REPLACEX B256COUNTX B256EXPLODEX B256IMPLODE
%token B256OSTYPE B256MSEC
%token B256EDITVISIBLE B256GRAPHVISIBLE B256OUTPUTVISIBLE B256EDITSIZE B256OUTPUTSIZE
%token B256FUNCTION B256ENDFUNCTION B256THROWERROR B256SUBROUTINE B256ENDSUBROUTINE B256CALL B256GLOBAL
%token B256READBYTE B256WRITEBYTE
%token B256ADD1 B256SUB1 B256ADDEQUAL B256SUBEQUAL B256MULEQUAL B256DIVEQUAL B256REF
%token B256FREEDB B256FREEFILE B256FREENET B256FREEDBSET B256DBNULL
%token B256ARC B256CHORD B256PIE B256PENWIDTH B256GETPENWIDTH B256GETBRUSHCOLOR B256VERSION
%token B256ALERT

%union
{
	int number;
	double floatnum;
	char *string;
}

%token <number> B256LINENUM
%token <number> B256INTEGER
%token <floatnum> B256FLOAT 
%token <string> B256STRING
%token <number> B256VARIABLE
%token <number> B256STRINGVAR
%token <string> B256NEWVAR
%token <number> B256COLOR
%token <number> B256LABEL

%type <floatnum> floatexpr
%type <string> stringexpr

%left B256XOR 
%left B256OR 
%left B256AND 
%nonassoc B256NOT B256ADD1 B256SUB1
%left '<' B256LTE '>' B256GTE '=' B256NE
%left B256BINARYOR B256BINARYAND
%left '-' '+'
%left '*' '/' B256MOD B256INTDIV
%nonassoc B256UMINUS B256BINARYNOT
%left '^'

%%

program: program '\n' validline
	| validline
;

validline: label validstatement
	| validstatement
;

label: B256LABEL {
	if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
		errorcode = COMPERR_FUNCTIONGOTO;
		return -1;
	}
	labeltable[$1] = byteOffset; 
	lastLineOffset = byteOffset;
	addIntOp(OP_CURRLINE, linenumber);
}
;

validstatement: compoundifstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| ifstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| elsestmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| endifstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| whilestmt {
		// push to iftable the byte location of the end of the last stmt (top of loop)
		newIf(lastLineOffset, linenumber-1, IFTABLETYPEWHILE);
		lastLineOffset = byteOffset; 
		addIntOp(OP_CURRLINE, linenumber);
	}
	| endwhilestmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| dostmt {
		// push to iftable the byte location of the end of the last stmt (top of loop)
		newIf(lastLineOffset, linenumber, IFTABLETYPEDO);
		lastLineOffset = byteOffset;
		addIntOp(OP_CURRLINE, linenumber);
	}
	| untilstmt    { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| compoundstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| functionstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| endfunctionstmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| subroutinestmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
	| endsubroutinestmt { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }

	| /*empty*/    { lastLineOffset = byteOffset; addIntOp(OP_CURRLINE, linenumber); }
;

functionstmt: B256FUNCTION B256VARIABLE functionvariablelist {
		if (numifs>0) {
			errorcode = COMPERR_FUNCTIONNOTHERE;
			return -1;
		}
		// $2 is the symbol for the function - add the start to the label table
		functionDefSymbol = $2;
		functionType = FUNCTIONTYPEFLOAT;
		addOp(OP_END); 
		labeltable[$2] = byteOffset;
		lastLineOffset = byteOffset;
		newIf(0, linenumber, IFTABLETYPEFUNCTION);
		// initialize return variable
		addIntOp(OP_PUSHINT, 0);
		addIntOp(OP_NUMASSIGN, $2);
		// check to see if there are enough values on the stack
		addIntOp(OP_PUSHINT, numargs);
		addOp(OP_ARGUMENTCOUNTTEST);
		// add the assigns of the function arguments
		addOp(OP_INCREASERECURSE);
		{ 	int t;
			for(t=numargs-1;t>=0;t--) {
				if (argstype[t]==ARGSTYPEINT) addIntOp(OP_NUMASSIGN, args[t]);
				if (argstype[t]==ARGSTYPESTR) addIntOp(OP_STRINGASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
			}
		}
		numargs=0;	// clear the list for next function
	}
	|
	B256FUNCTION B256STRINGVAR functionvariablelist {
		if (numifs>0) {
			errorcode = COMPERR_FUNCTIONNOTHERE;
			return -1;
		}
		// $2 is the symbol for the function - add the start to the label table
		functionDefSymbol = $2;
		functionType = FUNCTIONTYPESTRING;
		addOp(OP_END); 
		labeltable[$2] = byteOffset;
		lastLineOffset = byteOffset;
		newIf(0, linenumber, IFTABLETYPEFUNCTION);
		// initialize return variable
		addStringOp(OP_PUSHSTRING, "");
		addIntOp(OP_STRINGASSIGN, $2);
		// check to see if there are enough values on the stack
		addIntOp(OP_PUSHINT, numargs);
		addOp(OP_ARGUMENTCOUNTTEST);
		addOp(OP_INCREASERECURSE);
		{ 	int t;
			for(t=numargs-1;t>=0;t--) {
				if (argstype[t]==ARGSTYPEINT) addIntOp(OP_NUMASSIGN, args[t]);
				if (argstype[t]==ARGSTYPESTR) addIntOp(OP_STRINGASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
			}
		}
		numargs=0;	// clear the list for next function
	}
;

subroutinestmt: B256SUBROUTINE B256VARIABLE functionvariablelist {
		if (numifs>0) {
			errorcode = COMPERR_FUNCTIONNOTHERE;
			return -1;
		}
		// $2 is the symbol for the subroutine - add the start to the label table
		subroutineDefSymbol = $2;
		addOp(OP_END); 
		labeltable[$2] = byteOffset;
		lastLineOffset = byteOffset;
		newIf(0, linenumber, IFTABLETYPEFUNCTION);
		// check to see if there are enough values on the stack
		addIntOp(OP_PUSHINT, numargs);
		addOp(OP_ARGUMENTCOUNTTEST);
		// add the assigns of the function arguments
		addOp(OP_INCREASERECURSE);
		{ 	int t;
			for(t=numargs-1;t>=0;t--) {
				if (argstype[t]==ARGSTYPEINT) addIntOp(OP_NUMASSIGN, args[t]);
				if (argstype[t]==ARGSTYPESTR) addIntOp(OP_STRINGASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
				if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
			}
		}
		numargs=0;	// clear the list for next function
	}
;

functionvariablelist: '(' ')' | '(' functionvariables ')'
;


functionvariables: B256VARIABLE {
		args[numargs] = $1; argstype[numargs] = ARGSTYPEINT; numargs++;
		//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs); 
	}
	| B256STRINGVAR {
		args[numargs] = $1; argstype[numargs] = ARGSTYPESTR; numargs++;
		//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs); 
	}
	| B256REF '(' B256VARIABLE ')' {
		args[numargs] = $3; argstype[numargs] = ARGSTYPEVARREF; numargs++;
		//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs); 
	}
	| B256REF '(' B256STRINGVAR ')' {
		args[numargs] = $3; argstype[numargs] = ARGSTYPEVARREFSTR; numargs++;
		//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs); 
	}
	| functionvariables ',' functionvariables
;


endfunctionstmt: B256ENDFUNCTION {
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) {
			addIntOp(OP_PUSHVAR, functionDefSymbol);
			addOp(OP_DECREASERECURSE);
			functionDefSymbol = -1; 
			addOp(OP_RETURN);
			numifs--;
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_ENDFUNCTION;
		return -1;
	}
}
;

endsubroutinestmt: B256ENDSUBROUTINE {
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) {
			addOp(OP_DECREASERECURSE);
			subroutineDefSymbol = -1; 
			addOp(OP_RETURN);
			numifs--;
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_ENDFUNCTION;
		return -1;
	}
}
;

compoundifstmt: ifexpr B256THEN compoundstmt
	{
	// if there is an if branch or jump on the iftable stack get where it is
	// in the bytecode array and then put the current bytecode address there
	// - so we can jump over code
	if (numifs>0) {
		unsigned int *temp = NULL;
		numifs--;
		temp = (unsigned int *) (byteCode + iftable[numifs]);
		*temp = byteOffset;
	}
}
;

ifstmt: ifexpr B256THEN
	{
		// there is nothing to do with a multi line if (ifexp handles it)
	}
;

elsestmt: B256ELSE
	{
	unsigned int elsegototemp = 0;
	// on else create a jump point to the endif
	addIntOp(OP_PUSHINT, 0);	// false - always jump before else to endif
	addOp(OP_BRANCH);
	elsegototemp = addInt(0);
	// resolve the false jump on the if to the current location
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPEIF) {
			unsigned int *temp = NULL;
			numifs--;
			temp = (unsigned int *) (byteCode + iftable[numifs]);
			*temp = byteOffset; 
			// now add the elsegoto jump to the iftable
			newIf(elsegototemp, linenumber, IFTABLETYPEELSE);
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_ELSE;
		return -1;
	}
}
;


endifstmt: B256ENDIF
	{
	// if there is an if branch or jump on the iftable stack get where it is
	// in the bytecode array and then put the current bytecode address there
	// - so we can jump over code
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPEIF||iftabletype[numifs-1]==IFTABLETYPEELSE) {
			unsigned int *temp = NULL;
			numifs--;
			temp = (unsigned int *) (byteCode + iftable[numifs]);
			*temp = byteOffset; 
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_ENDIF;
		return -1;
	}
}
;

whilestmt: B256WHILE floatexpr
	{
	// create temp
	//if true, don't branch. If false, go to next line do the loop.
	addOp(OP_BRANCH);
	// after branch add a placeholder for the final end of the loop
	// it will be resolved in the endwhile statement, push the
	// location of this location on the iftable
	newIf(addInt(0), linenumber-1, IFTABLETYPEWHILE);
}
;

endwhileexpr: B256ENDWHILE;

endwhilestmt: endwhileexpr
	{
	// there should be two bytecode locations.  the TOP is the
	// location to jump to at the top of the loopthe , TOP-1 is the location
	// the exit jump needs to be written back to jump point on WHILE
	if (numifs>1) {
		if (iftabletype[numifs-1]==IFTABLETYPEWHILE) {
			unsigned int *temp = NULL;
			addIntOp(OP_PUSHINT, 0);	// false - always jump back to the beginning
			addIntOp(OP_BRANCH, iftable[numifs-1]);
			// resolve the false jump on the while to the current location
			temp = (unsigned int *) (byteCode + iftable[numifs-2]);
			*temp = byteOffset; 
			numifs-=2;
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_ENDWHILE;
		return -1;
	}
}
;

dostmt: B256DO
	{
		// need nothing done at top of a do
	}
;

untilstmt: B256UNTIL floatexpr
	{
	// create temp
	//if If false, go to to the corresponding do.
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPEDO) {
			addIntOp(OP_BRANCH, iftable[numifs-1]);
			numifs--;
		} else {
			errorcode = testIfOnTableError();
			linenumber = testIfOnTable();
			return -1;
		}
	} else {
		errorcode = COMPERR_UNTIL;
		return -1;
	}
}

compoundstmt: statement | compoundstmt ':' statement
;

statement: gotostmt
	| gosubstmt
	| callstmt
	| offerrorstmt
	| onerrorstmt
	| returnstmt
	| printstmt
	| plotstmt
	| circlestmt
	| rectstmt
	| polystmt
	| stampstmt
	| linestmt
	| numassign
	| stringassign
	| forstmt
	| nextstmt
	| colorstmt
	| inputstmt
	| endstmt
	| clearstmt
	| refreshstmt
	| fastgraphicsstmt
	| graphsizestmt
	| dimstmt
	| redimstmt
	| pausestmt
	| arrayassign
	| strarrayassign
	| openstmt
	| writestmt
	| writelinestmt
	| writebytestmt
	| closestmt
	| resetstmt
	| killstmt
	| soundstmt
	| textstmt
	| fontstmt
	| saystmt
	| systemstmt
	| volumestmt
	| wavplaystmt
	| wavstopstmt
	| wavwaitstmt
	| putslicestmt
	| imgloadstmt
	| spritedimstmt
	| spriteloadstmt
	| spriteslicestmt
	| spriteplacestmt
	| spritemovestmt
	| spritehidestmt
	| spriteshowstmt
	| seekstmt
	| clickclearstmt
	| changedirstmt
	| decimalstmt
	| dbopenstmt
	| dbclosestmt
	| dbexecutestmt
	| dbopensetstmt
	| dbclosesetstmt
	| netlistenstmt
	| netconnectstmt
	| netwritestmt
	| netclosestmt
	| setsettingstmt
	| portoutstmt
	| imgsavestmt
	| editvisiblestmt
	| graphvisiblestmt
	| outputvisiblestmt
	| globalstmt
	| throwerrorstmt
	| arcstmt
	| chordstmt
	| piestmt
	| penwidthstmt
;

dimstmt: B256DIM B256VARIABLE floatexpr {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_DIM, $2);
	}
	| B256DIM B256STRINGVAR floatexpr {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_DIMSTR, $2);
	}
	| B256DIM B256VARIABLE '(' floatexpr ',' floatexpr ')' { addIntOp(OP_DIM, $2); }
	| B256DIM B256STRINGVAR '(' floatexpr ',' floatexpr ')' { addIntOp(OP_DIMSTR, $2); }
	| B256DIM B256VARIABLE '[' floatexpr ']' {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_DIM, $2);
	}
	| B256DIM B256STRINGVAR '[' floatexpr ']' {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_DIMSTR, $2);
	}
	| B256DIM B256VARIABLE '[' floatexpr ',' floatexpr ']' { addIntOp(OP_DIM, $2); }
	| B256DIM B256STRINGVAR '[' floatexpr ',' floatexpr ']' { addIntOp(OP_DIMSTR, $2); }

;

redimstmt: B256REDIM B256VARIABLE floatexpr {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_REDIM, $2);
	}
	| B256REDIM B256STRINGVAR floatexpr {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_REDIMSTR, $2);
	}
	| B256REDIM B256VARIABLE '(' floatexpr ',' floatexpr ')' { addIntOp(OP_REDIM, $2); }
	| B256REDIM B256STRINGVAR '(' floatexpr ',' floatexpr ')' { addIntOp(OP_REDIMSTR, $2); }
	| B256REDIM B256VARIABLE '[' floatexpr ']' {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_REDIM, $2);
	}
	| B256REDIM B256STRINGVAR '[' floatexpr ']' {
		addIntOp(OP_PUSHINT, 1);
		addIntOp(OP_REDIMSTR, $2);
	}
	| B256REDIM B256VARIABLE '[' floatexpr ',' floatexpr ']' { addIntOp(OP_REDIM, $2); }
	| B256REDIM B256STRINGVAR '[' floatexpr ',' floatexpr ']' { addIntOp(OP_REDIMSTR, $2); }
;

pausestmt: B256PAUSE floatexpr { addOp(OP_PAUSE); }
;

throwerrorstmt: B256THROWERROR floatexpr { addOp(OP_THROWERROR); }
;

clearstmt: B256CLS { addOp(OP_CLS); }
	| B256CLG { addOp(OP_CLG); } 
;

fastgraphicsstmt: B256FASTGRAPHICS { addOp(OP_FASTGRAPHICS); }
;

graphsizestmt: B256GRAPHSIZE floatexpr ',' floatexpr { addOp(OP_GRAPHSIZE); }
	| B256GRAPHSIZE '(' floatexpr ',' floatexpr ')' { addOp(OP_GRAPHSIZE); }
;

refreshstmt: B256REFRESH { addOp(OP_REFRESH); }
;

endstmt: B256END { addOp(OP_END); }
;

ifexpr: B256IF floatexpr
	{
	//if true, don't branch. If false, go to next line.
	addOp(OP_BRANCH);
	// after branch add a placeholder for the final end of the if
	// it will be resolved in the if/else/endif statement, push the
	// location of this location on the iftable
	checkByteMem(sizeof(int));
	newIf(byteOffset, linenumber, IFTABLETYPEIF);
	byteOffset += sizeof(int);
	}
;

strarrayassign: B256STRINGVAR '[' floatexpr ']' '=' expr { addIntOp(OP_STRARRAYASSIGN, $1); }
	| B256STRINGVAR '[' floatexpr ']' B256ADDEQUAL expr {
		// a$[n] += s$
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addOp(OP_CONCAT);
		addIntOp(OP_STRARRAYASSIGN, $1);
	}
	| B256STRINGVAR '[' floatexpr ',' floatexpr ']' '=' expr { addIntOp(OP_STRARRAYASSIGN2D, $1); }
	| B256STRINGVAR '[' floatexpr ',' floatexpr ']' B256ADDEQUAL expr {
		// a$[b,c] += s$
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKSWAP2);
		addOp(OP_STACKSWAP);
		addOp(OP_CONCAT);
		addIntOp(OP_STRARRAYASSIGN2D, $1);
	}

	| B256STRINGVAR '=' immediatestrlist {
		addInt2Op(OP_STRARRAYLISTASSIGN, $1, listlen);
		listlen = 0;
	}
	| B256STRINGVAR '=' B256EXPLODE '(' expr ',' expr ')' { addIntOp(OP_EXPLODESTR, $1);}
	| B256STRINGVAR '=' B256EXPLODE '(' expr ',' expr ',' floatexpr ')' { addIntOp(OP_EXPLODESTR_C, $1); }
	| B256STRINGVAR '=' B256EXPLODEX '(' expr ',' stringexpr ')' { addIntOp(OP_EXPLODEXSTR, $1);}
;

arrayassignerrors: B256VARIABLE '[' floatexpr ']' '=' stringexpr
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' '=' stringexpr
	| B256VARIABLE '[' floatexpr ']' B256ADDEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256ADDEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ']' B256SUBEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256SUBEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ']' B256MULEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256MULEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ']' B256DIVEQUAL stringexpr
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256DIVEQUAL stringexpr
	| B256VARIABLE '=' immediatestrlist
;


arrayassign: B256VARIABLE '[' floatexpr ']' '=' floatexpr {
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256ADD1 {
		// a[n]++ (Statement)
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256SUB1 {
		// a[n]-- (Statement)
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256ADDEQUAL floatexpr {
		// a[n] += n
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256SUBEQUAL floatexpr {
		// a[n] -= n
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256MULEQUAL floatexpr {
		// a[n] *= n
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addOp(OP_MUL);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256DIVEQUAL floatexpr {
		// a[n] /= n
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addOp(OP_DIV);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' '=' floatexpr {
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256ADD1 {
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256SUB1 {
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256ADDEQUAL floatexpr {
		// a[b,c] += n
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKSWAP2);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256SUBEQUAL floatexpr {
		// a[b,c] -= n
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKSWAP2);
		addOp(OP_STACKSWAP);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256MULEQUAL floatexpr {
		// a[b,c] *= n
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKSWAP2);
		addOp(OP_MUL);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256DIVEQUAL floatexpr {
		// a[b,c] /= n
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D,$1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKSWAP2);
		addOp(OP_STACKSWAP);
		addOp(OP_DIV);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '=' immediatelist {
		addInt2Op(OP_ARRAYLISTASSIGN, $1, listlen);
		listlen = 0;
	}
	| B256VARIABLE '=' B256EXPLODE '(' expr ',' expr ')' {
		addIntOp(OP_EXPLODE, $1);
	}
	| B256VARIABLE '=' B256EXPLODE '(' expr ',' expr ',' floatexpr ')' {
		addIntOp(OP_EXPLODE_C, $1);
	}
	| B256VARIABLE '=' B256EXPLODEX '(' expr ',' stringexpr ')' {
		addIntOp(OP_EXPLODEX, $1);
	}
	| arrayassignerrors {
		errorcode = COMPERR_ASSIGNS2N;
		if (yytext[0] == '\n') { linenumber--; };
		return -1;
	}
;

numassignerrors: B256VARIABLE '=' stringexpr
	| B256VARIABLE B256ADDEQUAL stringexpr
	| B256VARIABLE B256SUBEQUAL stringexpr
	| B256VARIABLE B256MULEQUAL stringexpr
	| B256VARIABLE B256DIVEQUAL stringexpr
;


numassign: B256VARIABLE '=' floatexpr { addIntOp(OP_NUMASSIGN, $1); }
	| B256VARIABLE B256ADD1 {
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256SUB1 {
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256ADDEQUAL floatexpr {
		addIntOp(OP_PUSHVAR,$1);
		addOp(OP_ADD);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256SUBEQUAL floatexpr {
		addIntOp(OP_PUSHVAR,$1);
		addOp(OP_STACKSWAP);
		addOp(OP_SUB);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256MULEQUAL floatexpr {
		addIntOp(OP_PUSHVAR,$1);
		addOp(OP_MUL);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256DIVEQUAL floatexpr {
		addIntOp(OP_PUSHVAR,$1);
		addOp(OP_STACKSWAP);
		addOp(OP_DIV);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| numassignerrors {
		errorcode = COMPERR_ASSIGNS2N;
		if (yytext[0] == '\n') { linenumber--; };
		return -1;
	}

;

stringassign: B256STRINGVAR '=' expr { addIntOp(OP_STRINGASSIGN, $1); }
	| B256STRINGVAR B256ADDEQUAL expr {
		addIntOp(OP_PUSHVAR,$1);
		addOp(OP_STACKSWAP);
		addOp(OP_CONCAT);
		addIntOp(OP_STRINGASSIGN, $1);
	}
;

forstmt: B256FOR B256VARIABLE '=' floatexpr B256TO floatexpr
	{
		addIntOp(OP_PUSHINT, 1); //step
		addIntOp(OP_FOR, $2);
		// add to iftable to make sure it is not broken with an if
		// do, while, else, and to report if it is
		// next ed before end of program
		newIf(0, linenumber-1, IFTABLETYPEFOR);
	}
	| B256FOR B256VARIABLE '=' floatexpr B256TO floatexpr B256STEP floatexpr
	{
		addIntOp(OP_FOR, $2);
		// add to iftable to make sure it is not broken with an if
		// do, while, else, and to report if it is
		// next ed before end of program
		newIf(0, linenumber-1, IFTABLETYPEFOR);
	}
;

nextstmt: B256NEXT B256VARIABLE
	{
		if (numifs>0) {
			if (iftabletype[numifs-1]==IFTABLETYPEFOR) {
				addIntOp(OP_NEXT, $2);
				numifs--;
			} else {
				errorcode = testIfOnTableError();
				linenumber = testIfOnTable();
				return -1;
			}
		} else {
			errorcode = COMPERR_NEXT;
			return -1;
		}
	}
;

gotostmt: B256GOTO B256VARIABLE {
	if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
		errorcode = COMPERR_FUNCTIONGOTO;
		return -1;
	}
	addIntOp(OP_GOTO, $2);
}
;

gosubstmt: B256GOSUB B256VARIABLE {
	if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
		errorcode = COMPERR_FUNCTIONGOTO;
		return -1;
	}
	addIntOp(OP_GOSUB, $2);
}
;

callstmt: B256CALL B256VARIABLE  { addIntOp(OP_GOSUB, $2); }
	| B256CALL B256VARIABLE '(' ')'  { addIntOp(OP_GOSUB, $2); }
	| B256CALL B256VARIABLE '(' explist ')'  { addIntOp(OP_GOSUB, $2); }
;

offerrorstmt: B256OFFERROR { addExtendedOp(OPX_OFFERROR); }
;

onerrorstmt: B256ONERROR B256VARIABLE { addIntOp(OP_ONERROR, $2); }
;

returnstmt: B256RETURN { 
		if (functionDefSymbol!=-1) {
			// if we are defining a function return pushes a variable value
			addIntOp(OP_PUSHVAR, functionDefSymbol);
			addOp(OP_DECREASERECURSE);
		}
		if (subroutineDefSymbol!=-1) {
			// if we are defining a subroutine
			addOp(OP_DECREASERECURSE);
		}
		addOp(OP_RETURN);
	}
	| B256RETURN floatexpr { 
		if (functionDefSymbol!=-1) {
			if (functionType==FUNCTIONTYPEFLOAT) {
				// value on stack gets returned
				addOp(OP_DECREASERECURSE);
				addOp(OP_RETURN);
			} else {
				errorcode = COMPERR_RETURNTYPE;
				return -1;
			}
		} else {
			errorcode = COMPERR_RETURNVALUE;
			return -1;
		}
	}
	| B256RETURN stringexpr { 
		if (functionDefSymbol!=-1) {
			if (functionType==FUNCTIONTYPESTRING) {
				// value on stack gets returned
				addOp(OP_DECREASERECURSE);
				addOp(OP_RETURN);
			} else {
				errorcode = COMPERR_RETURNTYPE;
				return -1;
			}
		} else {
			errorcode = COMPERR_RETURNVALUE;
			return -1;
		}
	}
;

colorstmt: B256SETCOLOR floatexpr ',' floatexpr ',' floatexpr {
		addIntOp(OP_PUSHINT, 255); 
		addOp(OP_RGB);
		addOp(OP_STACKDUP);
		addOp(OP_SETCOLORINT);
		addIntOp(OP_PUSHINT, WARNING_DEPRECATED_FORM); 
		addExtendedOp(OPX_RUNTIMEWARNING);
	}
	| B256SETCOLOR '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT, 255); 
		addOp(OP_RGB);
		addOp(OP_STACKDUP);
		addOp(OP_SETCOLORINT);
		addIntOp(OP_PUSHINT, WARNING_DEPRECATED_FORM); 
		addExtendedOp(OPX_RUNTIMEWARNING);
	}
	| B256SETCOLOR floatexpr {
		addOp(OP_STACKDUP);
		addOp(OP_SETCOLORINT);
	}
	| B256SETCOLOR floatexpr ',' floatexpr  { addOp(OP_SETCOLORINT); }
	| B256SETCOLOR '(' floatexpr ',' floatexpr ')'  { addOp(OP_SETCOLORINT); }
;

soundstmt: B256SOUND '(' B256VARIABLE ')' { addIntOp(OP_SOUND_ARRAY, $3); }
	| B256SOUND B256VARIABLE { addIntOp(OP_SOUND_ARRAY, $2); }
	| B256SOUND immediatelist { addIntOp(OP_SOUND_LIST, listlen); listlen=0; }
	| B256SOUND '(' floatexpr ',' floatexpr ')' { addOp(OP_SOUND); }
	| B256SOUND floatexpr ',' floatexpr { addOp(OP_SOUND); }
;

plotstmt: B256PLOT floatexpr ',' floatexpr { addOp(OP_PLOT); }
	| B256PLOT '(' floatexpr ',' floatexpr ')' { addOp(OP_PLOT); }
;

linestmt: B256LINE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addOp(OP_LINE); }
	| B256LINE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_LINE); }
;


circlestmt: B256CIRCLE floatexpr ',' floatexpr ',' floatexpr { addOp(OP_CIRCLE); }
	| B256CIRCLE '(' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_CIRCLE); }
;

arcstmt: B256ARC floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addExtendedOp(OPX_ARC); }
	| B256ARC '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_ARC); }
;

chordstmt: B256CHORD floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addExtendedOp(OPX_CHORD); }
	| B256CHORD '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_CHORD); }
;

piestmt: B256PIE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addExtendedOp(OPX_PIE); }
	| B256PIE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_PIE); }
;

rectstmt: B256RECT floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addOp(OP_RECT); }
	| B256RECT '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_RECT); }
;

textstmt: B256TEXT floatexpr ',' floatexpr ',' stringexpr { addOp(OP_TEXT); }
	| B256TEXT '(' floatexpr ',' floatexpr ',' stringexpr ')' { addOp(OP_TEXT); }
	| B256TEXT floatexpr ',' floatexpr ',' floatexpr { addOp(OP_TEXT); }
	| B256TEXT '(' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_TEXT); }
;

fontstmt: B256FONT stringexpr ',' floatexpr ',' floatexpr { addOp(OP_FONT); }
	| B256FONT '(' stringexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_FONT); }
;

saystmt: B256SAY expr { addOp(OP_SAY); }
;

systemstmt: B256SYSTEM stringexpr { addOp(OP_SYSTEM); }
;

volumestmt: B256VOLUME floatexpr { addOp(OP_VOLUME); } // parens added by single floatexpr
;

polystmt: B256POLY B256VARIABLE { addIntOp(OP_POLY, $2); }
	| B256POLY '(' B256VARIABLE ')' { addIntOp(OP_POLY, $3); }
	| B256POLY immediatelist { addIntOp(OP_POLY_LIST, listlen); listlen=0; }
;

stampstmt: B256STAMP floatexpr ',' floatexpr ',' floatexpr ',' B256VARIABLE {
		addFloatOp(OP_PUSHFLOAT, 0);
		addIntOp(OP_STAMP, $8);
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' floatexpr ',' B256VARIABLE ')' {
		addFloatOp(OP_PUSHFLOAT, 0);
		addIntOp(OP_STAMP, $9);
	}
	| B256STAMP floatexpr ',' floatexpr ',' floatexpr ',' immediatelist {
		addIntOp(OP_STAMP_S_LIST, listlen);
		listlen=0;
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' floatexpr ',' immediatelist ')' {
		addIntOp(OP_STAMP_S_LIST, listlen);
		listlen=0;
	}
	| B256STAMP floatexpr ',' floatexpr ',' B256VARIABLE {
		addFloatOp(OP_PUSHFLOAT, 1);
		addFloatOp(OP_PUSHFLOAT, 0);
		addIntOp(OP_STAMP, $6);
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' B256VARIABLE ')' {
		addFloatOp(OP_PUSHFLOAT, 1);
		addFloatOp(OP_PUSHFLOAT, 0);
		addIntOp(OP_STAMP, $7);
	}
	| B256STAMP floatexpr ',' floatexpr ',' immediatelist {
		addIntOp(OP_STAMP_LIST, listlen);
		listlen=0;
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' immediatelist ')' {
		addIntOp(OP_STAMP_LIST, listlen);
		listlen=0;
	}
	| B256STAMP floatexpr ',' floatexpr ','  floatexpr ',' floatexpr ',' B256VARIABLE {
		addIntOp(OP_STAMP, $10);
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' B256VARIABLE ')' {
		addIntOp(OP_STAMP, $11);
	}
	| B256STAMP floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' immediatelist {
		addIntOp(OP_STAMP_SR_LIST, listlen);
		listlen=0;
	}
	| B256STAMP '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' immediatelist ')' {
		addIntOp(OP_STAMP_SR_LIST, listlen);
		listlen=0;
	}
;

openstmt: B256OPEN stringexpr  {
		addIntOp(OP_PUSHINT, 0); // file number zero
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT, 0); // not binary
		addOp(OP_OPEN);
	}
	| B256OPEN '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT, 0); // not binary
		addOp(OP_OPEN);
	} 
	| B256OPEN floatexpr ',' stringexpr {
		addIntOp(OP_PUSHINT, 0); // not binary
		addOp(OP_OPEN);
	}
	| B256OPENB stringexpr  {
		addIntOp(OP_PUSHINT, 0); // file number zero
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT, 1); // binary
		addOp(OP_OPEN);
	}
	| B256OPENB '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT, 1); // binary
		addOp(OP_OPEN);
	} 
	| B256OPENB floatexpr ',' stringexpr {
		addIntOp(OP_PUSHINT, 1); // binary
		addOp(OP_OPEN);
	}
;

writestmt: B256WRITE stringexpr {
		addIntOp(OP_PUSHINT, 0);  // file number zero
		addOp(OP_STACKSWAP);
		addOp(OP_WRITE);
	}
	| B256WRITE '(' floatexpr ','stringexpr ')' {
		addOp(OP_WRITE);
	}
	| B256WRITE floatexpr ','stringexpr {
		addOp(OP_WRITE);
	}
;

writelinestmt: B256WRITELINE stringexpr { addIntOp(OP_PUSHINT, 0); addOp(OP_STACKSWAP); addOp(OP_WRITELINE); }
	| B256WRITELINE '(' floatexpr ',' stringexpr ')' { addOp(OP_WRITELINE); }
	| B256WRITELINE floatexpr ',' stringexpr { addOp(OP_WRITELINE); }
;

writebytestmt: B256WRITEBYTE floatexpr { addIntOp(OP_PUSHINT, 0); addOp(OP_STACKSWAP); addOp(OP_WRITEBYTE); }
	| B256WRITEBYTE '(' floatexpr ',' floatexpr ')' { addOp(OP_WRITEBYTE); }
	| B256WRITEBYTE floatexpr ',' floatexpr { addOp(OP_WRITEBYTE); }
;

closestmt: B256CLOSE { addIntOp(OP_PUSHINT, 0); addOp(OP_CLOSE); }
	| B256CLOSE '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_CLOSE); }
	| B256CLOSE floatexpr { addOp(OP_CLOSE); }
;

resetstmt: B256RESET { addIntOp(OP_PUSHINT, 0); addOp(OP_RESET); }
	| B256RESET '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_RESET); }
	| B256RESET floatexpr { addOp(OP_RESET); }
;

seekstmt: B256SEEK floatexpr  {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKSWAP);
		addOp(OP_SEEK);
	}
	| B256SEEK '(' floatexpr ',' floatexpr ')' { addOp(OP_SEEK); }
	| B256SEEK floatexpr ',' floatexpr { addOp(OP_SEEK); }
;

inputexpr: B256INPUT stringexpr {
		addOp(OP_PRINT);
		addOp(OP_INPUT);
	}
;

inputstmt: inputexpr ',' B256STRINGVAR { addIntOp(OP_STRINGASSIGN, $3); }
	| inputexpr ',' B256STRINGVAR '[' floatexpr ']' {
		addOp(OP_STACKSWAP);
		addIntOp(OP_STRARRAYASSIGN, $3);
	}
	| inputexpr ',' B256VARIABLE { addIntOp(OP_NUMASSIGN, $3); }
	| inputexpr ',' B256VARIABLE '[' floatexpr ']' {
		addOp(OP_STACKSWAP);
		addIntOp(OP_ARRAYASSIGN, $3);
	}
	| B256INPUT B256STRINGVAR  {
		addOp(OP_INPUT);
		addIntOp(OP_STRINGASSIGN, $2);
	}
	| B256INPUT B256STRINGVAR '[' floatexpr ']' {
		addOp(OP_INPUT);
		addIntOp(OP_STRARRAYASSIGN, $2);
	}
	| B256INPUT B256STRINGVAR '[' floatexpr ',' floatexpr ']' {
		addOp(OP_INPUT);
		addIntOp(OP_STRARRAYASSIGN2D, $2);
	}
	| B256INPUT B256VARIABLE  {
		addOp(OP_INPUT);
		addIntOp(OP_NUMASSIGN, $2);
	}
	| B256INPUT B256VARIABLE '[' floatexpr ']' {
		addOp(OP_INPUT);
		addIntOp(OP_ARRAYASSIGN, $2);
	}
	| B256INPUT B256VARIABLE '[' floatexpr ',' floatexpr ']' {
		addOp(OP_INPUT);
		addIntOp(OP_ARRAYASSIGN2D, $2);
	}
;

printstmt: B256PRINT { addStringOp(OP_PUSHSTRING, ""); addOp(OP_PRINTN); }
	| B256PRINT expr { addOp(OP_PRINTN); }
	| B256PRINT expr ';' { addOp(OP_PRINT); }
;

wavplaystmt: B256WAVPLAY stringexpr {addOp(OP_WAVPLAY);  }
;

wavstopstmt: B256WAVSTOP { addOp(OP_WAVSTOP); }
	| B256WAVSTOP '(' ')' { addOp(OP_WAVSTOP); }
;

wavwaitstmt: B256WAVWAIT { addExtendedOp(OPX_WAVWAIT); }
	| B256WAVWAIT '(' ')' { addExtendedOp(OPX_WAVWAIT); }
;

putslicestmt: B256PUTSLICE floatexpr ',' floatexpr ',' stringexpr  {addOp(OP_PUTSLICE);  }
	| B256PUTSLICE '(' floatexpr ',' floatexpr ',' stringexpr ')' { addOp(OP_PUTSLICE); }
	| B256PUTSLICE floatexpr ',' floatexpr ',' stringexpr ',' floatexpr  {addOp(OP_PUTSLICEMASK);  }
	| B256PUTSLICE '(' floatexpr ',' floatexpr ',' stringexpr  ',' floatexpr')' { addOp(OP_PUTSLICEMASK); };

imgloadstmt: B256IMGLOAD floatexpr ',' floatexpr ',' stringexpr
	{
		addIntOp(OP_PUSHINT, 1); // scale
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT, 0); // rotate
		addOp(OP_STACKSWAP);
		addOp(OP_IMGLOAD);
	}
	| B256IMGLOAD '(' floatexpr ',' floatexpr ',' stringexpr ')'
	{
		addIntOp(OP_PUSHINT, 1); // scale
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT, 0); // rotate
		addOp(OP_STACKSWAP);
		addOp(OP_IMGLOAD);
	}
	| B256IMGLOAD floatexpr ',' floatexpr ',' floatexpr ',' stringexpr
	{
		addIntOp(OP_PUSHINT, 0); // rotate
		addOp(OP_STACKSWAP);
		addOp(OP_IMGLOAD);
	}
	| B256IMGLOAD '(' floatexpr ',' floatexpr ',' floatexpr ',' stringexpr ')'
	{
		addIntOp(OP_PUSHINT, 0); // rotate
		addOp(OP_STACKSWAP);
		addOp(OP_IMGLOAD);
	}
	| B256IMGLOAD floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' stringexpr
	{
		addOp(OP_IMGLOAD);
	}
	| B256IMGLOAD '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' stringexpr ')'
	{
		addOp(OP_IMGLOAD);
	}
;

spritedimstmt: B256SPRITEDIM floatexpr { addExtendedOp(OPX_SPRITEDIM); } 
;

spriteloadstmt: B256SPRITELOAD floatexpr ',' stringexpr  {addExtendedOp(OPX_SPRITELOAD);  }
	| B256SPRITELOAD '(' floatexpr ',' stringexpr ')' { addExtendedOp(OPX_SPRITELOAD); }
;

spriteslicestmt: B256SPRITESLICE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr  {addExtendedOp(OPX_SPRITESLICE);  }
	| B256SPRITESLICE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_SPRITESLICE); }
;

spriteplacestmt: B256SPRITEPLACE floatexpr ',' floatexpr ',' floatexpr  {
		addIntOp(OP_PUSHINT,1);	// scale
		addIntOp(OP_PUSHINT,0);	// rotate
		addExtendedOp(OPX_SPRITEPLACE);
	}
	| B256SPRITEPLACE '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,1);	// scale
		addIntOp(OP_PUSHINT,0);	// rotate
		addExtendedOp(OPX_SPRITEPLACE);
	}
	| B256SPRITEPLACE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr {
		addIntOp(OP_PUSHINT,0);	// rotate
		addExtendedOp(OPX_SPRITEPLACE);
	}
	| B256SPRITEPLACE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// rotate
		addExtendedOp(OPX_SPRITEPLACE);
	}
	| B256SPRITEPLACE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr {
		addExtendedOp(OPX_SPRITEPLACE);
	}
	| B256SPRITEPLACE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_SPRITEPLACE);
	}
;

spritemovestmt: B256SPRITEMOVE floatexpr ',' floatexpr ',' floatexpr  {
		addIntOp(OP_PUSHINT,0);	// scale (change in scale)
		addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
		addExtendedOp(OPX_SPRITEMOVE);
	}
	| B256SPRITEMOVE '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// scale (change in scale)
		addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
		addExtendedOp(OPX_SPRITEMOVE);
	}
	| B256SPRITEMOVE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr  {
		addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
		addExtendedOp(OPX_SPRITEMOVE);
	}
	| B256SPRITEMOVE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
		addExtendedOp(OPX_SPRITEMOVE);
	}
	| B256SPRITEMOVE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr  {
		addExtendedOp(OPX_SPRITEMOVE);
	}
	| B256SPRITEMOVE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_SPRITEMOVE);
	}
;

spritehidestmt: B256SPRITEHIDE floatexpr { addExtendedOp(OPX_SPRITEHIDE); } 
;

spriteshowstmt: B256SPRITESHOW floatexpr { addExtendedOp(OPX_SPRITESHOW); } 
;

clickclearstmt: B256CLICKCLEAR  {addOp(OP_CLICKCLEAR);  }
	| B256CLICKCLEAR '(' ')' { addOp(OP_CLICKCLEAR); }
;

changedirstmt: B256CHANGEDIR stringexpr { addExtendedOp(OPX_CHANGEDIR); }
;

decimalstmt: B256DECIMAL floatexpr { addExtendedOp(OPX_DECIMAL); }
;

dbopenstmt: B256DBOPEN stringexpr {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBOPEN);
	}
	| B256DBOPEN floatexpr ',' stringexpr {
		addExtendedOp(OPX_DBOPEN);
	}
	| B256DBOPEN '(' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBOPEN);
	}
;

dbclosestmt: B256DBCLOSE {
		addIntOp(OP_PUSHINT,0);	// default db number
		addExtendedOp(OPX_DBCLOSE);
	}
	| B256DBCLOSE '(' ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addExtendedOp(OPX_DBCLOSE);
	}
	| B256DBCLOSE floatexpr {
		addExtendedOp(OPX_DBCLOSE);
	}
;

dbexecutestmt: B256DBEXECUTE stringexpr {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBEXECUTE);
	}
	| B256DBEXECUTE floatexpr ',' stringexpr {
		addExtendedOp(OPX_DBEXECUTE);
	}
	| B256DBEXECUTE '(' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBEXECUTE);
	}
;

dbopensetstmt: B256DBOPENSET stringexpr {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBOPENSET);
	}
	| B256DBOPENSET floatexpr ',' stringexpr {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBOPENSET);
	}
	| B256DBOPENSET '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBOPENSET);
	}
	| B256DBOPENSET floatexpr ',' floatexpr ',' stringexpr {
		addExtendedOp(OPX_DBOPENSET);
	}
	| B256DBOPENSET '(' floatexpr ',' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBOPENSET);
	}
;

dbclosesetstmt: B256DBCLOSESET {
		addIntOp(OP_PUSHINT,0);	// default db number
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBCLOSESET);
	}
	| B256DBCLOSESET '(' ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBCLOSESET);
	}
	| B256DBCLOSESET floatexpr {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBCLOSESET);
	}
	| B256DBCLOSESET '(' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_DBCLOSESET);
	}
	| B256DBCLOSESET floatexpr ',' floatexpr {
		addExtendedOp(OPX_DBCLOSESET);
	}
;

netlistenstmt: B256NETLISTEN floatexpr {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_NETLISTEN);
	}
	| B256NETLISTEN '(' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_NETLISTEN); } 
	| B256NETLISTEN floatexpr ',' floatexpr { addExtendedOp(OPX_NETLISTEN); }
;

netconnectstmt: B256NETCONNECT stringexpr ',' floatexpr {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKTOPTO2);
		addExtendedOp(OPX_NETCONNECT);
	}
	| B256NETCONNECT '(' stringexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKTOPTO2);
		addExtendedOp(OPX_NETCONNECT);
	}
	| B256NETCONNECT floatexpr ',' stringexpr ',' floatexpr { addExtendedOp(OPX_NETCONNECT); }
	| B256NETCONNECT '(' floatexpr ',' stringexpr ',' floatexpr ')' { addExtendedOp(OPX_NETCONNECT); }
;

netwritestmt: B256NETWRITE stringexpr {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_NETWRITE);
	}
	| B256NETWRITE '(' floatexpr ',' stringexpr ')' { addExtendedOp(OPX_NETWRITE); } 
	| B256NETWRITE floatexpr ',' stringexpr { addExtendedOp(OPX_NETWRITE); }
;

netclosestmt: B256NETCLOSE { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETCLOSE); }
	| B256NETCLOSE '(' ')' { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETCLOSE); }
	| B256NETCLOSE floatexpr { addExtendedOp(OPX_NETCLOSE); }
;

killstmt: B256KILL stringexpr { addExtendedOp(OPX_KILL); }
;

setsettingstmt: B256SETSETTING expr ',' expr ',' expr { addExtendedOp(OPX_SETSETTING); }
	| B256SETSETTING '(' expr ',' expr ',' expr ')' { addExtendedOp(OPX_SETSETTING); } 
;

portoutstmt: B256PORTOUT floatexpr ',' floatexpr { addExtendedOp(OPX_PORTOUT); }
	| B256PORTOUT '(' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_PORTOUT); } 
;

imgsavestmt: B256IMGSAVE stringexpr  {
		addStringOp(OP_PUSHSTRING, "PNG");
		addExtendedOp(OPX_IMGSAVE);
	} 
	| B256IMGSAVE '(' stringexpr ',' stringexpr ')' { addExtendedOp(OPX_IMGSAVE); }
	| B256IMGSAVE stringexpr ',' stringexpr { addExtendedOp(OPX_IMGSAVE); }
;

editvisiblestmt: B256EDITVISIBLE floatexpr { addExtendedOp(OPX_EDITVISIBLE); }
;

graphvisiblestmt: B256GRAPHVISIBLE floatexpr { addExtendedOp(OPX_GRAPHVISIBLE); }
;

outputvisiblestmt: B256OUTPUTVISIBLE floatexpr { addExtendedOp(OPX_OUTPUTVISIBLE); }
;

globalstmt: B256GLOBAL functionvariables {
	// create ops to make all of the variables listed globals
	if (numifs>0) {
		errorcode = COMPERR_GLOBALNOTHERE;
		return -1;
	}
 	int t;
	for(t=numargs-1;t>=0;t--) {
		addIntOp(OP_GLOBAL, args[t]);
	}
	numargs=0;	// clear the list for next function
}
;

penwidthstmt: B256PENWIDTH floatexpr {
		addExtendedOp(OPX_PENWIDTH);
	}
;





immediatestrlist: '{' stringlist '}'
;

immediatelist: '{' floatlist '}'
;

floatlist: floatexpr { listlen = 1; }
	| floatexpr ',' floatlist { listlen++; }
;

stringlist: stringexpr { listlen = 1; }
	| stringexpr ',' stringlist { listlen++; }
;

explist: expr { listlen = 1; }
	| explist ',' explist {listlen++;}
;

expr: floatexpr
	| stringexpr
;


floatexpr: '(' floatexpr ')' { $$ = $2; }
	| floatexpr '+' floatexpr { addOp(OP_ADD); }
	| floatexpr '-' floatexpr { addOp(OP_SUB); }
	| floatexpr '*' floatexpr { addOp(OP_MUL); }
	| floatexpr B256MOD floatexpr { addOp(OP_MOD); }
	| floatexpr B256INTDIV floatexpr { addOp(OP_INTDIV); }
	| floatexpr '/' floatexpr { addOp(OP_DIV); }
	| floatexpr '^' floatexpr { addOp(OP_EX); }
	| floatexpr B256BINARYOR floatexpr { addExtendedOp(OPX_BINARYOR); }
	| floatexpr B256BINARYAND floatexpr { addExtendedOp(OPX_BINARYAND); }
	| B256BINARYNOT floatexpr { addExtendedOp(OPX_BINARYNOT); }
	| '-' floatexpr %prec B256UMINUS { addOp(OP_NEGATE); }
	| floatexpr B256AND floatexpr {addOp(OP_AND); }
	| floatexpr B256OR floatexpr { addOp(OP_OR); }
	| floatexpr B256XOR floatexpr { addOp(OP_XOR); }
	| B256NOT floatexpr %prec B256UMINUS { addOp(OP_NOT); }
	| floatexpr '=' floatexpr { addOp(OP_EQUAL); }
	| stringexpr '=' stringexpr { addOp(OP_EQUAL); } 
	| floatexpr B256NE floatexpr { addOp(OP_NEQUAL); }
	| stringexpr B256NE stringexpr { addOp(OP_NEQUAL); }
	| floatexpr '<' floatexpr { addOp(OP_LT); }
	| stringexpr '<' stringexpr { addOp(OP_LT); }
	| floatexpr '>' floatexpr { addOp(OP_GT); }
	| stringexpr '>' stringexpr { addOp(OP_GT); }
	| floatexpr B256GTE floatexpr { addOp(OP_GTE); }
	| stringexpr B256GTE stringexpr { addOp(OP_GTE); }
	| floatexpr B256LTE floatexpr { addOp(OP_LTE); }
	| stringexpr B256LTE stringexpr { addOp(OP_LTE); }
	| B256FLOAT   { addFloatOp(OP_PUSHFLOAT, $1); }
	| B256INTEGER { addIntOp(OP_PUSHINT, $1); }
	| B256VARIABLE '[' '?' ']' { addIntOp(OP_ALEN, $1); }
	| B256STRINGVAR '[' '?' ']' { addIntOp(OP_ALEN, $1); }
	| B256VARIABLE '[' '?' ',' ']' { addIntOp(OP_ALENX, $1); }
	| B256STRINGVAR '[' '?' ',' ']' { addIntOp(OP_ALENX, $1); }
	| B256VARIABLE '[' ',' '?' ']' { addIntOp(OP_ALENY, $1); }
	| B256STRINGVAR '[' ',' '?' ']' { addIntOp(OP_ALENY, $1); }
	| B256VARIABLE '[' floatexpr ']' { addIntOp(OP_DEREF, $1); }
	| B256VARIABLE '[' floatexpr ']' B256ADD1 {
		// a[n]++
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $1);
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256VARIABLE '[' floatexpr ']' B256SUB1 {
		// a[n]--
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $1);
		addOp(OP_STACKSWAP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN, $1);
	}
	| B256ADD1 B256VARIABLE '[' floatexpr ']' {
		// ++a[n]
		addOp(OP_STACKDUP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN, $2);
		addIntOp(OP_DEREF, $2);
	}
	| B256SUB1 B256VARIABLE '[' floatexpr ']' {
		// --a[n]
		addOp(OP_STACKDUP);
		addOp(OP_STACKDUP);
		addIntOp(OP_DEREF, $2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN, $2);
		addIntOp(OP_DEREF, $2);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' { addIntOp(OP_DEREF2D, $1); }
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256ADD1 {
		// a[b,c]++
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256VARIABLE '[' floatexpr ',' floatexpr ']' B256SUB1 {
		// a[b,c]--
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $1);
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN2D, $1);
	}
	| B256ADD1 B256VARIABLE '[' floatexpr ',' floatexpr ']' {
		// ++a[b,c]
		addOp(OP_STACKDUP2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_ARRAYASSIGN2D, $2);
		addIntOp(OP_DEREF2D, $2);
	}
	| B256SUB1 B256VARIABLE '[' floatexpr ',' floatexpr ']' {
		// --a[b,c]
		addOp(OP_STACKDUP2);
		addOp(OP_STACKDUP2);
		addIntOp(OP_DEREF2D, $2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_ARRAYASSIGN2D, $2);
		addIntOp(OP_DEREF2D, $2);
	}
	| B256VARIABLE '(' explist ')' {
		// function call with arguments
		addIntOp(OP_GOSUB, $1);
	}
	| B256VARIABLE '(' ')' {
		// function call without arguments
		addIntOp(OP_GOSUB, $1);
	}
	| B256VARIABLE {
		if ($1 < 0) {
			return -1;
		} else {
			addIntOp(OP_PUSHVAR, $1);
		}
	}
	| B256VARIABLE B256ADD1 {
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256VARIABLE B256SUB1 {
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHVAR,$1);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_NUMASSIGN, $1);
	}
	| B256ADD1 B256VARIABLE {
		addIntOp(OP_PUSHVAR,$2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_NUMASSIGN, $2);
		addIntOp(OP_PUSHVAR,$2);
	}
	| B256SUB1 B256VARIABLE {
		addIntOp(OP_PUSHVAR,$2);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_SUB);
		addIntOp(OP_NUMASSIGN, $2);
		addIntOp(OP_PUSHVAR,$2);
	}
	| B256TOINT '(' expr ')' { addOp(OP_INT); }
	| B256TOFLOAT '(' expr ')' { addOp(OP_FLOAT); }
	| B256LENGTH '(' expr ')' { addOp(OP_LENGTH); }
	| B256ASC '(' expr ')' { addOp(OP_ASC); }
	| B256INSTR '(' expr ',' expr ')' { addOp(OP_INSTR); }
	| B256INSTR '(' expr ',' expr ',' floatexpr ')' { addOp(OP_INSTR_S); }
	| B256INSTR '(' expr ',' expr ',' floatexpr ',' floatexpr')' { addOp(OP_INSTR_SC); }
	| B256INSTRX '(' expr ',' stringexpr ')' { addOp(OP_INSTRX); }
	| B256INSTRX '(' expr ',' stringexpr ',' floatexpr ')' { addOp(OP_INSTRX_S); }
	| B256CEIL '(' floatexpr ')' { addOp(OP_CEIL); }
	| B256FLOOR '(' floatexpr ')' { addOp(OP_FLOOR); }
	| B256SIN '(' floatexpr ')' { addOp(OP_SIN); }
	| B256COS '(' floatexpr ')' { addOp(OP_COS); }
	| B256TAN '(' floatexpr ')' { addOp(OP_TAN); }
	| B256ASIN '(' floatexpr ')' { addOp(OP_ASIN); }
	| B256ACOS '(' floatexpr ')' { addOp(OP_ACOS); }
	| B256ATAN '(' floatexpr ')' { addOp(OP_ATAN); }
	| B256DEGREES '(' floatexpr ')' { addOp(OP_DEGREES); }
	| B256RADIANS '(' floatexpr ')' { addOp(OP_RADIANS); }
	| B256LOG '(' floatexpr ')' { addOp(OP_LOG); }
	| B256LOGTEN '(' floatexpr ')' { addOp(OP_LOGTEN); }
	| B256SQR '(' floatexpr ')' { addOp(OP_SQR); }
	| B256EXP '(' floatexpr ')' { addOp(OP_EXP); }
	| B256ABS '(' floatexpr ')' { addOp(OP_ABS); }
	| B256RAND { addOp(OP_RAND); }
	| B256RAND '(' ')' { addOp(OP_RAND); }
	| B256PI { addFloatOp(OP_PUSHFLOAT, 3.14159265); }
	| B256PI '(' ')' { addFloatOp(OP_PUSHFLOAT, 3.14159265); }
	| B256BOOLTRUE { addIntOp(OP_PUSHINT, 1); }
	| B256BOOLTRUE '(' ')' { addIntOp(OP_PUSHINT, 1); }
	| B256BOOLFALSE { addIntOp(OP_PUSHINT, 0); }
	| B256BOOLFALSE '(' ')' { addIntOp(OP_PUSHINT, 0); }
	| B256BOOLEOF {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_EOF);
	}
	| B256BOOLEOF '(' ')' {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_EOF);
	}
	| B256BOOLEOF '(' floatexpr ')' { addOp(OP_EOF); }
	| B256EXISTS '(' stringexpr ')' { addOp(OP_EXISTS); }
	| B256YEAR { addOp(OP_YEAR); }
	| B256YEAR '(' ')' { addOp(OP_YEAR); }
	| B256MONTH { addOp(OP_MONTH); }
	| B256MONTH '(' ')' { addOp(OP_MONTH); }
	| B256DAY { addOp(OP_DAY); }
	| B256DAY '(' ')' { addOp(OP_DAY); }
	| B256HOUR { addOp(OP_HOUR); }
	| B256HOUR '(' ')' { addOp(OP_HOUR); }
	| B256MINUTE { addOp(OP_MINUTE); }
	| B256MINUTE '(' ')' { addOp(OP_MINUTE); }
	| B256SECOND { addOp(OP_SECOND); }
	| B256SECOND '(' ')' { addOp(OP_SECOND); }
	| B256GRAPHWIDTH { addOp(OP_GRAPHWIDTH); }
	| B256GRAPHWIDTH '(' ')' { addOp(OP_GRAPHWIDTH); }
	| B256GRAPHHEIGHT { addOp(OP_GRAPHHEIGHT); }
	| B256GRAPHHEIGHT '(' ')' { addOp(OP_GRAPHHEIGHT); }
	| B256SIZE { addIntOp(OP_PUSHINT, 0); addOp(OP_SIZE); }
	| B256SIZE '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_SIZE); }
	| B256SIZE '(' floatexpr ')' { addOp(OP_SIZE); }
	| B256KEY     { addOp(OP_KEY); }
	| B256KEY '(' ')'     { addOp(OP_KEY); }
	| B256MOUSEX { addOp(OP_MOUSEX); }
	| B256MOUSEX '(' ')' { addOp(OP_MOUSEX); }
	| B256MOUSEY { addOp(OP_MOUSEY); }
	| B256MOUSEY '(' ')' { addOp(OP_MOUSEY); }
	| B256MOUSEB { addOp(OP_MOUSEB); }
	| B256MOUSEB '(' ')' { addOp(OP_MOUSEB); }
	| B256CLICKX { addOp(OP_CLICKX); }
	| B256CLICKX '(' ')' { addOp(OP_CLICKX); }
	| B256CLICKY { addOp(OP_CLICKY); }
	| B256CLICKY '(' ')' { addOp(OP_CLICKY); }
	| B256CLICKB { addOp(OP_CLICKB); }
	| B256CLICKB '(' ')' { addOp(OP_CLICKB); }
	| B256CLEAR { addIntOp(OP_PUSHINT, 0x00000000); }
	| B256CLEAR '(' ')' { addIntOp(OP_PUSHINT, 0x00000000); }
	| B256BLACK { addIntOp(OP_PUSHINT, 0xff000000); }
	| B256BLACK '(' ')' { addIntOp(OP_PUSHINT, 0xff000000); }
	| B256WHITE { addIntOp(OP_PUSHINT, 0xfff8f8f8); }
	| B256WHITE '(' ')' { addIntOp(OP_PUSHINT, 0xfff8f8f8); }
	| B256RED { addIntOp(OP_PUSHINT, 0xffff0000); }
	| B256RED '(' ')' { addIntOp(OP_PUSHINT, 0xffff0000); }
	| B256DARKRED { addIntOp(OP_PUSHINT, 0xff800000); }
	| B256DARKRED '(' ')' { addIntOp(OP_PUSHINT, 0xff800000); }
	| B256GREEN { addIntOp(OP_PUSHINT, 0xff00ff00); }
	| B256GREEN '(' ')' { addIntOp(OP_PUSHINT, 0xff00ff00); }
	| B256DARKGREEN { addIntOp(OP_PUSHINT, 0xff008000); }
	| B256DARKGREEN '(' ')' { addIntOp(OP_PUSHINT, 0xff008000); }
	| B256BLUE { addIntOp(OP_PUSHINT, 0xff0000ff); }
	| B256BLUE '(' ')' { addIntOp(OP_PUSHINT, 0xff0000ff); }
	| B256DARKBLUE { addIntOp(OP_PUSHINT, 0xff000080); }
	| B256DARKBLUE '(' ')' { addIntOp(OP_PUSHINT, 0xff000080); }
	| B256CYAN { addIntOp(OP_PUSHINT, 0xff00ffff); }
	| B256CYAN '(' ')' { addIntOp(OP_PUSHINT, 0xff00ffff); }
	| B256DARKCYAN { addIntOp(OP_PUSHINT, 0xff008080); }
	| B256DARKCYAN '(' ')' { addIntOp(OP_PUSHINT, 0xff008080); }
	| B256PURPLE { addIntOp(OP_PUSHINT, 0xffff00ff); }
	| B256PURPLE '(' ')' { addIntOp(OP_PUSHINT, 0xffff00ff); }
	| B256DARKPURPLE { addIntOp(OP_PUSHINT, 0xff800080); }
	| B256DARKPURPLE '(' ')' { addIntOp(OP_PUSHINT, 0xff800080); }
	| B256YELLOW { addIntOp(OP_PUSHINT, 0xffffff00); }
	| B256YELLOW '(' ')' { addIntOp(OP_PUSHINT, 0xffffff00); }
	| B256DARKYELLOW { addIntOp(OP_PUSHINT, 0xff808000); }
	| B256DARKYELLOW '(' ')' { addIntOp(OP_PUSHINT, 0xff808000); }
	| B256ORANGE { addIntOp(OP_PUSHINT, 0xffff6600); }
	| B256ORANGE '(' ')' { addIntOp(OP_PUSHINT, 0xffff6600); }
	| B256DARKORANGE { addIntOp(OP_PUSHINT, 0xffaa3300); }
	| B256DARKORANGE '(' ')' { addIntOp(OP_PUSHINT, 0xffaa3300); }
	| B256GREY { addIntOp(OP_PUSHINT, 0xffa4a4a4); }
	| B256GREY '(' ')' { addIntOp(OP_PUSHINT, 0xffa4a4a4); }
	| B256DARKGREY { addIntOp(OP_PUSHINT, 0xff808080); }
	| B256DARKGREY '(' ')' { addIntOp(OP_PUSHINT, 0xff808080); }
	| B256PIXEL '(' floatexpr ',' floatexpr ')' { addOp(OP_PIXEL); }
	| B256RGB '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT, 255); 
		addOp(OP_RGB);
	}
	| B256RGB '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
		addOp(OP_RGB);
	}
	| B256GETCOLOR { addOp(OP_GETCOLOR); }
	| B256GETCOLOR '(' ')' { addOp(OP_GETCOLOR); }
	| B256GETBRUSHCOLOR { addExtendedOp(OPX_GETBRUSHCOLOR); }
	| B256GETBRUSHCOLOR '(' ')' { addExtendedOp(OPX_GETBRUSHCOLOR); }
	| B256GETPENWIDTH { addExtendedOp(OPX_GETPENWIDTH); }
	| B256GETPENWIDTH '(' ')' { addExtendedOp(OPX_GETPENWIDTH); }
	| B256SPRITECOLLIDE '(' floatexpr ',' floatexpr ')' { addExtendedOp(OPX_SPRITECOLLIDE); }
	| B256SPRITEX '(' floatexpr ')' { addExtendedOp(OPX_SPRITEX); }
	| B256SPRITEY '(' floatexpr ')' { addExtendedOp(OPX_SPRITEY); }
	| B256SPRITEH '(' floatexpr ')' { addExtendedOp(OPX_SPRITEH); }
	| B256SPRITEW '(' floatexpr ')' { addExtendedOp(OPX_SPRITEW); }
	| B256SPRITEV '(' floatexpr ')' { addExtendedOp(OPX_SPRITEV); }
	| B256SPRITER '(' floatexpr ')' { addExtendedOp(OPX_SPRITER); }
	| B256SPRITES '(' floatexpr ')' { addExtendedOp(OPX_SPRITES); }
	| B256DBROW {
		addIntOp(OP_PUSHINT,0);	// default db number
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBROW);
	}
	| B256DBROW '(' ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBROW);
	}
	| B256DBROW '(' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addExtendedOp(OPX_DBROW);
	}
	| B256DBROW '(' floatexpr ',' floatexpr')' {
		addExtendedOp(OPX_DBROW);
	}
	| B256DBINT '(' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBINT); }
	| B256DBINT '(' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBINT); }
	| B256DBINT '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_DBINT); }
	| B256DBINT '(' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBINTS); }
	| B256DBINT '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBINTS); }
	| B256DBINT '(' floatexpr ',' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBINTS); }
	| B256DBFLOAT '(' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBFLOAT); }
	| B256DBFLOAT '(' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBFLOAT); }
	| B256DBFLOAT '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_DBFLOAT); }
	| B256DBFLOAT '(' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBFLOATS); }
	| B256DBFLOAT '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBFLOATS); }
	| B256DBFLOAT '(' floatexpr ',' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBFLOATS); }
	| B256DBNULL '(' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBNULL); }
	| B256DBNULL '(' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBNULL); }
	| B256DBNULL '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_DBNULL); }
	| B256DBNULL '(' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBNULLS); }
	| B256DBNULL '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBNULLS); }
	| B256DBNULL '(' floatexpr ',' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBNULLS); }
	| B256LASTERROR { addExtendedOp(OPX_LASTERROR); }
	| B256LASTERROR '(' ')' { addExtendedOp(OPX_LASTERROR); }
	| B256LASTERRORLINE { addExtendedOp(OPX_LASTERRORLINE); }
	| B256LASTERRORLINE '(' ')' { addExtendedOp(OPX_LASTERRORLINE); }
	| B256NETDATA { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETDATA); }
	| B256NETDATA '(' ')' { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETDATA); }
	| B256NETDATA '(' floatexpr ')' { addExtendedOp(OPX_NETDATA); }
	| B256PORTIN '(' floatexpr ')' { addExtendedOp(OPX_PORTIN); }
	| B256COUNT '(' stringexpr ',' stringexpr ')' { addExtendedOp(OPX_COUNT); }
	| B256COUNT '(' stringexpr ',' stringexpr ',' floatexpr ')' { addExtendedOp(OPX_COUNT_C); }
	| B256COUNTX '(' stringexpr ',' stringexpr ')' { addExtendedOp(OPX_COUNTX); }
	| B256OSTYPE { addExtendedOp(OPX_OSTYPE); }
	| B256OSTYPE '(' ')' { addExtendedOp(OPX_OSTYPE); }
	| B256MSEC { addExtendedOp(OPX_MSEC); }
	| B256MSEC '(' ')' { addExtendedOp(OPX_MSEC); }
	| B256TEXTWIDTH '(' stringexpr ')' { addExtendedOp(OPX_TEXTWIDTH); }
	| B256READBYTE { addIntOp(OP_PUSHINT, 0); addOp(OP_READBYTE); }
	| B256READBYTE '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_READBYTE); }
	| B256READBYTE '(' floatexpr ')' { addOp(OP_READBYTE); }
	| B256REF '(' B256VARIABLE ')' { addIntOp(OP_PUSHVARREF, $3); }
	| B256REF '(' B256STRINGVAR ')' { addIntOp(OP_PUSHVARREFSTR, $3); }
	| B256FREEDB { addExtendedOp(OPX_FREEDB); }
	| B256FREEDB '(' ')' { addExtendedOp(OPX_FREEDB); }
	| B256FREEDBSET {
		addIntOp(OP_PUSHINT,0);	// default db number
		addExtendedOp(OPX_FREEDBSET);
	}
	| B256FREEDBSET '(' ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addExtendedOp(OPX_FREEDBSET);
	}
	| B256FREEDBSET '(' floatexpr ')' { addExtendedOp(OPX_FREEDBSET); }
	| B256FREEFILE { addExtendedOp(OPX_FREEFILE); }
	| B256FREEFILE '(' ')' { addExtendedOp(OPX_FREEFILE); }
	| B256FREENET { addExtendedOp(OPX_FREENET); }
	| B256FREENET '(' ')' { addExtendedOp(OPX_FREENET); }
	| B256VERSION { addIntOp(OP_PUSHINT, VERSIONSIGNATURE); }
	| B256VERSION '(' ')' { addIntOp(OP_PUSHINT, VERSIONSIGNATURE); }

;


stringexpr: '(' stringexpr ')' { $$ = $2; }
	| stringexpr '+' stringexpr { addOp(OP_CONCAT); }
	| floatexpr '+' stringexpr { addOp(OP_CONCAT); }
	| stringexpr '+' floatexpr { addOp(OP_CONCAT); }
	| B256STRING { addStringOp(OP_PUSHSTRING, $1); }
	| B256STRINGVAR '[' floatexpr ']' { addIntOp(OP_DEREF, $1); }
	| B256STRINGVAR '[' floatexpr ',' floatexpr ']' { addIntOp(OP_DEREF2D, $1); }
	| B256STRINGVAR '(' explist ')' { addIntOp(OP_GOSUB, $1); }
	| B256STRINGVAR '(' ')' { addIntOp(OP_GOSUB, $1); }
	| B256STRINGVAR {
		if ($1 < 0) {
			return -1;
		} else {
			addIntOp(OP_PUSHVAR, $1);
		}
	}
	| B256CHR '(' floatexpr ')' { addOp(OP_CHR); }
	| B256TOSTRING '(' expr ')' { addOp(OP_STRING); }
	| B256UPPER '(' expr ')' { addOp(OP_UPPER); }
	| B256LOWER '(' expr ')' { addOp(OP_LOWER); }
	| B256MID '(' expr ',' floatexpr ',' floatexpr ')' { addOp(OP_MID); }
	| B256LEFT '(' expr ',' floatexpr ')' { addOp(OP_LEFT); }
	| B256RIGHT '(' expr ',' floatexpr ')' { addOp(OP_RIGHT); }
	| B256GETSLICE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_GETSLICE); }
	| B256READ { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
	| B256READ '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
	| B256READ '(' floatexpr ')' { addOp(OP_READ); }
	| B256READLINE { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
	| B256READLINE '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
	| B256READLINE '(' floatexpr ')' { addOp(OP_READLINE); }
	| B256CURRENTDIR { addExtendedOp(OPX_CURRENTDIR); }
	| B256CURRENTDIR '(' ')' { addExtendedOp(OPX_CURRENTDIR); }
	| B256DBSTRING '(' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBSTRING); }
	| B256DBSTRING '(' floatexpr ',' floatexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBSTRING); }
	| B256DBSTRING '(' floatexpr ',' floatexpr ',' floatexpr ')' {
		addExtendedOp(OPX_DBSTRING); }
	| B256DBSTRING '(' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBSTRINGS); }
	| B256DBSTRING '(' floatexpr ',' stringexpr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addExtendedOp(OPX_DBSTRINGS); }
	| B256DBSTRING '(' floatexpr ',' floatexpr ',' stringexpr ')' {
		addExtendedOp(OPX_DBSTRINGS); }
	| B256LASTERRORMESSAGE { addExtendedOp(OPX_LASTERRORMESSAGE); }
	| B256LASTERRORMESSAGE '(' ')' { addExtendedOp(OPX_LASTERRORMESSAGE); }
	| B256LASTERROREXTRA { addExtendedOp(OPX_LASTERROREXTRA); }
	| B256LASTERROREXTRA '(' ')' { addExtendedOp(OPX_LASTERROREXTRA); }
	| B256NETREAD { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETREAD); }
	| B256NETREAD '(' ')' { addIntOp(OP_PUSHINT, 0); addExtendedOp(OPX_NETREAD); }
	| B256NETREAD '(' floatexpr ')' { addExtendedOp(OPX_NETREAD); }
	| B256NETADDRESS { addExtendedOp(OPX_NETADDRESS); }
	| B256NETADDRESS '(' ')' { addExtendedOp(OPX_NETADDRESS); }
	| B256MD5 '(' stringexpr ')' { addExtendedOp(OPX_MD5); }
	| B256GETSETTING '(' expr ',' expr ')' { addExtendedOp(OPX_GETSETTING); }
	| B256DIR '(' stringexpr ')' { addExtendedOp(OPX_DIR); }
	| B256DIR '(' ')' { addStringOp(OP_PUSHSTRING, ""); addExtendedOp(OPX_DIR); }
	| B256REPLACE '(' expr ',' expr ',' expr ')' { addExtendedOp(OPX_REPLACE); }
	| B256REPLACE '(' expr ',' expr ',' expr ',' floatexpr ')' { addExtendedOp(OPX_REPLACE_C); }
	| B256REPLACEX '(' expr ',' stringexpr ',' expr ')' { addExtendedOp(OPX_REPLACEX); }
	| B256IMPLODE '(' B256STRINGVAR ')' {  addStringOp(OP_PUSHSTRING, ""); addIntOp(OP_IMPLODE, $3); }
	| B256IMPLODE '(' B256STRINGVAR ',' stringexpr ')' {  addIntOp(OP_IMPLODE, $3); }
	| B256IMPLODE '(' B256VARIABLE ')' {  addStringOp(OP_PUSHSTRING, ""); addIntOp(OP_IMPLODE, $3); }
	| B256IMPLODE '(' B256VARIABLE ',' stringexpr ')' {  addIntOp(OP_IMPLODE, $3); }

;

%%

int
yyerror(const char *msg) {
	errorcode = COMPERR_SYNTAX;
	if (yytext[0] == '\n') { linenumber--; } // error happened on previous line
	return -1;
}
