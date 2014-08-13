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

/** For statements without arguments you NEED to create rules like {stmt}":" that push back the
 ** colon.  this is so that if the stmt with a : and no space between them is not interperted
 ** as a label- flex is greedy and will match the stmt: first
 **/
 
%{
#include "basicParse.tab.h"
#include "../CompileErrors.h"
#include "../WordCodes.h"


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

extern int numifs;
extern struct symbol symtable[];
extern void clearSymbolTable();
extern void dumpSynbolTable();
extern void clearLabelTable();
extern void clearIfTable();
extern int testIfOnTable(int);
extern int testIfOnTableError(int);
extern void newLineOffsets(int);

extern int getSymbol(char *);
extern void addIntOp(int, int);
extern void addOp(int);
extern void addString(char*);
extern void addStringOp(int, char*);


extern int yyparse();
//extern YYLTYPE yylloc;
extern int errorcode;
extern int wordOffset;
extern int functionDefSymbol;
extern int subroutineDefSymbol;

// arrays to return warnings from compiler
// defined in basicParse.y and also used in interperter.cpp
extern int parsewarningtable[];
extern int parsewarningtablesourceline[];
extern int parsewarningtablecolumn[];
extern char *parsewarningcolumnlexingfilename[];
extern int numparsewarnings;

int linenumber;
int column;
char *lexingfilename;

#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_linenumber[MAX_INCLUDE_DEPTH];
char* include_filename[MAX_INCLUDE_DEPTH];
int numincludes=0;

#ifdef __cplusplus
}
#endif

void count();
void unputcolon();

%}

%x INCLUDE

constinteger ([0-9]{1,9}|1[0-9]{9}|2(0[0-9]{8}|1([0-3][0-9]{7}|4([0-6][0-9]{6}|7([0-3][0-9]{5}|4([0-7][0-9]{4}|8([0-2][0-9]{3}|3([0-5][0-9]{2}|6([0-3][0-9]|4[0-7])))))))))
constdecimal [0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
constbinary 0[bB][01]{1,32}
consthex 0[xX][0-9a-fA-F]{1,8}
constoctal 0[oO](([0-7]{0,10})|([0-3][0-7]{10}))
conststring (\"[^\"\n]*\")|(\'[^\'\n]*\')
constfalse [Ff][Aa][Ll][Ss][Ee]([ \t]*\([ \t]*\))?
consttrue [Tt][Rr][Uu][Ee]([ \t]*\([ \t]*\))?


blankline ^[ \t]*[\n]
nws [^ \t\n]+
stringvar [a-zA-Z][a-zA-Z0-9_]*\$
variable [a-zA-Z][a-zA-Z0-9_]*
whitespace [ \t]+

abs [Aa][Bb][Ss]
acos [Aa][Cc][Oo][Ss]
alert [Aa][Ll][Ee][Rr][Tt]
and [Aa][Nn][Dd]
arc [Aa][Rr][Cc]
asc [Aa][Ss][Cc]
asin [Aa][Ss][Ii][Nn]
atan [Aa][Tt][Aa][Nn]
begincase [Bb][Ee][Gg][Ii][Nn][ \t]*[Cc][Aa][Ss][Ee]
black [Bb][Ll][Aa][Cc][Kk]([ \t]*\([ \t]*\))?
blue [Bb][Ll][Uu][Ee]([ \t]*\([ \t]*\))?
booleof [Ee][Oo][Ff]
call [Cc][Aa][Ll][Ll]
case [Cc][Aa][Ss][Ee]
catch [Cc][Aa][Tt][Cc][Hh]
ceil [Cc][Ee][Ii][Ll]
changedir [Cc][Hh][Aa][Nn][Gg][Ee][Dd][Ii][Rr]
chord [Cc][Hh][Oo][Rr][Dd]
chr [Cc][Hh][Rr]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
clear [Cc][Ll][Ee][Aa][Rr]
clg [Cc][Ll][Gg]
clickb [Cc][Ll][Ii][Cc][Kk][Bb]([ \t]*\([ \t]*\))?
clickclear [Cc][Ll][Ii][Cc][Kk][Cc][Ll][Ee][Aa][Rr]([ \t]*\([ \t]*\))?
clickx [Cc][Ll][Ii][Cc][Kk][Xx]([ \t]*\([ \t]*\))?
clicky [Cc][Ll][Ii][Cc][Kk][Yy]([ \t]*\([ \t]*\))?
close [Cc][Ll][Oo][Ss][Ee]
cls [Cc][Ll][Ss]
color [Cc][Oo][Ll][Oo][Uu]?[Rr]
confirm [Cc][Oo][Nn][Ff][Ii][Rr][Mm]
continuedo [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Dd][Oo]
continuefor [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ff][Oo][Rr]
continuewhile [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ww][Hh][Ii][Ll][Ee]
cos [Cc][Oo][Ss]
count [Cc][Oo][Uu][Nn][Tt]
countx [Cc][Oo][Uu][Nn][Tt][Xx]
currentdir [Cc][Uu][Rr][Rr][Ee][Nn][Tt][Dd][Ii][Rr]
cyan [Cc][Yy][Aa][Nn]([ \t]*\([ \t]*\))?
dark [Dd][Aa][Rr][Kk]
day [Dd][Aa][Yy]([ \t]*\([ \t]*\))?
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
debuginfo [Dd][Ee][Bb][Uu][Gg][Ii][Nn][Ff][Oo]
degrees [Dd][Ee][Gg][Rr][Ee][Ee][Ss]
dim [Dd][Ii][Mm]
dir [Dd][Ii][Rr]
do [Dd][Oo]
editvisible [Ee][Dd][Ii][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
else [Ee][Ll][Ss][Ee]
end [Ee][Nn][Dd]
endcase [Ee][Nn][Dd][ \t]*[Cc][Aa][Ss][Ee]
endfunction [Ee][Nn][Dd][ \t]*[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
endif [Ee][Nn][Dd][ \t]*[Ii][Ff]
endtry [Ee][Nn][Dd][ \t]*[Tt][Rr][Yy]
endsubroutine [Ee][Nn][Dd][ \t]*[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]
endwhile [Ee][Nn][Dd][ \t]*[Ww][Hh][Ii][Ll][Ee]
error_none [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Nn][Ee]([ \t]*\([ \t]*\))?
error_for1 [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Rr][1]([ \t]*\([ \t]*\))?
error_for2 [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Rr][2]([ \t]*\([ \t]*\))?
error_filenumber [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_fileopen [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_filenotopen [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Oo][Tt][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_filewrite [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Ww][Rr][Ii][Tt][Ee]([ \t]*\([ \t]*\))?
error_filereset [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Rr][Ee][Ss][Ee][Tt]([ \t]*\([ \t]*\))?
error_arraysizelarge [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ll][Aa][Rr][Gg][Ee]([ \t]*\([ \t]*\))?
error_arraysizesmall [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ss][Mm][Aa][Ll][Ll]([ \t]*\([ \t]*\))?
error_nosuchvariable [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Ss][Uu][Cc][Hh][Vv][Aa][Rr][Ii][Aa][Bb][Ll][Ee]([ \t]*\([ \t]*\))?
error_arrayindex [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx]([ \t]*\([ \t]*\))?
error_strneglen [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Nn][Ee][Gg][Ll][Ee][Nn]([ \t]*\([ \t]*\))?
error_strstart [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ss][Tt][Aa][Rr][Tt]([ \t]*\([ \t]*\))?
error_nonnumeric [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Nn][Nn][Uu][Mm][Ee][Rr][Ii][Cc]([ \t]*\([ \t]*\))?
error_rgb [Ee][Rr][Rr][Oo][Rr][_][Rr][Gg][Bb]([ \t]*\([ \t]*\))?
error_putbitformat [Ee][Rr][Rr][Oo][Rr][_][Pp][Uu][Tt][Bb][Ii][Tt][Ff][Oo][Rr][Mm][Aa][Tt]([ \t]*\([ \t]*\))?
error_polyarray [Ee][Rr][Rr][Oo][Rr][_][Pp][Oo][Ll][Yy][Aa][Rr][Rr][Aa][Yy]([ \t]*\([ \t]*\))?
error_polypoints [Ee][Rr][Rr][Oo][Rr][_][Pp][Oo][Ll][Yy][Pp][Oo][Ii][Nn][Tt][Ss]([ \t]*\([ \t]*\))?
error_imagefile [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ff][Ii][Ll][Ee]([ \t]*\([ \t]*\))?
error_spritenumber [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_spritena [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Aa]([ \t]*\([ \t]*\))?
error_spriteslice [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]([ \t]*\([ \t]*\))?
error_folder [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Ll][Dd][Ee][Rr]([ \t]*\([ \t]*\))?
error_infinity [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Ff][Ii][Nn][Ii][Tt][Yy]([ \t]*\([ \t]*\))?
error_dbopen [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_dbquery [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Qq][Uu][Ee][Rr][Yy]([ \t]*\([ \t]*\))?
error_dbnotopen [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_dbcolno [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Ll][Nn][Oo]([ \t]*\([ \t]*\))?
error_dbnotset [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt]([ \t]*\([ \t]*\))?
error_netsock [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk]([ \t]*\([ \t]*\))?
error_nethost [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Hh][Oo][Ss][Tt]([ \t]*\([ \t]*\))?
error_netconn [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Cc][Oo][Nn][Nn]([ \t]*\([ \t]*\))?
error_netread [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Rr][Ee][Aa][Dd]([ \t]*\([ \t]*\))?
error_netnone [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Nn][Oo][Nn][Ee]([ \t]*\([ \t]*\))?
error_netwrite [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]([ \t]*\([ \t]*\))?
error_netsockopt [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Oo][Pp][Tt]([ \t]*\([ \t]*\))?
error_netbind [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Bb][Ii][Nn][Dd]([ \t]*\([ \t]*\))?
error_netaccept [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Aa][Cc][Cc][Ee][Pp][Tt]([ \t]*\([ \t]*\))?
error_netsocknumber [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_permission [Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Rr][Mm][Ii][Ss][Ss][Ii][Oo][Nn]([ \t]*\([ \t]*\))?
error_imagesavetype [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Aa][Vv][Ee][Tt][Yy][Pp][Ee]([ \t]*\([ \t]*\))?
error_divzero [Ee][Rr][Rr][Oo][Rr][_][Dd][Ii][Vv][Zz][Ee][Rr][Oo]([ \t]*\([ \t]*\))?
error_byref [Ee][Rr][Rr][Oo][Rr][_][Bb][Yy][Rr][Ee][Ff]([ \t]*\([ \t]*\))?
error_byreftype [Ee][Rr][Rr][Oo][Rr][_][Bb][Yy][Rr][Ee][Ff][Tt][Yy][Pp][Ee]([ \t]*\([ \t]*\))?
error_freefile [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]([ \t]*\([ \t]*\))?
error_freenet [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Nn][Ee][Tt]([ \t]*\([ \t]*\))?
error_freedb [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb]([ \t]*\([ \t]*\))?
error_dbconnnumber [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Nn][Nn][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_freedbset [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]([ \t]*\([ \t]*\))?
error_dbsetnumber [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Ss][Ee][Tt][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_dbnotsetrow [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt][Rr][Oo][Ww]([ \t]*\([ \t]*\))?
error_penwidth [Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]([ \t]*\([ \t]*\))?
error_colornumber [Ee][Rr][Rr][Oo][Rr][_][Cc][Oo][Ll][Oo][Rr][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_arrayindexmissing [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx][Mm][Ii][Ss][Ss][Ii][Nn][Gg]([ \t]*\([ \t]*\))?
error_imagescale [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Cc][Aa][Ll][Ee]([ \t]*\([ \t]*\))?
error_fontsize [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Nn][Tt][Ss][Ii][Zz][Ee]([ \t]*\([ \t]*\))?
error_fontweight [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Nn][Tt][Ww][Ee][Ii][Gg][Hh][Tt]([ \t]*\([ \t]*\))?
error_radixstring [Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx][Ss][Tt][Rr][Ii][Nn][Gg]([ \t]*\([ \t]*\))?
error_radix [Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx]([ \t]*\([ \t]*\))?
error_logrange [Ee][Rr][Rr][Oo][Rr][_][Ll][Oo][Gg][Rr][Aa][Nn][Gg][Ee]([ \t]*\([ \t]*\))?
error_stringmaxlen [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][Mm][Aa][Xx][Ll][Ee][Nn]([ \t]*\([ \t]*\))?
error_notanumber [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Tt][Aa][Nn][Uu][Mm][Bb][Ee][Rr]([ \t]*\([ \t]*\))?
error_printernoton [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Nn]([ \t]*\([ \t]*\))?
error_printernotoff [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Ff][Ff]([ \t]*\([ \t]*\))?
error_printeropen [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_wavfileformat [Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Ff][Ii][Ll][Ee][Ff][Oo][Rr][Mm][Aa][Tt]([ \t]*\([ \t]*\))?
error_wavnotopen [Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Nn][Oo][Tt][Oo][Pp][Ee][Nn]([ \t]*\([ \t]*\))?
error_notimplemented [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Tt][Ii][Mm][Pp][Ll][Ee][Mm][Ee][Nn][Tt][Ee][Dd]([ \t]*\([ \t]*\))?
exists [Ee][Xx][Ii][Ss][Tt][Ss]
exitdo [Ee][Xx][Ii][Tt][ \t]*[Dd][Oo]
exitfor [Ee][Xx][Ii][Tt][ \t]*[Ff][Oo][Rr]
exitwhile [Ee][Xx][Ii][Tt][ \t]*[Ww][Hh][Ii][Ll][Ee]
exp [Ee][Xx][Pp]
explode [Ee][Xx][Pp][Ll][Oo][Dd][Ee]
explodex [Ee][Xx][Pp][Ll][Oo][Dd][Ee][Xx]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
floor [Ff][Ll][Oo][Oo][Rr]
font [Ff][Oo][Nn][Tt]
for [Ff][Oo][Rr]
freedb [Ff][Rr][Ee][Ee][Dd][Bb]([ \t]*\([ \t]*\))?
freedbset [Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]
freefile [Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]([ \t]*\([ \t]*\))?
freenet [Ff][Rr][Ee][Ee][Nn][Ee][Tt]([ \t]*\([ \t]*\))?
frombinary [Ff][Rr][Oo][Mm][Bb][Ii][Nn][Aa][Rr][Yy]
fromhex [Ff][Rr][Oo][Mm][Hh][Ee][Xx]
fromoctal [Ff][Rr][Oo][Mm][Oo][Cc][Tt][Aa][Ll]
fromradix [Ff][Rr][Oo][Mm][Rr][Aa][Dd][Ii][Xx]
function [Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
getbrushcolor [Gg][Ee][Tt][Bb][Rr][Uu][Ss][Hh][Cc][Oo][Ll][Oo][Rr]([ \t]*\([ \t]*\))?
getcolor [Gg][Ee][Tt][Cc][Oo][Ll][Oo][Rr]([ \t]*\([ \t]*\))?
getpenwidth [Gg][Ee][Tt][Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]([ \t]*\([ \t]*\))?
getsetting [Gg][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
getslice [Gg][Ee][Tt][Ss][Ll][Ii][Cc][Ee]
global [Gg][Ll][Oo][Bb][Aa][Ll]
gosub [Gg][Oo][Ss][Uu][Bb]
goto [Gg][Oo][Tt][Oo]
graphheight [Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]([ \t]*\([ \t]*\))?
graphsize [Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]
graphvisible [Gg][Rr][Aa][Pp][Hh][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
graphwidth [Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]([ \t]*\([ \t]*\))?
gray [Gg][Rr][AaEe][Yy]([ \t]*\([ \t]*\))?
green [Gg][Rr][Ee][Ee][Nn]([ \t]*\([ \t]*\))?
hour [Hh][Oo][Uu][Rr]([ \t]*\([ \t]*\))?
if [Ii][Ff]
imgload [Ii][Mm][Gg][Ll][Oo][Aa][Dd]
imgsave [Ii][Mm][Gg][Ss][Aa][Vv][Ee]
implode [Ii][Mm][Pp][Ll][Oo][Dd][Ee]
input [Ii][Nn][Pp][Uu][Tt]
include [Ii][Nn][Cc][Ll][Uu][Dd][Ee]
instr [Ii][Nn][Ss][Tt][Rr]
instrx [Ii][Nn][Ss][Tt][Rr][Xx]
key [Kk][Ee][Yy]([ \t]*\([ \t]*\))?
kill [Kk][Ii][Ll][Ll]
label ^[ \t]*{variable}:
lasterror [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr]([ \t]*\([ \t]*\))?
lasterrorextra [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ee][Xx][Tt][Rr][Aa]([ \t]*\([ \t]*\))?
lasterrorline [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ll][Ii][Nn][Ee]([ \t]*\([ \t]*\))?
lasterrormessage [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Mm][Ee][Ss][Ss][Aa][Gg][Ee]([ \t]*\([ \t]*\))?
left [Ll][Ee][Ff][Tt]
length [Ll][Ee][Nn][Gg][Tt][Hh]
let [Ll][Ee][Tt]
line [Ll][Ii][Nn][Ee]
log [Ll][Oo][Gg]
logten [Ll][Oo][Gg][1][0]
lower [Ll][Oo][Ww][Ee][Rr]
md5 [Mm][Dd][5]
mid [Mm][Ii][Dd]
midx [Mm][Ii][Dd][Xx]
minute [Mm][Ii][Nn][Uu][Tt][Ee]([ \t]*\([ \t]*\))?
month [Mm][Oo][Nn][Tt][Hh]([ \t]*\([ \t]*\))?
mouseb [Mm][Oo][Uu][Ss][Ee][Bb]([ \t]*\([ \t]*\))?
mousex [Mm][Oo][Uu][Ss][Ee][Xx]([ \t]*\([ \t]*\))?
mousey [Mm][Oo][Uu][Ss][Ee][Yy]([ \t]*\([ \t]*\))?
msec [Mm][Ss][Ee][Cc]([ \t]*\([ \t]*\))?
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
orange [Oo][Rr][Aa][Nn][Gg][Ee]([ \t]*\([ \t]*\))?
ostype [Oo][Ss][Tt][Yy][Pp][Ee]([ \t]*\([ \t]*\))?
outputvisible [Oo][Uu][Tt][Pp][Uu][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
pause [Pp][Aa][Uu][Ss][Ee]
penwidth [Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]
pi [Pp][Ii]([ \t]*\([ \t]*\))?
pie [Pp][Ii][Ee]
pixel [Pp][Ii][Xx][Ee][Ll]
plot [Pp][Ll][Oo][Tt]
poly [Pp][Oo][Ll][Yy]
portin [Pp][Oo][Rr][Tt][Ii][Nn]
portout [Pp][Oo][Rr][Tt][Oo][Uu][Tt]
print [Pp][Rr][Ii][Nn][Tt]
printercancel [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Cc][Aa][Nn][Cc][Ee][Ll]
printeroff [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Oo][Ff][Ff]
printeron [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Oo][Nn]
printerpage [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Pp][Aa][Gg][Ee]
prompt [Pp][Rr][Oo][Mm][Pp][Tt]
purple [Pp][Uu][Rr][Pp][Ll][Ee]([ \t]*\([ \t]*\))?
putslice [Pp][Uu][Tt][Ss][Ll][Ii][Cc][Ee]
radians [Rr][Aa][Dd][Ii][Aa][Nn][Ss]
rand [Rr][Aa][Nn][Dd]([ \t]*\([ \t]*\))?
read [Rr][Ee][Aa][Dd]
readbyte [Rr][Ee][Aa][Dd][Bb][Yy][Tt][Ee]
readline [Rr][Ee][Aa][Dd][Ll][Ii][Nn][Ee]
rect [Rr][Ee][Cc][Tt]
red [Rr][Ee][Dd]([ \t]*\([ \t]*\))?
redim [Rr][Ee][Dd][Ii][Mm]
ref [Rr][Ee][Ff]
refresh [Rr][Ee][Ff][Rr][Ee][Ss][Hh]
regexminimal [Rr][Ee][Gg][Ee][Xx][Mm][Ii][Nn][Ii][Mm][Aa][Ll]
rem (:[ \t]*)?[Rr][Ee][Mm](([ \t:].+)|([ \t:]*))$
remalt (:[ \t]*)?#.*$
replace [Rr][Ee][Pp][Ll][Aa][Cc][Ee]
replacex [Rr][Ee][Pp][Ll][Aa][Cc][Ee][Xx]
reset [Rr][Ee][Ss][Ee][Tt]
return [Rr][Ee][Tt][Uu][Rr][Nn]
rgb [Rr][Gg][Bb]
right [Rr][Ii][Gg][Hh][Tt]
say [Ss][Aa][Yy]
second [Ss][Ee][Cc][Oo][Nn][Dd]([ \t]*\([ \t]*\))?
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
spritepoly [Ss][Pp][Rr][Ii][Tt][Ee][Pp][Oo][Ll][Yy]
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
textheight [Tt][Ee][Xx][Tt][Hh][Ee][Ii][Gg][Hh][Tt]([ \t]*\([ \t]*\))?
then [Tt][Hh][Ee][Nn]
throwerror [Tt][Hh][Rr][Oo][Ww][Ee][Rr][Rr][Oo][Rr]
to [Tt][Oo]
tobinary [Tt][Oo][Bb][Ii][Nn][Aa][Rr][Yy]
tofloat [Ff][Ll][Oo][Aa][Tt]
tohex [Tt][Oo][Hh][Ee][Xx]
toint [Ii][Nn][Tt]
tooctal [Tt][Oo][Oo][Cc][Tt][Aa][Ll]
toradix [Tt][Oo][Rr][Aa][Dd][Ii][Xx]
tostring [Ss][Tt][Rr][Ii][Nn][Gg]
try [Tt][Rr][Yy]
until [Uu][Nn][Tt][Ii][Ll]
upper [Uu][Pp][Pp][Ee][Rr]
version [Vv][Ee][Rr][Ss][Ii][Oo][Nn]([ \t]*\([ \t]*\))?
volume [Vv][Oo][Ll][Uu][Mm][Ee]
warning_typeconv [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Tt][Yy][Pp][Ee][Cc][Oo][Nn][Vv]([ \t]*\([ \t]*\))?
warning_wavnotseekable [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ww][Aa][Vv][Nn][Oo][Tt][Ss][Ee][Ee][Kk][Aa][Bb][Ll][Ee]([ \t]*\([ \t]*\))?
warning_wavnoduration [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ww][Aa][Vv][Nn][Oo][Dd][Uu][Rr][Aa][Tt][Ii][Oo][Nn]([ \t]*\([ \t]*\))?
wavlength [Ww][Aa][Vv][Ll][Ee][Nn][Gg][Tt][Hh]([ \t]*\([ \t]*\))?
wavpause [Ww][Aa][Vv][Pp][Aa][Uu][Ss][Ee]
wavplay [Ww][Aa][Vv][Pp][Ll][Aa][Yy]
wavpos [Ww][Aa][Vv][Pp][Oo][Ss]([ \t]*\([ \t]*\))?
wavseek [Ww][Aa][Vv][Ss][Ee][Ee][Kk]
wavstate [Ww][Aa][Vv][Ss][Tt][Aa][Tt][Ee]([ \t]*\([ \t]*\))?
wavstop [Ww][Aa][Vv][Ss][Tt][Oo][Pp]
wavwait [Ww][Aa][Vv][Ww][Aa][Ii][Tt]
while [Ww][Hh][Ii][Ll][Ee]
white [Ww][Hh][Ii][Tt][Ee]([ \t]*\([ \t]*\))?
write [Ww][Rr][Ii][Tt][Ee]
writebyte [Ww][Rr][Ii][Tt][Ee][Bb][Yy][Tt][Ee]
writeline [Ww][Rr][Ii][Tt][Ee][Ll][Ii][Nn][Ee]
xor [Xx][Oo][Rr]
year [Yy][Ee][Aa][Rr]([ \t]*\([ \t]*\))?
yellow [Yy][Ee][Ll][Ll][Oo][Ww]([ \t]*\([ \t]*\))?

%%

 /**********************************************************************/
 /* data type rules                                                    */
 /**********************************************************************/

{constfalse}				{ count(); return B256BOOLFALSE; }
{consttrue}					{ count(); return B256BOOLTRUE; }
{constinteger}				{ count(); yylval.number = atoi(yytext); return B256INTEGER; }
{constdecimal}				{ count(); yylval.floatnum = atof(yytext); return B256FLOAT; }
{constbinary}				{
								int len;
								count();
								len = strlen(yytext);
								yylval.string = strdup(yytext + 2);
								return B256BINCONST;
}
{consthex}				{
								int len;
								count();
								len = strlen(yytext);
								yylval.string = strdup(yytext + 2);
								return B256HEXCONST;
}
{constoctal}				{
								int len;
								count();
								len = strlen(yytext);
								yylval.string = strdup(yytext + 2);
								return B256OCTCONST;
}
{conststring}				{
								int len;
								count();
								len = strlen(yytext);
								yylval.string = strdup(yytext + 1);
								yylval.string[len - 2] = 0;
								return B256STRING;
}

 /**********************************************************************/
 /* compile time rules */
 /**********************************************************************/

{include} 					{ 
								count();
								BEGIN(INCLUDE);
							}

 /**********************************************************************/
 /* functions and constants rules                                      */
 /**********************************************************************/

{abs}						{ count(); return B256ABS; }
{acos}						{ count(); return B256ACOS; }
{arc}						{ count(); return B256ARC; }
{asc}						{ count(); return B256ASC; }
{asin}						{ count(); return B256ASIN; }
{atan}						{ count(); return B256ATAN; }
{black}						{ count(); return B256BLACK; }
{blue}						{ count(); return B256BLUE; }
{booleof}					{ count(); return B256BOOLEOF; }
{ceil}						{ count(); return B256CEIL; }
{chr}						{ count(); return B256CHR; }
{clear}						{ count(); return B256CLEAR; }
{clickb}					{ count(); return B256CLICKB; }
{clickx}					{ count(); return B256CLICKX; }
{clicky}					{ count(); return B256CLICKY; }
{confirm}					{ count(); return B256CONFIRM; }
{cos}						{ count(); return B256COS; }
{count}						{ count(); return B256COUNT; }
{countx}					{ count(); return B256COUNTX; }
{currentdir}				{ count(); return B256CURRENTDIR; }
{cyan}						{ count(); return B256CYAN; }
{dark}{blue}				{ count(); return B256DARKBLUE; }
{dark}{cyan}				{ count(); return B256DARKCYAN; }
{dark}{gray}				{ count(); return B256DARKGREY; }
{dark}{green}				{ count(); return B256DARKGREEN; }
{dark}{orange}				{ count(); return B256DARKORANGE; }
{dark}{purple}				{ count(); return B256DARKPURPLE; }
{dark}{red}					{ count(); return B256DARKRED; }
{dark}{yellow}				{ count(); return B256DARKYELLOW; }
{day}						{ count(); return B256DAY; }
{dbfloat}					{ count(); return B256DBFLOAT; }
{dbint}						{ count(); return B256DBINT; }
{dbnull}					{ count(); return B256DBNULL; }
{dbrow}						{ count(); return B256DBROW; }
{dbstring}					{ count(); return B256DBSTRING; }
{debuginfo}					{ count(); return B256DEBUGINFO; }
{degrees}					{ count(); return B256DEGREES; }
{dir}						{ count(); return B256DIR; }
{error_none}				{ count(); return B256ERROR_NONE; }
{error_for1} 				{ count(); return B256ERROR_FOR1; }
{error_for2}				{ count(); return B256ERROR_FOR2; }
{error_filenumber}			{ count(); return B256ERROR_FILENUMBER; }
{error_fileopen}			{ count(); return B256ERROR_FILEOPEN; }
{error_filenotopen}			{ count(); return B256ERROR_FILENOTOPEN; }
{error_filewrite}			{ count(); return B256ERROR_FILEWRITE; }
{error_filereset}			{ count(); return B256ERROR_FILERESET; }
{error_arraysizelarge}		{ count(); return B256ERROR_ARRAYSIZELARGE; }
{error_arraysizesmall}		{ count(); return B256ERROR_ARRAYSIZESMALL; }
{error_nosuchvariable}		{ count(); return B256ERROR_NOSUCHVARIABLE; }
{error_arrayindex}			{ count(); return B256ERROR_ARRAYINDEX; }
{error_strneglen}			{ count(); return B256ERROR_STRNEGLEN; }
{error_strstart}			{ count(); return B256ERROR_STRSTART; }
{error_nonnumeric}			{ count(); return B256ERROR_NONNUMERIC; }
{error_rgb}					{ count(); return B256ERROR_RGB; }
{error_putbitformat}		{ count(); return B256ERROR_PUTBITFORMAT; }
{error_polyarray}			{ count(); return B256ERROR_POLYARRAY; }
{error_polypoints}			{ count(); return B256ERROR_POLYPOINTS; }
{error_imagefile}			{ count(); return B256ERROR_IMAGEFILE; }
{error_spritenumber}		{ count(); return B256ERROR_SPRITENUMBER; }
{error_spritena}			{ count(); return B256ERROR_SPRITENA; }
{error_spriteslice}			{ count(); return B256ERROR_SPRITESLICE; }
{error_folder}				{ count(); return B256ERROR_FOLDER; }
{error_infinity}			{ count(); return B256ERROR_INFINITY; }
{error_dbopen}				{ count(); return B256ERROR_DBOPEN; }
{error_dbquery}				{ count(); return B256ERROR_DBQUERY; }
{error_dbnotopen}			{ count(); return B256ERROR_DBNOTOPEN; }
{error_dbcolno}				{ count(); return B256ERROR_DBCOLNO; }
{error_dbnotset}			{ count(); return B256ERROR_DBNOTSET; }
{error_netsock}				{ count(); return B256ERROR_NETSOCK; }
{error_nethost}				{ count(); return B256ERROR_NETHOST; }
{error_netconn}				{ count(); return B256ERROR_NETCONN; }
{error_netread}				{ count(); return B256ERROR_NETREAD; }
{error_netnone}				{ count(); return B256ERROR_NETNONE; }
{error_netwrite}			{ count(); return B256ERROR_NETWRITE; }
{error_netsockopt}			{ count(); return B256ERROR_NETSOCKOPT; }
{error_netbind}				{ count(); return B256ERROR_NETBIND; }
{error_netaccept}			{ count(); return B256ERROR_NETACCEPT; }
{error_netsocknumber}		{ count(); return B256ERROR_NETSOCKNUMBER; }
{error_permission}			{ count(); return B256ERROR_PERMISSION; }
{error_imagesavetype}		{ count(); return B256ERROR_IMAGESAVETYPE; }
{error_divzero}				{ count(); return B256ERROR_DIVZERO; }
{error_byref}				{ count(); return B256ERROR_BYREF; }
{error_byreftype}			{ count(); return B256ERROR_BYREFTYPE; }
{error_freefile}			{ count(); return B256ERROR_FREEFILE; }
{error_freenet}				{ count(); return B256ERROR_FREENET; }
{error_freedb}				{ count(); return B256ERROR_FREEDB; }
{error_dbconnnumber}		{ count(); return B256ERROR_DBCONNNUMBER; }
{error_freedbset}			{ count(); return B256ERROR_FREEDBSET; }
{error_dbsetnumber}			{ count(); return B256ERROR_DBSETNUMBER; }
{error_dbnotsetrow}			{ count(); return B256ERROR_DBNOTSETROW; }
{error_penwidth}			{ count(); return B256ERROR_PENWIDTH; }
{error_colornumber}			{ count(); return B256ERROR_COLORNUMBER; }
{error_arrayindexmissing}	{ count(); return B256ERROR_ARRAYINDEXMISSING; }
{error_imagescale}			{ count(); return B256ERROR_IMAGESCALE; }
{error_fontsize}			{ count(); return B256ERROR_FONTSIZE; }
{error_fontweight}			{ count(); return B256ERROR_FONTWEIGHT; }
{error_radixstring}			{ count(); return B256ERROR_RADIXSTRING; }
{error_radix}				{ count(); return B256ERROR_RADIX; }
{error_logrange}			{ count(); return B256ERROR_LOGRANGE; }
{error_stringmaxlen}		{ count(); return B256ERROR_STRINGMAXLEN; }
{error_notanumber}			{ count(); return B256ERROR_NOTANUMBER; }
{error_printernoton}		{ count(); return B256ERROR_PRINTERNOTON; }
{error_printernotoff}		{ count(); return B256ERROR_PRINTERNOTOFF; }
{error_printeropen}			{ count(); return B256ERROR_PRINTEROPEN; }
{error_wavfileformat}		{ count(); return B256ERROR_WAVFILEFORMAT; }
{error_wavnotopen}			{ count(); return B256ERROR_WAVNOTOPEN; }
{error_notimplemented}		{ count(); return B256ERROR_NOTIMPLEMENTED; }
{exists}					{ count(); return B256EXISTS; }
{exp}						{ count(); return B256EXP; }
{explode}					{ count(); return B256EXPLODE; }
{explodex}					{ count(); return B256EXPLODEX; }
{floor}						{ count(); return B256FLOOR; }
{freedb}					{ count(); return B256FREEDB; }
{freedbset}					{ count(); return B256FREEDBSET; }
{freefile}					{ count(); return B256FREEFILE; }
{freenet}					{ count(); return B256FREENET; }
{frombinary}				{ count(); return B256FROMBINARY; }
{fromhex}					{ count(); return B256FROMHEX; }
{fromoctal}					{ count(); return B256FROMOCTAL; }
{fromradix}					{ count(); return B256FROMRADIX; }
{getbrushcolor}				{ count(); return B256GETBRUSHCOLOR; }
{getcolor}					{ count(); return B256GETCOLOR; }
{getpenwidth}				{ count(); return B256GETPENWIDTH; }
{getsetting}				{ count(); return B256GETSETTING; }
{getslice}					{ count(); return B256GETSLICE; }
{graphheight}				{ count(); return B256GRAPHHEIGHT; }
{graphwidth}				{ count(); return B256GRAPHWIDTH; }
{gray}						{ count(); return B256GREY; }
{green}						{ count(); return B256GREEN; }
{hour}						{ count(); return B256HOUR; }
{implode}					{ count(); return B256IMPLODE; }
{instr}						{ count(); return B256INSTR; }
{instrx}					{ count(); return B256INSTRX; }
{key}						{ count(); return B256KEY; }
{lasterror}					{ count(); return B256LASTERROR; }
{lasterrorextra} 			{ count(); return B256LASTERROREXTRA; }
{lasterrorline}				{ count(); return B256LASTERRORLINE; }
{lasterrormessage}			{ count(); return B256LASTERRORMESSAGE; }
{left}						{ count(); return B256LEFT; }
{length}					{ count(); return B256LENGTH; }
{log}						{ count(); return B256LOG; }
{logten}					{ count(); return B256LOGTEN; }
{lower}						{ count(); return B256LOWER; }
{md5}						{ count(); return B256MD5; }
{mid}						{ count(); return B256MID; }
{midx}						{ count(); return B256MIDX; }
{minute}					{ count(); return B256MINUTE; }
{month}						{ count(); return B256MONTH; }
{mouseb}					{ count(); return B256MOUSEB; }
{mousex}					{ count(); return B256MOUSEX; }
{mousey}					{ count(); return B256MOUSEY; }
{msec}						{ count(); return B256MSEC; }
{netaddress}				{ count(); return B256NETADDRESS; }
{netdata}					{ count(); return B256NETDATA; }
{netread}					{ count(); return B256NETREAD; }
{orange}					{ count(); return B256ORANGE; }
{ostype}					{ count(); return B256OSTYPE; }
{pi}						{ count(); return B256PI; }
{pixel}						{ count(); return B256PIXEL; }
{portin}					{ count(); return B256PORTIN; }
{purple}					{ count(); return B256PURPLE; }
{radians}					{ count(); return B256RADIANS; }
{rand}						{ count(); return B256RAND; }
{read}						{ count(); return B256READ; }
{readbyte}					{ count(); return B256READBYTE; }
{readline}					{ count(); return B256READLINE; }
{red}						{ count(); return B256RED; }
{ref}						{ count(); return B256REF; }
{replace}					{ count(); return B256REPLACE; }
{replacex}					{ count(); return B256REPLACEX; }
{rgb}						{ count(); return B256RGB; }
{right}						{ count(); return B256RIGHT; }
{second}					{ count(); return B256SECOND; }
{sin}						{ count(); return B256SIN; }
{size}						{ count(); return B256SIZE; }
{spritecollide}				{ count(); return B256SPRITECOLLIDE; }
{spriteh}					{ count(); return B256SPRITEH; }
{spriter}					{ count(); return B256SPRITER; }
{sprites}					{ count(); return B256SPRITES; }
{spritev}					{ count(); return B256SPRITEV; }
{spritew}					{ count(); return B256SPRITEW; }
{spritex}					{ count(); return B256SPRITEX; }
{spritey}					{ count(); return B256SPRITEY; }
{sqr}						{ count(); return B256SQR; }
{sqrt}						{ count(); return B256SQR; }
{tan}						{ count(); return B256TAN; }
{textwidth}					{ count(); return B256TEXTWIDTH; }
{textheight}				{ count(); return B256TEXTHEIGHT; }
{tobinary}					{ count(); return B256TOBINARY; }
{tofloat}					{ count(); return B256TOFLOAT; }
{tohex}						{ count(); return B256TOHEX; }
{toint}						{ count(); return B256TOINT; }
{tooctal}					{ count(); return B256TOOCTAL; }
{toradix}					{ count(); return B256TORADIX; }
{tostring}					{ count(); return B256TOSTRING; }
{upper}						{ count(); return B256UPPER; }
{version}					{ count(); return B256VERSION; }
{warning_typeconv}			{ count(); return B256WARNING_TYPECONV; }
{warning_wavnoduration}		{ count(); return B256WARNING_WAVNODURATION; }
{warning_wavnotseekable}	{ count(); return B256WARNING_WAVNOTSEEKABLE; }
{wavlength}					{ count(); return B256WAVLENGTH; }
{wavpos}					{ count(); return B256WAVPOS; }
{wavstate}					{ count(); return B256WAVSTATE; }
{white}						{ count(); return B256WHITE; }
{year}						{ count(); return B256YEAR; }
{yellow}					{ count(); return B256YELLOW; }

 /**********************************************************************/
 /* statement rules                                                    */
 /* For statements with no arguments (or optionally with no arguments) */
 /* you NEED to include a version with a ':' on the end that unputs    */
 /* the ':' - multiple statements on a line and confusion with labels  */
 /* require this.                                                      */
 /**********************************************************************/

{alert}						{ count(); return B256ALERT; }
{begincase}					{ count(); return B256BEGINCASE; }
{begincase}":"				{ count(); unputcolon(); return B256BEGINCASE; }
{call}						{ count(); return B256CALL; }
{case}						{ count(); return B256CASE; }
{catch}						{ count(); return B256CATCH; }
{catch}":"					{ count(); unputcolon(); return B256CATCH; }
{changedir}					{ count(); return B256CHANGEDIR; }
{chord}						{ count(); return B256CHORD; }
{circle}					{ count(); return B256CIRCLE; }
{clg}						{ count(); return B256CLG; }
{clg}":"					{ count(); unputcolon(); return B256CLG; }
{clickclear}				{ count(); return B256CLICKCLEAR; }
{clickclear}":"				{ count(); unputcolon(); return B256CLICKCLEAR; }
{close}						{ count(); return B256CLOSE; }
{close}":"					{ count(); unputcolon(); return B256CLOSE; }
{cls}						{ count(); return B256CLS; }
{cls}":"					{ count(); unputcolon(); return B256CLS; }
{color}						{ count(); return B256SETCOLOR; }
{continuedo}				{ count(); return B256CONTINUEDO; }
{continuedo}":"				{ count(); unputcolon(); return B256CONTINUEDO; }
{continuefor}				{ count(); return B256CONTINUEFOR; }
{continuefor}":"			{ count(); unputcolon(); return B256CONTINUEFOR; }
{continuewhile}				{ count(); return B256CONTINUEWHILE; }
{continuewhile}":"			{ count(); unputcolon(); return B256CONTINUEWHILE; }
{dbclose}					{ count(); return B256DBCLOSE; }
{dbclose}":"				{ count(); unputcolon(); return B256DBCLOSE; }
{dbcloseset}				{ count(); return B256DBCLOSESET; }
{dbcloseset}":"				{ count(); unputcolon(); return B256DBCLOSESET; }
{dbexecute}					{ count(); return B256DBEXECUTE; }
{dbopen}					{ count(); return B256DBOPEN; }
{dbopenset}					{ count(); return B256DBOPENSET; }
{dim}						{ count(); return B256DIM; }
{do}						{ count(); return B256DO; }
{do}":"						{ count(); unputcolon(); return B256DO; }
{editvisible}				{ count(); return B256EDITVISIBLE; }
{else}						{ count(); return B256ELSE; }
{else}":"					{ count(); unputcolon(); return B256ELSE; }
{end}						{ count(); return B256END; }
{end}":"					{ count(); unputcolon(); return B256END; }
{endcase}					{ count(); return B256ENDCASE; }
{endcase}":"				{ count(); unputcolon(); return B256ENDCASE; }
{endfunction}				{ count(); return B256ENDFUNCTION; }
{endfunction}":"			{ count(); unputcolon(); return B256ENDFUNCTION; }
{endif}						{ count(); return B256ENDIF; }
{endif}":"					{ count(); unputcolon(); return B256ENDIF; }
{endsubroutine}				{ count(); return B256ENDSUBROUTINE; }
{endsubroutine}":"			{ count(); unputcolon(); return B256ENDSUBROUTINE; }
{endtry}					{ count(); return B256ENDTRY; }
{endtry}":"					{ count(); unputcolon(); return B256ENDTRY; }
{endwhile}					{ count(); return B256ENDWHILE; }
{endwhile}":"				{ count(); unputcolon(); return B256ENDWHILE; }
{exitdo}					{ count(); return B256EXITDO; }
{exitdo}":"					{ count(); unputcolon(); return B256EXITDO; }
{exitfor}					{ count(); return B256EXITFOR; }
{exitfor}":"				{ count(); unputcolon(); return B256EXITFOR; }
{exitwhile}					{ count(); return B256EXITWHILE; }
{exitwhile}":"				{ count(); unputcolon(); return B256EXITWHILE; }
{fastgraphics}				{ count(); return B256FASTGRAPHICS; }
{fastgraphics}":"			{ count(); unputcolon(); return B256FASTGRAPHICS; }
{font}						{ count(); return B256FONT; }
{for}						{ count(); return B256FOR; }
{function}					{ count(); return B256FUNCTION; }
{global}					{ count(); return B256GLOBAL; }
{gosub}						{ count(); return B256GOSUB; }
{goto}						{ count(); return B256GOTO; }
{graphsize}					{ count(); return B256GRAPHSIZE; }
{graphvisible}				{ count(); return B256GRAPHVISIBLE; }
{if}						{ count(); return B256IF; }
{imgload}					{ count(); return B256IMGLOAD; }
{imgsave}					{ count(); return B256IMGSAVE; }
{input}						{ count(); return B256INPUT; }
{kill}						{ count(); return B256KILL; }
{let}						{ count(); return B256LET; }
{line}						{ count(); return B256LINE; }
{netclose}					{ count(); return B256NETCLOSE; }
{netclose}":"				{ count(); unputcolon(); return B256NETCLOSE; }
{netconnect}				{ count(); return B256NETCONNECT; }
{netlisten}					{ count(); return B256NETLISTEN; }
{netwrite}					{ count(); return B256NETWRITE; }
{next}						{ count(); return B256NEXT; }
{offerror}					{ count(); return B256OFFERROR; }
{offerror}":"				{ count(); unputcolon(); return B256OFFERROR; }
{onerror}					{ count(); return B256ONERROR; }
{open}						{ count(); return B256OPEN; }
{openb}						{ count(); return B256OPENB; }
{outputvisible}				{ count(); return B256OUTPUTVISIBLE; }
{pause}						{ count(); return B256PAUSE; }
{penwidth}					{ count(); return B256PENWIDTH; }
{pie}						{ count(); return B256PIE; }
{plot}						{ count(); return B256PLOT; }
{poly}						{ count(); return B256POLY; }
{portout}					{ count(); return B256PORTOUT; }
{print}						{ count(); return B256PRINT; }
{print}":"					{ count(); unputcolon(); return B256PRINT; }
{printercancel}				{ count(); return B256PRINTERCANCEL; }
{printercancel}":"			{ count(); unputcolon(); return B256PRINTERCANCEL; }
{printeroff}				{ count(); return B256PRINTEROFF; }
{printeroff}":"				{ count(); unputcolon(); return B256PRINTEROFF; }
{printeron}					{ count(); return B256PRINTERON; }
{printeron}":"				{ count(); unputcolon(); return B256PRINTERON; }
{printerpage}				{ count(); return B256PRINTERPAGE; }
{printerpage}":"			{ count(); unputcolon(); return B256PRINTERPAGE; }
{prompt}					{ count(); return B256PROMPT; }
{putslice}					{ count(); return B256PUTSLICE; }
{rect}						{ count(); return B256RECT; }
{redim}						{ count(); return B256REDIM; }
{refresh}					{ count(); return B256REFRESH; }
{refresh}":"				{ count(); unputcolon(); return B256REFRESH; }
{regexminimal}				{ count(); return B256REGEXMINIMAL; }
{rem}						{ count(); return B256REM; }
{remalt}					{ count(); return B256REM; }
{reset}						{ count(); return B256RESET; }
{reset}":"					{ count(); unputcolon(); return B256RESET; }
{return}					{ count(); return B256RETURN; }
{return}":"					{ count(); unputcolon(); return B256RETURN; }
{say}						{ count(); return B256SAY; }
{seek}						{ count(); return B256SEEK; }
{setsetting}				{ count(); return B256SETSETTING; }
{sound}						{ count(); return B256SOUND; }
{spritedim}					{ count(); return B256SPRITEDIM; }
{spritehide}				{ count(); return B256SPRITEHIDE; }
{spriteload}				{ count(); return B256SPRITELOAD; }
{spritemove}				{ count(); return B256SPRITEMOVE; }
{spriteplace}				{ count(); return B256SPRITEPLACE; }
{spritepoly}				{ count(); return B256SPRITEPOLY; }
{spriteshow}				{ count(); return B256SPRITESHOW; }
{spriteslice}				{ count(); return B256SPRITESLICE; }
{stamp}						{ count(); return B256STAMP; }
{step}						{ count(); return B256STEP; }
{subroutine}				{ count(); return B256SUBROUTINE; }
{system}					{ count(); return B256SYSTEM; }
{text}						{ count(); return B256TEXT; }
{then}						{ count(); return B256THEN; }
{then}":"					{ count(); unputcolon(); return B256THEN; }
{throwerror}				{ count(); return B256THROWERROR; }
{to}						{ count(); return B256TO; }
{try}						{ count(); return B256TRY; }
{try}":"					{ count(); unputcolon(); return B256TRY; }
{until}						{ count(); return B256UNTIL; }
{volume}					{ count(); return B256VOLUME; }
{wavpause}					{ count(); return B256WAVPAUSE; }
{wavpause}":"				{ count(); unputcolon(); return B256WAVPAUSE; }
{wavplay}					{ count(); return B256WAVPLAY; }
{wavplay}":"				{ count(); unputcolon(); return B256WAVPLAY; }
{wavseek}					{ count(); return B256WAVSEEK; }
{wavstop}					{ count(); return B256WAVSTOP; }
{wavstop}":"				{ count(); unputcolon(); return B256WAVSTOP; }
{wavwait}					{ count(); return B256WAVWAIT; }
{wavwait}":"				{ count(); unputcolon(); return B256WAVWAIT; }
{while}						{ count(); return B256WHILE; }
{write}						{ count(); return B256WRITE; }
{writebyte}					{ count(); return B256WRITEBYTE; }
{writeline}					{ count(); return B256WRITELINE; }

 /**********************************************************************/
 /* operator rules                                                     */
 /**********************************************************************/

{and}						{ count(); return B256AND; }
{not}						{ count(); return B256NOT; }
{or}						{ count(); return B256OR; }
{xor}						{ count(); return B256XOR; }
">="						{ count(); return B256GTE; }
"<="						{ count(); return B256LTE; }
"<>"						{ count(); return B256NE; }
"+"							{ count(); return '+'; }
"++"						{ count(); return B256ADD1; }
"+="						{ count(); return B256ADDEQUAL; }
"-"							{ count(); return '-'; }
"--"						{ count(); return B256SUB1; }
"-="						{ count(); return B256SUBEQUAL; }
"*"							{ count(); return '*'; }
"*="						{ count(); return B256MULEQUAL; }
"\\"						{ count(); return B256INTDIV; }
"\%"						{ count(); return B256MOD; }
"|"							{ count(); return B256BINARYOR; }
"&"							{ count(); return B256BINARYAND; }
"~"							{ count(); return B256BINARYNOT; }
"/"							{ count(); return '/'; }
"/="						{ count(); return B256DIVEQUAL; }
"^"							{ count(); return '^'; }
"="							{ count(); return '='; }
"<"							{ count(); return '<'; }
">"							{ count(); return '>'; }
","							{ count(); return ','; }
";"							{ count(); return ';'; }
":"							{ count(); return ':'; }
"("							{ count(); return '('; }
")"							{ count(); return ')'; }
"{"							{ count(); return '{'; }
"}"							{ count(); return '}'; }
"["							{ count(); return '['; }
"]"							{ count(); return ']'; }

{label} {
	char *temp;
	char *c, *d;

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
	yylval.number = getSymbol(temp); // get existing or create new
	free(temp);
	//
	return B256LABEL;
}
	
{variable} {
	count();
	yylval.number = getSymbol(yytext);
	return B256VARIABLE;
}

{stringvar} {
	count();
	yylval.number = getSymbol(yytext);
	return B256STRINGVAR;
}

"\n"	{ return '\n'; }

{whitespace}	{
	/* eat the whitespace at the begining of a token - but count */
	count();
}

.	{
	/* we have a single character - count it and send it through */
 	column++;
 	return yytext[0];
}

<INCLUDE>[ \t]*  	{
    // eat the whitespace before the file
}

<INCLUDE>((\"[^\"\n]*\")|(\'[^\'\n]*\'))   {
	// got the include file name (with quotes)
	//original code from http://flex.sourceforge.net/manual/Multiple-Input-Buffers.html
               
	if ( numincludes >= MAX_INCLUDE_DEPTH ) {
		BEGIN(INITIAL);
		errorcode = COMPERR_INCLUDEDEPTH;
		return -1;
	}
     
	include_stack[numincludes] = YY_CURRENT_BUFFER;
	
	char *filename = strdup(yytext+1);
	filename[strlen(filename)-1] = 0x00;
                  
	yyin = fopen( filename, "r" );
	if ( ! yyin ) {
		BEGIN(INITIAL);
		errorcode = COMPERR_INCLUDEFILE;
		return -1;
	}     
     
	include_linenumber[numincludes] = linenumber;	// save last level's line number
	include_filename[numincludes] = lexingfilename;	// save the previous file name to the stack
	numincludes++;  

	lexingfilename = filename;
	addStringOp(OP_INCLUDEFILE, lexingfilename);
	linenumber = 1;  
	addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);

	yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
    BEGIN(INITIAL);
}


<<EOF>> {
	if ( numincludes == 0 ) {
		// eof of the top level program
		yyterminate();
	} else {

		// check for unclosed if and other structures
		if (testIfOnTable(numincludes)!=-1) {
			linenumber = testIfOnTable(numincludes);
			return testIfOnTableError(numincludes);
		}

		// eof of an include - return to the includer
		numincludes--;
		linenumber=include_linenumber[numincludes];
		addIntOp(OP_CURRLINE, numincludes * 0x1000000 + linenumber);
		
		free(lexingfilename);
		lexingfilename=include_filename[numincludes];
		addStringOp(OP_INCLUDEFILE, lexingfilename);
		
		yy_delete_buffer( YY_CURRENT_BUFFER );
		yy_switch_to_buffer(include_stack[numincludes] );
	}
}

%%

int
yywrap(void) {
	return 1;
}

void unputcolon() {
	unput(':');
	if (column>0) column--;
}

void count() {
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		column++;
	// printf("\"%s\" %i\n", yytext, column);
}

int basicParse(char *code) {

	//int t;
	//for(t=0;t<MAX_INCLUDE_DEPTH;t++) {
	//	yy_delete_buffer( include_stack[t]  );
	//	include_stack[t] = NULL;
	//}
	YY_BUFFER_STATE bp;
	
	numincludes=0;
	numparsewarnings=0;
	
	clearSymbolTable();
	clearLabelTable();
	clearIfTable();

	linenumber = 1;
	column = 0;
	errorcode = COMPERR_NONE;
	functionDefSymbol = -1;
	subroutineDefSymbol = -1;
	
	lexingfilename = strdup("");

	yy_delete_buffer(YY_CURRENT_BUFFER);

	bp = yy_scan_string(code);
	yy_switch_to_buffer(bp);
	yyparse();
	yy_delete_buffer(bp);
	

	if (testIfOnTable(0)!=-1 && errorcode==COMPERR_NONE) {
		linenumber = testIfOnTable(0);
		errorcode = testIfOnTableError(0);
	}

	return errorcode;
}
