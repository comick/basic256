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
    #include <stdlib.h>

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
    extern void clearIfTable();
    extern void newLineOffsets(int);

    extern int getSymbol(char *);
    extern int newSymbol(char *);
    extern int yyparse();
    //extern YYLTYPE yylloc;
    extern int errorcode;
    extern int byteOffset;
    extern int lastLineOffset;
    int linenumber;
    int column;

#ifdef __cplusplus
  }
#endif

void count();

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
else  [Ee][Ll][Ss][Ee]
endif  [Ee][Nn][Dd][Ii][Ff]
while [Ww][Hh][Ii][Ll][Ee]
endwhile [Ee][Nn][Dd][Ww][Hh][Ii][Ll][Ee]
do [Dd][Oo]
until [Uu][Nn][Tt][Ii][Ll]
rem   [Rr][Ee][Mm][ ].+$
remeol   [Rr][Ee][Mm]$
remalt #.*$
dim   [Dd][Ii][Mm]
redim   [Rr][Ee][Dd][Ii][Mm]
alen	[Aa][Ll][Ee][Nn]
alenx	[Aa][Ll][Ee][Nn][Xx]
aleny	[Aa][Ll][Ee][Nn][Yy]
end   [Ee][Nn][Dd]
for   [Ff][Oo][Rr]
to    [Tt][Oo]
next  [Nn][Ee][Xx][Tt]
step  [Ss][Tt][Ee][Pp]
open  [Oo][Pp][Ee][Nn]
read  [Rr][Ee][Aa][Dd]
write [Ww][Rr][Ii][Tt][Ee]
close [Cc][Ll][Oo][Ss][Ee]
reset [Rr][Ee][Ss][Ee][Tt]
plot  [Pp][Ll][Oo][Tt]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
rect  [Rr][Ee][Cc][Tt]
poly  [Pp][Oo][Ll][Yy]
stamp  [Ss][Tt][Aa][Mm][Pp]
line  [Ll][Ii][Nn][Ee]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
graphsize [Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]
graphwidth [Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]
graphheight [Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]
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
mid     [Mm][Ii][Dd]
left    [Ll][Ee][Ff][Tt]
right   [Rr][Ii][Gg][Hh][Tt]
upper	[Uu][Pp][Pp][Ee][Rr]
lower	[Ll][Oo][Ww][Ee][Rr]
instr   [Ii][Nn][Ss][Tt][Rr]
ceil    [Cc][Ee][Ii][Ll]
floor   [Ff][Ll][Oo][Oo][Rr]
abs     [Aa][Bb][Ss]
sin     [Ss][Ii][Nn]
cos     [Cc][Oo][Ss]
tan     [Tt][Aa][Nn]
asin     [Aa][Ss][Ii][Nn]
acos     [Aa][Cc][Oo][Ss]
atan     [Aa][Tt][Aa][Nn]
degrees     [Dd][Ee][Gg][Rr][Ee][Ee][Ss]
radians     [Rr][Aa][Dd][Ii][Aa][Nn][Ss]
rand    [Rr][Aa][Nn][Dd]
pi      [Pp][Ii]
and [Aa][Nn][Dd]
or  [Oo][Rr]
xor [Xx][Oo][Rr]
not [Nn][Oo][Tt]
pause [Pp][Aa][Uu][Ss][Ee]
sound [Ss][Oo][Uu][Nn][Dd]
integer [0-9]+
hexinteger 0[xX][0-9a-fA-F]+
octalinteger 0[oO][0-7]+
binaryinteger 0[bB][01]+
floatnum [0-9]*\.?[0-9]*
string \"[^\"\n]*\"
variable [a-zA-Z][a-zA-Z0-9]*
stringvar [a-zA-Z][a-zA-Z0-9]*\$
label     ^[ \t]*{variable}:
blankline ^[ \t]*[\n]
asc [Aa][Ss][Cc]
chr [Cc][Hh][Rr]
tofloat [Ff][Ll][Oo][Aa][Tt]
readline [Rr][Ee][Aa][Dd][Ll][Ii][Nn][Ee]
writeline [Ww][Rr][Ii][Tt][Ee][Ll][Ii][Nn][Ee]
booleof [Ee][Oo][Ff]
year [Yy][Ee][Aa][Rr]
month [Mm][Oo][Nn][Tt][Hh]
day [Dd][Aa][Yy]
hour [Hh][Oo][Uu][Rr]
minute [Mm][Ii][Nn][Uu][Tt][Ee]
second [Ss][Ee][Cc][Oo][Nn][Dd]
text [Tt][Ee][Xx][Tt]
font [Ff][Oo][Nn][Tt]
say [Ss][Aa][Yy]
wavplay [Ww][Aa][Vv][Pp][Ll][Aa][Yy]
wavstop [Ww][Aa][Vv][Ss][Tt][Oo][Pp]
seek [Ss][Ee][Ee][Kk]
size [Ss][Ii][Zz][Ee]
exists [Ee][Xx][Ii][Ss][Tt][Ss]
booltrue [Tt][Rr][Uu][Ee]
boolfalse [Ff][Aa][Ll][Ss][Ee]
mousex [Mm][Oo][Uu][Ss][Ee][Xx]
mousey [Mm][Oo][Uu][Ss][Ee][Yy]
mouseb [Mm][Oo][Uu][Ss][Ee][Bb]
clickclear [Cc][Ll][Ii][Cc][Kk][Cc][Ll][Ee][Aa][Rr]
clickx [Cc][Ll][Ii][Cc][Kk][Xx]
clicky [Cc][Ll][Ii][Cc][Kk][Yy]
clickb [Cc][Ll][Ii][Cc][Kk][Bb]
system [Ss][Yy][Ss][Tt][Ee][Mm]
volume [Vv][Oo][Ll][Uu][Mm][Ee]
rgb [Rr][Gg][Bb]
pixel [Pp][Ii][Xx][Ee][Ll]
getcolor [Gg][Ee][Tt][Cc][Oo][Ll][Oo][Rr]
log [Ll][Oo][Gg]
logten [Ll][Oo][Gg][1][0]
getslice [Gg][Ee][Tt][Ss][Ll][Ii][Cc][Ee]
putslice [Pp][Uu][Tt][Ss][Ll][Ii][Cc][Ee]
imgload [Ii][Mm][Gg][Ll][Oo][Aa][Dd]
%%


{label} { 
	  char *temp;
	  char *c;
	  int num;
          
          count();
          temp = c = strdup(yytext);
          while (*c != ':') c++;
          *c = 0;
          num = getSymbol(temp);
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
{integer}       { count(); yylval.number = atoi(yytext); return INTEGER; }
{hexinteger}       { count(); yylval.number = strtol(yytext+2, NULL, 16); return INTEGER; }
{octalinteger}       { count(); yylval.number = strtol(yytext+2, NULL, 8); return INTEGER; }
{binaryinteger}       { count(); yylval.number = strtol(yytext+2, NULL, 2); return INTEGER; }
{floatnum}      { count(); yylval.floatnum = atof(yytext); return FLOAT; }
{string}        { int len; count(); len = strlen(yytext); 
                  yylval.string = strdup(yytext + 1); 
                  yylval.string[len - 2] = 0; 
                  return STRING; }
{toint}         { count(); return TOINT; }
{tostring}      { count(); return TOSTRING; }
{length}        { count(); return LENGTH; }
{mid}           { count(); return MID; }
{left}          { count(); return LEFT; }
{right}         { count(); return RIGHT; }
{upper}         { count(); return UPPER; }
{lower}         { count(); return LOWER; }
{instr}         { count(); return INSTR; }
{ceil}          { count(); return CEIL; }
{floor}         { count(); return FLOOR; }
{abs}           { count(); return ABS; }
{sin}           { count(); return SIN; }
{cos}           { count(); return COS; }
{tan}           { count(); return TAN; }
{asin}           { count(); return ASIN; }
{acos}           { count(); return ACOS; }
{atan}           { count(); return ATAN; }
{degrees}           { count(); return DEGREES; }
{radians}           { count(); return RADIANS; }
{log}           { count(); return LOG; }
{logten}           { count(); return LOGTEN; }
{rand}          { count(); return RAND; }
{pi}            { count(); return PI; }
{and}           { count(); return AND; }
{or}            { count(); return OR; }
{xor}           { count(); return XOR; }
{not}           { count(); return NOT; }
{print}		{ count(); return PRINT; }
{dim}		{ count(); return DIM; }
{redim}		{ count(); return REDIM; }
{alen}		{ count(); return ALEN; }
{alenx}		{ count(); return ALENX; }
{aleny}		{ count(); return ALENY; }
{cls}		{ count(); return CLS; }
{clg}		{ count(); return CLG; }
{plot}		{ count(); return PLOT; }
{circle}	{ count(); return CIRCLE; }
{rect}	        { count(); return RECT; }
{poly}		{ count(); return POLY; }
{stamp}		{ count(); return STAMP; }
{line}	        { count(); return LINE; }
{sound}         { count(); return SOUND; }
{fastgraphics}	{ count(); return FASTGRAPHICS; }
{graphsize}	{ count(); return GRAPHSIZE; }
{graphwidth}	{ count(); return GRAPHWIDTH; }
{graphheight}	{ count(); return GRAPHHEIGHT; }
{refresh}	{ count(); return REFRESH; }
{pixel}		{ count(); return PIXEL; }
{rgb}		{ count(); return RGB; }
{color}		{ count(); return SETCOLOR; }
{getcolor}		{ count(); return GETCOLOR; }
{colour}	{ count(); return SETCOLOR; }
{clear}		{ count(); return CLEAR; }
{white}		{ count(); return WHITE; }
{black}		{ count(); return BLACK; }
{red}		{ count(); return RED; }
{dark}{red}	{ count(); return DARKRED; }
{green}		{ count(); return GREEN; }
{dark}{green}	{ count(); return DARKGREEN; }
{blue}		{ count(); return BLUE; }
{dark}{blue}	{ count(); return DARKBLUE; }
{cyan}		{ count(); return CYAN; }
{dark}{cyan}	{ count(); return DARKCYAN; }
{purple}	{ count(); return PURPLE; }
{dark}{purple}	{ count(); return DARKPURPLE; }
{yellow}	{ count(); return YELLOW; }
{dark}{yellow}	{ count(); return DARKYELLOW; }
{orange}	{ count(); return ORANGE; }
{dark}{orange}	{ count(); return DARKORANGE; }
{gray}	        { count(); return GREY; }
{dark}{gray}	{ count(); return DARKGREY; }
{goto}		{ count(); return GOTO; }
{if}		{ count(); return IF; }
{then}		{ count(); return THEN; }
{else}		{ count(); return ELSE; }
{endif}		{ count(); return ENDIF; }
{while}		{ count(); return WHILE; }
{endwhile}		{ count(); return ENDWHILE; }
{do}		{ count(); return DO; }
{until}		{ count(); return UNTIL; }
{for}		{ count(); return FOR; }
{to}		{ count(); return TO; }
{step}		{ count(); return STEP; }
{next}		{ count(); return NEXT; }
{open}		{ count(); return OPEN; }
{write}		{ count(); return WRITE; }
{close}		{ count(); return CLOSE; }
{reset}		{ count(); return RESET; }
{input}         { count(); return INPUT; }
{key}           { count(); return KEY; }
{gosub}         { count(); return GOSUB; }
{return}        { count(); return RETURN; }
{pause}         { count(); return PAUSE; }
{asc}           { count(); return ASC; }
{chr}           { count(); return CHR; }
{tofloat}       { count(); return TOFLOAT; }
{readline}      { count(); return READLINE; }
{writeline}     { count(); return WRITELINE; }
{booleof}       { count(); return BOOLEOF; }
{booltrue}       { count(); return BOOLTRUE; }
{boolfalse}       { count(); return BOOLFALSE; }
{year}          { count(); return YEAR; }
{month}         { count(); return MONTH; }
{day}           { count(); return DAY; }
{hour}          { count(); return HOUR; }
{minute}        { count(); return MINUTE; }
{second}        { count(); return SECOND; }
{text}          { count(); return TEXT; }
{font}          { count(); return FONT; }
{say}           { count(); return SAY; }
{wavplay}       { count(); return WAVPLAY; }
{wavstop}       { count(); return WAVSTOP; }
{getslice}       { count(); return GETSLICE; }
{putslice}       { count(); return PUTSLICE; }
{imgload}       { count(); return IMGLOAD; }
{seek}       { count(); return SEEK; }
{size}       { count(); return SIZE; }
{exists}       { count(); return EXISTS; }
{mousex}       { count(); return MOUSEX; }
{mousey}       { count(); return MOUSEY; }
{mouseb}       { count(); return MOUSEB; }
{clickclear}       { count(); return CLICKCLEAR; }
{clickx}       { count(); return CLICKX; }
{clicky}       { count(); return CLICKY; }
{clickb}       { count(); return CLICKB; }
{system}       { count(); return SYSTEM; }
{volume}       { count(); return VOLUME; }
{rem}           { count(); return '\n'; }
{remeol}        { count(); return '\n'; }
{remalt}        { count(); return '\n'; }
{end}           { count(); return END; }
">="            { count(); return GTE; }
"<="            { count(); return LTE; }
"<>"            { count(); return NE; }
"+"             { count(); return '+'; }
"-"             { count(); return '-'; }
"*"             { count(); return '*'; }
"\\"            { count(); return INTDIV; }
"\%"            { count(); return MOD; }
"/"             { count(); return '/'; }
"^"             { count(); return '^'; }
"="             { count(); return '='; }
"<"             { count(); return '<'; }
">"             { count(); return '>'; }
","             { count(); return ','; }
";"             { count(); return ';'; }
":"             { count(); return ':'; }
"("             { count(); return '('; }
")"             { count(); return ')'; }
"{"             { count(); return '{'; }
"}"             { count(); return '}'; }
"["             { count(); return '['; }
"]"             { count(); return ']'; }
{variable}      { 
		  int num;
		  count(); 
                  num = getSymbol(yytext);
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
                  int num;
                  count(); 
                  num = getSymbol(yytext);
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

"\n"            {count(); linenumber++; return '\n'; }

{whitespace} 	/* ignore whitespace */
. 		{ count(); return yytext[0]; }


%%

int 
yywrap(void) {
  return 1;
}

void count()
{
        int i;

        for (i = 0; yytext[i] != '\0'; i++)
                if (yytext[i] == '\n')
                        column = 0;
                else if (yytext[i] == '\t')
                        column += 8 - (column % 8);
                else
                        column++;
}


int 
basicParse(char *code){

  yy_delete_buffer(YY_CURRENT_BUFFER);
  yy_scan_string(code);

  clearSymbolTable();
  clearLabelTable();
  clearIfTable();

  linenumber = 1;
  lastLineOffset = 0;
  column = 0;
  errorcode = 0;

  yyparse();

  return errorcode;
}
