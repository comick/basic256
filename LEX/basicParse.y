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

    #define SYMTABLESIZE 2000

    extern int yylex();
    int yyerror(const char *);
    int errorcode;
    extern int linenumber;

    char *byteCode = NULL;
    unsigned int byteOffset = 0;
    unsigned int oldByteOffset = 0;

    struct label 
    {
      char *name;
      int offset;
    };

    unsigned int  *branchAddr = NULL;

    char *EMPTYSTR = "";
    char *symtable[SYMTABLESIZE];
    int labeltable[SYMTABLESIZE];
    int numsyms = 0;
    int numlabels = 0;
    int maxbyteoffset = 0;

    int
    basicParse(char *);

    void 
    clearLabelTable()
    {
      int j;
      for (j = 0; j < SYMTABLESIZE; j++)
	{
	  labeltable[j] = -1;
	}
    }

    void
    clearSymbolTable()
    {
      int j;
      if (numsyms == 0)
	{
	  for (j = 0; j < SYMTABLESIZE; j++)
	    {
	      symtable[j] = 0;
	    }
	}
      for (j = 0; j < numsyms; j++)
	{
	  if (symtable[j])
	    {
	      free(symtable[j]);
	    }
	  symtable[j] = 0;
	}
      numsyms = 0;
    }

    int 
    getSymbol(char *name)
    {
      int i;
      for (i = 0; i < numsyms; i++)
	{
	  if (symtable[i] && !strcmp(name, symtable[i]))
	    return i;
	}
      return -1;
    }

    int
    newSymbol(char *name) 
    {
      symtable[numsyms] = name;
      numsyms++;
      return numsyms - 1;
    }


    int
    newByteCode(unsigned int size) 
    {
      if (byteCode)
	{
	  free(byteCode);
	}
      maxbyteoffset = size + 64;
      byteCode = malloc(maxbyteoffset);
      memset(byteCode, 0, maxbyteoffset);
      

      if (byteCode)
	{
	  byteOffset = 0;
	  return 0;
	}
      
      return -1;
    }
    
    void 
    checkByteMem(int addedbytes)
    {
      if (byteOffset + addedbytes + 1 >= maxbyteoffset)
	{
	  maxbyteoffset += maxbyteoffset;
	  byteCode = realloc(byteCode, maxbyteoffset);
	  memset(byteCode + byteOffset, 0, maxbyteoffset);
	}
    }

    void
    addOp(char op)
    {
      checkByteMem(sizeof(char));
      byteCode[byteOffset] = op;
      byteOffset++;
    }

    void 
    addIntOp(char op, int data)
    {
      checkByteMem(sizeof(char) + sizeof(int));
      int *temp;
      addOp(op);
      
      temp = (void *) byteCode + byteOffset;
      *temp = data;
      byteOffset += sizeof(int);
    }

    void 
    addInt2Op(char op, int data1, int data2)
    {
      checkByteMem(sizeof(char) + 2 * sizeof(int));
      int *temp;
      addOp(op);
      
      temp = (void *) byteCode + byteOffset;
      temp[0] = data1;
      temp[1] = data2;
      byteOffset += 2 * sizeof(int);
    }

    void 
    addFloatOp(char op, double data)
    {
      checkByteMem(sizeof(char) + sizeof(double));
      double *temp;
      addOp(op);
      
      temp = (void *) byteCode + byteOffset;
      *temp = data;
      byteOffset += sizeof(double);
    }

    void 
    addStringOp(char op, char *data)
    {
      int len = strlen(data) + 1;
      checkByteMem(sizeof(char) + len);
      double *temp;
      addOp(op);
      
      temp = (void *) byteCode + byteOffset;
      strncpy((char *) byteCode + byteOffset, data, len);
      byteOffset += len;
    }


#ifdef __cplusplus
  }
#endif

%}

%token PRINT INPUT KEY 
%token PLOT CIRCLE RECT LINE FASTGRAPHICS REFRESH CLS CLG
%token IF THEN FOR TO STEP NEXT 
%token GOTO GOSUB RETURN REM END SETCOLOR
%token GTE LTE NE
%token DIM NOP LABEL
%token TOINT TOSTRING CEIL FLOOR RAND SIN COS TAN ABS PI
%token AND OR XOR NOT
%token PAUSE

%union 
{
  int number;
  double floatnum;
  char *string;
}

%token <number> LINENUM
%token <number> INTEGER
%token <floatnum> FLOAT 
%token <string> STRING
%token <number> VARIABLE
%token <number> STRINGVAR
%token <string> NEWVAR
%token <number> COLOR
%token <number> LABEL

%type <floatnum> floatexpr
%type <string> stringexpr

%left '-'
%left '+'
%left '*'
%left '/'
%left '^'
%left AND 
%left OR 
%left XOR 
%right '='
%nonassoc UMINUS


%%


program: validline '\n'         { addOp(OP_END); }
       | validline '\n' program { addOp(OP_END); }
;

validline: ifstmt       { addIntOp(OP_CURRLINE, linenumber); }
         | compoundstmt { addIntOp(OP_CURRLINE, linenumber); }
         | /*empty*/    { addIntOp(OP_CURRLINE, linenumber); }
         | LABEL        { labeltable[$1] = byteOffset; addIntOp(OP_CURRLINE, linenumber + 1); }
;

ifstmt: ifexpr THEN compoundstmt 
        { 
	  if (branchAddr) 
	    { 
	      *branchAddr = byteOffset; 
	      branchAddr = NULL; 
	    } 
	}
;

compoundstmt: statement | compoundstmt ':' statement
;

statement: gotostmt
         | gosubstmt
         | returnstmt
         | printstmt
         | plotstmt
         | circlestmt
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
         | dimstmt
         | pausestmt
         | arrayassign
         | strarrayassign
;

dimstmt: DIM VARIABLE '(' floatexpr ')'  { addIntOp(OP_DIM, $2); }
       | DIM STRINGVAR '(' floatexpr ')' { addIntOp(OP_DIMSTR, $2); }
;

pausestmt: PAUSE floatexpr { addOp(OP_PAUSE); }
;

clearstmt: CLS { addOp(OP_CLS); }
         | CLG { addOp(OP_CLG); } 
;

fastgraphicsstmt: FASTGRAPHICS { addOp(OP_FASTGRAPHICS); }
;

refreshstmt: REFRESH { addOp(OP_REFRESH); }
;

endstmt: END { addOp(OP_END); }
;

ifexpr: IF compoundboolexpr 
         { 
	   //if true, don't branch. If false, go to next line.
	   addOp(OP_BRANCH);
	   checkByteMem(sizeof(int));
	   branchAddr = (void *) byteCode + byteOffset;
	   byteOffset += sizeof(int);
         }
;


compoundboolexpr: boolexpr
                | compoundboolexpr AND compoundboolexpr {addOp(OP_AND); }
                | compoundboolexpr OR compoundboolexpr { addOp(OP_OR); }
                | compoundboolexpr XOR compoundboolexpr { addOp(OP_XOR); }
                | NOT compoundboolexpr %prec UMINUS { addOp(OP_NOT); }
                | '(' compoundboolexpr ')'
;

boolexpr: stringexpr '=' stringexpr  { addOp(OP_EQUAL); } 
        | stringexpr NE stringexpr   { addOp(OP_NEQUAL); }
        | floatexpr '=' floatexpr    { addOp(OP_EQUAL); }
        | floatexpr NE floatexpr     { addOp(OP_NEQUAL); }
        | floatexpr '<' floatexpr    { addOp(OP_LT); }
        | floatexpr '>' floatexpr    { addOp(OP_GT); }
        | floatexpr GTE floatexpr    { addOp(OP_GTE); }
        | floatexpr LTE floatexpr    { addOp(OP_LTE); }
;

strarrayassign: STRINGVAR '[' floatexpr ']' '=' stringexpr { addIntOp(OP_STRARRAYASSIGN, $1); }
;

arrayassign: VARIABLE '[' floatexpr ']' '=' floatexpr { addIntOp(OP_ARRAYASSIGN, $1); }
;

numassign: VARIABLE '=' floatexpr { addIntOp(OP_NUMASSIGN, $1); }
;

stringassign: STRINGVAR '=' stringexpr { addIntOp(OP_STRINGASSIGN, $1); }
;

forstmt: FOR VARIABLE '=' floatexpr TO floatexpr 
          { 
	    addIntOp(OP_PUSHINT, 1); //step
	    addIntOp(OP_FOR, $2);
	  }
       | FOR VARIABLE '=' floatexpr TO floatexpr STEP floatexpr
          { 
	    addIntOp(OP_FOR, $2);
	  }
;


nextstmt: NEXT VARIABLE { addIntOp(OP_NEXT, $2); }
;

gotostmt: GOTO VARIABLE     { addIntOp(OP_GOTO, $2); }
;

gosubstmt: GOSUB VARIABLE   { addIntOp(OP_GOSUB, $2); }
;

returnstmt: RETURN          { addOp(OP_RETURN); }
;

colorstmt: SETCOLOR COLOR   { addIntOp(OP_SETCOLOR, $2); }
;

plotstmt: PLOT floatexpr ',' floatexpr { addOp(OP_PLOT); }
;

linestmt: LINE floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addOp(OP_LINE); }
;


circlestmt: CIRCLE floatexpr ',' floatexpr ',' floatexpr { addOp(OP_CIRCLE); }
;

circlestmt: RECT floatexpr ',' floatexpr ',' floatexpr ',' floatexpr { addOp(OP_RECT); }
;

inputstmt: inputexpr ',' STRINGVAR  { addIntOp(OP_STRINGASSIGN, $3); }
         | inputexpr ',' STRINGVAR '[' floatexpr ']'  { addIntOp(OP_STRARRAYINPUT, $3); }
;

inputexpr: INPUT stringexpr { addOp(OP_PRINT);  addOp(OP_INPUT); }
;

printstmt: PRINT stringexpr { addOp(OP_PRINTN); }
         | PRINT floatexpr  { addOp(OP_PRINTN); }
         | PRINT stringexpr ';' { addOp(OP_PRINT); }
         | PRINT floatexpr  ';' { addOp(OP_PRINT); }
;

floatexpr: '(' floatexpr ')' { $$ = $2; }
         | floatexpr '+' floatexpr { addOp(OP_ADD); }
         | floatexpr '-' floatexpr { addOp(OP_SUB); }
         | floatexpr '*' floatexpr { addOp(OP_MUL); }
         | floatexpr '/' floatexpr { addOp(OP_DIV); }
         | floatexpr '^' floatexpr { addOp(OP_EXP); }
         | '-' FLOAT %prec UMINUS  { addFloatOp(OP_PUSHFLOAT, -$2); }
         | '-' INTEGER %prec UMINUS { addIntOp(OP_PUSHINT, -$2); }
         | '-' VARIABLE %prec UMINUS 
           { 
	     if ($2 < 0)
	       {
		 return -1;
	       }
	     else
	       {
		 addIntOp(OP_PUSHVAR, $2);
		 addOp(OP_NEGATE);
	       }
	   }
         | FLOAT   { addFloatOp(OP_PUSHFLOAT, $1); }
         | INTEGER { addIntOp(OP_PUSHINT, $1); }
         | KEY     { addOp(OP_KEY); }
         | VARIABLE '[' floatexpr ']' { addIntOp(OP_DEREF, $1); }
         | VARIABLE 
           { 
	     if ($1 < 0)
	       {
		 return -1;
	       }
	     else
	       {
		 addIntOp(OP_PUSHVAR, $1);
	       }
	   }
         | TOINT '(' floatexpr ')' { addOp(OP_INT); }
         | TOINT '(' stringexpr ')' { addOp(OP_INT); }
         | CEIL '(' floatexpr ')' { addOp(OP_CEIL); }
         | FLOOR '(' floatexpr ')' { addOp(OP_FLOOR); }
         | SIN '(' floatexpr ')' { addOp(OP_SIN); }
         | COS '(' floatexpr ')' { addOp(OP_COS); }
         | TAN '(' floatexpr ')' { addOp(OP_TAN); }
         | ABS '(' floatexpr ')' { addOp(OP_ABS); }
         | RAND { addOp(OP_RAND); }
         | PI { addFloatOp(OP_PUSHFLOAT, 3.14159265); }
;

stringexpr: stringexpr '+' stringexpr     { addOp(OP_CONCAT); }
          | STRING    { addStringOp(OP_PUSHSTRING, $1); }
          | STRINGVAR '[' floatexpr ']' { addIntOp(OP_DEREF, $1); }
          | STRINGVAR 
            { 
	      if ($1 < 0)
		{
		  return -1;
		}
	      else
		{
		  addIntOp(OP_PUSHVAR, $1);
		}
	    }
          | TOSTRING '(' floatexpr ')' { addOp(OP_STRING); }
;


%%

int
yyerror(const char *msg) {
  errorcode = -1;
  return -1;
}

