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
#include "basicParse.tab.h"

#ifdef __cplusplus
  extern "C" {
#endif

    #include <string.h>

    struct symbol 
    {
      char *name;
      int type;
      union 
      {
	double floatnum;
	char *string;
      } value;
    };

    extern struct symbol symtable[];
    extern void clearSymbolTable();
    extern void clearLabelTable();
    extern void newLineOffsets(int);

    extern int getSymbol(char *);
    extern int newSymbol(char *);
    extern int yyparse();
    extern int errorcode;
    extern int byteOffset;
    int linenumber;

#ifdef __cplusplus
  }
#endif

%}


whitespace [ \t]+
nws [^ \t\n]+
print [Pp][Rr][Ii][Nn][Tt]
goto  [Gg][Oo][Tt][Oo]
gosub [Gg][Oo][Ss][Uu][Bb]
input [Ii][Nn][Pp][Uu][Tt]
key   [Kk][Ee][Yy]
return [Rr][Ee][Tt][Uu][Rr][Nn]
if    [Ii][Ff]
then  [Tt][Hh][Ee][Nn]
rem   [Rr][Ee][Mm].*$
dim   [Dd][Ii][Mm]
end   [Ee][Nn][Dd]
for   [Ff][Oo][Rr]
to    [Tt][Oo]
next  [Nn][Ee][Xx][Tt]
step  [Ss][Tt][Ee][Pp]
plot  [Pp][Ll][Oo][Tt]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
rect  [Rr][Ee][Cc][Tt]
poly  [Pp][Oo][Ll][Yy]
line  [Ll][Ii][Nn][Ee]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
refresh [Rr][Ee][Ff][Rr][Ee][Ss][Hh]
cls   [Cc][Ll][Ss]
clg   [Cc][Ll][Gg]
color [Cc][Oo][Ll][Oo][Rr]
colour [Cc][Oo][Ll][Oo][Uu][Rr]
clear [Cc][Ll][Ee][Aa][Rr]
black [Bb][Ll][Aa][Cc][Kk]
white [Ww][Hh][Ii][Tt][Ee]
red   [Rr][Ee][Dd]
green [Gg][Rr][Ee][Ee][Nn]
blue  [Bb][Ll][Uu][Ee]
cyan  [Cc][Yy][Aa][Nn]
purple [Pp][Uu][Rr][Pp][Ll][Ee]
yellow [Yy][Ee][Ll][Ll][Oo][Ww]
orange [Oo][Rr][Aa][Nn][Gg][Ee]
gray  [Gg][Rr][AaEe][Yy]
dark  [Dd][Aa][Rr][Kk]
toint   [Ii][Nn][Tt]
tostring [Ss][Tt][Rr][Ii][Nn][Gg]
length  [Ll][Ee][Nn][Gg][Tt][Hh]
ceil    [Cc][Ee][Ii][Ll]
floor   [Ff][Ll][Oo][Oo][Rr]
abs     [Aa][Bb][Ss]
sin     [Ss][Ii][Nn]
cos     [Cc][Oo][Ss]
tan     [Tt][Aa][Nn]
rand    [Rr][Aa][Nn][Dd]
pi      [Pp][Ii]
and [Aa][Nn][Dd]
or  [Oo][Rr]
xor [Xx][Oo][Rr]
not [Nn][Oo][Tt]
pause [Pp][Aa][Uu][Ss][Ee]
integer [0-9]+
floatnum [0-9]*\.?[0-9]*
string \"[^\"\n]*\"
variable [a-zA-Z][a-zA-Z0-9]*
stringvar [a-zA-Z][a-zA-Z0-9]*\$
label     {variable}:
blankline ^{whitespace}*[\n]

%%


{label} { 
          char *temp, *c;
          temp = c = strdup(yytext);
          while (*c != ':') c++;
          *c = 0;
          int num = getSymbol(temp);
          if (num < 0)
            {
              yylval.number = newSymbol(temp); //Doesn't conflict, because label only resolved after branch
            }
          else 
            {
              free(temp);
              yylval.number = num;
            }
          return LABEL; 
        }
{integer}       { yylval.number = atoi(yytext); return INTEGER; }
{floatnum}      { yylval.floatnum = atof(yytext); return FLOAT; }
{string}        { int len = strlen(yytext); 
                  yylval.string = strdup(yytext + 1); 
                  yylval.string[len - 2] = 0; 
                  return STRING; }
{toint}         { return TOINT; }
{tostring}      { return TOSTRING; }
{length}        { return LENGTH; }
{ceil}          { return CEIL; }
{floor}         { return FLOOR; }
{abs}           { return ABS; }
{sin}           { return SIN; }
{cos}           { return COS; }
{tan}           { return TAN; }
{rand}          { return RAND; }
{pi}            { return PI; }
{and}           { return AND; }
{or}            { return OR; }
{xor}           { return XOR; }
{not}           { return NOT; }
{print}		{ return PRINT; }
{dim}		{ return DIM; }
{cls}		{ return CLS; }
{clg}		{ return CLG; }
{plot}		{ return PLOT; }
{circle}	{ return CIRCLE; }
{rect}	        { return RECT; }
{poly}		{ return POLY; }
{line}	        { return LINE; }
{fastgraphics}	{ return FASTGRAPHICS; }
{refresh}	{ return REFRESH; }
{color}		{ return SETCOLOR; }
{colour}	{ return SETCOLOR; }
{clear}		{ yylval.number = 0; return COLOR; }
{white}		{ yylval.number = 1; return COLOR; }
{black}		{ yylval.number = 2; return COLOR; }
{red}		{ yylval.number = 3; return COLOR; }
{dark}{red}	{ yylval.number = 4; return COLOR; }
{green}		{ yylval.number = 5; return COLOR; }
{dark}{green}	{ yylval.number = 6; return COLOR; }
{blue}		{ yylval.number = 7; return COLOR; }
{dark}{blue}	{ yylval.number = 8; return COLOR; }
{cyan}		{ yylval.number = 9; return COLOR; }
{dark}{cyan}	{ yylval.number = 10; return COLOR; }
{purple}	{ yylval.number = 11; return COLOR; }
{dark}{purple}	{ yylval.number = 12; return COLOR; }
{yellow}	{ yylval.number = 13; return COLOR; }
{dark}{yellow}	{ yylval.number = 14; return COLOR; }
{orange}	{ yylval.number = 15; return COLOR; }
{dark}{orange}	{ yylval.number = 16; return COLOR; }
{gray}	        { yylval.number = 17; return COLOR; }
{dark}{gray}	{ yylval.number = 18; return COLOR; }
{goto}		{ return GOTO; }
{if}		{ return IF; }
{then}		{ return THEN; }
{for}		{ return FOR; }
{to}		{ return TO; }
{step}		{ return STEP; }
{next}		{ return NEXT; }
{input}         { return INPUT; }
{key}           { return KEY; }
{gosub}         { return GOSUB; }
{return}        { return RETURN; }
{pause}         { return PAUSE; }
{rem}           { return '\n'; }
{end}           { return END; }
">="            { return GTE; }
"<="            { return LTE; }
"<>"            { return NE; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"^"             { return '^'; }
"="             { return '='; }
"<"             { return '<'; }
">"             { return '>'; }
","             { return ','; }
";"             { return ';'; }
":"             { return ':'; }
"("             { return '('; }
")"             { return ')'; }
"["             { return '['; }
"]"             { return ']'; }
"\n"            { linenumber++; return '\n'; }
{variable}      { 
                  int num = getSymbol(yytext);
                  if (num < 0)
                   {
                     yylval.number = newSymbol(strdup(yytext));
                   }
                  else 
                   {
                     yylval.number = num;
                   }
                  return VARIABLE;
                }
{stringvar}     { 
                  int num = getSymbol(yytext);
                  if (num < 0)
                   {
                     yylval.number = newSymbol(strdup(yytext));
                   }
                  else 
                   {
                     yylval.number = num;
                   }
                  return STRINGVAR;
                }
{whitespace} 	/* ignore whitespace */
. 		{ printf("char: %s\n", yytext); return yytext[0]; }


%%

int 
yywrap(void) {
  return 1;
}


int 
basicParse(char *code){

  yy_delete_buffer(YY_CURRENT_BUFFER);
  yy_scan_string(code);

  clearSymbolTable();
  clearLabelTable();

  linenumber = 1;
  errorcode = 0;

  yyparse();

  return errorcode;
}
