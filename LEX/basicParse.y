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

unsigned int varnumber[IFTABLESIZE];	// stack of variable numbers in a statement to return the varmumber
int nvarnumber=0;


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

#define ARGSTYPEINT 0
#define ARGSTYPESTR 1
#define ARGSTYPEVARREF 2
#define ARGSTYPEVARREFSTR 3

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
clearLabelTable() {
	int j;
	for (j = 0; j < SYMTABLESIZE; j++) {
		labeltable[j] = -1;
	}
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



#ifdef __cplusplus
}
#endif

%}

%token B256PRINT B256INPUT B256KEY 
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
%token B256ALERT B256CONFIRM B256PROMPT
%token B256FROMBINARY B256FROMHEX B256FROMOCTAL B256FROMRADIX B256TOBINARY B256TOHEX B256TOOCTAL B256TORADIX
%token B256DEBUGINFO
%token B256CONTINUEDO B256CONTINUEFOR B256CONTINUEWHILE B256EXITDO B256EXITFOR B256EXITWHILE
%token B256PRINTERPAGE B256PRINTERON B256PRINTEROFF B256PRINTERCANCEL
%token B256TRY B256CATCH B256ENDTRY B256LET
%token B256ERROR_NONE B256ERROR_FOR1 B256ERROR_FOR2 B256ERROR_FILENUMBER B256ERROR_FILEOPEN
%token B256ERROR_FILENOTOPEN B256ERROR_FILEWRITE B256ERROR_FILERESET B256ERROR_ARRAYSIZELARGE
%token B256ERROR_ARRAYSIZESMALL B256ERROR_NOSUCHVARIABLE B256ERROR_ARRAYINDEX B256ERROR_STRNEGLEN
%token B256ERROR_STRSTART B256ERROR_NONNUMERIC B256ERROR_RGB B256ERROR_PUTBITFORMAT
%token B256ERROR_POLYARRAY B256ERROR_POLYPOINTS B256ERROR_IMAGEFILE B256ERROR_SPRITENUMBER
%token B256ERROR_SPRITENA B256ERROR_SPRITESLICE B256ERROR_FOLDER B256ERROR_INFINITY B256ERROR_DBOPEN
%token B256ERROR_DBQUERY B256ERROR_DBNOTOPEN B256ERROR_DBCOLNO B256ERROR_DBNOTSET B256ERROR_NETSOCK
%token B256ERROR_NETHOST B256ERROR_NETCONN B256ERROR_NETREAD B256ERROR_NETNONE B256ERROR_NETWRITE
%token B256ERROR_NETSOCKOPT B256ERROR_NETBIND B256ERROR_NETACCEPT B256ERROR_NETSOCKNUMBER
%token B256ERROR_PERMISSION B256ERROR_IMAGESAVETYPE B256ERROR_DIVZERO B256ERROR_BYREF
%token B256ERROR_BYREFTYPE B256ERROR_FREEFILE B256ERROR_FREENET B256ERROR_FREEDB
%token B256ERROR_DBCONNNUMBER B256ERROR_FREEDBSET B256ERROR_DBSETNUMBER B256ERROR_DBNOTSETROW
%token B256ERROR_PENWIDTH B256ERROR_COLORNUMBER B256ERROR_ARRAYINDEXMISSING B256ERROR_IMAGESCALE
%token B256ERROR_FONTSIZE B256ERROR_FONTWEIGHT B256ERROR_RADIXSTRING B256ERROR_RADIX B256ERROR_LOGRANGE
%token B256ERROR_STRINGMAXLEN B256ERROR_NOTANUMBER B256ERROR_PRINTERNOTON B256ERROR_PRINTERNOTOFF
%token B256ERROR_PRINTEROPEN B256ERROR_WAVFILEFORMAT B256ERROR_WAVNOTOPEN
%token B256ERROR_NOTIMPLEMENTED
%token B256WARNING_TYPECONV B256WARNING_WAVNODURATION B256WARNING_WAVNOTSEEKABLE
%token B256REGEXMINIMAL


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
%token <string> B256HEXCONST
%token <string> B256BINCONST
%token <string> B256OCTCONST
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
					labeltable[$1] = wordOffset; 
				}
				;

functionvariable:
			B256VARIABLE {
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
			statement ':' compoundstmt
			| statement
			;
/* array reference - make everything 2d */

arrayref:
			'[' floatexpr ']' {
				addIntOp(OP_PUSHINT, 0);
				// addOp(OP_STACKSWAP);
			}
			| '[' floatexpr ',' floatexpr ']'
			;

/* statement argument paterns */

args_none:
			| '(' ')';
			
/* one argument (only ones that do not have a native or need to setvarnumber) */


args_a:
			B256VARIABLE arrayref {
				varnumber[nvarnumber++] = $1
			}
			;

args_A:
			B256STRINGVAR arrayref {
				varnumber[nvarnumber++] = $1
			}
			;

args_v:
			B256VARIABLE {
				varnumber[nvarnumber++] = $1
			};

args_V:
			B256STRINGVAR {
				varnumber[nvarnumber++] = $1
			};

			
/* two arguments */

			
args_ee:
			expr ',' expr
			| '(' args_ee ')';
			
args_es:
			expr ',' stringexpr
			| '(' args_es ')';
			
args_ff:
			floatexpr ',' floatexpr
			| '(' args_ff ')';
			
args_fi:
			floatexpr ',' immediatelist
			| '(' args_fi ')';
			
args_fs:
			floatexpr ',' stringexpr
			| '(' args_fs ')';
			
args_fv:
			floatexpr ',' B256VARIABLE {
				varnumber[nvarnumber++] = $3;
			}
			| '(' args_fv ')';

	
args_sa:
			stringexpr ',' B256VARIABLE arrayref {
				varnumber[nvarnumber++] = $3
			}
			|'(' args_sa ')';

args_sA:
			stringexpr ',' B256STRINGVAR arrayref {
				varnumber[nvarnumber++] = $3
			}
			|'(' args_sA ')';

args_sf:
			stringexpr ',' floatexpr
			| '(' args_sf ')';
			
args_ss:
			stringexpr ',' stringexpr
			| '(' args_ss ')';
			
args_sv:
			stringexpr ',' B256VARIABLE {
				varnumber[nvarnumber++] = $3
			}
			|'(' args_sv ')';

args_sV:
			stringexpr ',' B256STRINGVAR {
				varnumber[nvarnumber++] = $3
			}
			|'(' args_sV ')';
		
		
/* three arguments */
args_eee:
			expr ',' expr ',' expr
			| '(' args_eee ')';
			
args_eef:
			expr ',' expr ',' floatexpr
			| '(' args_eef ')';

args_fff:
			floatexpr ',' floatexpr ',' floatexpr
			| '(' args_fff ')';

args_ffi:
			floatexpr ',' floatexpr ',' immediatelist
			| '(' args_ffi ')';

args_ffs:
			floatexpr ',' floatexpr ',' stringexpr
			| '(' args_ffs ')';

args_ffv:
			floatexpr ',' floatexpr ',' B256VARIABLE {
				varnumber[nvarnumber++] = $5;
			}
			| '(' args_ffv ')';

args_ffx:
			floatexpr ',' floatexpr ',' expr
			| '(' args_ffx ')';

args_fsf:
			floatexpr ',' stringexpr ',' floatexpr
			| '(' args_fsf ')';

args_sff:
			stringexpr ',' floatexpr ',' floatexpr
			| '(' args_sff ')';

			
/* four arguments */
args_ffff:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_ffff ')';
			
args_fffi:
			floatexpr ',' floatexpr ',' floatexpr ',' immediatelist
			| '(' args_fffi ')';

args_fffv:
			floatexpr ',' floatexpr ',' floatexpr ',' B256VARIABLE {
				varnumber[nvarnumber++] = $7;
			}
			| '(' args_fffv ')';

args_fffs:
			floatexpr ',' floatexpr ',' floatexpr ',' stringexpr
			| '(' args_fffs ')';

args_ffsf:
			floatexpr ',' floatexpr ',' stringexpr ',' floatexpr
			| '(' args_ffsf ')';

args_fsff:
			floatexpr ',' stringexpr ',' floatexpr ',' floatexpr
			| '(' args_fsff ')';


/* five arguments */

args_fffff:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_fffff ')';

args_ffffi:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' immediatelist
			| '(' args_ffffi ')';

args_ffffs:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' stringexpr
			| '(' args_ffffs ')';

args_ffffv:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' B256VARIABLE {
				varnumber[nvarnumber++] = $9
			}
			| '(' args_ffffv ')';

args_fsfff:
			floatexpr ',' stringexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_fsfff ')';

			
/* six arguments */
args_ffffff:
			floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_ffffff ')';
			
args_fsffff:
			floatexpr ',' stringexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_fsffff ')';

/* seven arguments */

args_fsfffff:
			floatexpr ',' stringexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr
			| '(' args_fsfffff ')';


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
			| untilstmt
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
						//
						numifs--;
					}
				}
				//
			}
			;

casestmt:	caseexpr floatexpr {
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
				labeltable[getInternalSymbol(nextifid,INTERNALSYMBOLTOP)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
							labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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

ifstmt:		B256IF floatexpr B256THEN {
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
					labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
					numifs--;
				}
			}
			;
			
ifthenelsestmt:
			ifthenelse compoundstmt {
				//
				// resolve the label on the else to the current location
				labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
				labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset; 
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

untilstmt:	until floatexpr {
				//
				// branch back to top if condition holds
				addIntOp(OP_BRANCH, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLTOP));
				//
				// create label for EXIT DO
				labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
				numifs--;
			}
			;

while: 		B256WHILE {
				//
				// create internal symbol and add to the label table for the top of the loop
				labeltable[getInternalSymbol(nextifid,INTERNALSYMBOLCONTINUE)] = wordOffset; 
			}
			;

whilestmt: 	while floatexpr {
				//
				// add branch to end if false
				addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// add to if frame
				newIf(linenumber, IFTABLETYPEWHILE);
			};

letstmt:	B256LET numassign
			| B256LET stringassign
			| B256LET arrayassign
			| B256LET strarrayassign
			| numassign
			| stringassign
			| arrayassign
			| strarrayassign
			;

dimstmt: 	B256DIM args_a {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_A {
				addIntOp(OP_DIMSTR, varnumber[--nvarnumber]);
			}
			| B256DIM args_v floatexpr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_V floatexpr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_DIMSTR, varnumber[--nvarnumber]);
			}
			| B256DIM args_v args_ff {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM args_V args_ff {
				addIntOp(OP_DIMSTR, varnumber[--nvarnumber]);
			}
			;

redimstmt:	B256REDIM args_a {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_A {
				addIntOp(OP_REDIMSTR, varnumber[--nvarnumber]);
			}
			| B256REDIM args_v floatexpr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_V floatexpr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_REDIMSTR, varnumber[--nvarnumber]);
			}
			| B256REDIM args_v args_ff {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM args_V args_ff {
				addIntOp(OP_REDIMSTR, varnumber[--nvarnumber]);
			}
			;

pausestmt:	B256PAUSE floatexpr {
				addOp(OP_PAUSE);
			}
			;

throwerrorstmt:
			B256THROWERROR floatexpr {
				addOp(OP_THROWERROR);
			}
			;

clearstmt:	B256CLS args_none {
				addOp(OP_CLS);
			}
			| B256CLG args_none {
				addOp(OP_CLG);
			} 
			;

fastgraphicsstmt:
			B256FASTGRAPHICS args_none {
				addOp(OP_FASTGRAPHICS);
			}
			;

graphsizestmt:
			B256GRAPHSIZE args_ff {
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

strarrayassign:
			args_A '=' expr {
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| args_A B256ADDEQUAL expr {
				// a$[b,c] += s$
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCAT);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_V '=' immediatestrlist {
				addIntOp(OP_PUSHINT, listlen);
				listlen = 0;
				addIntOp(OP_STRARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| args_V '=' B256EXPLODE args_ee {
				addIntOp(OP_PUSHINT, 0);	// case sensitive flag
				addIntOp(OP_EXPLODESTR, varnumber[--nvarnumber]);
			}
			| args_V '=' B256EXPLODE args_eef {
				addIntOp(OP_EXPLODESTR, varnumber[--nvarnumber]);
			}
			| args_V'=' B256EXPLODEX args_es {
				addIntOp(OP_EXPLODEXSTR, varnumber[--nvarnumber]);
			}
			;

arrayassignerrors:
			args_a '=' stringexpr
			| args_a B256ADDEQUAL stringexpr
			| args_a B256SUBEQUAL stringexpr
			| args_a B256MULEQUAL stringexpr
			| args_a B256DIVEQUAL stringexpr
			| args_v '=' immediatestrlist
			;

arrayassign:
			args_a '=' floatexpr {
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
			| args_a B256ADDEQUAL floatexpr {
				// a[b,c] += n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_ADD);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256SUBEQUAL floatexpr {
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
			| args_a B256MULEQUAL floatexpr {
				// a[b,c] *= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_DEREF,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_MUL);
				addIntOp(OP_ARRAYASSIGN, varnumber[nvarnumber]);
			}
			| args_a B256DIVEQUAL floatexpr {
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
			| args_v '=' immediatelist {
				addIntOp(OP_PUSHINT, listlen);
				listlen = 0;
				addIntOp(OP_ARRAYLISTASSIGN, varnumber[--nvarnumber]);
			}
			| args_v '=' B256EXPLODE args_ee {
				addIntOp(OP_PUSHINT, 0);	// case sensitive flag
				addIntOp(OP_EXPLODE, varnumber[--nvarnumber]);
			}
			| args_v '=' B256EXPLODE args_eef {
				addIntOp(OP_EXPLODE, varnumber[--nvarnumber]);
			}
			| args_v '=' B256EXPLODEX args_es{
				addIntOp(OP_EXPLODEX, varnumber[--nvarnumber]);
			}
			| arrayassignerrors {
				errorcode = COMPERR_ASSIGNS2N;
				return -1;
			}
			;

numassignerrors:
			args_v '=' stringexpr
			| args_v B256ADDEQUAL stringexpr
			| args_v B256SUBEQUAL stringexpr
			| args_v B256MULEQUAL stringexpr
			| args_v B256DIVEQUAL stringexpr
			;

numassign: 
			args_v '=' floatexpr {
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
			| args_v B256ADDEQUAL floatexpr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_ADD);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256SUBEQUAL floatexpr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_SUB);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256MULEQUAL floatexpr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_MUL);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| args_v B256DIVEQUAL floatexpr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_DIV);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			| numassignerrors {
				errorcode = COMPERR_ASSIGNS2N;
				return -1;
			}
			;

stringassign:
			args_V '=' expr {
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| args_V B256ADDEQUAL expr {
				addIntOp(OP_PUSHVAR,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCAT);
				addIntOp(OP_ASSIGN, varnumber[nvarnumber]);
			}
			;

forstmt: 	B256FOR args_v '=' floatexpr B256TO floatexpr {
				addIntOp(OP_PUSHINT, 1); //step
				addIntOp(OP_FOR, varnumber[--nvarnumber]);
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				newIf(linenumber, IFTABLETYPEFOR);
			}
			| B256FOR args_v '=' floatexpr B256TO floatexpr B256STEP floatexpr {
				addIntOp(OP_FOR, varnumber[--nvarnumber]);
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				newIf(linenumber, IFTABLETYPEFOR);
			}
			;

nextstmt:	B256NEXT args_v {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEFOR) {
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset; 
						addIntOp(OP_NEXT, varnumber[--nvarnumber]);
						labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
			}
			;

callstmt:	B256CALL args_v args_none {
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
			}
			| B256CALL args_v '(' exprlist ')' {
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
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

colorstmt:	B256SETCOLOR args_fff {
				addIntOp(OP_PUSHINT, 255); 
				addOp(OP_RGB);
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
				newParseWarning(COMPWARNING_DEPRECATED_FORM);
			}
			| B256SETCOLOR floatexpr {
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
			}
			| B256SETCOLOR args_ff  {
				addOp(OP_SETCOLOR);
			}
			;

soundstmt:	B256SOUND args_v {
				addIntOp(OP_ARRAY2STACK,varnumber[--nvarnumber]);
				addOp(OP_SOUND_LIST);
			}
			| B256SOUND immediatelist {
				addIntOp(OP_PUSHINT, listlen);
				listlen = 0;
				addOp(OP_SOUND_LIST);
			}
			| B256SOUND args_ff {
				addIntOp(OP_PUSHINT, 2);
				addOp(OP_SOUND_LIST);
			}
			;

plotstmt: 	B256PLOT args_ff {
				addOp(OP_PLOT);
			}
			;

linestmt:	B256LINE args_ffff {
				addOp(OP_LINE);
			}
			;


circlestmt:
			B256CIRCLE args_fff {
				addOp(OP_CIRCLE);
			}
			;

arcstmt: 	B256ARC args_ffffff {
				addOp(OP_ARC);
			}
			;

chordstmt:	
			B256CHORD args_ffffff {
				addOp(OP_CHORD);
			}
			;

piestmt:
			B256PIE args_ffffff {
				addOp(OP_PIE);
			}
			;

rectstmt:
			B256RECT args_ffff {
				addOp(OP_RECT);
			}
			;

textstmt: 
			B256TEXT args_ffx {
				addOp(OP_TEXT);
			}
			;

fontstmt:
			B256FONT args_sff {
				addOp(OP_FONT);
			}
			;

saystmt: 	B256SAY expr {
				addOp(OP_SAY);
			}
			;

systemstmt:
			B256SYSTEM stringexpr {
				addOp(OP_SYSTEM);
			}
			;

volumestmt:
			B256VOLUME floatexpr {
				addOp(OP_VOLUME);
			}
			;

polystmt: 
			B256POLY args_v {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_POLY_LIST);
			}
			| B256POLY immediatelist {
				addIntOp(OP_PUSHINT, listlen);
				listlen=0;
				addOp(OP_POLY_LIST); 
			}
			;

stampstmt: 	B256STAMP args_fffv {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_STAMP_S_LIST);
			}
			| B256STAMP args_fffi {
				addIntOp(OP_PUSHINT, listlen);
				listlen=0;
				addOp(OP_STAMP_S_LIST);
			}
			| B256STAMP args_ffv {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_STAMP_LIST);
			}
			| B256STAMP args_ffi {
				addIntOp(OP_PUSHINT, listlen);
				listlen=0;
				addOp(OP_STAMP_LIST);
			}
			| B256STAMP args_ffffv {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_STAMP_SR_LIST);
			}
			| B256STAMP args_ffffi {
				addIntOp(OP_PUSHINT, listlen);
				listlen=0;
				addOp(OP_STAMP_SR_LIST);
			}
			;

openstmt:	B256OPEN stringexpr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			}
			| B256OPEN args_fs {
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			} 
			| B256OPENB stringexpr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			}
			| B256OPENB args_fs {
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			} 
			| B256OPENSERIAL args_fs {
				addIntOp(OP_PUSHINT, 9600); // baud
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_fsf {
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_fsff {
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_fsfff {
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_fsffff {
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_fsfffff {
				addOp(OP_OPENSERIAL);
			}
			;

writestmt:	B256WRITE stringexpr {
				addIntOp(OP_PUSHINT, 0);  // file number zero
				addOp(OP_STACKSWAP);
				addOp(OP_WRITE);
			}
			| B256WRITE args_fs {
				addOp(OP_WRITE);
			}
			;
	
writelinestmt:
			B256WRITELINE stringexpr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITELINE);
			}
			| B256WRITELINE args_fs {
				addOp(OP_WRITELINE);
			}
			;

writebytestmt:
			B256WRITEBYTE floatexpr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITEBYTE);
			}
			| B256WRITEBYTE args_ff {
				addOp(OP_WRITEBYTE);
			}
			;

closestmt:	B256CLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_CLOSE);
			}
			| B256CLOSE floatexpr {
				addOp(OP_CLOSE);
			}
			;

resetstmt:	B256RESET args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_RESET);
			}
			| B256RESET floatexpr {
				addOp(OP_RESET);
			}
			;

seekstmt:	B256SEEK floatexpr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_SEEK);
			}
			| B256SEEK args_ff {
				addOp(OP_SEEK);
			}
			;

inputstmt:	B256INPUT args_sV {
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_V  {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_INPUT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_sv {
				addOp(OP_INPUT);
				addOp(OP_FLOAT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_v  {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_INPUT);
				addOp(OP_FLOAT);
				addIntOp(OP_ASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_sA {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_A {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_INPUT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_sa {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addOp(OP_INPUT);
				addOp(OP_FLOAT);
				addIntOp(OP_ARRAYASSIGN, varnumber[--nvarnumber]);
			}
			| B256INPUT args_a {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_INPUT);
				addOp(OP_FLOAT);
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
			| B256PRINT expr ';' {
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
			| B256WAVPLAY stringexpr {
				addOp(OP_WAVPLAY);
			}
			;
wavseekstmt:
			B256WAVSEEK floatexpr {
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
			B256PUTSLICE args_ffs  {
				addOp(OP_PUTSLICE);
			}
			| B256PUTSLICE args_ffsf  {
				addOp(OP_PUTSLICEMASK);
			}
			;

imgloadstmt:
			B256IMGLOAD args_ffs
			{
				addIntOp(OP_PUSHINT, 1); // scale
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_fffs
			{
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_ffffs
			{
				addOp(OP_IMGLOAD);
			}
			;

spritedimstmt:
			B256SPRITEDIM floatexpr {
				addOp(OP_SPRITEDIM);
			} 
			;

spriteloadstmt:
			B256SPRITELOAD args_fs  {
				addOp(OP_SPRITELOAD);
			}
			;

spriteslicestmt:
			B256SPRITESLICE args_fffff {
				addOp(OP_SPRITESLICE);
			}
			;

spritepolystmt:
			B256SPRITEPOLY args_fv {
				addIntOp(OP_ARRAY2STACK, varnumber[--nvarnumber]);
				addOp(OP_SPRITEPOLY_LIST);
			}
			| B256SPRITEPOLY args_fi {
				addIntOp(OP_PUSHINT, listlen);
				listlen=0;
				addOp(OP_SPRITEPOLY_LIST);
			}
			;

spriteplacestmt: 
			B256SPRITEPLACE args_fff  
			{
				addIntOp(OP_PUSHINT,1);	// scale
				addIntOp(OP_PUSHINT,0);	// rotate
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_ffff 
			{
				addIntOp(OP_PUSHINT,0);	// rotate
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_fffff
			{
				addOp(OP_SPRITEPLACE);
			}
			;

spritemovestmt:
			B256SPRITEMOVE args_fff
			{
				addIntOp(OP_PUSHINT,0);	// scale (change in scale)
				addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_ffff  {
				addIntOp(OP_PUSHINT,0);	// rotate (change in rotation)
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_fffff {
				addOp(OP_SPRITEMOVE);
			}
			;

spritehidestmt:
			B256SPRITEHIDE floatexpr {
				addOp(OP_SPRITEHIDE);
			} 
			;

spriteshowstmt:
			B256SPRITESHOW floatexpr {
				addOp(OP_SPRITESHOW);
			} 
			;

clickclearstmt:
			B256CLICKCLEAR args_none {
				addOp(OP_CLICKCLEAR);
			}
			;

changedirstmt: 
			B256CHANGEDIR stringexpr {
				addOp(OP_CHANGEDIR);
			}
			;

dbopenstmt:
			B256DBOPEN stringexpr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPEN);
			}
			| B256DBOPEN args_fs {
				addOp(OP_DBOPEN);
			}
			;

dbclosestmt:
			B256DBCLOSE args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_DBCLOSE);
			}
			| B256DBCLOSE floatexpr {
				addOp(OP_DBCLOSE);
			}
			;

dbexecutestmt:
			B256DBEXECUTE stringexpr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBEXECUTE);
			}
			| B256DBEXECUTE args_fs {
				addOp(OP_DBEXECUTE);
			}
			;

dbopensetstmt:
			B256DBOPENSET stringexpr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_fs {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_ffs {
				addOp(OP_DBOPENSET);
			}
			;

dbclosesetstmt:
			B256DBCLOSESET args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET floatexpr {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET args_ff {
				addOp(OP_DBCLOSESET);
			}
			;

netlistenstmt:
			B256NETLISTEN floatexpr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETLISTEN);
			}
			| B256NETLISTEN args_ff {
				addOp(OP_NETLISTEN);
			} 
			;

netconnectstmt: 
			B256NETCONNECT args_sf {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKTOPTO2);
				addOp(OP_NETCONNECT);
			}
			| B256NETCONNECT args_fsf {
				addOp(OP_NETCONNECT);
			}
			;

netwritestmt:
			B256NETWRITE stringexpr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETWRITE);
			}
			| B256NETWRITE args_fs {
				addOp(OP_NETWRITE);
			} 
			;

netclosestmt: 
			B256NETCLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_NETCLOSE);
			}
			| B256NETCLOSE floatexpr {
				addOp(OP_NETCLOSE);
			}
			;

killstmt: 	B256KILL stringexpr {
				addOp(OP_KILL);
			}
			;

setsettingstmt:
			B256SETSETTING args_eee {
				addOp(OP_SETSETTING);
			}
			;

portoutstmt:
			B256PORTOUT args_ff {
				addOp(OP_PORTOUT);
			}
			;

imgsavestmt:
			B256IMGSAVE stringexpr  {
				addStringOp(OP_PUSHSTRING, "PNG");
				addOp(OP_IMGSAVE);
			} 
			| B256IMGSAVE args_ss {
				addOp(OP_IMGSAVE);
			}
			;

editvisiblestmt:
			B256EDITVISIBLE floatexpr {
				addOp(OP_EDITVISIBLE);
			}
			;

graphvisiblestmt:
			B256GRAPHVISIBLE floatexpr {
				addOp(OP_GRAPHVISIBLE);
			}
			;

outputvisiblestmt:
			B256OUTPUTVISIBLE floatexpr {
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
			B256PENWIDTH floatexpr {
				addOp(OP_PENWIDTH);
			}
			;

alertstmt:
			B256ALERT stringexpr {
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
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
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
			B256FUNCTION B256VARIABLE functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the function - add the start to the label table
				functionDefSymbol = $2;
				functionType = FUNCTIONTYPEFLOAT;
				//
				// create jump around function definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this function
				labeltable[functionDefSymbol] = wordOffset;
				newIf(linenumber, IFTABLETYPEFUNCTION);
				//
				// check to see if there are enough values on the stack
				addIntOp(OP_PUSHINT, numargs);
				addOp(OP_ARGUMENTCOUNTTEST);
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						if (argstype[t]==ARGSTYPEINT) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPESTR) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
					}
				}
				//
				// initialize return variable
				addIntOp(OP_PUSHINT, 0);
				addIntOp(OP_ASSIGN, functionDefSymbol);
				//
				numargs=0;	// clear the list for next function
			}
			| B256FUNCTION B256STRINGVAR functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the function - add the start to the label table
				functionDefSymbol = $2;
				functionType = FUNCTIONTYPESTRING;
				//
				// create jump around function definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this function
				labeltable[functionDefSymbol] = wordOffset;
				newIf(linenumber, IFTABLETYPEFUNCTION);
				// 
				// initialize return variable
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_ASSIGN, functionDefSymbol);
				//
				// check to see if there are enough values on the stack
				addIntOp(OP_PUSHINT, numargs);
				addOp(OP_ARGUMENTCOUNTTEST);
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						if (argstype[t]==ARGSTYPEINT) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPESTR) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
					}
				}
				// 
				// initialize return variable
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_ASSIGN, functionDefSymbol);
				//
				numargs=0;	// clear the list for next function
			}
			;

subroutinestmt:
			B256SUBROUTINE B256VARIABLE functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the subroutine - add the start to the label table
				subroutineDefSymbol = $2;
				//
				// create jump around subroutine definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				// 
				// create the new if frame for this subroutine
				labeltable[subroutineDefSymbol] = wordOffset;
				newIf(linenumber, IFTABLETYPEFUNCTION);
				//
				// check to see if there are enough values on the stack
				addIntOp(OP_PUSHINT, numargs);
				addOp(OP_ARGUMENTCOUNTTEST);
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						if (argstype[t]==ARGSTYPEINT) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPESTR) addIntOp(OP_ASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREF) addIntOp(OP_VARREFASSIGN, args[t]);
						if (argstype[t]==ARGSTYPEVARREFSTR) addIntOp(OP_VARREFSTRASSIGN, args[t]);
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
					labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
					labeltable[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset; 
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
			B256REGEXMINIMAL floatexpr {
				addOp(OP_REGEXMINIMAL);
			}
			;




/* ####################################	
   ### INSERT NEW Statements BEFORE ###
   #################################### */


immediatestrlist:
			'{' stringlist '}'
			;

immediatelist:
			'{' floatlist '}'
			;

floatlist:
			floatexpr { listlen = 1; }
			| floatexpr ',' floatlist { listlen++; }
			;

stringlist:
			stringexpr { listlen = 1; }
			| stringexpr ',' stringlist { listlen++; }
			;

exprlist:
			expr { listlen = 1; }
			| expr ',' exprlist {listlen++;}
			;

expr:
			floatexpr
			| stringexpr
			;






floatexpr:
			'(' floatexpr ')' { $$ = $2; }
			| floatexpr '+' floatexpr {
				addOp(OP_ADD);
			}
			| floatexpr '-' floatexpr {
				addOp(OP_SUB);
			}
			| floatexpr '*' floatexpr {
				addOp(OP_MUL);
			}
			| floatexpr B256MOD floatexpr {
				addOp(OP_MOD);
			}
			| floatexpr B256INTDIV floatexpr {
				addOp(OP_INTDIV);
			}
			| floatexpr '/' floatexpr {
				addOp(OP_DIV);
			}
			| floatexpr '^' floatexpr { addOp(OP_EX); }
			| floatexpr B256BINARYOR floatexpr { addOp(OP_BINARYOR); }
			| floatexpr B256BINARYAND floatexpr { addOp(OP_BINARYAND); }
			| B256BINARYNOT floatexpr { addOp(OP_BINARYNOT); }
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
			| args_a {
				addIntOp(OP_DEREF, varnumber[--nvarnumber]);
			}
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
			| B256VARIABLE '(' exprlist ')' {
				// function call with arguments
				addIntOp(OP_GOSUB, $1);
			}
			| B256VARIABLE '(' ')' {
				// function call without arguments
				addIntOp(OP_GOSUB, $1);
			}
			| args_v {
				addIntOp(OP_PUSHVAR, varnumber[--nvarnumber]);
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
			| B256INSTR '(' expr ',' expr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT, 0);	// case sens flag
				addOp(OP_INSTR);
			 }
			| B256INSTR '(' expr ',' expr ',' floatexpr ',' floatexpr')' { addOp(OP_INSTR); }
			| B256INSTRX '(' expr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT, 1);	//start
				addOp(OP_INSTRX);
			}
			| B256INSTRX '(' expr ',' stringexpr ',' floatexpr ')' { addOp(OP_INSTRX); }
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
			| B256RAND args_none { addOp(OP_RAND); }
			| B256PI args_none { addFloatOp(OP_PUSHFLOAT, 3.14159265358979323846); }
			| B256BOOLTRUE args_none { addIntOp(OP_PUSHINT, 1); }
			| B256BOOLFALSE args_none { addIntOp(OP_PUSHINT, 0); }
			| B256BOOLEOF args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_EOF);
			}
			| B256BOOLEOF '(' floatexpr ')' { addOp(OP_EOF); }
			| B256EXISTS '(' stringexpr ')' { addOp(OP_EXISTS); }
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
			| B256SIZE '(' floatexpr ')' { addOp(OP_SIZE); }
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
				addIntOp(OP_PUSHINT, 0xf8);
				addIntOp(OP_PUSHINT, 0xf8);
				addIntOp(OP_PUSHINT, 0xf8);
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
				addIntOp(OP_PUSHINT, 0xFF);
				addIntOp(OP_PUSHINT, 0x33);
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
			| B256PIXEL '(' floatexpr ',' floatexpr ')' { addOp(OP_PIXEL); }
			| B256RGB '(' floatexpr ',' floatexpr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT, 0xff); 
				addOp(OP_RGB);
			}
			| B256RGB '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' {
				addOp(OP_RGB);
			}
			| B256GETCOLOR { addOp(OP_GETCOLOR); }
			| B256GETBRUSHCOLOR { addOp(OP_GETBRUSHCOLOR); }
			| B256GETPENWIDTH { addOp(OP_GETPENWIDTH); }
			| B256SPRITECOLLIDE '(' floatexpr ',' floatexpr ')' { addOp(OP_SPRITECOLLIDE); }
			| B256SPRITEX '(' floatexpr ')' { addOp(OP_SPRITEX); }
			| B256SPRITEY '(' floatexpr ')' { addOp(OP_SPRITEY); }
			| B256SPRITEH '(' floatexpr ')' { addOp(OP_SPRITEH); }
			| B256SPRITEW '(' floatexpr ')' { addOp(OP_SPRITEW); }
			| B256SPRITEV '(' floatexpr ')' { addOp(OP_SPRITEV); }
			| B256SPRITER '(' floatexpr ')' { addOp(OP_SPRITER); }
			| B256SPRITES '(' floatexpr ')' { addOp(OP_SPRITES); }
			| B256DBROW args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBROW);
			}
			| B256DBROW '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBROW);
			}
			| B256DBROW '(' floatexpr ',' floatexpr')' {
				addOp(OP_DBROW);
			}
			| B256DBINT '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINT); }
			| B256DBINT '(' floatexpr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINT); }
			| B256DBINT '(' floatexpr ',' floatexpr ',' floatexpr ')' {
				addOp(OP_DBINT); }
			| B256DBINT '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINTS); }
			| B256DBINT '(' floatexpr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBINTS); }
			| B256DBINT '(' floatexpr ',' floatexpr ',' stringexpr ')' {
				addOp(OP_DBINTS); }
			| B256DBFLOAT '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOAT); }
			| B256DBFLOAT '(' floatexpr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOAT); }
			| B256DBFLOAT '(' floatexpr ',' floatexpr ',' floatexpr ')' {
				addOp(OP_DBFLOAT); }
			| B256DBFLOAT '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOATS); }
			| B256DBFLOAT '(' floatexpr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBFLOATS); }
			| B256DBFLOAT '(' floatexpr ',' floatexpr ',' stringexpr ')' {
				addOp(OP_DBFLOATS); }
			| B256DBNULL '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULL); }
			| B256DBNULL '(' floatexpr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULL); }
			| B256DBNULL '(' floatexpr ',' floatexpr ',' floatexpr ')' {
				addOp(OP_DBNULL); }
			| B256DBNULL '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULLS); }
			| B256DBNULL '(' floatexpr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBNULLS); }
			| B256DBNULL '(' floatexpr ',' floatexpr ',' stringexpr ')' {
				addOp(OP_DBNULLS); }
			| B256LASTERROR args_none { addOp(OP_LASTERROR); }
			| B256LASTERRORLINE args_none { addOp(OP_LASTERRORLINE); }
			| B256NETDATA args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETDATA); }
			| B256NETDATA '(' floatexpr ')' { addOp(OP_NETDATA); }
			| B256PORTIN '(' floatexpr ')' { addOp(OP_PORTIN); }
			| B256COUNT '(' stringexpr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT, 0); // case sens flag
				addOp(OP_COUNT);
			 }
			| B256COUNT '(' stringexpr ',' stringexpr ',' floatexpr ')' { addOp(OP_COUNT); }
			| B256COUNTX '(' stringexpr ',' stringexpr ')' { addOp(OP_COUNTX); }
			| B256OSTYPE args_none { addOp(OP_OSTYPE); }
			| B256MSEC args_none { addOp(OP_MSEC); }
			| B256TEXTWIDTH '(' stringexpr ')' { addOp(OP_TEXTWIDTH); }
			| B256TEXTHEIGHT args_none { addOp(OP_TEXTHEIGHT); }
			| B256READBYTE args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READBYTE); }
			| B256READBYTE '(' floatexpr ')' { addOp(OP_READBYTE); }
			| B256REF '(' B256VARIABLE ')' { addIntOp(OP_PUSHVARREF, $3); }
			| B256REF '(' B256STRINGVAR ')' { addIntOp(OP_PUSHVARREF, $3); }
			| B256FREEDB args_none { addOp(OP_FREEDB); }
			| B256FREEDBSET args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_FREEDBSET);
			}
			| B256FREEDBSET '(' floatexpr ')' { addOp(OP_FREEDBSET); }
			| B256FREEFILE args_none { addOp(OP_FREEFILE); }
			| B256FREENET args_none { addOp(OP_FREENET); }
			| B256VERSION args_none { addIntOp(OP_PUSHINT, VERSIONSIGNATURE); }
			| B256CONFIRM '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,-1);	// no default
				addOp(OP_CONFIRM);
			}
			| B256CONFIRM '(' stringexpr ',' floatexpr ')' {
				addOp(OP_CONFIRM);
			}
			| B256FROMBINARY '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,2);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMHEX '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,16);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMOCTAL '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,8);	// radix
				addOp(OP_FROMRADIX);
			}
			| B256FROMRADIX '(' stringexpr ',' floatexpr ')' {
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
			| B256ERROR_FOR1 args_none { addIntOp(OP_PUSHINT, ERROR_FOR1); }
			| B256ERROR_FOR2 args_none { addIntOp(OP_PUSHINT, ERROR_FOR2); }
			| B256ERROR_FILENUMBER args_none { addIntOp(OP_PUSHINT, ERROR_FILENUMBER); }
			| B256ERROR_FILEOPEN args_none { addIntOp(OP_PUSHINT, ERROR_FILEOPEN); }
			| B256ERROR_FILENOTOPEN args_none { addIntOp(OP_PUSHINT, ERROR_FILENOTOPEN); }
			| B256ERROR_FILEWRITE args_none { addIntOp(OP_PUSHINT, ERROR_FILEWRITE); }
			| B256ERROR_FILERESET args_none { addIntOp(OP_PUSHINT, ERROR_FILERESET); }
			| B256ERROR_ARRAYSIZELARGE args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYSIZELARGE); }
			| B256ERROR_ARRAYSIZESMALL args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYSIZESMALL); }
			| B256ERROR_NOSUCHVARIABLE args_none { addIntOp(OP_PUSHINT, ERROR_NOSUCHVARIABLE); }
			| B256ERROR_ARRAYINDEX args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYINDEX); }
			| B256ERROR_STRNEGLEN args_none { addIntOp(OP_PUSHINT, ERROR_STRNEGLEN); }
			| B256ERROR_STRSTART args_none { addIntOp(OP_PUSHINT, ERROR_STRSTART); }
			| B256ERROR_NONNUMERIC args_none { addIntOp(OP_PUSHINT, ERROR_NONNUMERIC); }
			| B256ERROR_RGB args_none { addIntOp(OP_PUSHINT, ERROR_RGB); }
			| B256ERROR_PUTBITFORMAT args_none { addIntOp(OP_PUSHINT, ERROR_PUTBITFORMAT); }
			| B256ERROR_POLYARRAY args_none { addIntOp(OP_PUSHINT, ERROR_POLYARRAY); }
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
			| B256ERROR_BYREFTYPE args_none { addIntOp(OP_PUSHINT, ERROR_BYREFTYPE); }
			| B256ERROR_FREEFILE args_none { addIntOp(OP_PUSHINT, ERROR_FREEFILE); }
			| B256ERROR_FREENET args_none { addIntOp(OP_PUSHINT, ERROR_FREENET); }
			| B256ERROR_FREEDB args_none { addIntOp(OP_PUSHINT, ERROR_FREEDB); }
			| B256ERROR_DBCONNNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_DBCONNNUMBER); }
			| B256ERROR_FREEDBSET args_none { addIntOp(OP_PUSHINT, ERROR_FREEDBSET); }
			| B256ERROR_DBSETNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_DBSETNUMBER); }
			| B256ERROR_DBNOTSETROW args_none { addIntOp(OP_PUSHINT, ERROR_DBNOTSETROW); }
			| B256ERROR_PENWIDTH args_none { addIntOp(OP_PUSHINT, ERROR_PENWIDTH); }
			| B256ERROR_COLORNUMBER args_none { addIntOp(OP_PUSHINT, ERROR_COLORNUMBER); }
			| B256ERROR_ARRAYINDEXMISSING args_none { addIntOp(OP_PUSHINT, ERROR_ARRAYINDEXMISSING); }
			| B256ERROR_IMAGESCALE args_none { addIntOp(OP_PUSHINT, ERROR_IMAGESCALE); }
			| B256ERROR_FONTSIZE args_none { addIntOp(OP_PUSHINT, ERROR_FONTSIZE); }
			| B256ERROR_FONTWEIGHT args_none { addIntOp(OP_PUSHINT, ERROR_FONTWEIGHT); }
			| B256ERROR_RADIXSTRING args_none { addIntOp(OP_PUSHINT, ERROR_RADIXSTRING); }
			| B256ERROR_RADIX args_none { addIntOp(OP_PUSHINT, ERROR_RADIX); }
			| B256ERROR_LOGRANGE args_none { addIntOp(OP_PUSHINT, ERROR_LOGRANGE); }
			| B256ERROR_STRINGMAXLEN args_none { addIntOp(OP_PUSHINT, ERROR_STRINGMAXLEN); }
			| B256ERROR_NOTANUMBER args_none { addIntOp(OP_PUSHINT, ERROR_NOTANUMBER); }
			| B256ERROR_PRINTERNOTON args_none { addIntOp(OP_PUSHINT, ERROR_PRINTERNOTON); }
			| B256ERROR_PRINTERNOTOFF args_none { addIntOp(OP_PUSHINT, ERROR_PRINTERNOTOFF); }
			| B256ERROR_PRINTEROPEN args_none { addIntOp(OP_PUSHINT, ERROR_PRINTEROPEN); }
			| B256ERROR_WAVFILEFORMAT args_none { addIntOp(OP_PUSHINT, ERROR_WAVFILEFORMAT); }
			| B256ERROR_WAVNOTOPEN args_none { addIntOp(OP_PUSHINT, ERROR_WAVNOTOPEN); }
			| B256ERROR_NOTIMPLEMENTED args_none { addIntOp(OP_PUSHINT, ERROR_NOTIMPLEMENTED); }

			| B256WARNING_TYPECONV args_none { addIntOp(OP_PUSHINT, WARNING_TYPECONV); }
			| B256WARNING_WAVNODURATION args_none { addIntOp(OP_PUSHINT, WARNING_WAVNODURATION); }
			| B256WARNING_WAVNOTSEEKABLE args_none { addIntOp(OP_PUSHINT, WARNING_WAVNOTSEEKABLE); }

			/* ###########################################
			   ### INSERT NEW Numeric Functions BEFORE ###
			   ########################################### */
			;


stringexpr: 
			'(' stringexpr ')' { $$ = $2; }
			| stringexpr '+' stringexpr { addOp(OP_CONCAT); }
			| floatexpr '+' stringexpr { addOp(OP_CONCAT); }
			| stringexpr '+' floatexpr { addOp(OP_CONCAT); }
			| B256STRING { addStringOp(OP_PUSHSTRING, $1); }
			| args_A { addIntOp(OP_DEREF, varnumber[--nvarnumber]); }
			| B256STRINGVAR '(' exprlist ')' {
				addIntOp(OP_GOSUB, $1);
			}
			| B256STRINGVAR '(' ')' {
				addIntOp(OP_GOSUB, $1);
			}
			| args_V {
				addIntOp(OP_PUSHVAR, varnumber[--nvarnumber]);
			}
			| B256CHR '(' floatexpr ')' { addOp(OP_CHR); }
			| B256TOSTRING '(' expr ')' { addOp(OP_STRING); }
			| B256UPPER '(' expr ')' { addOp(OP_UPPER); }
			| B256LOWER '(' expr ')' { addOp(OP_LOWER); }
			| B256MID '(' expr ',' floatexpr ',' floatexpr ')' { addOp(OP_MID); }
			| B256MIDX '(' expr ',' stringexpr ')' { addIntOp(OP_PUSHINT, 1); addOp(OP_MIDX); }
			| B256MIDX '(' expr ',' stringexpr ',' floatexpr ')' { addOp(OP_MIDX); }
			| B256LEFT '(' expr ',' floatexpr ')' { addOp(OP_LEFT); }
			| B256RIGHT '(' expr ',' floatexpr ')' { addOp(OP_RIGHT); }
			| B256GETSLICE '(' floatexpr ',' floatexpr ',' floatexpr ',' floatexpr ')' { addOp(OP_GETSLICE); }
			| B256READ { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
			| B256READ '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
			| B256READ '(' floatexpr ')' { addOp(OP_READ); }
			| B256READLINE { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
			| B256READLINE '(' ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
			| B256READLINE '(' floatexpr ')' { addOp(OP_READLINE); }
			| B256CURRENTDIR { addOp(OP_CURRENTDIR); }
			| B256CURRENTDIR '(' ')' { addOp(OP_CURRENTDIR); }
			| B256DBSTRING '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRING); }
			| B256DBSTRING '(' floatexpr ',' floatexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRING); }
			| B256DBSTRING '(' floatexpr ',' floatexpr ',' floatexpr ')' {
				addOp(OP_DBSTRING); }
			| B256DBSTRING '(' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRINGS); }
			| B256DBSTRING '(' floatexpr ',' stringexpr ')' {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBSTRINGS); }
			| B256DBSTRING '(' floatexpr ',' floatexpr ',' stringexpr ')' {
				addOp(OP_DBSTRINGS); }
			| B256LASTERRORMESSAGE args_none { addOp(OP_LASTERRORMESSAGE); }
			| B256LASTERROREXTRA args_none { addOp(OP_LASTERROREXTRA); }
			| B256NETREAD  args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETREAD); }
			| B256NETREAD '(' floatexpr ')' { addOp(OP_NETREAD); }
			| B256NETADDRESS args_none { addOp(OP_NETADDRESS); }
			| B256MD5 '(' stringexpr ')' { addOp(OP_MD5); }
			| B256GETSETTING '(' expr ',' expr ')' { addOp(OP_GETSETTING); }
			| B256DIR '(' stringexpr ')' { addOp(OP_DIR); }
			| B256DIR args_none { addStringOp(OP_PUSHSTRING, ""); addOp(OP_DIR); }
			| B256REPLACE '(' expr ',' expr ',' expr ')' {
				addIntOp(OP_PUSHINT, 0);	// case sens flag
				addOp(OP_REPLACE);
			}
			| B256REPLACE '(' expr ',' expr ',' expr ',' floatexpr ')' { addOp(OP_REPLACE); }
			| B256REPLACEX '(' expr ',' stringexpr ',' expr ')' { addOp(OP_REPLACEX); }
			| B256IMPLODE '(' B256STRINGVAR ')' {
				addStringOp(OP_PUSHSTRING, ""); // no delimiter
				addIntOp(OP_IMPLODE, $3);
			}
			| B256IMPLODE '(' B256STRINGVAR ',' stringexpr ')' {  addIntOp(OP_IMPLODE, $3); }
			| B256IMPLODE '(' B256VARIABLE ')' {
				addStringOp(OP_PUSHSTRING, ""); // no delimiter
				addIntOp(OP_IMPLODE, $3);
			}
			| B256IMPLODE '(' B256VARIABLE ',' stringexpr ')' {  addIntOp(OP_IMPLODE, $3); }
			| B256PROMPT '(' stringexpr ')' {
				addStringOp(OP_PUSHSTRING, "");	
				addOp(OP_PROMPT); }
			| B256PROMPT '(' stringexpr ',' stringexpr ')' {
				addOp(OP_PROMPT); }
			| B256TOBINARY '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,2);	// radix
				addOp(OP_TORADIX);
			}
			| B256TOHEX '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,16);	// radix
				addOp(OP_TORADIX);
			}
			| B256TOOCTAL '(' floatexpr ')' {
				addIntOp(OP_PUSHINT,8);	// radix
				addOp(OP_TORADIX);
			}
			| B256TORADIX '(' floatexpr ',' floatexpr ')' {
				addOp(OP_TORADIX);
			}
			| B256DEBUGINFO '(' floatexpr ')' {
				addOp(OP_DEBUGINFO);
			}


			/* ##########################################
			   ### INSERT NEW String Functions BEFORE ###
			   ########################################## */
			;

%%

int
yyerror(const char *msg) {
	errorcode = COMPERR_SYNTAX;
	return -1;
}
