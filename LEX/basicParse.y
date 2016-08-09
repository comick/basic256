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
	#include "../BasicTypes.h"
	#include "../Constants.h"
	#include "../WordCodes.h"
	#include "../CompileErrors.h"
	#include "../ErrorCodes.h"
	#include "../Version.h"


	#define SYMTABLESIZE 2000
	#define IFTABLESIZE 1000
	#define PARSEWARNINGTABLESIZE 10

	extern int yylex();
	extern char *yytext;
	int yyerror(const char *);
	int errorcode;
	extern int column;
	extern int linenumber;
	extern char *lexingfilename;
	extern int numincludes;

	int *wordCode = NULL;
	unsigned int maxwordoffset = 0;		// size of the current wordCode array
	unsigned int wordOffset = 0;		// current location on the WordCode array

	unsigned int listlen = 0;
        unsigned int numberoflists = 0;

	unsigned int varnumber[IFTABLESIZE];	// stack of variable numbers in a statement to return the varmumber
	int nvarnumber=0;


	int functionDefSymbol = -1;	// if in a function definition (what is the symbol number) -1 = not in fundef
	int subroutineDefSymbol = -1;	// if in a subroutine definition (what is the symbol number) -1 = not in fundef

	struct label
	{
		char *name;
		int offset;
	};

	char *EMPTYSTR = "";
	char *symtable[SYMTABLESIZE];
	int symtableaddress[SYMTABLESIZE];
	int numsyms = 0;


	// array to hold stack of if statement branch locations
	// that need to have final jump location added to them
	// the iftable is also used by for, subroutine, and function to insure
	// that no if,do,while,... is nested incorrectly
	unsigned int iftablesourceline[IFTABLESIZE];
	unsigned int iftabletype[IFTABLESIZE];
	int iftableid[IFTABLESIZE];			// used to store a sequential number for this if - unique label creation
	int iftableincludes[IFTABLESIZE];			// used to store the include depth of the code
	int numifs = 0;
	int nextifid;

	#define IFTABLETYPEIF 1
	#define IFTABLETYPEELSE 2
	#define IFTABLETYPEDO 3
	#define IFTABLETYPEWHILE 4
	#define IFTABLETYPEFOR 5
	#define IFTABLETYPEFUNCTION 6
	#define IFTABLETYPETRY 7
	#define IFTABLETYPECATCH 8
	#define IFTABLETYPEBEGINCASE 9
	#define IFTABLETYPECASE 10


	// store the function variables here during a function definition
	unsigned int args[100];
	unsigned int argstype[100];
	int numargs = 0;

	#define ARGSTYPEVALUE 0
	#define ARGSTYPEVARREF 1

	// compiler workings - store in array so that interperter can display all of them
	int parsewarningtable[PARSEWARNINGTABLESIZE];
	int parsewarningtablelinenumber[PARSEWARNINGTABLESIZE];
	int parsewarningtablecolumn[PARSEWARNINGTABLESIZE];
	char *parsewarningtablelexingfilename[PARSEWARNINGTABLESIZE];
	int numparsewarnings = 0;

	int
	basicParse(char *);

	void checkWordMem(unsigned int addedwords) {
		unsigned int t;
		if (wordOffset + addedwords + 1 >= maxwordoffset) {
			maxwordoffset += maxwordoffset + addedwords + 1024;
			wordCode = realloc(wordCode, maxwordoffset * sizeof(int));
			for (t=wordOffset; t<maxwordoffset; t++) {
				*(wordCode+t) = 0;
			}
		}
	}

	int bytesToFullWords(unsigned int size) {
		// return how many words will be needed to store "size" bytes
		return((size + sizeof(int) - 1) / sizeof(int));
	}

	void addOp(int op) {
		checkWordMem(1);
		wordCode[wordOffset] = op;
		wordOffset++;
	}

	void addIntOp(int op, int data) {
		addOp(op);
		addOp(data);
	}


	void addFloatOp(int op, double data) {
		addOp(op);
		unsigned int wlen = bytesToFullWords(sizeof(double));
		checkWordMem(wlen);
		double *temp = (double *) (wordCode + wordOffset);
		*temp = data;
		wordOffset += wlen;
	}

	void addStringOp(int op, char *data) {
		addOp(op);
		unsigned int len = strlen(data) + 1;
		unsigned int wlen = bytesToFullWords(len);
		checkWordMem(wlen);
		strncpy((char *) (wordCode + wordOffset), data, len);
		wordOffset += wlen;
	}

	void
	clearIfTable() {
		int j;
		for (j = 0; j < IFTABLESIZE; j++) {
			iftablesourceline[j] = -1;
			iftabletype[j] = -1;
			iftableid[j] = -1;
			iftableincludes[j] = -1;
		}
		numifs = 0;
		nextifid = 0;
	}

	int testIfOnTable(int includelevel) {
		// return line number if there is an unfinished while.do.if.else
		// or send back -1
		if (numifs >=1 ) {
			if (iftableincludes[numifs-1]>=includelevel) {
				return iftablesourceline[numifs-1];
			}
		}
		return -1;
	}

	int testIfOnTableError(int includelevel) {
		// return Error number if there is an unfinished while.do.if.else
		// or send back 0
		if (numifs >=1 ) {
			if (iftableincludes[numifs-1]>=includelevel) {
				if (iftabletype[numifs-1]==IFTABLETYPEIF) return COMPERR_IFNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEELSE) return COMPERR_ELSENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEDO) return COMPERR_DONOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEWHILE) return COMPERR_WHILENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEFOR) return COMPERR_FORNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) return COMPERR_FUNCTIONNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPETRY) return COMPERR_TRYNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPECATCH) return COMPERR_CATCHNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEBEGINCASE) return COMPERR_BEGINCASENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPECASE) return COMPERR_CASENOEND;
			}
		}
		return 0;
	}

	void
	clearSymbolTable() {
		int j;
		if (numsyms == 0) {
			for (j = 0; j < SYMTABLESIZE; j++) {
				symtable[j] = 0;
				symtableaddress[j] = -1;
			}
		}
		for (j = 0; j < numsyms; j++) {
			if (symtable[j]) {
				free(symtable[j]);
			}
			symtable[j] = NULL;
			symtableaddress[j] = -1;
		}
		numsyms = 0;
	}

	int newIf(int sourceline, int type) {
		iftablesourceline[numifs] = sourceline;
		iftabletype[numifs] = type;
		iftableid[numifs] = nextifid;
		iftableincludes[numifs] = numincludes;
		nextifid++;
		numifs++;
		return numifs - 1;
	}

	void newParseWarning(int type) {
		// add warning to warnings table (if not maximum)
		if (numparsewarnings<PARSEWARNINGTABLESIZE) {
			parsewarningtable[numparsewarnings] = type;
			parsewarningtablelinenumber[numparsewarnings] = linenumber;
			parsewarningtablecolumn[numparsewarnings] = column;
			parsewarningtablelexingfilename[numparsewarnings] = strdup(lexingfilename);
			numparsewarnings++;
		} else {
			parsewarningtable[numparsewarnings-1] = COMPWARNING_MAXIMUMWARNINGS;
			parsewarningtablelinenumber[numparsewarnings-1] = 0;
			parsewarningtablecolumn[numparsewarnings-1] = 0;
			free(parsewarningtablelexingfilename[numparsewarnings-1]);
			parsewarningtablelexingfilename[numparsewarnings-1] = strdup("");
		}
	}

	int getSymbol(char *name) {
		// get a symbol if it exists or create a new one on the symbol table
		int i;
		for (i = 0; i < numsyms; i++) {
			if (symtable[i] && !strcmp(name, symtable[i]))
				return i;
		}
		symtable[numsyms] = strdup(name);
		numsyms++;
		return numsyms - 1;
	}

	#define INTERNALSYMBOLEXIT 0 //at the end of the loop - all done
	#define INTERNALSYMBOLCONTINUE 1 //at the test of the loop
	#define INTERNALSYMBOLTOP 2 // at the end of the loop - all done

	int getInternalSymbol(int id, int type) {
		// an internal symbol used to jump an if
		char name[32];
		sprintf(name,"___%d_%d", id, type);
		return getSymbol(name);
	}

	int newWordCode() {
		unsigned int t;
		if (wordCode) {
			free(wordCode);
		}
		maxwordoffset = 1024;
		wordCode = malloc(maxwordoffset * sizeof(int));

		if (wordCode) {
			for (t=0; t<maxwordoffset; t++) {
				*(wordCode+t) = 0;
			}
			wordOffset = 0;
			linenumber = 1;
			addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			return 0; 	// success in creating and filling
		}
		return -1;
	}

	void freeBasicParse() {
		// free all dynamically allocated stuff
		int i;
		for(i=0; i<numparsewarnings; i++) {
			if (parsewarningtablelexingfilename[i]) {
				free(parsewarningtablelexingfilename[i]);
				parsewarningtablelexingfilename[i]=NULL;
			}
		}
		clearSymbolTable();
		if (wordCode) {
			free(wordCode);
			wordCode = NULL;
		}
	}


	#ifdef __cplusplus
	}
	#endif

%}

%token B256PRINT B256INPUT B256INPUTSTRING B256INPUTINT B256INPUTFLOAT B256KEY
%token B256PIXEL B256RGB B256PLOT B256CIRCLE B256RECT B256POLY B256STAMP B256LINE B256FASTGRAPHICS B256GRAPHSIZE B256REFRESH B256CLS B256CLG
%token B256IF B256THEN B256ELSE B256ENDIF B256BEGINCASE B256CASE B256ENDCASE
%token B256WHILE B256ENDWHILE B256DO B256UNTIL B256FOR B256TO B256STEP B256NEXT
%token B256OPEN B256OPENB B256OPENSERIAL B256READ B256WRITE B256CLOSE B256RESET
%token B256GOTO B256GOSUB B256RETURN B256REM B256END B256SETCOLOR
%token B256GTE B256LTE B256NE
%token B256DIM B256REDIM B256NOP
%token B256TOINT B256TOSTRING B256LENGTH B256MID B256LEFT B256RIGHT B256UPPER B256LOWER B256INSTR B256INSTRX B256MIDX
%token B256CEIL B256FLOOR B256RAND B256SIN B256COS B256TAN B256ASIN B256ACOS B256ATAN B256ABS B256PI B256DEGREES B256RADIANS B256LOG B256LOGTEN B256SQR B256EXP
%token B256AND B256OR B256XOR B256NOT
%token B256PAUSE B256SOUND
%token B256ASC B256CHR B256TOFLOAT B256READLINE B256WRITELINE B256BOOLEOF B256MOD B256INTDIV
%token B256YEAR B256MONTH B256DAY B256HOUR B256MINUTE B256SECOND B256TEXT B256FONT B256TEXTWIDTH B256TEXTHEIGHT
%token B256SAY B256SYSTEM
%token B256VOLUME
%token B256GRAPHWIDTH B256GRAPHHEIGHT B256GETSLICE B256PUTSLICE B256IMGLOAD
%token B256SPRITEDIM B256SPRITELOAD B256SPRITESLICE B256SPRITEMOVE B256SPRITEHIDE B256SPRITESHOW B256SPRITEPLACE
%token B256SPRITECOLLIDE B256SPRITEX B256SPRITEY B256SPRITEH B256SPRITEW B256SPRITEV
%token B256SPRITEPOLY B256SPRITER B256SPRITES
%token B256WAVLENGTH B256WAVPAUSE B256WAVPOS B256WAVPLAY B256WAVSTATE B256WAVSEEK B256WAVSTOP B256WAVWAIT
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
%token B256BINARYOR B256AMP B256AMPEQUAL B256BINARYNOT
%token B256IMGSAVE
%token B256REPLACE B256COUNT B256EXPLODE B256REPLACEX B256COUNTX B256EXPLODEX B256IMPLODE
%token B256OSTYPE B256MSEC
%token B256EDITVISIBLE B256GRAPHVISIBLE B256OUTPUTVISIBLE B256EDITSIZE B256OUTPUTSIZE
%token B256FUNCTION B256ENDFUNCTION B256THROWERROR B256SUBROUTINE B256ENDSUBROUTINE B256CALL B256GLOBAL
%token B256READBYTE B256WRITEBYTE
%token B256ADD1 B256SUB1 B256ADDEQUAL B256SUBEQUAL B256MULEQUAL B256DIVEQUAL B256REF
%token B256FREEDB B256FREEFILE B256FREENET B256FREEDBSET B256DBNULL
%token B256ARC B256CHORD B256PIE B256PENWIDTH B256GETPENWIDTH B256GETBRUSHCOLOR B256VERSION
%token B256ALERT B256CONFIRM B256PROMPT
%token B256FROMBINARY B256FROMHEX B256FROMOCTAL B256FROMRADIX B256TOBINARY B256TOHEX B256TOOCTAL B256TORADIX
%token B256DEBUGINFO
%token B256CONTINUEDO B256CONTINUEFOR B256CONTINUEWHILE B256EXITDO B256EXITFOR B256EXITWHILE
%token B256PRINTERPAGE B256PRINTERON B256PRINTEROFF B256PRINTERCANCEL
%token B256TRY B256CATCH B256ENDTRY B256LET
%token B256ERROR_NONE
%token B256ERROR_NOSUCHLABEL
%token B256ERROR_NEXTNOFOR
%token B256ERROR_NOTARRAY
%token B256ERROR_ARGUMENTCOUNT
%token B256ERROR_MAXRECURSE
%token B256ERROR_STACKUNDERFLOW
%token B256ERROR_BADCALLFUNCTION
%token B256ERROR_BADCALLSUBROUTINE
%token B256ERROR_FILENUMBER
%token B256ERROR_FILEOPEN
%token B256ERROR_FILENOTOPEN
%token B256ERROR_FILEWRITE
%token B256ERROR_FILERESET
%token B256ERROR_ARRAYSIZELARGE
%token B256ERROR_ARRAYSIZESMALL
%token B256ERROR_VARNOTASSIGNED
%token B256ERROR_ARRAYNITEMS
%token B256ERROR_ARRAYINDEX
%token B256ERROR_STRSTART
%token B256ERROR_RGB
%token B256ERROR_PUTBITFORMAT
%token B256ERROR_POLYPOINTS
%token B256ERROR_IMAGEFILE
%token B256ERROR_SPRITENUMBER
%token B256ERROR_SPRITENA
%token B256ERROR_SPRITESLICE
%token B256ERROR_FOLDER
%token B256ERROR_INFINITY
%token B256ERROR_DBOPEN
%token B256ERROR_DBQUERY
%token B256ERROR_DBNOTOPEN
%token B256ERROR_DBCOLNO
%token B256ERROR_DBNOTSET
%token B256ERROR_TYPECONV
%token B256ERROR_NETSOCK
%token B256ERROR_NETHOST
%token B256ERROR_NETCONN
%token B256ERROR_NETREAD
%token B256ERROR_NETNONE
%token B256ERROR_NETWRITE
%token B256ERROR_NETSOCKOPT
%token B256ERROR_NETBIND
%token B256ERROR_NETACCEPT
%token B256ERROR_NETSOCKNUMBER
%token B256ERROR_PERMISSION
%token B256ERROR_IMAGESAVETYPE
%token B256ERROR_DIVZERO
%token B256ERROR_BYREF
%token B256ERROR_FREEFILE
%token B256ERROR_FREENET
%token B256ERROR_FREEDB
%token B256ERROR_DBCONNNUMBER
%token B256ERROR_FREEDBSET
%token B256ERROR_DBSETNUMBER
%token B256ERROR_DBNOTSETROW
%token B256ERROR_PENWIDTH
%token B256ERROR_ARRAYINDEXMISSING
%token B256ERROR_IMAGESCALE
%token B256ERROR_FONTSIZE
%token B256ERROR_FONTWEIGHT
%token B256ERROR_RADIXSTRING
%token B256ERROR_RADIX
%token B256ERROR_LOGRANGE
%token B256ERROR_STRINGMAXLEN
%token B256ERROR_PRINTERNOTON
%token B256ERROR_PRINTERNOTOFF
%token B256ERROR_PRINTEROPEN
%token B256ERROR_WAVFILEFORMAT
%token B256ERROR_WAVNOTOPEN
%token B256ERROR_WAVNOTSEEKABLE
%token B256ERROR_WAVNODURATION
%token B256ERROR_FILEOPERATION
%token B256ERROR_SERIALPARAMETER
%token B256ERROR_LONGRANGE
%token B256ERROR_INTEGERRANGE
%token B256ERROR_NOTIMPLEMENTED
%token B256WARNING_START
%token B256WARNING_TYPECONV
%token B256WARNING_WAVNOTSEEKABLE
%token B256WARNING_WAVNODURATION
%token B256WARNING_VARNOTASSIGNED
%token B256WARNING_LONGRANGE
%token B256WARNING_INTEGERRANGE
%token B256REGEXMINIMAL B256TYPEOF B256UNASSIGN
%token B256TYPE_UNASSIGNED B256TYPE_INT B256TYPE_FLOAT B256TYPE_STRING B256TYPE_ARRAY B256TYPE_REF
%token B256ISNUMERIC B256LTRIM B256RTRIM B256TRIM B256SEMICOLON B256SEMICOLONEQUAL
%token B256KEYPRESSED B256VARIABLEWATCH B256FILL


%union anytype {
	int number;
	double floatnum;
	char *string;
}

%token <number> B256LINENUM
%token <number> B256INTEGER
%token <floatnum> B256FLOAT
%token <string> B256STRING
%token <string> B256HEXCONST
%token <string> B256BINCONST
%token <string> B256OCTCONST
%token <number> B256VARIABLE
%token <string> B256NEWVAR
%token <number> B256COLOR
%token <number> B256LABEL


%left B256XOR
%left B256OR
%left B256AND
%nonassoc B256NOT B256ADD1 B256SUB1
%left '<' B256LTE '>' B256GTE '=' B256NE
%left B256BINARYOR B256AMP
%left '-' '+' B256SEMICOLON
%left '*' '/' B256MOD B256INTDIV
%nonassoc B256UMINUS B256BINARYNOT
%left '^'

%%

program: 		programline programnewline program
				| programline
				;

programnewline:
				'\n' {
					linenumber++;
					column=0;
					addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
				}
				;

programline: 	label compoundstmt
				| label compoundstmt B256REM
				| compoundstmt
				| compoundstmt B256REM
				| label
				| label B256REM
				| B256REM
				| /* empty */
				;

label:			B256LABEL {
					if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
						errorcode = COMPERR_FUNCTIONGOTO;
						return -1;
					}
					symtableaddress[$1] = wordOffset;
				}
				;

functionvariable:
			args_v {
				args[numargs] = varnumber[--nvarnumber]; argstype[numargs] = ARGSTYPEVALUE; numargs++;
				//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs);
			}
			| B256REF '(' args_v ')' {
				args[numargs] = varnumber[--nvarnumber]; argstype[numargs] = ARGSTYPEVARREF; numargs++;
				//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs);
			}
			;

functionvariablelist:
				'(' ')'
				| '(' functionvariables ')'
				;


functionvariables:
				functionvariable
				| functionvariable ',' functionvariables
				;

compoundstmt:
			compoundstmt ':' statement
			| statement
			;
/* array reference - make everything 2d */

arrayref:
			'[' expr ']' {
				addIntOp(OP_PUSHINT, 0);
                                addOp(OP_STACKSWAP);
			}
			| '[' expr ',' expr ']'
			;

/* array assigment a[]={0, 1, 2}*/

array_empty:
                        args_v '[' ']';

/* statement argument paterns */

args_none:
			| '(' ')';

/* one argument (only ones that do not have a native or need to setvarnumber) */

/* X - Meaning */
/* a - array element with [e] or [e,e] following */
/* A - array data pushed to the stack exactly like listoflists */
/* e - data element */
/* i - listoflists {} or {{},{}} */
/* v - variable	*/


args_a:
			args_v arrayref
			| '(' args_a ')';

// Array Variable Data as a list of lists
args_A:
			args_v {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
			}

args_v:
			B256VARIABLE {
				varnumber[nvarnumber++] = $1;
			}
			| '(' args_v ')';


/* two arguments */


args_ee:
			expr ',' expr
			| '(' args_ee ')';

args_ei:
			expr ',' listoflists
			| '(' args_ei ')';

args_eA:
			expr ',' args_A
			| '(' args_eA ')';

args_ea:
			expr ',' args_a
			|'(' args_ea ')';

args_ev:
			expr ',' args_v
			|'(' args_ev ')';

args_Ae:
			args_A ',' expr
			|'(' args_Ae ')';

args_ie:
			listoflists ',' expr
			| '(' args_ie ')';



/* three arguments */

args_eee:
			expr ',' expr ',' expr
			| '(' args_eee ')';


args_eei:
			expr ',' expr ',' listoflists
			| '(' args_eei ')';


args_eeA:
			expr ',' expr ',' args_A
			| '(' args_eeA ')';

args_Aee:
			args_A ',' expr ',' expr
			|'(' args_Aee ')';

args_iee:
			listoflists ',' expr ',' expr
			| '(' args_iee ')';

/* four arguments */
args_eeee:
			expr ',' expr ',' expr ',' expr
			| '(' args_eeee ')';

args_eeei:
			expr ',' expr ',' expr ',' listoflists
			| '(' args_eeei ')';

args_eeeA:
			expr ',' expr ',' expr ',' args_A
			| '(' args_eeeA ')';



/* five arguments */

args_eeeee:
			expr ',' expr ',' expr ',' expr ',' expr
			| '(' args_eeeee ')';


args_eeeei:
			expr ',' expr ',' expr ',' expr ',' listoflists
			| '(' args_eeeei ')';


args_eeeeA:
			expr ',' expr ',' expr ',' expr ',' args_A
			| '(' args_eeeeA ')';


/* six arguments */
args_eeeeee:
			expr ',' expr ',' expr ',' expr ',' expr ',' expr
			| '(' args_eeeeee ')';


/* seven arguments */

args_eeeeeee:
			expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr
			| '(' args_eeeeeee ')';



/* now the list of statements */

statement:
			alertstmt
			| arcstmt
			| begincasestmt
			| callstmt
			| casestmt
			| catchstmt
			| changedirstmt
			| chordstmt
			| circlestmt
			| clearstmt
			| clickclearstmt
			| closestmt
			| colorstmt
			| continuedostmt
			| continueforstmt
			| continuewhilestmt
			| dbclosesetstmt
			| dbclosestmt
			| dbexecutestmt
			| dbopensetstmt
			| dbopenstmt
			| dimstmt
			| dostmt
			| editvisiblestmt
			| elsestmt
			| endcasestmt
			| endfunctionstmt
			| endifstmt
			| endstmt
			| endsubroutinestmt
			| endtrystmt
			| endwhilestmt
			| exitdostmt
			| exitforstmt
			| exitwhilestmt
			| fastgraphicsstmt
			| fontstmt
			| forstmt
			| functionstmt
			| globalstmt
			| gosubstmt
			| gotostmt
			| graphsizestmt
			| graphvisiblestmt
			| ifstmt
			| ifthenstmt
			| ifthenelsestmt
			| imgloadstmt
			| imgsavestmt
			| inputstmt
			| killstmt
			| letstmt
			| linestmt
			| netclosestmt
			| netconnectstmt
			| netlistenstmt
			| netwritestmt
			| nextstmt
			| offerrorstmt
			| onerrorstmt
			| openstmt
			| outputvisiblestmt
			| pausestmt
			| penwidthstmt
			| piestmt
			| plotstmt
			| polystmt
			| portoutstmt
			| printercancelstmt
			| printeroffstmt
			| printeronstmt
			| printerpagestmt
			| printstmt
			| putslicestmt
			| rectstmt
			| redimstmt
			| refreshstmt
			| regexminimalstmt
			| resetstmt
			| returnstmt
			| saystmt
			| seekstmt
			| setsettingstmt
			| soundstmt
			| spritedimstmt
			| spritehidestmt
			| spriteloadstmt
			| spritemovestmt
			| spriteplacestmt
			| spritepolystmt
			| spriteshowstmt
			| spriteslicestmt
			| stampstmt
			| subroutinestmt
			| systemstmt
			| textstmt
			| throwerrorstmt
			| trystmt
			| unassignstmt
			| untilstmt
			| variablewatchstmt
			| volumestmt
			| wavpausestmt
			| wavplaystmt
			| wavseekstmt
			| wavstopstmt
			| wavwaitstmt
			| whilestmt
			| writebytestmt
			| writelinestmt
			| writestmt
			;


begincasestmt:
			B256BEGINCASE {
				// start a case block
				newIf(linenumber, IFTABLETYPEBEGINCASE);
			}
			;

caseexpr:	B256CASE {
				// if not first case then add jump to to "endcase" and resolve the branchfalse
				if (numifs>1) {
					if (iftabletype[numifs-1]==IFTABLETYPECASE) {
						if (iftabletype[numifs-2]==IFTABLETYPEBEGINCASE) {
							//
							// create jump around from end of the CASE to end of the END CASE
							addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-2],INTERNALSYMBOLEXIT));
						} else {
							errorcode = COMPERR_ENDBEGINCASE;
							linenumber = iftablesourceline[numifs-1];
							return -1;
						}
						//
						// resolve branchfalse from previous case
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						//
						numifs--;
					}
				}
				//
			}
			;

casestmt:	caseexpr expr {
				//
				// add branch to the end if false
				addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// put new CASE on the frame for the IF
				newIf(linenumber, IFTABLETYPECASE);
			}
			;

catchstmt: 	B256CATCH {
				//
				// create jump around from end of the TRY to end of the CATCH
				addOp(OP_OFFERROR);
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPETRY) {
						//
						// resolve the try onerrorcatch to the catch address
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
						//
						// put new if on the frame for the catch
						newIf(linenumber, IFTABLETYPECATCH);
						addOp(OP_OFFERROR);
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_CATCH;
					return -1;
				}
			}
			;

dostmt: 	B256DO {
				//
				// create internal symbol and add to the label table for the top of the loop
				symtableaddress[getInternalSymbol(nextifid,INTERNALSYMBOLTOP)] = wordOffset;
				//
				// add to if frame
				newIf(linenumber, IFTABLETYPEDO);
			}
			;

elsestmt:	B256ELSE {
				//
				// create jump around from end of the THEN to end of the ELSE
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEIF) {
						//
						// resolve the label on the if to the current location
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
						//
						// put new if on the frame for the else
						newIf(linenumber, IFTABLETYPEELSE);
					} else if (iftabletype[numifs-1]==IFTABLETYPECASE) {
						if (numifs>1) {
							if (iftabletype[numifs-2]==IFTABLETYPEBEGINCASE) {
								//
								// create jump around from end of the CASE to end of the END CASE
								addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-2],INTERNALSYMBOLEXIT));
							} else {
								errorcode = COMPERR_ENDBEGINCASE;
								linenumber = iftablesourceline[numifs-1];
								return -1;
							}
							//
							// resolve branchfalse from previous case
							symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
							//
							numifs--;
							// put new if on the frame for the else
							newIf(linenumber, IFTABLETYPEELSE);
						}
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ELSE;
					return -1;
				}
			}
			;

endcasestmt:
			B256ENDCASE {
				// add label for last case branchfalse to jump to
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPECASE || iftabletype[numifs-1]==IFTABLETYPEELSE) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = COMPERR_ENDENDCASE;
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDENDCASE;
					return -1;
				}
				// add label for all cases to jump to after execution
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEBEGINCASE) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDENDCASEBEGIN;
					return -1;
				}
			}
			;

endifstmt:	B256ENDIF {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then put the current wordcode address there
				// - so we can jump over code
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEIF||iftabletype[numifs-1]==IFTABLETYPEELSE) {
						//
						// resolve the label on the if/else to the current location
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDIF;
					return -1;
				}
			}
			;

endtrystmt:	B256ENDTRY {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then put the current wordcode address there
				// - so we can jump over code
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPECATCH) {
						//
						// resolve the label on the Catch to the current location
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDTRY;
					return -1;
				}
			}
			;

endwhilestmt:
			B256ENDWHILE {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEWHILE) {
						//
						// jump to the top
						addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE));
						//
						// resolve the label to the bottom
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						//
						// remove the single placeholder from the if frame
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDWHILE;
					return -1;
				}
			}
			;

ifstmt:		B256IF expr B256THEN {
					//
					// add branch to the end if false
					addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
					//
					// put new if on the frame for the IF
					newIf(linenumber, IFTABLETYPEIF);
			}
			;

ifthenstmt:
			ifstmt compoundstmt {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then resolve the lable
				if (numifs>0) {
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					numifs--;
				}
			}
			;

ifthenelsestmt:
			ifthenelse compoundstmt {
				//
				// resolve the label on the else to the current location
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
			}
			;

ifthenelse:
			ifstmt compoundstmt B256ELSE {
				//
				// create jump around from end of the THEN to end of the ELSE
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// jump point for else
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
				//
				// put new if on the frame for the else
				newIf(linenumber, IFTABLETYPEELSE);
			}
			;

trystmt: 	B256TRY	{
				//
				// add on error branch
				addIntOp(OP_ONERRORCATCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// put new if on the frame for the TRY
				newIf(linenumber, IFTABLETYPETRY);
			}
			;

until:		B256UNTIL {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEDO) {
						//
						// create label for CONTINUE DO
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
						//
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_UNTIL;
					return -1;
				}
			}
			;

untilstmt:	until expr {
				//
				// branch back to top if condition holds
				addIntOp(OP_BRANCH, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLTOP));
				//
				// create label for EXIT DO
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
			}
			;

while: 		B256WHILE {
				//
				// create internal symbol and add to the label table for the top of the loop
				symtableaddress[getInternalSymbol(nextifid,INTERNALSYMBOLCONTINUE)] = wordOffset;
			}
			;

whilestmt: 	while expr {
				//
				// add branch to end if false
				addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// add to if frame
				newIf(linenumber, IFTABLETYPEWHILE);
			};

letstmt:	B256LET assign
			| B256LET arrayassign
			| B256LET arrayelementassign
			| assign
			| arrayassign
			| arrayelementassign
			;

dimstmt: 	B256DIM args_a {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_a B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM args_v expr {
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_v expr B256FILL expr {
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM args_v args_ee {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_v args_ee B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM args_v '=' listoflists {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| B256DIM args_v '=' args_A {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| B256DIM array_empty '=' listoflists {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| B256DIM array_empty '=' args_A {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| B256DIM args_v B256FILL expr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			| B256DIM array_empty B256FILL expr {
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			;

redimstmt:	B256REDIM args_a {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_a B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256REDIM args_v expr {
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_v expr B256FILL expr {
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256REDIM args_v args_ee {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_v args_ee B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			;

pausestmt:	B256PAUSE expr {
				addOp(OP_PAUSE);
			}
			;

throwerrorstmt:
			B256THROWERROR expr {
				addOp(OP_THROWERROR);
			}
			;

clearstmt:	B256CLS args_none {
				addOp(OP_CLS);
			}
			| B256CLG args_none {
				// push the color clear if there are no arguments
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addOp(OP_RGB);
				addOp(OP_CLG);
			}
			| B256CLG expr {
				addOp(OP_CLG);
			}
			;

fastgraphicsstmt:
			B256FASTGRAPHICS args_none {
				addOp(OP_FASTGRAPHICS);
			}
			;

graphsizestmt:
			B256GRAPHSIZE args_ee {
				addOp(OP_GRAPHSIZE);
			}
			;

refreshstmt:
			B256REFRESH args_none {
				addOp(OP_REFRESH);
			}
			;

endstmt: 	B256END args_none {
				addOp(OP_END);
			}
			;


/* assign an expression to a single array element */
arrayelementassign:
			args_a '=' expr {
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| args_a B256ADD1 {
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256SUB1 {
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256ADDEQUAL expr {
				// a[b,c] += n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_ADD);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256SUBEQUAL expr {
				// a[b,c] -= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_SUB);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256MULEQUAL expr {
				// a[b,c] *= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_MUL);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256DIVEQUAL expr {
				// a[b,c] /= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_DIV);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256AMPEQUAL expr {
				// a[b,c] &= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_BINARYAND);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256SEMICOLONEQUAL expr {
				// a[b,c] /= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCATENATE);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			;
			
/* assign an entire array in one statement */
arrayassign:
			args_v B256FILL expr {
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			| array_empty B256FILL expr {
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			| args_v '=' listoflists {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| array_empty '=' listoflists {
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			;

/* assign an expression to a normal variable */
assign:
			args_v '=' expr {
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| args_v B256ADD1 {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256SUB1 {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256ADDEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_ADD);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256SUBEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_SUB);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256MULEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_MUL);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256DIVEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_DIV);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256AMPEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_BINARYAND);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256SEMICOLONEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCATENATE);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			;


forstmt: 	B256FOR args_v '=' expr B256TO expr {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				newIf(linenumber, IFTABLETYPEFOR);
				// push default step 1 and exit address
				addIntOp(OP_PUSHINT, 1); //step
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntOp(OP_FOR, varnumber[--nvarnumber]);
			}
			| B256FOR args_v '=' expr B256TO expr B256STEP expr {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				newIf(linenumber, IFTABLETYPEFOR);
				// push exit address
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntOp(OP_FOR, varnumber[--nvarnumber]);
			}
			;

nextstmt:	B256NEXT args_v {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEFOR) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
						addIntOp(OP_NEXT, varnumber[--nvarnumber]);
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_NEXT;
					return -1;
				}
			}
			;

gotostmt:	B256GOTO args_v {
				if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
					errorcode = COMPERR_FUNCTIONGOTO;
					return -1;
				}
				addIntOp(OP_GOTO, varnumber[--nvarnumber]);
			}
			;

gosubstmt:	B256GOSUB args_v {
				if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
					errorcode = COMPERR_FUNCTIONGOTO;
					return -1;
				}
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			}
			;

callstmt:	B256CALL args_v args_none {
				addIntOp(OP_PUSHINT, CALLSIG_SUBROUTINE); // used to check of subroutine was really called
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			}
			| B256CALL args_v '(' exprlist ')' {
				addIntOp(OP_PUSHINT, CALLSIG_SUBROUTINE); // used to check of subroutine was really called
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			}
			;

offerrorstmt:
			B256OFFERROR args_none {
				int i;
				for(i=0; i < numifs; i++) {
					if (iftabletype[i]==IFTABLETYPETRY || iftabletype[i]==IFTABLETYPECATCH) {
						errorcode = COMPERR_NOTINTRYCATCH;
						return -1;
					}
				}
				addOp(OP_OFFERROR);
			}
			;

onerrorstmt:
			B256ONERROR args_v {
				int i;
				for(i=0; i < numifs; i++) {
					if (iftabletype[i]==IFTABLETYPETRY || iftabletype[i]==IFTABLETYPECATCH) {
						errorcode = COMPERR_NOTINTRYCATCH;
						return -1;
					}
				}
				addIntOp(OP_ONERRORGOSUB, varnumber[--nvarnumber]);
			}
			;

returnstmt:	B256RETURN args_none {
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
			| B256RETURN expr {
				if (functionDefSymbol!=-1) {
					// value on stack gets returned
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
				} else {
					errorcode = COMPERR_RETURNVALUE;
					return -1;
				}
			}
			;

colorstmt:	B256SETCOLOR args_eee {
				addIntOp(OP_PUSHINT, 255);
				addOp(OP_RGB);
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
				newParseWarning(COMPWARNING_DEPRECATED_FORM);
			}
			| B256SETCOLOR expr {
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
			}
			| B256SETCOLOR args_ee  {
				addOp(OP_SETCOLOR);
			}
			;

soundstmt:	B256SOUND args_A {
				addOp(OP_SOUND_LIST);
			}
			| B256SOUND listoflists {
				addOp(OP_SOUND_LIST);
			}
			| B256SOUND args_ee {
				addIntOp(OP_PUSHINT, 2);	// 2 columns
				addIntOp(OP_PUSHINT, 1);	// 1 row
				addOp(OP_SOUND_LIST);
			}
			;

plotstmt: 	B256PLOT args_ee {
				addOp(OP_PLOT);
			}
			;

linestmt:	B256LINE args_eeee {
				addOp(OP_LINE);
			}
			;


circlestmt:
			B256CIRCLE args_eee {
				addOp(OP_CIRCLE);
			}
			;

arcstmt: 	B256ARC args_eeeeee {
				addOp(OP_ARC);
			}
			;

chordstmt:
			B256CHORD args_eeeeee {
				addOp(OP_CHORD);
			}
			;

piestmt:
			B256PIE args_eeeeee {
				addOp(OP_PIE);
			}
			;

rectstmt:
			B256RECT args_eeee {
				addOp(OP_RECT);
			}
			;

textstmt:
			B256TEXT args_eee {
				addOp(OP_TEXT);
			}
			;

fontstmt:
			B256FONT args_eee {
				addOp(OP_FONT);
			}
			;

saystmt: 	B256SAY expr {
				addOp(OP_SAY);
			}
			;

systemstmt:
			B256SYSTEM expr {
				addOp(OP_SYSTEM);
			}
			;

volumestmt:
			B256VOLUME expr {
				addOp(OP_VOLUME);
			}
			;

polystmt:
			B256POLY args_v {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_POLY_LIST);
			}
			| B256POLY listoflists {
				addOp(OP_POLY_LIST);
			}
			;

stampstmt: 	B256STAMP args_eeeA {
				addOp(OP_STAMP_S_LIST);
			}
			| B256STAMP args_eeei {
				addOp(OP_STAMP_S_LIST);
			}
			| B256STAMP args_eeA {
				addOp(OP_STAMP_LIST);
			}
			| B256STAMP args_eei {
				addOp(OP_STAMP_LIST);
			}
			| B256STAMP args_eeeeA {
				addOp(OP_STAMP_SR_LIST);
			}
			| B256STAMP args_eeeei {
				addOp(OP_STAMP_SR_LIST);
			}
			;

openstmt:	B256OPEN expr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			}
			| B256OPEN args_ee {
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			}
			| B256OPENB expr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			}
			| B256OPENB args_ee {
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			}
			| B256OPENSERIAL args_ee {
				addIntOp(OP_PUSHINT, 9600); // baud
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eee {
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeee {
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeee {
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeeee {
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeeeee {
				addOp(OP_OPENSERIAL);
			}
			;

writestmt:	B256WRITE expr {
				addIntOp(OP_PUSHINT, 0);  // file number zero
				addOp(OP_STACKSWAP);
				addOp(OP_WRITE);
			}
			| B256WRITE args_ee {
				addOp(OP_WRITE);
			}
			;

writelinestmt:
			B256WRITELINE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITELINE);
			}
			| B256WRITELINE args_ee {
				addOp(OP_WRITELINE);
			}
			;

writebytestmt:
			B256WRITEBYTE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITEBYTE);
			}
			| B256WRITEBYTE args_ee {
				addOp(OP_WRITEBYTE);
			}
			;

closestmt:	B256CLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_CLOSE);
			}
			| B256CLOSE expr {
				addOp(OP_CLOSE);
			}
			;

resetstmt:	B256RESET args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_RESET);
			}
			| B256RESET expr {
				addOp(OP_RESET);
			}
			;

seekstmt:	B256SEEK expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_SEEK);
			}
			| B256SEEK args_ee {
				addOp(OP_SEEK);
			}
			;

inputstmt:	B256INPUT args_ev {
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_v  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
                        | B256INPUT args_a {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_ev {
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_v  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_a {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_ev {
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_v  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_a {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_ev {
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_v  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_a {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			;

printstmt:
			B256PRINT args_none {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_PRINTN);
			}
			| B256PRINT expr {
				addOp(OP_PRINTN);
			}
			| B256PRINT expr B256SEMICOLON {
				addOp(OP_PRINT);
			}
			;

wavpausestmt:
			B256WAVPAUSE args_none {
				addOp(OP_WAVPAUSE);
			}
			;

wavplaystmt:
			B256WAVPLAY args_none {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_WAVPLAY);
			}
			| B256WAVPLAY expr {
				addOp(OP_WAVPLAY);
			}
			;
wavseekstmt:
			B256WAVSEEK expr {
				addOp(OP_WAVSEEK);
			}
			;

wavstopstmt:
			B256WAVSTOP args_none {
				addOp(OP_WAVSTOP);
			}
			;

wavwaitstmt:
			B256WAVWAIT args_none {
				addOp(OP_WAVWAIT);
			}
			;

putslicestmt:
			B256PUTSLICE args_eee  {
				addOp(OP_PUTSLICE);
			}
			| B256PUTSLICE args_eeee  {
				addOp(OP_PUTSLICEMASK);
			}
			;

imgloadstmt:
			B256IMGLOAD args_eee
			{
				addIntOp(OP_PUSHINT, 1); // scale
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_eeee
			{
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_eeeee
			{
				addOp(OP_IMGLOAD);
			}
			;

spritedimstmt:
			B256SPRITEDIM expr {
				addOp(OP_SPRITEDIM);
			}
			;

spriteloadstmt:
			B256SPRITELOAD args_ee  {
				addOp(OP_SPRITELOAD);
			}
			;

spriteslicestmt:
			B256SPRITESLICE args_eeeee {
				addOp(OP_SPRITESLICE);
			}
			;

spritepolystmt:
			B256SPRITEPOLY args_eA {
				addOp(OP_SPRITEPOLY_LIST);
			}
			| B256SPRITEPOLY args_ei {
				addOp(OP_SPRITEPOLY_LIST);
			}
			;

spriteplacestmt:
			B256SPRITEPLACE args_eee
			{
				addIntOp(OP_PUSHINT,1);	// scale
				addIntOp(OP_PUSHINT,0);	// rotate
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_eeee
			{
				addIntOp(OP_PUSHINT,0);	// rotate
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_eeeee
			{
				addOp(OP_SPRITEPLACE);
			}
			;

spritemovestmt:
			B256SPRITEMOVE args_eee
			{
				addIntOp(OP_PUSHINT,0);	// scale (change in scale)
				addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_eeee  {
				addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_eeeee {
				addOp(OP_SPRITEMOVE);
			}
			;

spritehidestmt:
			B256SPRITEHIDE expr {
				addOp(OP_SPRITEHIDE);
			}
			;

spriteshowstmt:
			B256SPRITESHOW expr {
				addOp(OP_SPRITESHOW);
			}
			;

clickclearstmt:
			B256CLICKCLEAR args_none {
				addOp(OP_CLICKCLEAR);
			}
			;

changedirstmt:
			B256CHANGEDIR expr {
				addOp(OP_CHANGEDIR);
			}
			;

dbopenstmt:
			B256DBOPEN expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPEN);
			}
			| B256DBOPEN args_ee {
				addOp(OP_DBOPEN);
			}
			;

dbclosestmt:
			B256DBCLOSE args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_DBCLOSE);
			}
			| B256DBCLOSE expr {
				addOp(OP_DBCLOSE);
			}
			;

dbexecutestmt:
			B256DBEXECUTE expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBEXECUTE);
			}
			| B256DBEXECUTE args_ee {
				addOp(OP_DBEXECUTE);
			}
			;

dbopensetstmt:
			B256DBOPENSET expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_ee {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_eee {
				addOp(OP_DBOPENSET);
			}
			;

dbclosesetstmt:
			B256DBCLOSESET args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET expr {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET args_ee {
				addOp(OP_DBCLOSESET);
			}
			;

netlistenstmt:
			B256NETLISTEN expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETLISTEN);
			}
			| B256NETLISTEN args_ee {
				addOp(OP_NETLISTEN);
			}
			;

netconnectstmt:
			B256NETCONNECT args_ee {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKTOPTO2);
				addOp(OP_NETCONNECT);
			}
			| B256NETCONNECT args_eee {
				addOp(OP_NETCONNECT);
			}
			;

netwritestmt:
			B256NETWRITE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETWRITE);
			}
			| B256NETWRITE args_ee {
				addOp(OP_NETWRITE);
			}
			;

netclosestmt:
			B256NETCLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_NETCLOSE);
			}
			| B256NETCLOSE expr {
				addOp(OP_NETCLOSE);
			}
			;

killstmt: 	B256KILL expr {
				addOp(OP_KILL);
			}
			;

setsettingstmt:
			B256SETSETTING args_eee {
				addOp(OP_SETSETTING);
			}
			;

portoutstmt:
			B256PORTOUT args_ee {
				addOp(OP_PORTOUT);
			}
			;

imgsavestmt:
			B256IMGSAVE expr  {
				addStringOp(OP_PUSHSTRING, "PNG");
				addOp(OP_IMGSAVE);
			}
			| B256IMGSAVE args_ee {
				addOp(OP_IMGSAVE);
			}
			;

editvisiblestmt:
			B256EDITVISIBLE expr {
				addOp(OP_EDITVISIBLE);
			}
			;

graphvisiblestmt:
			B256GRAPHVISIBLE expr {
				addOp(OP_GRAPHVISIBLE);
			}
			;

outputvisiblestmt:
			B256OUTPUTVISIBLE expr {
				addOp(OP_OUTPUTVISIBLE);
			}
			;

globalstmt:
			B256GLOBAL functionvariables {
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

penwidthstmt:
			B256PENWIDTH expr {
				addOp(OP_PENWIDTH);
			}
			;

alertstmt:
			B256ALERT expr {
				addOp(OP_ALERT);
			}
			;

continuedostmt:
			B256CONTINUEDO {
				// find most recent DO and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEDO) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
					errorcode = COMPERR_CONTINUEDO;
					return -1;
				}
			}
			;

continueforstmt:
			B256CONTINUEFOR {
				// find most recent FOR and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEFOR) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
					errorcode = COMPERR_CONTINUEFOR;
					return -1;
				}
			}
			;

continuewhilestmt:
			B256CONTINUEWHILE {
				// find most recent WHILE and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEWHILE) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
					errorcode = COMPERR_EXITWHILE;
					return -1;
				}
			}
			;

exitdostmt:
			B256EXITDO {
				// find most recent DO and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEDO) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITDO;
					return -1;
				}
			}
			;

exitforstmt:
			B256EXITFOR {
				// find most recent FOR and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEFOR) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_EXITFOR, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITFOR;
					return -1;
				}
			}
			;

exitwhilestmt:
			B256EXITWHILE {
				// find most recent WHILE and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEWHILE) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITWHILE;
					return -1;
				}
			}
			;

printercancelstmt:
			B256PRINTERCANCEL args_none {
				addOp(OP_PRINTERCANCEL);
			}
			;

printeroffstmt:
			B256PRINTEROFF args_none {
				addOp(OP_PRINTEROFF);
			}
			;

printeronstmt:
			B256PRINTERON args_none {
				addOp(OP_PRINTERON);
			}
			;

printerpagestmt:
			B256PRINTERPAGE args_none {
				addOp(OP_PRINTERPAGE);
			}
			;

functionstmt:
			B256FUNCTION args_v functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the function - add the start to the label table
				functionDefSymbol = varnumber[--nvarnumber];
				//
				// create jump around function definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this function
				symtableaddress[functionDefSymbol] = wordOffset;
				newIf(linenumber, IFTABLETYPEFUNCTION);
				//
				// test that this is a real function call or error - before the call a CALLSIG
				// should have bene pushed to the stack.  Pop it and test that it is the correct one
				addIntOp(OP_PUSHINT, CALLSIG_FUNCTION);
				addOp(OP_NEQUAL);
				addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				newIf(linenumber, IFTABLETYPEIF);
				addIntOp(OP_PUSHINT, ERROR_BADCALLFUNCTION);
				addOp(OP_THROWERROR);
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
				//
				// check to see if there are enough values on the stack
				addIntOp(OP_PUSHINT, numargs);
				addOp(OP_ARGUMENTCOUNTTEST);
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						if (argstype[t]==ARGSTYPEVALUE) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
					}
				}
				//
				// initialize return variable
				addIntOp(OP_PUSHINT, 0);
				addIntOp(OP_ASSIGN, functionDefSymbol);
				//
				numargs=0;	// clear the list for next function
			}
			;

subroutinestmt:
			B256SUBROUTINE args_v functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the subroutine - add the start to the label table
				subroutineDefSymbol = varnumber[--nvarnumber];
				//
				// create jump around subroutine definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this subroutine
				symtableaddress[subroutineDefSymbol] = wordOffset;
				newIf(linenumber, IFTABLETYPEFUNCTION);
				//
				// test that this is a real subroutine call or error - before the call a CALLSIG
				// should have bene pushed to the stack.  Pop it and test that it is the correct one
				addIntOp(OP_PUSHINT, CALLSIG_SUBROUTINE);
				addOp(OP_NEQUAL);
				addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				newIf(linenumber, IFTABLETYPEIF);
				addIntOp(OP_PUSHINT, ERROR_BADCALLSUBROUTINE);
				addOp(OP_THROWERROR);
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
				//
				// check to see if there are enough values on the stack
				addIntOp(OP_PUSHINT, numargs);
				addOp(OP_ARGUMENTCOUNTTEST);
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						if (argstype[t]==ARGSTYPEVALUE) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
					}
				}
				numargs=0;	// clear the list for next function
			}
			;

endfunctionstmt:
			B256ENDFUNCTION {
				if (numifs>0) {
				if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) {
					//
					// add return if there is not one
					addIntOp(OP_PUSHVAR, functionDefSymbol);
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
					//
					// add address for jump around function definition
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					functionDefSymbol = -1;
					//
					numifs--;
				//
				} else {
					errorcode = testIfOnTableError(numincludes);
					linenumber = testIfOnTable(numincludes);
					return -1;
				}
			} else {
				errorcode = COMPERR_ENDFUNCTION;
				return -1;
			}
		}
		;

endsubroutinestmt:
		B256ENDSUBROUTINE {
			if (numifs>0) {
				if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) {
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
					//
					// add address for jump around function definition
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					subroutineDefSymbol = -1;
					//
					numifs--;
				} else {
					errorcode = testIfOnTableError(numincludes);
					linenumber = testIfOnTable(numincludes);
					return -1;
				}
			} else {
				errorcode = COMPERR_ENDFUNCTION;
				return -1;
			}
		}
		;

regexminimalstmt:
			B256REGEXMINIMAL expr {
				addOp(OP_REGEXMINIMAL);
			}
			;

unassignstmt:
			B256UNASSIGN args_v {
				addIntOp(OP_UNASSIGN, varnumber[--nvarnumber]);
			}
			| B256UNASSIGN args_a {
				addIntOp(OP_UNASSIGNA, varnumber[--nvarnumber]);
			}
			;


variablewatchstmt:
			B256VARIABLEWATCH args_v {
				addIntOp(OP_VARIABLEWATCH, varnumber[--nvarnumber]);
			}
			;




/* ####################################
   ### INSERT NEW Statements BEFORE ###
   #################################### */

listoflists:
			'{' listinlist '}'{addIntOp(OP_PUSHINT, numberoflists); numberoflists = 0;}
			| B256EXPLODE args_ee {
				addIntOp(OP_PUSHINT, 0);	// case sensitive flag
				addOp(OP_EXPLODE);
			}
			| B256EXPLODE args_eee {
				addOp(OP_EXPLODE);
			}
			|  B256EXPLODEX args_ee{
				addOp(OP_EXPLODEX);
			}
			;

listinlist:
			exprlist {addIntOp(OP_PUSHINT, listlen); listlen = 0; numberoflists = 1; }
			| immediatelist {numberoflists = 1; }
			| immediatelist ',' listinlist {numberoflists++;}
			;

immediatelist:
			'{' exprlist '}' {addIntOp(OP_PUSHINT, listlen); listlen = 0;}
			;


exprlist:
			expr { listlen = 1; }
			| expr ',' exprlist {listlen++;}
			;




expr:

			/* *** expressions that can ge EITHER numeric or string *** */
			'(' expr ')'
			| expr '+' expr {
				addOp(OP_ADD);
			}
			| expr B256SEMICOLON expr {
				addOp(OP_CONCATENATE);
			}
			| expr B256AMP expr {
				addOp(OP_BINARYAND);
			}


			/* *** variable and function expressions *** */

			| args_v '[' '?' ']' { addIntOp(OP_ALEN, varnumber[--nvarnumber]); }
			| args_v '[' '?' ',' ']' { addIntOp(OP_ALENX, varnumber[--nvarnumber]); }
			| args_v '[' ',' '?' ']' { addIntOp(OP_ALENY, varnumber[--nvarnumber]); }
			| args_v '(' exprlist ')' {
				// function call with arguments
				addIntOp(OP_PUSHINT, CALLSIG_FUNCTION); // used to check of function was really called
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			}
			| args_v '(' ')' {
				// function call without arguments
				addIntOp(OP_PUSHINT, CALLSIG_FUNCTION); // used to check of function was really called
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
			}
			| args_v {
				addIntOp(OP_PUSHVAR, varnumber[--nvarnumber]);
			}
			| args_a {
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
			}


			/* *** numeric Experssions *** */

			| expr '-' expr {
				addOp(OP_SUB);
			}
			| expr '*' expr {
				addOp(OP_MUL);
			}
			| expr B256MOD expr {
				addOp(OP_MOD);
			}
			| expr B256INTDIV expr {
				addOp(OP_INTDIV);
			}
			| expr '/' expr {
				addOp(OP_DIV);
			}
			| expr '^' expr { addOp(OP_EX); }
			| expr B256BINARYOR expr { addOp(OP_BINARYOR); }
			| B256BINARYNOT expr { addOp(OP_BINARYNOT); }
			| '-' expr %prec B256UMINUS { addOp(OP_NEGATE); }
			| expr B256AND expr {addOp(OP_AND); }
			| expr B256OR expr { addOp(OP_OR); }
			| expr B256XOR expr { addOp(OP_XOR); }
			| B256NOT expr %prec B256UMINUS { addOp(OP_NOT); }
			| expr '=' expr { addOp(OP_EQUAL); }
			| expr B256NE expr { addOp(OP_NEQUAL); }
			| expr '<' expr { addOp(OP_LT); }
			| expr '>' expr { addOp(OP_GT); }
			| expr B256GTE expr { addOp(OP_GTE); }
			| expr B256LTE expr { addOp(OP_LTE); }
			| B256FLOAT   { addFloatOp(OP_PUSHFLOAT, $1); }
			| B256INTEGER { addIntOp(OP_PUSHINT, $1); }
			| args_a B256ADD1 {
				// a[b,c]++
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256SUB1 {
				// a[b,c]--
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| B256ADD1 args_a {
				// ++a[b,c]
				addOp(OP_STACKDUP2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
				addIntOp(OP_DEREF, varnumber[nvarnumber]);
			}
			| B256SUB1 args_a {
				// --a[b,c]
				addOp(OP_STACKDUP2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
				addIntOp(OP_DEREF, varnumber[nvarnumber]);
			}
			| args_v B256ADD1 {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHVAR,varnumber[nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256SUB1 {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHVAR,varnumber[nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| B256ADD1 args_v {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
				addIntOp(OP_PUSHVAR,varnumber[nvarnumber]);
			}
			| B256SUB1 args_v {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_SUB);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
				addIntOp(OP_PUSHVAR,varnumber[nvarnumber]);
			}
			| B256TOINT '(' expr ')' { addOp(OP_INT); }
			| B256TOFLOAT '(' expr ')' { addOp(OP_FLOAT); }
			| B256LENGTH '(' expr ')' { addOp(OP_LENGTH); }
			| B256ASC '(' expr ')' { addOp(OP_ASC); }
			| B256INSTR '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 1);	// start
				addIntOp(OP_PUSHINT, 0);	// case sens flag
				addOp(OP_INSTR);
			}
			| B256INSTR '(' expr ',' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 0);	// case sens flag
				addOp(OP_INSTR);
			 }
			| B256INSTR '(' expr ',' expr ',' expr ',' expr')' { addOp(OP_INSTR); }
			| B256INSTRX '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 1);	//start
				addOp(OP_INSTRX);
			}
			| B256INSTRX '(' expr ',' expr ',' expr ')' { addOp(OP_INSTRX); }
			| B256CEIL '(' expr ')' { addOp(OP_CEIL); }
			| B256FLOOR '(' expr ')' { addOp(OP_FLOOR); }
			| B256SIN '(' expr ')' { addOp(OP_SIN); }
			| B256COS '(' expr ')' { addOp(OP_COS); }
			| B256TAN '(' expr ')' { addOp(OP_TAN); }
			| B256ASIN '(' expr ')' { addOp(OP_ASIN); }
			| B256ACOS '(' expr ')' { addOp(OP_ACOS); }
			| B256ATAN '(' expr ')' { addOp(OP_ATAN); }
			| B256DEGREES '(' expr ')' { addOp(OP_DEGREES); }
			| B256RADIANS '(' expr ')' { addOp(OP_RADIANS); }
			| B256LOG '(' expr ')' { addOp(OP_LOG); }
			| B256LOGTEN '(' expr ')' { addOp(OP_LOGTEN); }
			| B256SQR '(' expr ')' { addOp(OP_SQR); }
			| B256EXP '(' expr ')' { addOp(OP_EXP); }
			| B256ABS '(' expr ')' { addOp(OP_ABS); }
			| B256RAND args_none { addOp(OP_RAND); }
			| B256PI args_none { addFloatOp(OP_PUSHFLOAT, 3.14159265358979323846); }
			| B256BOOLTRUE args_none { addIntOp(OP_PUSHINT, 1); }
			| B256BOOLFALSE args_none { addIntOp(OP_PUSHINT, 0); }
			| B256BOOLEOF args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_EOF);
			}
			| B256BOOLEOF '(' expr ')' { addOp(OP_EOF); }
			| B256EXISTS '(' expr ')' { addOp(OP_EXISTS); }
			| B256YEAR args_none { addOp(OP_YEAR); }
			| B256MONTH args_none { addOp(OP_MONTH); }
			| B256DAY args_none { addOp(OP_DAY); }
			| B256HOUR args_none { addOp(OP_HOUR); }
			| B256MINUTE args_none { addOp(OP_MINUTE); }
			| B256SECOND args_none { addOp(OP_SECOND); }
			| B256GRAPHWIDTH args_none { addOp(OP_GRAPHWIDTH); }
			| B256GRAPHHEIGHT args_none { addOp(OP_GRAPHHEIGHT); }
			| B256SIZE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_SIZE);
			}
			| B256SIZE '(' expr ')' { addOp(OP_SIZE); }
			| B256KEYPRESSED args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addOp(OP_KEYPRESSED);
			}
			| B256KEYPRESSED '(' expr ')' { addOp(OP_KEYPRESSED); }
			| B256KEY args_none     { addOp(OP_KEY); }
			| B256MOUSEX args_none { addOp(OP_MOUSEX); }
			| B256MOUSEY args_none { addOp(OP_MOUSEY); }
			| B256MOUSEB args_none { addOp(OP_MOUSEB); }
			| B256CLICKX args_none { addOp(OP_CLICKX); }
			| B256CLICKY args_none { addOp(OP_CLICKY); }
			| B256CLICKB args_none { addOp(OP_CLICKB); }
			| B256CLEAR args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addOp(OP_RGB);
				}
			| B256BLACK args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256WHITE args_none {
				addIntOp(OP_PUSHINT, 0xff);
				addIntOp(OP_PUSHINT, 0xff);
				addIntOp(OP_PUSHINT, 0xff);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256RED args_none {
				addIntOp(OP_PUSHINT, 0xff);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKRED args_none {
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256GREEN args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKGREEN args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256BLUE args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKBLUE args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256CYAN args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKCYAN args_none {
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256PURPLE args_none {
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKPURPLE args_none {
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256YELLOW args_none {
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKYELLOW args_none {
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256ORANGE args_none {
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0x66);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKORANGE args_none {
				addIntOp(OP_PUSHINT, 0xb0);
				addIntOp(OP_PUSHINT, 0x3d);
				addIntOp(OP_PUSHINT, 0x00);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256GREY args_none {
				addIntOp(OP_PUSHINT, 0xA4);
				addIntOp(OP_PUSHINT, 0xA4);
				addIntOp(OP_PUSHINT, 0xA4);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256DARKGREY args_none {
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0x80);
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
				}
			| B256PIXEL '(' expr ',' expr ')' { addOp(OP_PIXEL); }
			| B256RGB '(' expr ',' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 0xff);
				addOp(OP_RGB);
			}
			| B256RGB '(' expr ',' expr ',' expr ',' expr ')' {
				addOp(OP_RGB);
			}
			| B256GETCOLOR args_none { addOp(OP_GETCOLOR); }
			| B256GETBRUSHCOLOR args_none { addOp(OP_GETBRUSHCOLOR); }
			| B256GETPENWIDTH args_none { addOp(OP_GETPENWIDTH); }
			| B256SPRITECOLLIDE '(' expr ',' expr ')' { addOp(OP_SPRITECOLLIDE); }
			| B256SPRITEX '(' expr ')' { addOp(OP_SPRITEX); }
			| B256SPRITEY '(' expr ')' { addOp(OP_SPRITEY); }
			| B256SPRITEH '(' expr ')' { addOp(OP_SPRITEH); }
			| B256SPRITEW '(' expr ')' { addOp(OP_SPRITEW); }
			| B256SPRITEV '(' expr ')' { addOp(OP_SPRITEV); }
			| B256SPRITER '(' expr ')' { addOp(OP_SPRITER); }
			| B256SPRITES '(' expr ')' { addOp(OP_SPRITES); }
			| B256DBROW args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBROW);
			}
			| B256DBROW '(' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBROW);
			}
			| B256DBROW '(' expr ',' expr')' {
				addOp(OP_DBROW);
			}
			| B256DBINT '(' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINT); }
			| B256DBINT '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINT); }
			| B256DBINT '(' expr ',' expr ',' expr ')' {
				addOp(OP_DBINT); }
			| B256DBFLOAT '(' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOAT); }
			| B256DBFLOAT '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOAT); }
			| B256DBFLOAT '(' expr ',' expr ',' expr ')' {
				addOp(OP_DBFLOAT); }
			| B256DBNULL '(' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULL); }
			| B256DBNULL '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULL); }
			| B256DBNULL '(' expr ',' expr ',' expr ')' {
				addOp(OP_DBNULL); }
			| B256LASTERROR args_none { addOp(OP_LASTERROR); }
			| B256LASTERRORLINE args_none { addOp(OP_LASTERRORLINE); }
			| B256NETDATA args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETDATA); }
			| B256NETDATA '(' expr ')' { addOp(OP_NETDATA); }
			| B256PORTIN '(' expr ')' { addOp(OP_PORTIN); }
			| B256COUNT '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 0); // case sens flag
				addOp(OP_COUNT);
			 }
			| B256COUNT '(' expr ',' expr ',' expr ')' { addOp(OP_COUNT); }
			| B256COUNTX '(' expr ',' expr ')' { addOp(OP_COUNTX); }
			| B256OSTYPE args_none { addOp(OP_OSTYPE); }
			| B256MSEC args_none { addOp(OP_MSEC); }
			| B256TEXTWIDTH '(' expr ')' { addOp(OP_TEXTWIDTH); }
			| B256TEXTHEIGHT args_none { addOp(OP_TEXTHEIGHT); }
			| B256READBYTE args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READBYTE); }
			| B256READBYTE '(' expr ')' { addOp(OP_READBYTE); }
			| B256REF '(' args_v ')' { addIntOp(OP_PUSHVARREF, varnumber[--nvarnumber]); }
			| B256FREEDB args_none { addOp(OP_FREEDB); }
			| B256FREEDBSET args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_FREEDBSET);
			}
			| B256FREEDBSET '(' expr ')' { addOp(OP_FREEDBSET); }
			| B256FREEFILE args_none { addOp(OP_FREEFILE); }
			| B256FREENET args_none { addOp(OP_FREENET); }
			| B256VERSION args_none { addIntOp(OP_PUSHINT, VERSIONSIGNATURE); }
			| B256CONFIRM '(' expr ')' {
				addIntOp(OP_PUSHINT,-1);	// no default
				addOp(OP_CONFIRM);
			}
			| B256CONFIRM '(' expr ',' expr ')' {
				addOp(OP_CONFIRM);
			}
			| B256FROMBINARY '(' expr ')' {
				addIntOp(OP_PUSHINT,2);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMHEX '(' expr ')' {
				addIntOp(OP_PUSHINT,16);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMOCTAL '(' expr ')' {
				addIntOp(OP_PUSHINT,8);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMRADIX '(' expr ',' expr ')' {
				addOp(OP_FROMRADIX);
			}
			| B256BINCONST {
				addStringOp(OP_PUSHSTRING, $1);
				addIntOp(OP_PUSHINT,2);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256HEXCONST {
				addStringOp(OP_PUSHSTRING, $1);
				addIntOp(OP_PUSHINT,16);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256OCTCONST {
				addStringOp(OP_PUSHSTRING, $1);
				addIntOp(OP_PUSHINT,8);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256WAVLENGTH args_none { addOp(OP_WAVLENGTH); }
			| B256WAVPOS args_none { addOp(OP_WAVPOS); }
			| B256WAVSTATE args_none { addOp(OP_WAVSTATE); }

			| B256ERROR_NONE args_none { addIntOp(OP_PUSHINT, ERROR_NONE); }
			| B256ERROR_NOSUCHLABEL args_none { addIntOp(OP_PUSHINT, ERROR_NOSUCHLABEL); }
			| B256ERROR_NEXTNOFOR args_none { addIntOp(OP_PUSHINT, ERROR_NEXTNOFOR); }
			| B256ERROR_NOTARRAY args_none { addIntOp(OP_PUSHINT, ERROR_NOTARRAY); }
			| B256ERROR_ARGUMENTCOUNT args_none { addIntOp(OP_PUSHINT, ERROR_ARGUMENTCOUNT); }
			| B256ERROR_MAXRECURSE args_none { addIntOp(OP_PUSHINT, ERROR_MAXRECURSE); }
			| B256ERROR_STACKUNDERFLOW args_none { addIntOp(OP_PUSHINT, ERROR_STACKUNDERFLOW); }
			| B256ERROR_BADCALLFUNCTION args_none { addIntOp(OP_PUSHINT, ERROR_BADCALLFUNCTION); }
			| B256ERROR_BADCALLSUBROUTINE args_none { addIntOp(OP_PUSHINT, ERROR_BADCALLSUBROUTINE); }
			| B256ERROR_FILENUMBER args_none { addIntOp(OP_PUSHINT, ERROR_FILENUMBER); }
			| B256ERROR_FILEOPEN args_none { addIntOp(OP_PUSHINT, ERROR_FILEOPEN); }
			| B256ERROR_FILENOTOPEN args_none { addIntOp(OP_PUSHINT, ERROR_FILENOTOPEN); }
			| B256ERROR_FILEWRITE args_none { addIntOp(OP_PUSHINT, ERROR_FILEWRITE); }
			| B256ERROR_FILERESET args_none { addIntOp(OP_PUSHINT, ERROR_FILERESET); }
			| B256ERROR_ARRAYSIZELARGE args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYSIZELARGE); }
			| B256ERROR_ARRAYSIZESMALL args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYSIZESMALL); }
			| B256ERROR_VARNOTASSIGNED args_none { addIntOp(OP_PUSHINT, ERROR_VARNOTASSIGNED); }
			| B256ERROR_ARRAYNITEMS args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYNITEMS); }
			| B256ERROR_ARRAYINDEX args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYINDEX); }
			| B256ERROR_STRSTART args_none { addIntOp(OP_PUSHINT, ERROR_STRSTART); }
			| B256ERROR_RGB args_none { addIntOp(OP_PUSHINT, ERROR_RGB); }
			| B256ERROR_PUTBITFORMAT args_none { addIntOp(OP_PUSHINT, ERROR_PUTBITFORMAT); }
			| B256ERROR_POLYPOINTS args_none { addIntOp(OP_PUSHINT, ERROR_POLYPOINTS); }
			| B256ERROR_IMAGEFILE args_none { addIntOp(OP_PUSHINT, ERROR_IMAGEFILE); }
			| B256ERROR_SPRITENUMBER args_none { addIntOp(OP_PUSHINT, ERROR_SPRITENUMBER); }
			| B256ERROR_SPRITENA args_none { addIntOp(OP_PUSHINT, ERROR_SPRITENA); }
			| B256ERROR_SPRITESLICE args_none { addIntOp(OP_PUSHINT, ERROR_SPRITESLICE); }
			| B256ERROR_FOLDER args_none { addIntOp(OP_PUSHINT, ERROR_FOLDER); }
			| B256ERROR_INFINITY args_none { addIntOp(OP_PUSHINT, ERROR_INFINITY); }
			| B256ERROR_DBOPEN args_none { addIntOp(OP_PUSHINT, ERROR_DBOPEN); }
			| B256ERROR_DBQUERY args_none { addIntOp(OP_PUSHINT, ERROR_DBQUERY); }
			| B256ERROR_DBNOTOPEN args_none { addIntOp(OP_PUSHINT, ERROR_DBNOTOPEN); }
			| B256ERROR_DBCOLNO args_none { addIntOp(OP_PUSHINT, ERROR_DBCOLNO); }
			| B256ERROR_DBNOTSET args_none { addIntOp(OP_PUSHINT, ERROR_DBNOTSET); }
			| B256ERROR_TYPECONV args_none { addIntOp(OP_PUSHINT, ERROR_TYPECONV); }
			| B256ERROR_NETSOCK args_none { addIntOp(OP_PUSHINT, ERROR_NETSOCK); }
			| B256ERROR_NETHOST args_none { addIntOp(OP_PUSHINT, ERROR_NETHOST); }
			| B256ERROR_NETCONN args_none { addIntOp(OP_PUSHINT, ERROR_NETCONN); }
			| B256ERROR_NETREAD args_none { addIntOp(OP_PUSHINT, ERROR_NETREAD); }
			| B256ERROR_NETNONE args_none { addIntOp(OP_PUSHINT, ERROR_NETNONE); }
			| B256ERROR_NETWRITE args_none { addIntOp(OP_PUSHINT, ERROR_NETWRITE); }
			| B256ERROR_NETSOCKOPT args_none { addIntOp(OP_PUSHINT, ERROR_NETSOCKOPT); }
			| B256ERROR_NETBIND args_none { addIntOp(OP_PUSHINT, ERROR_NETBIND); }
			| B256ERROR_NETACCEPT args_none { addIntOp(OP_PUSHINT, ERROR_NETACCEPT); }
			| B256ERROR_NETSOCKNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_NETSOCKNUMBER); }
			| B256ERROR_PERMISSION args_none { addIntOp(OP_PUSHINT, ERROR_PERMISSION); }
			| B256ERROR_IMAGESAVETYPE args_none { addIntOp(OP_PUSHINT, ERROR_IMAGESAVETYPE); }
			| B256ERROR_DIVZERO args_none { addIntOp(OP_PUSHINT, ERROR_DIVZERO); }
			| B256ERROR_BYREF args_none { addIntOp(OP_PUSHINT, ERROR_BYREF); }
			| B256ERROR_FREEFILE args_none { addIntOp(OP_PUSHINT, ERROR_FREEFILE); }
			| B256ERROR_FREENET args_none { addIntOp(OP_PUSHINT, ERROR_FREENET); }
			| B256ERROR_FREEDB args_none { addIntOp(OP_PUSHINT, ERROR_FREEDB); }
			| B256ERROR_DBCONNNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_DBCONNNUMBER); }
			| B256ERROR_FREEDBSET args_none { addIntOp(OP_PUSHINT, ERROR_FREEDBSET); }
			| B256ERROR_DBSETNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_DBSETNUMBER); }
			| B256ERROR_DBNOTSETROW args_none { addIntOp(OP_PUSHINT, ERROR_DBNOTSETROW); }
			| B256ERROR_PENWIDTH args_none { addIntOp(OP_PUSHINT, ERROR_PENWIDTH); }
			| B256ERROR_ARRAYINDEXMISSING args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYINDEXMISSING); }
			| B256ERROR_IMAGESCALE args_none { addIntOp(OP_PUSHINT, ERROR_IMAGESCALE); }
			| B256ERROR_FONTSIZE args_none { addIntOp(OP_PUSHINT, ERROR_FONTSIZE); }
			| B256ERROR_FONTWEIGHT args_none { addIntOp(OP_PUSHINT, ERROR_FONTWEIGHT); }
			| B256ERROR_RADIXSTRING args_none { addIntOp(OP_PUSHINT, ERROR_RADIXSTRING); }
			| B256ERROR_RADIX args_none { addIntOp(OP_PUSHINT, ERROR_RADIX); }
			| B256ERROR_LOGRANGE args_none { addIntOp(OP_PUSHINT, ERROR_LOGRANGE); }
			| B256ERROR_STRINGMAXLEN args_none { addIntOp(OP_PUSHINT, ERROR_STRINGMAXLEN); }
			| B256ERROR_PRINTERNOTON args_none { addIntOp(OP_PUSHINT, ERROR_PRINTERNOTON); }
			| B256ERROR_PRINTERNOTOFF args_none { addIntOp(OP_PUSHINT, ERROR_PRINTERNOTOFF); }
			| B256ERROR_PRINTEROPEN args_none { addIntOp(OP_PUSHINT, ERROR_PRINTEROPEN); }
			| B256ERROR_WAVFILEFORMAT args_none { addIntOp(OP_PUSHINT, ERROR_WAVFILEFORMAT); }
			| B256ERROR_WAVNOTOPEN args_none { addIntOp(OP_PUSHINT, ERROR_WAVNOTOPEN); }
			| B256ERROR_WAVNOTSEEKABLE args_none { addIntOp(OP_PUSHINT, ERROR_WAVNOTSEEKABLE); }
			| B256ERROR_WAVNODURATION args_none { addIntOp(OP_PUSHINT, ERROR_WAVNODURATION); }
			| B256ERROR_FILEOPERATION args_none { addIntOp(OP_PUSHINT, ERROR_FILEOPERATION); }
			| B256ERROR_SERIALPARAMETER args_none { addIntOp(OP_PUSHINT, ERROR_SERIALPARAMETER); }
			| B256ERROR_LONGRANGE args_none { addIntOp(OP_PUSHINT, ERROR_LONGRANGE); }
			| B256ERROR_INTEGERRANGE args_none { addIntOp(OP_PUSHINT, ERROR_INTEGERRANGE); }
			| B256ERROR_NOTIMPLEMENTED args_none { addIntOp(OP_PUSHINT, ERROR_NOTIMPLEMENTED); }
			| B256WARNING_START args_none { addIntOp(OP_PUSHINT, WARNING_START); }
			| B256WARNING_TYPECONV args_none { addIntOp(OP_PUSHINT, WARNING_TYPECONV); }
			| B256WARNING_WAVNOTSEEKABLE args_none { addIntOp(OP_PUSHINT, WARNING_WAVNOTSEEKABLE); }
			| B256WARNING_WAVNODURATION args_none { addIntOp(OP_PUSHINT, WARNING_WAVNODURATION); }
			| B256WARNING_VARNOTASSIGNED args_none { addIntOp(OP_PUSHINT, WARNING_VARNOTASSIGNED); }
			| B256WARNING_LONGRANGE args_none { addIntOp(OP_PUSHINT, WARNING_LONGRANGE); }
			| B256WARNING_INTEGERRANGE args_none { addIntOp(OP_PUSHINT, WARNING_INTEGERRANGE); }
			| B256TYPEOF '(' expr ')' {
				addOp(OP_TYPEOF);
			}
			| B256TYPE_UNASSIGNED args_none { addIntOp(OP_PUSHINT, T_UNASSIGNED); }
			| B256TYPE_INT args_none { addIntOp(OP_PUSHINT, T_INT); }
			| B256TYPE_FLOAT args_none { addIntOp(OP_PUSHINT, T_FLOAT); }
			| B256TYPE_STRING args_none { addIntOp(OP_PUSHINT, T_STRING); }
			| B256TYPE_ARRAY args_none { addIntOp(OP_PUSHINT, T_ARRAY); }
			| B256TYPE_REF args_none { addIntOp(OP_PUSHINT, T_REF); }

			/* ###########################################
			   ### INSERT NEW Numeric Functions BEFORE ###
			   ########################################### */


			| B256STRING { addStringOp(OP_PUSHSTRING, $1); }
			| B256CHR '(' expr ')' { addOp(OP_CHR); }
			| B256TOSTRING '(' expr ')' { addOp(OP_STRING); }
			| B256UPPER '(' expr ')' { addOp(OP_UPPER); }
			| B256LOWER '(' expr ')' { addOp(OP_LOWER); }
			| B256MID '(' expr ',' expr ',' expr ')' { addOp(OP_MID); }
			| B256MIDX '(' expr ',' expr ')' { addIntOp(OP_PUSHINT, 1); addOp(OP_MIDX); }
			| B256MIDX '(' expr ',' expr ',' expr ')' { addOp(OP_MIDX); }
			| B256LEFT '(' expr ',' expr ')' { addOp(OP_LEFT); }
			| B256RIGHT '(' expr ',' expr ')' { addOp(OP_RIGHT); }
			| B256GETSLICE '(' expr ',' expr ',' expr ',' expr ')' { addOp(OP_GETSLICE); }
			| B256READ args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
			| B256READ '(' expr ')' { addOp(OP_READ); }
			| B256READLINE args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
			| B256READLINE '(' expr ')' { addOp(OP_READLINE); }
			| B256CURRENTDIR args_none { addOp(OP_CURRENTDIR); }
			| B256DBSTRING '(' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRING); }
			| B256DBSTRING '(' expr ',' expr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRING); }
			| B256DBSTRING '(' expr ',' expr ',' expr ')' {
				addOp(OP_DBSTRING); }
			| B256LASTERRORMESSAGE args_none { addOp(OP_LASTERRORMESSAGE); }
			| B256LASTERROREXTRA args_none { addOp(OP_LASTERROREXTRA); }
			| B256NETREAD  args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETREAD); }
			| B256NETREAD '(' expr ')' { addOp(OP_NETREAD); }
			| B256NETADDRESS args_none { addOp(OP_NETADDRESS); }
			| B256MD5 '(' expr ')' { addOp(OP_MD5); }
			| B256GETSETTING '(' expr ',' expr ')' { addOp(OP_GETSETTING); }
			| B256DIR '(' expr ')' { addOp(OP_DIR); }
			| B256DIR args_none { addStringOp(OP_PUSHSTRING, ""); addOp(OP_DIR); }
			| B256REPLACE '(' expr ',' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 0);	// case sens flag
				addOp(OP_REPLACE);
			}
			| B256REPLACE '(' expr ',' expr ',' expr ',' expr ')' { addOp(OP_REPLACE); }
			| B256REPLACEX '(' expr ',' expr ',' expr ')' { addOp(OP_REPLACEX); }
			| B256IMPLODE '(' args_A ')' {
				addStringOp(OP_PUSHSTRING, ""); // no delimiter
				addOp(OP_STACKDUP);
				addOp(OP_IMPLODE_LIST);
			}
			| B256IMPLODE '(' args_Ae ')' {
				addOp(OP_STACKDUP);				// same delimiter for rows and columns
				addOp(OP_IMPLODE_LIST);
			}
			| B256IMPLODE '(' args_Aee ')' {
				addOp(OP_IMPLODE_LIST);
			}
			| B256IMPLODE '(' listoflists ')' {
				addStringOp(OP_PUSHSTRING, ""); // no delimiter
				addOp(OP_STACKDUP);
				addOp(OP_IMPLODE_LIST);
			}
			| B256IMPLODE '(' args_ie ')' {
				addOp(OP_STACKDUP);				// same delimiter for rows and columns
				addOp(OP_IMPLODE_LIST);
			}
			| B256IMPLODE '(' args_iee ')' {
				addOp(OP_IMPLODE_LIST);
			}
			| B256PROMPT '(' expr ')' {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_PROMPT); }
			| B256PROMPT '(' expr ',' expr ')' {
				addOp(OP_PROMPT); }
			| B256TOBINARY '(' expr ')' {
				addIntOp(OP_PUSHINT,2);	// radix
				addOp(OP_TORADIX);
			}
			| B256TOHEX '(' expr ')' {
				addIntOp(OP_PUSHINT,16);	// radix
				addOp(OP_TORADIX);
			}
			| B256TOOCTAL '(' expr ')' {
				addIntOp(OP_PUSHINT,8);	// radix
				addOp(OP_TORADIX);
			}
			| B256TORADIX '(' expr ',' expr ')' {
				addOp(OP_TORADIX);
			}
			| B256DEBUGINFO '(' expr ')' {
				addOp(OP_DEBUGINFO);
			}
			| B256ISNUMERIC '(' expr ')' { addOp(OP_ISNUMERIC); }
			| B256LTRIM '(' expr ')' { addOp(OP_LTRIM); }
			| B256RTRIM '(' expr ')' { addOp(OP_RTRIM); }
			| B256TRIM '(' expr ')' { addOp(OP_TRIM); }


			/* ##########################################
			   ### INSERT NEW String Functions BEFORE ###
			   ########################################## */
			;

%%

int
yyerror(const char *msg) {
	(void) msg;
	errorcode = COMPERR_SYNTAX;
	return -1;
}
