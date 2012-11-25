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
#include "../CompileErrors.h"

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
extern void dumpSynbolTable();
extern void clearLabelTable();
extern void clearIfTable();
extern int testIfOnTable();
extern int testIfOnTableError();
extern void newLineOffsets(int);

extern int getSymbol(char *);
extern int newSymbol(char *);
extern int yyparse();
//extern YYLTYPE yylloc;
extern int errorcode;
extern int byteOffset;
extern int lastLineOffset;
extern int functionDefSymbol;
extern int subroutineDefSymbol;

int linenumber;
int column;

#ifdef __cplusplus
}
#endif

void count();

%}

binaryinteger 0[bB][01]+
blankline ^[ \t]*[\n]
floatnum [0-9]*\.?[0-9]*
integer [0-9]+
hexinteger 0[xX][0-9a-fA-F]+
nws [^ \t\n]+
octalinteger 0[oO][0-7]+
string (\"[^\"\n]*\")|(\'[^\'\n]*\')
stringvar [a-zA-Z][a-zA-Z0-9_]*\$
variable [a-zA-Z][a-zA-Z0-9_]*
whitespace [ \t]+

abs [Aa][Bb][Ss]
acos [Aa][Cc][Oo][Ss]
and [Aa][Nn][Dd]
asc [Aa][Ss][Cc]
asin [Aa][Ss][Ii][Nn]
atan [Aa][Tt][Aa][Nn]
black [Bb][Ll][Aa][Cc][Kk]
blue [Bb][Ll][Uu][Ee]
booleof [Ee][Oo][Ff]
boolfalse [Ff][Aa][Ll][Ss][Ee]
booltrue [Tt][Rr][Uu][Ee]
call [Cc][Aa][Ll][Ll]
ceil [Cc][Ee][Ii][Ll]
changedir [Cc][Hh][Aa][Nn][Gg][Ee][Dd][Ii][Rr]
chr [Cc][Hh][Rr]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
clear [Cc][Ll][Ee][Aa][Rr]
clg [Cc][Ll][Gg]
clickb [Cc][Ll][Ii][Cc][Kk][Bb]
clickclear [Cc][Ll][Ii][Cc][Kk][Cc][Ll][Ee][Aa][Rr]
clickx [Cc][Ll][Ii][Cc][Kk][Xx]
clicky [Cc][Ll][Ii][Cc][Kk][Yy]
close [Cc][Ll][Oo][Ss][Ee]
cls [Cc][Ll][Ss]
color [Cc][Oo][Ll][Oo][Rr]
colour [Cc][Oo][Ll][Oo][Uu][Rr]
cos [Cc][Oo][Ss]
count [Cc][Oo][Uu][Nn][Tt]
countx [Cc][Oo][Uu][Nn][Tt][Xx]
currentdir [Cc][Uu][Rr][Rr][Ee][Nn][Tt][Dd][Ii][Rr]
cyan [Cc][Yy][Aa][Nn]
dark [Dd][Aa][Rr][Kk]
day [Dd][Aa][Yy]
dbclose [Dd][Bb][Cc][Ll][Oo][Ss][Ee]
dbcloseset [Dd][Bb][Cc][Ll][Oo][Ss][Ee][Ss][Ee][Tt]
dbexecute [Dd][Bb][Ee][Xx][Ee][Cc][Uu][Tt][Ee]
dbfloat [Dd][Bb][Ff][Ll][Oo][Aa][Tt]
dbint [Dd][Bb][Ii][Nn][Tt]
dbnull [Dd][Bb][Nn][Uu][Ll][Ll]
dbopen [Dd][Bb][Oo][Pp][Ee][Nn]
dbopenset [Dd][Bb][Oo][Pp][Ee][Nn][Ss][Ee][Tt]
dbrow [Dd][Bb][Rr][Oo][Ww]
dbstring [Dd][Bb][Ss][Tt][Rr][Ii][Nn][Gg]
decimal [Dd][Ee][Cc][Ii][Mm][Aa][Ll]
degrees [Dd][Ee][Gg][Rr][Ee][Ee][Ss]
dim [Dd][Ii][Mm]
dir [Dd][Ii][Rr]
do [Dd][Oo]
editvisible [Ee][Dd][Ii][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
else [Ee][Ll][Ss][Ee]
end [Ee][Nn][Dd]
endfunction ([Ee][Nn][Dd][Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn])|([Ee][Nn][Dd][ \t]+[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn])
endif ([Ee][Nn][Dd][Ii][Ff])|([Ee][Nn][Dd][ \t]+[Ii][Ff])
endsubroutine ([Ee][Nn][Dd][Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee])|([Ee][Nn][Dd][ \t]+[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee])
endwhile ([Ee][Nn][Dd][Ww][Hh][Ii][Ll][Ee])|([Ee][Nn][Dd][ \t]+[Ww][Hh][Ii][Ll][Ee])
exists [Ee][Xx][Ii][Ss][Tt][Ss]
exp [Ee][Xx][Pp]
explode [Ee][Xx][Pp][Ll][Oo][Dd][Ee]
explodex [Ee][Xx][Pp][Ll][Oo][Dd][Ee][Xx]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
floor [Ff][Ll][Oo][Oo][Rr]
font [Ff][Oo][Nn][Tt]
for [Ff][Oo][Rr]
freedb [Ff][Rr][Ee][Ee][Dd][Bb]
freedbset [Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]
freefile [Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]
freenet [Ff][Rr][Ee][Ee][Nn][Ee][Tt]
function [Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
getcolor [Gg][Ee][Tt][Cc][Oo][Ll][Oo][Rr]
getsetting [Gg][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
getslice [Gg][Ee][Tt][Ss][Ll][Ii][Cc][Ee]
global [Gg][Ll][Oo][Bb][Aa][Ll]
gosub [Gg][Oo][Ss][Uu][Bb]
goto [Gg][Oo][Tt][Oo]
graphheight [Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]
graphsize [Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]
graphvisible [Gg][Rr][Aa][Pp][Hh][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
graphwidth [Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]
gray [Gg][Rr][AaEe][Yy]
green [Gg][Rr][Ee][Ee][Nn]
hour [Hh][Oo][Uu][Rr]
if [Ii][Ff]
imgload [Ii][Mm][Gg][Ll][Oo][Aa][Dd]
imgsave [Ii][Mm][Gg][Ss][Aa][Vv][Ee]
implode [Ii][Mm][Pp][Ll][Oo][Dd][Ee]
input [Ii][Nn][Pp][Uu][Tt]
instr [Ii][Nn][Ss][Tt][Rr]
instrx [Ii][Nn][Ss][Tt][Rr][Xx]
key [Kk][Ee][Yy]
kill [Kk][Ii][Ll][Ll]
label ^[ \t]*{variable}:
lasterror [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr]
lasterrorextra [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ee][Xx][Tt][Rr][Aa]
lasterrorline [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ll][Ii][Nn][Ee]
lasterrormessage [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Mm][Ee][Ss][Ss][Aa][Gg][Ee]
left [Ll][Ee][Ff][Tt]
length [Ll][Ee][Nn][Gg][Tt][Hh]
line [Ll][Ii][Nn][Ee]
log [Ll][Oo][Gg]
logten [Ll][Oo][Gg][1][0]
lower [Ll][Oo][Ww][Ee][Rr]
md5 [Mm][Dd][5]
mid [Mm][Ii][Dd]
minute [Mm][Ii][Nn][Uu][Tt][Ee]
month [Mm][Oo][Nn][Tt][Hh]
mouseb [Mm][Oo][Uu][Ss][Ee][Bb]
mousex [Mm][Oo][Uu][Ss][Ee][Xx]
mousey [Mm][Oo][Uu][Ss][Ee][Yy]
msec [Mm][Ss][Ee][Cc]
netaddress [Nn][Ee][Tt][Aa][Dd][Dd][Rr][Ee][Ss][Ss]
netclose [Nn][Ee][Tt][Cc][Ll][Oo][Ss][Ee]
netconnect [Nn][Ee][Tt][Cc][Oo][Nn][Nn][Ee][Cc][Tt]
netdata [Nn][Ee][Tt][Dd][Aa][Tt][Aa]
netlisten [Nn][Ee][Tt][Ll][Ii][Ss][Tt][Ee][Nn]
netread [Nn][Ee][Tt][Rr][Ee][Aa][Dd]
netwrite [Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]
next [Nn][Ee][Xx][Tt]
not [Nn][Oo][Tt]
offerror [Oo][Ff][Ff][Ee][Rr][Rr][Oo][Rr]
onerror [Oo][Nn][Ee][Rr][Rr][Oo][Rr]
open [Oo][Pp][Ee][Nn]
openb [Oo][Pp][Ee][Nn][Bb]
or [Oo][Rr]
orange [Oo][Rr][Aa][Nn][Gg][Ee]
ostype [Oo][Ss][Tt][Yy][Pp][Ee]
outputvisible [Oo][Uu][Tt][Pp][Uu][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
pause [Pp][Aa][Uu][Ss][Ee]
pi [Pp][Ii]
pixel [Pp][Ii][Xx][Ee][Ll]
plot [Pp][Ll][Oo][Tt]
poly [Pp][Oo][Ll][Yy]
portin [Pp][Oo][Rr][Tt][Ii][Nn]
portout [Pp][Oo][Rr][Tt][Oo][Uu][Tt]
print [Pp][Rr][Ii][Nn][Tt]
purple [Pp][Uu][Rr][Pp][Ll][Ee]
putslice [Pp][Uu][Tt][Ss][Ll][Ii][Cc][Ee]
radians [Rr][Aa][Dd][Ii][Aa][Nn][Ss]
rand [Rr][Aa][Nn][Dd]
read [Rr][Ee][Aa][Dd]
readbyte [Rr][Ee][Aa][Dd][Bb][Yy][Tt][Ee]
readline [Rr][Ee][Aa][Dd][Ll][Ii][Nn][Ee]
rect [Rr][Ee][Cc][Tt]
red [Rr][Ee][Dd]
redim [Rr][Ee][Dd][Ii][Mm]
ref [Rr][Ee][Ff]
refresh [Rr][Ee][Ff][Rr][Ee][Ss][Hh]
rem [Rr][Ee][Mm][ ].+$
remalt #.*$
remeol [Rr][Ee][Mm]$
replace [Rr][Ee][Pp][Ll][Aa][Cc][Ee]
replacex [Rr][Ee][Pp][Ll][Aa][Cc][Ee][Xx]
reset [Rr][Ee][Ss][Ee][Tt]
return [Rr][Ee][Tt][Uu][Rr][Nn]
rgb [Rr][Gg][Bb]
right [Rr][Ii][Gg][Hh][Tt]
say [Ss][Aa][Yy]
second [Ss][Ee][Cc][Oo][Nn][Dd]
seek [Ss][Ee][Ee][Kk]
setsetting [Ss][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
sin [Ss][Ii][Nn]
size [Ss][Ii][Zz][Ee]
sound [Ss][Oo][Uu][Nn][Dd]
spritecollide [Ss][Pp][Rr][Ii][Tt][Ee][Cc][Oo][Ll][Ll][Ii][Dd][Ee]
spritedim [Ss][Pp][Rr][Ii][Tt][Ee][Dd][Ii][Mm]
spriteh [Ss][Pp][Rr][Ii][Tt][Ee][Hh]
spritehide [Ss][Pp][Rr][Ii][Tt][Ee][Hh][Ii][Dd][Ee]
spriteload [Ss][Pp][Rr][Ii][Tt][Ee][Ll][Oo][Aa][Dd]
spritemove [Ss][Pp][Rr][Ii][Tt][Ee][Mm][Oo][Vv][Ee]
spriteplace [Ss][Pp][Rr][Ii][Tt][Ee][Pp][Ll][Aa][Cc][Ee]
spriter [Ss][Pp][Rr][Ii][Tt][Ee][Rr]
sprites [Ss][Pp][Rr][Ii][Tt][Ee][Ss]
spriteshow [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Hh][Oo][Ww]
spriteslice [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]
spritev [Ss][Pp][Rr][Ii][Tt][Ee][Vv]
spritew [Ss][Pp][Rr][Ii][Tt][Ee][Ww]
spritex [Ss][Pp][Rr][Ii][Tt][Ee][Xx]
spritey [Ss][Pp][Rr][Ii][Tt][Ee][Yy]
sqr [Ss][Qq][Rr]
sqrt [Ss][Qq][Rr][Tt]
stamp [Ss][Tt][Aa][Mm][Pp]
step [Ss][Tt][Ee][Pp]
subroutine [Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]
system [Ss][Yy][Ss][Tt][Ee][Mm]
tan [Tt][Aa][Nn]
text [Tt][Ee][Xx][Tt]
textwidth [Tt][Ee][Xx][Tt][Ww][Ii][Dd][Tt][Hh]
then [Tt][Hh][Ee][Nn]
throwerror [Tt][Hh][Rr][Oo][Ww][Ee][Rr][Rr][Oo][Rr]
to [Tt][Oo]
tofloat [Ff][Ll][Oo][Aa][Tt]
toint [Ii][Nn][Tt]
tostring [Ss][Tt][Rr][Ii][Nn][Gg]
until [Uu][Nn][Tt][Ii][Ll]
upper [Uu][Pp][Pp][Ee][Rr]
volume [Vv][Oo][Ll][Uu][Mm][Ee]
wavplay [Ww][Aa][Vv][Pp][Ll][Aa][Yy]
wavstop [Ww][Aa][Vv][Ss][Tt][Oo][Pp]
wavwait [Ww][Aa][Vv][Ww][Aa][Ii][Tt]
while [Ww][Hh][Ii][Ll][Ee]
white [Ww][Hh][Ii][Tt][Ee]
write [Ww][Rr][Ii][Tt][Ee]
writebyte [Ww][Rr][Ii][Tt][Ee][Bb][Yy][Tt][Ee]
writeline [Ww][Rr][Ii][Tt][Ee][Ll][Ii][Nn][Ee]
xor [Xx][Oo][Rr]
year [Yy][Ee][Aa][Rr]
yellow [Yy][Ee][Ll][Ll][Oo][Ww]

%%

{label} {
	char *temp;
	char *c, *d;
	int num;

	count();
	temp = strdup(yytext);
	// strip whitespace
	c = temp;
	while(*c == ' ' || *c == '\t') {
		c++;
	}
	if (c!=temp) {
		d = temp;
		while(*c!=0x00) {
			*d++ = *c++;
		}
		*d = 0x00;
	}
	// strip :
	c = temp;
	while (*c != ':') c++;
	*c = 0x0;
	//
	num = getSymbol(temp);
	if (num < 0) {
		yylval.number = newSymbol(temp); //Doesn't conflict, because label only resolved after branch
	} else {
		free(temp);
		yylval.number = num;
	}
	return B256LABEL;
}
{integer}	{ count(); yylval.number = atoi(yytext); return B256INTEGER; }
{floatnum}	{ count(); yylval.floatnum = atof(yytext); return B256FLOAT; }
{binaryinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 2); return B256INTEGER; }
{hexinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 16); return B256INTEGER; }
{octalinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 8); return B256INTEGER; }
{string}	{ int len; count(); len = strlen(yytext);
			yylval.string = strdup(yytext + 1);
			yylval.string[len - 2] = 0;
		return B256STRING; }

{toint}		{ count(); return B256TOINT; }
{tostring}	{ count(); return B256TOSTRING; }
{length}	{ count(); return B256LENGTH; }
{mid}		{ count(); return B256MID; }
{left}		{ count(); return B256LEFT; }
{right}		{ count(); return B256RIGHT; }
{upper}		{ count(); return B256UPPER; }
{lower}		{ count(); return B256LOWER; }
{instr}		{ count(); return B256INSTR; }
{instrx}		{ count(); return B256INSTRX; }
{ceil}		{ count(); return B256CEIL; }
{floor}		{ count(); return B256FLOOR; }
{abs}		{ count(); return B256ABS; }
{sin}		{ count(); return B256SIN; }
{cos}		{ count(); return B256COS; }
{tan}		{ count(); return B256TAN; }
{asin}		{ count(); return B256ASIN; }
{acos}		{ count(); return B256ACOS; }
{atan}		{ count(); return B256ATAN; }
{degrees}	{ count(); return B256DEGREES; }
{radians}	{ count(); return B256RADIANS; }
{log}		{ count(); return B256LOG; }
{logten}	{ count(); return B256LOGTEN; }
{sqr}		{ count(); return B256SQR; }
{sqrt}		{ count(); return B256SQR; }
{exp}		{ count(); return B256EXP; }
{rand}		{ count(); return B256RAND; }
{pi}		{ count(); return B256PI; }
{and}		{ count(); return B256AND; }
{or}		{ count(); return B256OR; }
{xor}		{ count(); return B256XOR; }
{not}		{ count(); return B256NOT; }
{print}		{ count(); return B256PRINT; }
{dim}		{ count(); return B256DIM; }
{redim}		{ count(); return B256REDIM; }
{cls}		{ count(); return B256CLS; }
{clg}		{ count(); return B256CLG; }
{plot}		{ count(); return B256PLOT; }
{circle}	{ count(); return B256CIRCLE; }
{rect}		{ count(); return B256RECT; }
{poly}		{ count(); return B256POLY; }
{stamp}		{ count(); return B256STAMP; }
{line}		{ count(); return B256LINE; }
{sound}		{ count(); return B256SOUND; }
{fastgraphics}	{ count(); return B256FASTGRAPHICS; }
{graphsize}	{ count(); return B256GRAPHSIZE; }
{graphwidth}	{ count(); return B256GRAPHWIDTH; }
{graphheight}	{ count(); return B256GRAPHHEIGHT; }
{refresh}	{ count(); return B256REFRESH; }
{pixel}		{ count(); return B256PIXEL; }
{rgb}		{ count(); return B256RGB; }
{color}		{ count(); return B256SETCOLOR; }
{getcolor}	{ count(); return B256GETCOLOR; }
{colour}	{ count(); return B256SETCOLOR; }
{clear}		{ count(); return B256CLEAR; }
{white}		{ count(); return B256WHITE; }
{black}		{ count(); return B256BLACK; }
{red}		{ count(); return B256RED; }
{dark}{red}	{ count(); return B256DARKRED; }
{green}		{ count(); return B256GREEN; }
{dark}{green}	{ count(); return B256DARKGREEN; }
{blue}		{ count(); return B256BLUE; }
{dark}{blue}	{ count(); return B256DARKBLUE; }
{cyan}		{ count(); return B256CYAN; }
{dark}{cyan}	{ count(); return B256DARKCYAN; }
{purple}	{ count(); return B256PURPLE; }
{dark}{purple}	{ count(); return B256DARKPURPLE; }
{yellow}	{ count(); return B256YELLOW; }
{dark}{yellow}	{ count(); return B256DARKYELLOW; }
{orange}	{ count(); return B256ORANGE; }
{dark}{orange}	{ count(); return B256DARKORANGE; }
{gray}	        { count(); return B256GREY; }
{dark}{gray}	{ count(); return B256DARKGREY; }
{goto}		{ count(); return B256GOTO; }
{if}		{ count(); return B256IF; }
{then}		{ count(); return B256THEN; }
{else}		{ count(); return B256ELSE; }
{endif}		{ count(); return B256ENDIF; }
{while}		{ count(); return B256WHILE; }
{endwhile}	{ count(); return B256ENDWHILE; }
{do}		{ count(); return B256DO; }
{until}		{ count(); return B256UNTIL; }
{for}		{ count(); return B256FOR; }
{to}		{ count(); return B256TO; }
{step}		{ count(); return B256STEP; }
{next}		{ count(); return B256NEXT; }
{open}		{ count(); return B256OPEN; }
{read}		{ count(); return B256READ; }
{write}		{ count(); return B256WRITE; }
{close}		{ count(); return B256CLOSE; }
{reset}		{ count(); return B256RESET; }
{input}		{ count(); return B256INPUT; }
{key}		{ count(); return B256KEY; }
{gosub}		{ count(); return B256GOSUB; }
{return}	{ count(); return B256RETURN; }
{pause}		{ count(); return B256PAUSE; }
{asc}		{ count(); return B256ASC; }
{chr}		{ count(); return B256CHR; }
{tofloat}	{ count(); return B256TOFLOAT; }
{readline}	{ count(); return B256READLINE; }
{writeline}	{ count(); return B256WRITELINE; }
{booleof}	{ count(); return B256BOOLEOF; }
{booltrue}	{ count(); return B256BOOLTRUE; }
{boolfalse}	{ count(); return B256BOOLFALSE; }
{year}		{ count(); return B256YEAR; }
{month}		{ count(); return B256MONTH; }
{day}		{ count(); return B256DAY; }
{hour}		{ count(); return B256HOUR; }
{minute}	{ count(); return B256MINUTE; }
{second}	{ count(); return B256SECOND; }
{text}		{ count(); return B256TEXT; }
{textwidth}		{ count(); return B256TEXTWIDTH; }
{font}		{ count(); return B256FONT; }
{say}		{ count(); return B256SAY; }
{wavplay}	{ count(); return B256WAVPLAY; }
{wavstop}	{ count(); return B256WAVSTOP; }
{wavwait}	{ count(); return B256WAVWAIT; }
{getslice}	{ count(); return B256GETSLICE; }
{putslice}	{ count(); return B256PUTSLICE; }
{imgload}	{ count(); return B256IMGLOAD; }
{imgsave}	{ count(); return B256IMGSAVE; }
{spritedim}	{ count(); return B256SPRITEDIM; }
{spriteload}	{ count(); return B256SPRITELOAD; }
{spriteslice}	{ count(); return B256SPRITESLICE; }
{spriteplace}	{ count(); return B256SPRITEPLACE; }
{spritemove}	{ count(); return B256SPRITEMOVE; }
{spritehide}	{ count(); return B256SPRITEHIDE; }
{spriteshow}	{ count(); return B256SPRITESHOW; }
{spritecollide}	{ count(); return B256SPRITECOLLIDE; }
{spritex}	{ count(); return B256SPRITEX; }
{spritey}	{ count(); return B256SPRITEY; }
{spriteh}	{ count(); return B256SPRITEH; }
{spritew}	{ count(); return B256SPRITEW; }
{spritev}	{ count(); return B256SPRITEV; }
{spriter}	{ count(); return B256SPRITER; }
{sprites}	{ count(); return B256SPRITES; }
{seek}		{ count(); return B256SEEK; }
{size}		{ count(); return B256SIZE; }
{exists}	{ count(); return B256EXISTS; }
{mousex}	{ count(); return B256MOUSEX; }
{mousey}	{ count(); return B256MOUSEY; }
{mouseb}	{ count(); return B256MOUSEB; }
{clickclear}	{ count(); return B256CLICKCLEAR; }
{clickx}	{ count(); return B256CLICKX; }
{clicky}	{ count(); return B256CLICKY; }
{clickb}	{ count(); return B256CLICKB; }
{system}	{ count(); return B256SYSTEM; }
{volume}	{ count(); return B256VOLUME; }
{currentdir}	{ count(); return B256CURRENTDIR; }
{changedir}	{ count(); return B256CHANGEDIR; }
{dir}	{ count(); return B256DIR; }
{decimal}	{ count(); return B256DECIMAL; }
{dbopen}	{ count(); return B256DBOPEN; }
{dbclose}	{ count(); return B256DBCLOSE; }
{dbexecute}	{ count(); return B256DBEXECUTE; }
{dbopenset}	{ count(); return B256DBOPENSET; }
{dbcloseset}	{ count(); return B256DBCLOSESET; }
{dbrow}		{ count(); return B256DBROW; }
{dbint}		{ count(); return B256DBINT; }
{dbfloat}	{ count(); return B256DBFLOAT; }
{dbnull}	{ count(); return B256DBNULL; }
{dbstring}	{ count(); return B256DBSTRING; }
{onerror}	{ count(); return B256ONERROR; }
{offerror}	{ count(); return B256OFFERROR; }
{lasterror}	{ count(); return B256LASTERROR; }
{lasterrormessage} { count(); return B256LASTERRORMESSAGE; }
{lasterrorline}	{ count(); return B256LASTERRORLINE; }
{lasterrorextra} { count(); return B256LASTERROREXTRA; }
{netlisten}	{ count(); return B256NETLISTEN; }
{netconnect}	{ count(); return B256NETCONNECT; }
{netwrite}	{ count(); return B256NETWRITE; }
{netread}	{ count(); return B256NETREAD; }
{netclose}	{ count(); return B256NETCLOSE; }
{netdata}	{ count(); return B256NETDATA; }
{netaddress}	{ count(); return B256NETADDRESS; }
{kill}		{ count(); return B256KILL; }
{md5}		{ count(); return B256MD5; }
{getsetting}	{ count(); return B256GETSETTING; }
{setsetting}	{ count(); return B256SETSETTING; }
{portin}	{ count(); return B256PORTIN; }
{portout}	{ count(); return B256PORTOUT; }
{replace}	{ count(); return B256REPLACE; }
{count}	{ count(); return B256COUNT; }
{explode}	{ count(); return B256EXPLODE; }
{implode}	{ count(); return B256IMPLODE; }
{replacex}	{ count(); return B256REPLACEX; }
{countx}	{ count(); return B256COUNTX; }
{explodex}	{ count(); return B256EXPLODEX; }
{ostype}	{ count(); return B256OSTYPE; }
{msec}		{ count(); return B256MSEC; }
{editvisible}	{ count(); return B256EDITVISIBLE; }
{graphvisible}	{ count(); return B256GRAPHVISIBLE; }
{outputvisible}	{ count(); return B256OUTPUTVISIBLE; }
{rem}		{ count(); return '\n'; }
{remeol}	{ count(); return '\n'; }
{remalt}	{ count(); return '\n'; }
{end}		{ count(); return B256END; }
">="		{ count(); return B256GTE; }
"<="		{ count(); return B256LTE; }
"<>"		{ count(); return B256NE; }
"+"		{ count(); return '+'; }
"++"		{ count(); return B256ADD1; }
"+="		{ count(); return B256ADDEQUAL; }
"-"		{ count(); return '-'; }
"--"		{ count(); return B256SUB1; }
"-="		{ count(); return B256SUBEQUAL; }
"*"		{ count(); return '*'; }
"*="		{ count(); return B256MULEQUAL; }
"\\"		{ count(); return B256INTDIV; }
"\%"		{ count(); return B256MOD; }
"|"		{ count(); return B256BINARYOR; }
"&"		{ count(); return B256BINARYAND; }
"~"		{ count(); return B256BINARYNOT; }
"/"		{ count(); return '/'; }
"/="		{ count(); return B256DIVEQUAL; }
"^"		{ count(); return '^'; }
"="		{ count(); return '='; }
"<"		{ count(); return '<'; }
">"		{ count(); return '>'; }
","		{ count(); return ','; }
";"		{ count(); return ';'; }
":"		{ count(); return ':'; }
"("		{ count(); return '('; }
")"		{ count(); return ')'; }
"{"		{ count(); return '{'; }
"}"		{ count(); return '}'; }
"["		{ count(); return '['; }
"]"		{ count(); return ']'; }
{function}	{ count(); return B256FUNCTION; }
{endfunction}	{ count(); return B256ENDFUNCTION; }
{throwerror}	{ count(); return B256THROWERROR; }
{subroutine}	{ count(); return B256SUBROUTINE; }
{endsubroutine}	{ count(); return B256ENDSUBROUTINE; }
{call}		{ count(); return B256CALL; }
{global}	{ count(); return B256GLOBAL; }
{readbyte}	{ count(); return B256READBYTE; }
{writebyte}	{ count(); return B256WRITEBYTE; }
{openb}		{ count(); return B256OPENB; }
{ref}		{ count(); return B256REF; }
{freedb}	{ count(); return B256FREEDB; }
{freedbset}	{ count(); return B256FREEDBSET; }
{freefile}	{ count(); return B256FREEFILE; }
{freenet}	{ count(); return B256FREENET; }

	
{variable} {
	int num;
	count();
	num = getSymbol(yytext);
	if (num < 0) {
		yylval.number = newSymbol(strdup(yytext));
	} else {
		yylval.number = num;
	}
	return B256VARIABLE;
}

{stringvar} {
	int num;
	count();
	num = getSymbol(yytext);
	if (num < 0) {
		yylval.number = newSymbol(strdup(yytext));
	} else {
		yylval.number = num;
	}
	return B256STRINGVAR;
}

"\n"	{ count(); linenumber++; return '\n'; }

{whitespace} /* ignore whitespace */
. 	{ count(); return yytext[0]; }

%%

int
yywrap(void) {
	return 1;
}

void
count() {
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
basicParse(char *code) {

	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_scan_string(code);

	clearSymbolTable();
	clearLabelTable();
	clearIfTable();

	linenumber = 1;
	lastLineOffset = 0;
	column = 0;
	errorcode = 0;
	functionDefSymbol = -1;
	subroutineDefSymbol = -1;


	yyparse();

	if (testIfOnTable()!=-1 && errorcode==0) {
		linenumber = testIfOnTable();
		errorcode = testIfOnTableError();
	}

	return errorcode;
}
