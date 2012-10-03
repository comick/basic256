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
goto [Gg][Oo][Tt][Oo]
gosub [Gg][Oo][Ss][Uu][Bb]
input [Ii][Nn][Pp][Uu][Tt]
key [Kk][Ee][Yy]
return [Rr][Ee][Tt][Uu][Rr][Nn]
if [Ii][Ff]
then [Tt][Hh][Ee][Nn]
else [Ee][Ll][Ss][Ee]
endif [Ee][Nn][Dd][Ii][Ff]
while [Ww][Hh][Ii][Ll][Ee]
endwhile [Ee][Nn][Dd][Ww][Hh][Ii][Ll][Ee]
do [Dd][Oo]
until [Uu][Nn][Tt][Ii][Ll]
rem [Rr][Ee][Mm][ ].+$
remeol [Rr][Ee][Mm]$
remalt #.*$
dim [Dd][Ii][Mm]
redim [Rr][Ee][Dd][Ii][Mm]
end [Ee][Nn][Dd]
for [Ff][Oo][Rr]
to [Tt][Oo]
next [Nn][Ee][Xx][Tt]
step [Ss][Tt][Ee][Pp]
open [Oo][Pp][Ee][Nn]
read [Rr][Ee][Aa][Dd]
write [Ww][Rr][Ii][Tt][Ee]
close [Cc][Ll][Oo][Ss][Ee]
reset [Rr][Ee][Ss][Ee][Tt]
plot [Pp][Ll][Oo][Tt]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
rect [Rr][Ee][Cc][Tt]
poly [Pp][Oo][Ll][Yy]
stamp [Ss][Tt][Aa][Mm][Pp]
line [Ll][Ii][Nn][Ee]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
graphsize [Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]
graphwidth [Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]
graphheight [Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]
refresh [Rr][Ee][Ff][Rr][Ee][Ss][Hh]
cls [Cc][Ll][Ss]
clg [Cc][Ll][Gg]
color [Cc][Oo][Ll][Oo][Rr]
colour [Cc][Oo][Ll][Oo][Uu][Rr]
clear [Cc][Ll][Ee][Aa][Rr]
black [Bb][Ll][Aa][Cc][Kk]
white [Ww][Hh][Ii][Tt][Ee]
red [Rr][Ee][Dd]
green [Gg][Rr][Ee][Ee][Nn]
blue [Bb][Ll][Uu][Ee]
cyan [Cc][Yy][Aa][Nn]
purple [Pp][Uu][Rr][Pp][Ll][Ee]
yellow [Yy][Ee][Ll][Ll][Oo][Ww]
orange [Oo][Rr][Aa][Nn][Gg][Ee]
gray [Gg][Rr][AaEe][Yy]
dark [Dd][Aa][Rr][Kk]
toint [Ii][Nn][Tt]
tostring [Ss][Tt][Rr][Ii][Nn][Gg]
length [Ll][Ee][Nn][Gg][Tt][Hh]
mid [Mm][Ii][Dd]
left [Ll][Ee][Ff][Tt]
right [Rr][Ii][Gg][Hh][Tt]
upper [Uu][Pp][Pp][Ee][Rr]
lower [Ll][Oo][Ww][Ee][Rr]
instr [Ii][Nn][Ss][Tt][Rr]
instrx [Ii][Nn][Ss][Tt][Rr][Xx]
ceil [Cc][Ee][Ii][Ll]
floor [Ff][Ll][Oo][Oo][Rr]
abs [Aa][Bb][Ss]
sin [Ss][Ii][Nn]
cos [Cc][Oo][Ss]
tan [Tt][Aa][Nn]
asin [Aa][Ss][Ii][Nn]
acos [Aa][Cc][Oo][Ss]
atan [Aa][Tt][Aa][Nn]
degrees [Dd][Ee][Gg][Rr][Ee][Ee][Ss]
radians [Rr][Aa][Dd][Ii][Aa][Nn][Ss]
rand [Rr][Aa][Nn][Dd]
pi [Pp][Ii]
and [Aa][Nn][Dd]
or [Oo][Rr]
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
variable [a-zA-Z][a-zA-Z0-9_]*
stringvar [a-zA-Z][a-zA-Z0-9_]*\$
label ^[ \t]*{variable}:
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
textwidth [Tt][Ee][Xx][Tt][Ww][Ii][Dd][Tt][Hh]
font [Ff][Oo][Nn][Tt]
say [Ss][Aa][Yy]
wavplay [Ww][Aa][Vv][Pp][Ll][Aa][Yy]
wavstop [Ww][Aa][Vv][Ss][Tt][Oo][Pp]
wavwait [Ww][Aa][Vv][Ww][Aa][Ii][Tt]
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
spritedim [Ss][Pp][Rr][Ii][Tt][Ee][Dd][Ii][Mm]
spriteload [Ss][Pp][Rr][Ii][Tt][Ee][Ll][Oo][Aa][Dd]
spriteslice [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]
spritemove [Ss][Pp][Rr][Ii][Tt][Ee][Mm][Oo][Vv][Ee]
spriteplace [Ss][Pp][Rr][Ii][Tt][Ee][Pp][Ll][Aa][Cc][Ee]
spritehide [Ss][Pp][Rr][Ii][Tt][Ee][Hh][Ii][Dd][Ee]
spriteshow [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Hh][Oo][Ww]
spritecollide [Ss][Pp][Rr][Ii][Tt][Ee][Cc][Oo][Ll][Ll][Ii][Dd][Ee]
spritex [Ss][Pp][Rr][Ii][Tt][Ee][Xx]
spritey [Ss][Pp][Rr][Ii][Tt][Ee][Yy]
spriteh [Ss][Pp][Rr][Ii][Tt][Ee][Hh]
spritew [Ss][Pp][Rr][Ii][Tt][Ee][Ww]
spritev [Ss][Pp][Rr][Ii][Tt][Ee][Vv]
changedir [Cc][Hh][Aa][Nn][Gg][Ee][Dd][Ii][Rr]
currentdir [Cc][Uu][Rr][Rr][Ee][Nn][Tt][Dd][Ii][Rr]
decimal [Dd][Ee][Cc][Ii][Mm][Aa][Ll]
dbopen [Dd][Bb][Oo][Pp][Ee][Nn]
dbclose [Dd][Bb][Cc][Ll][Oo][Ss][Ee]
dbexecute [Dd][Bb][Ee][Xx][Ee][Cc][Uu][Tt][Ee]
dbopenset [Dd][Bb][Oo][Pp][Ee][Nn][Ss][Ee][Tt]
dbcloseset [Dd][Bb][Cc][Ll][Oo][Ss][Ee][Ss][Ee][Tt]
dbrow [Dd][Bb][Rr][Oo][Ww]
dbint [Dd][Bb][Ii][Nn][Tt]
dbfloat [Dd][Bb][Ff][Ll][Oo][Aa][Tt]
dbstring [Dd][Bb][Ss][Tt][Rr][Ii][Nn][Gg]
onerror [Oo][Nn][Ee][Rr][Rr][Oo][Rr]
offerror [Oo][Ff][Ff][Ee][Rr][Rr][Oo][Rr]
lasterror [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr]
lasterrormessage [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Mm][Ee][Ss][Ss][Aa][Gg][Ee]
lasterrorline [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ll][Ii][Nn][Ee]
lasterrorextra [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ee][Xx][Tt][Rr][Aa]
netlisten [Nn][Ee][Tt][Ll][Ii][Ss][Tt][Ee][Nn]
netconnect [Nn][Ee][Tt][Cc][Oo][Nn][Nn][Ee][Cc][Tt]
netwrite [Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]
netread [Nn][Ee][Tt][Rr][Ee][Aa][Dd]
netclose [Nn][Ee][Tt][Cc][Ll][Oo][Ss][Ee]
netdata [Nn][Ee][Tt][Dd][Aa][Tt][Aa]
netaddress [Nn][Ee][Tt][Aa][Dd][Dd][Rr][Ee][Ss][Ss]
kill [Kk][Ii][Ll][Ll]
md5 [Mm][Dd][5]
getsetting [Gg][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
setsetting [Ss][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
portin [Pp][Oo][Rr][Tt][Ii][Nn]
portout [Pp][Oo][Rr][Tt][Oo][Uu][Tt]
imgsave [Ii][Mm][Gg][Ss][Aa][Vv][Ee]
sqr [Ss][Qq][Rr]
sqrt [Ss][Qq][Rr][Tt]
exp [Ee][Xx][Pp]
dir [Dd][Ii][Rr]
replace [Rr][Ee][Pp][Ll][Aa][Cc][Ee]
count [Cc][Oo][Uu][Nn][Tt]
explode [Ee][Xx][Pp][Ll][Oo][Dd][Ee]
replacex [Rr][Ee][Pp][Ll][Aa][Cc][Ee][Xx]
countx [Cc][Oo][Uu][Nn][Tt][Xx]
explodex [Ee][Xx][Pp][Ll][Oo][Dd][Ee][Xx]
implode [Ii][Mm][Pp][Ll][Oo][Dd][Ee]
ostype [Oo][Ss][Tt][Yy][Pp][Ee]
msec [Mm][Ss][Ee][Cc]
editvisible [Ee][Dd][Ii][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
graphvisible [Gg][Rr][Aa][Pp][Hh][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
outputvisible [Oo][Uu][Tt][Pp][Uu][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
function [Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
endfunction [Ee][Nn][Dd][Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
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
{hexinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 16); return B256INTEGER; }
{octalinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 8); return B256INTEGER; }
{binaryinteger}	{ count(); yylval.number = strtol(yytext+2, NULL, 2); return B256INTEGER; }
{floatnum}	{ count(); yylval.floatnum = atof(yytext); return B256FLOAT; }
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
"-"		{ count(); return '-'; }
"*"		{ count(); return '*'; }
"\\"		{ count(); return B256INTDIV; }
"\%"		{ count(); return B256MOD; }
"|"		{ count(); return B256BINARYOR; }
"&"		{ count(); return B256BINARYAND; }
"~"		{ count(); return B256BINARYNOT; }
"/"		{ count(); return '/'; }
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

	yyparse();

	if (testIfOnTable() != -1) {
		linenumber = testIfOnTable();
		errorcode = testIfOnTableError();
	}

	return errorcode;
}
