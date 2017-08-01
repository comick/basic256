/**
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
 **  You should have received a copy of the GNU General Public License
 **  along with this program; if not, write to the Free Software
 **  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 **/

#include <QTextDocument>

#include "EditSyntaxHighlighter.h"

EditSyntaxHighlighter::EditSyntaxHighlighter(QTextDocument *parent)
	: 	QSyntaxHighlighter(parent) {
	initLabels();
	initKeywords();
	initConstants();
	initNumbers();
	initQuotes();
	initComments();
}

void EditSyntaxHighlighter::highlightBlock(const QString &text) {
	HighlightRule rule;

	VecHighlightRules::iterator sIt = m_standardRules.begin();
	VecHighlightRules::iterator sItEnd = m_standardRules.end();
	while (sIt != sItEnd) {
		rule = (*sIt);
		QRegExp expression(rule.pattern);
		int index = text.indexOf(expression);
		while (index >= 0) {
			int length = expression.matchedLength();
			if (format(index).foreground().color() != m_quoteFmt.foreground().color()) {
				// dont set the color if we are in quotes
				setFormat(index, length, rule.format);
			}  
			index = text.indexOf(expression, index + length);
		}
		++sIt;
	}
}

void EditSyntaxHighlighter::initKeywords() {

	m_keywordFmt.setForeground(Qt::darkBlue);
	m_keywordFmt.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;

	keywordPatterns
			<< "abs"								//abs
			<< "acos"								//acos
			<< "alert"								//alert
			<< "and"								//and
			<< "arc"								//arc
			<< "asc"								//asc
			<< "asin"								//asin
			<< "atan"								//atan
            << "begin[ \t]*case"					//begincase or begin case
			<< "call"								//call
			<< "case"								//case
			<< "catch"								//catch
			<< "ceil"								//ceil
			<< "changedir"							//changedir
			<< "chord"								//chord
			<< "chr"								//chr
			<< "circle"								//circle
			<< "clickb"								//clickb
			<< "clickclear"							//clickclear
			<< "clickx"								//clickx
			<< "clicky"								//clicky
			<< "clg"								//clg
			<< "close"								//close
			<< "cls"								//cls
			<< "color"								//color
			<< "colour"								//colour
			<< "confirm"							//confirm
            << "continue[ \t]*do"					//continuedo or continue do
            << "continue[ \t]*for"					//continuefor or continue for
            << "continue[ \t]*while"				//continuewhile or continue while
			<< "cos"								//cos
			<< "count"								//count
			<< "countx"								//countx
			<< "currentdir"							//currentdir
			<< "day"								//day
			<< "dbclose"							//dbclose
			<< "dbcloseset"							//dbcloseset
			<< "dbexecute"							//dbexecute
			<< "dbfloat"							//dbfloat
			<< "dbint"								//dbint
			<< "dbnull"								//dbnull
			<< "dbopen"								//dbopen
			<< "dbopenset"							//dbopenset
			<< "dbrow"								//dbrow
			<< "dbstring"							//dbstring
			<< "degrees"							//degrees
			<< "debuginfo"							//debuginfo
			<< "dim"								//dim
			<< "dir"								//dir
			<< "do"									//do
			<< "editvisible"						//editvisible
			<< "else"								//else
			<< "end"								//end
            << "end[ \t]*case"						//endcase or end case
            << "end[ \t]*function"					//endfunction or end function
            << "end[ \t]*if"						//endif or end if
            << "end[ \t]*subroutine"				//endsubroutine or end subroutine
            << "end[ \t]*try"						//endtry or end try
            << "end[ \t]*while"						//endwhile or end while
			<< "eof"								//eof
			<< "exists"								//exists
            << "exit[ \t]*do"						//exitdo or exit do
            << "exit[ \t]*for"						//exitfor or exit for
            << "exit[ \t]*while"					//exitwhile or exit while
			<< "exp"								//exp
			<< "explode"							//explode
			<< "explodex"							//explodex
			<< "fastgraphics"						//fastgraphics
			<< "fill"								//fill
			<< "float"								//float
			<< "floor"								//floor
			<< "font"								//font
			<< "for"								//for
			<< "freedb"								//freedb
			<< "freedbset"							//freedbset
			<< "freefile"							//freefile
			<< "freenet"							//freenet
			<< "frombinary"							//frombinary
			<< "fromhex"							//fromhex
			<< "fromoctal"							//fromoctal
			<< "fromradix"							//fromradix
			<< "function"							//function
			<< "getbrushcolor"						//getbrushcolor
			<< "getcolor"							//getcolor
			<< "getpenwidth"						//getpenwidth
			<< "getsetting"							//getsetting
			<< "getslice"							//getslice
			<< "global"								//global
			<< "gosub"								//gosub
			<< "goto"								//goto
			<< "graphheight"						//graphheignt
			<< "graphsize"							//graphsize
			<< "graphvisible"						//graphvisible
			<< "graphwidth"							//graphwidth
			<< "hour"								//hour
			<< "if"									//if
			<< "imgload"							//imgload
			<< "imgsave"							//imgsave
			<< "implode"							//implode
			<< "include"							//include
			<< "input"								//input
            << "input[ \t]*float"					//inputfloat
            << "input[ \t]*int(eger)?"				//inputint
            << "input[ \t]*string"					//inputstring
			<< "instr"								//instr
			<< "instrx"								//instrx
			<< "int"								//int
			<< "isnumeric"							//isnumeric
			<< "key"								//key
			<< "keypressed"							//keypressed
			<< "kill"								//kill
			<< "lasterror"							//lasterror
			<< "lasterrorextra"						//lasterrorextra
			<< "lasterrorline"						//lasterrorline
			<< "lasterrormessage"					//lasterrormessage
			<< "left"								//left
			<< "let"								//let
			<< "length"								//length
			<< "line"								//line
			<< "log"								//log
			<< "log10"								//log10
			<< "lower"								//lower
			<< "ltrim"								//ltrim
			<< "md5"								//md5
			<< "mid"								//mid
			<< "midx"								//midx
			<< "minute"								//minute
			<< "month"								//month
			<< "mouseb"								//mouseb
			<< "mousex"								//mousex
			<< "mousey"								//mousey
			<< "msec"								//msec
			<< "netaddress"							//netaddress
			<< "netclose"							//netclose
			<< "netconnect"							//netconnect
			<< "netdata"							//netdata
			<< "netlisten"							//netlisten
			<< "netread"							//netread
			<< "netwrite"							//netwrite
			<< "next"								//next
			<< "not"								//not
			<< "offerror"							//offerror
			<< "onerror"							//onerror
			<< "open"								//open
			<< "openb"								//openb
			<< "openserial"							//openserial
			<< "or"									//or
			<< "ostype"								//ostype
			<< "outputvisible"						//outputvisible
			<< "pause"								//pause
			<< "penwidth"							//penwidth
			<< "pie"								//pie
			<< "pixel"								//pixel
			<< "plot"								//plot
			<< "poly"								//poly
			<< "portin"								//portin
			<< "portout"							//portout
			<< "print"								//print
            << "printer[ \t]*cancel"				//printercancel or printer cancel
            << "printer[ \t]*off"					//printeroff or printer off
            << "printer[ \t]*on"					//printeron or printer on
            << "printer[ \t]*page"					//printerpage or printer page
			<< "prompt"								//prompt
			<< "putslice"							//putslice
			<< "radians"							//radians
			<< "rand"								//rand
			<< "read"								//read
			<< "readbyte"							//readbyte
			<< "readline"							//readline
			<< "rect"								//rect
			<< "ref"								//ref
			<< "redim"								//redim
			<< "refresh"							//refresh
			<< "regexminimal"						//regexminimal
			<< "replace"							//replace
			<< "replacex"							//replacex
			<< "reset"								//reset
			<< "return"								//return
			<< "rgb"								//rgb
			<< "right"								//right
			<< "ltrim"								//ltrim
			<< "say"								//say
			<< "second"								//second
			<< "seed"								//seed
			<< "seek"								//seek
			<< "serialize"							//serialize
			<< "setsetting"							//setsetting
			<< "sin"								//sin
			<< "size"								//size
			<< "sound"								//sound
			<< "spritedcollide"						//spritedcollide
			<< "spritedim"							//spritedim
			<< "spriteh"							//spriteh
			<< "spritehide"							//spritehide
			<< "spriteload"							//spriteload
			<< "spritemove"							//spritemove
			<< "spriteo"							//spriteo
			<< "spriteplace"						//spriteplace
			<< "spritepoly"							//spritepoly
			<< "spriter"							//spriter
			<< "sprites"							//sprites
			<< "spriteshow"							//spriteshow
			<< "spriteslice"						//spriteslice
			<< "spritev"							//spritev
			<< "spritew"							//spritew
			<< "spritex"							//spritex
			<< "spritey"							//spritey
			<< "sqr"								//sqr
			<< "sqrt"								//sqrt
			<< "stamp"								//stamp
			<< "step"								//step
			<< "string"								//string
			<< "subroutine"							//subroutine
			<< "system"								//system
			<< "tan"								//tan
			<< "text"								//text
			<< "textwidth"							//textwidth
			<< "textheight"							//textheight
			<< "then"								//then
			<< "throwerror"							//throwerror
			<< "to"									//to
			<< "tobinary"							//tobinary
			<< "tohex"								//tohex
			<< "tooctal"							//tooctal
			<< "toradix"							//toradix
			<< "trim"								//trim
			<< "try"								//try
			<< "typeof"								//typeof
			<< "unassign"							//unassign
			<< "unserialize"						//unserialize
			<< "until"								//until
			<< "upper"								//upper
			<< "variablewatch"						//variablewatch
			<< "version"							//version
			<< "volume"								//volume
			<< "wavlength"							//wavlength
			<< "wavpause"							//wavpause
			<< "wavpos"								//wavpos
			<< "wavplay"							//wavplay
			<< "wavseek"							//wavseek
			<< "wavstate"							//wavstate
			<< "wavstop"							//wavstop
			<< "wavwait"							//wavwait
			<< "while"								//while
			<< "write"								//write
			<< "writebyte"							//writebyte
			<< "writeline"							//writeline
			<< "xor"								//xor
			<< "year"								//year
            << "soundenvelope"                      //soundenvelope
            << "soundharmonics"                     //soundharmonics
            << "soundfade"                      	//soundfade
            << "soundload"                      	//soundload
            << "soundloadraw"                      	//soundloadraw
            << "soundloop"                      	//soundloop
            << "soundplay"                  		//soundplay
            << "soundplayer"                  		//soundplayer
            << "soundplayeroff"                  	//soundplayeroff
            << "soundpause"                 		//soundpause
            << "soundseek"                  		//soundseek
            << "soundstop"                  		//soundstop
            << "soundwait"                  		//soundwait
            << "soundwaveform"                  	//soundwaveform
            << "soundsystem"                        //soundsystem
            << "soundvolume"						//soundvolume
            << "soundposition"						//soundposition
            << "soundsamplerate"					//soundsamplerate
            << "soundid"                            //soundid
            << "soundstate"                 		//soundstate
            << "soundlength"						//soundlength
            << "imageautocrop"						//imageautocrop
            << "imagecentered"						//imagecentered
            << "imagecopy"                      	//imagecopy
            << "imagecrop"              			//imagecrop
            << "imagedraw"                  		//imagedraw
            << "imageflip"                  		//imageflip
            << "imageheight"						//imageheight
            << "imageload"                  		//imageload
            << "imagenew"                   		//imagenew
            << "imagepixel"                 		//imagepixel
            << "imagesetpixel"						//imagesetpixel
            << "imageresize"						//imageresize
            << "imagerotate"						//imagerotate
            << "imagesmooth"						//imagesmooth
            << "imagetransformed"					//imagetransformed
            << "imagewidth"                 		//imagewidth
            << "setgraph"                           //setgraph
            << "unload"                             //unload
            << "ellipse"                            //ellipse
            ;
	for (QStringList::iterator it = keywordPatterns.begin(); it != keywordPatterns.end(); ++it) {
		HighlightRule *rule = new HighlightRule;
		rule->pattern = QRegExp("\\b" + *it + "\\b", Qt::CaseInsensitive);
		rule->format  = m_keywordFmt;
		m_standardRules.append(*rule);
	}
}

void EditSyntaxHighlighter::initConstants() {

	m_constantFmt.setForeground(Qt::darkCyan);
	QStringList constantPatterns;

	constantPatterns
			<< "pi"									//pi
			<< "TRUE"								//TRUE
			<< "FALSE"								//FALSE
			<< "clear"								//clear
			<< "black"								//black
			<< "white"								//white
			<< "(dark){0,1}red"						//red and darkred
			<< "(dark){0,1}green"					//green and darkgreen
			<< "(dark){0,1}blue"					//blue and darkblue
			<< "(dark){0,1}cyan"					//cyan and darkcyan
			<< "(dark){0,1}purple"					//purple and darkpurple
			<< "(dark){0,1}yellow"					//yellow and darkyellow
			<< "(dark){0,1}orange"					//orange and darkorange
			<< "(dark){0,1}gr[ea]y"					//gray, grey, darkgray and darkgrey

            <<"ERROR_NONE"
            <<"ERROR_NOSUCHLABEL"
            <<"ERROR_NEXTNOFOR"
            <<"ERROR_NOTARRAY"
            <<"ERROR_ARGUMENTCOUNT"
            <<"ERROR_MAXRECURSE"
            <<"ERROR_STACKUNDERFLOW"
            <<"ERROR_UNSERIALIZEFORMAT"
            <<"ERROR_NOSUCHSUBROUTINE"
            <<"ERROR_NOSUCHFUNCTION"
            <<"ERROR_FILENUMBER"
            <<"ERROR_FILEOPEN"
            <<"ERROR_FILENOTOPEN"
            <<"ERROR_FILEWRITE"
            <<"ERROR_FILERESET"
            <<"ERROR_ARRAYSIZELARGE"
            <<"ERROR_ARRAYSIZESMALL"
            <<"ERROR_ARRAYEVEN"
            <<"ERROR_VARNOTASSIGNED"
            <<"ERROR_ARRAYNITEMS"
            <<"ERROR_ARRAYINDEX"
            <<"ERROR_STRSTART"
            <<"ERROR_RGB"
            <<"ERROR_POLYPOINTS"
            <<"ERROR_IMAGEFILE"
            <<"ERROR_SPRITENUMBER"
            <<"ERROR_SPRITENA"
            <<"ERROR_SPRITESLICE"
            <<"ERROR_FOLDER"
            <<"ERROR_INFINITY"
            <<"ERROR_DBOPEN"
            <<"ERROR_DBQUERY"
            <<"ERROR_DBNOTOPEN"
            <<"ERROR_DBCOLNO"
            <<"ERROR_DBNOTSET"
            <<"ERROR_TYPECONV"
            <<"ERROR_NETSOCK"
            <<"ERROR_NETHOST"
            <<"ERROR_NETCONN"
            <<"ERROR_NETREAD"
            <<"ERROR_NETNONE"
            <<"ERROR_NETWRITE"
            <<"ERROR_NETSOCKOPT"
            <<"ERROR_NETBIND"
            <<"ERROR_NETACCEPT"
            <<"ERROR_NETSOCKNUMBER"
            <<"ERROR_PERMISSION"
            <<"ERROR_IMAGESAVETYPE"
            <<"ERROR_DIVZERO"
            <<"ERROR_FREEFILE"
            <<"ERROR_FREENET"
            <<"ERROR_FREEDB"
            <<"ERROR_DBCONNNUMBER"
            <<"ERROR_FREEDBSET"
            <<"ERROR_DBSETNUMBER"
            <<"ERROR_DBNOTSETROW"
            <<"ERROR_PENWIDTH"
            <<"ERROR_ARRAYINDEXMISSING"
            <<"ERROR_IMAGESCALE"
            <<"ERROR_RADIXSTRING"
            <<"ERROR_RADIX"
            <<"ERROR_LOGRANGE"
            <<"ERROR_STRINGMAXLEN"
            <<"ERROR_PRINTERNOTON"
            <<"ERROR_PRINTERNOTOFF"
            <<"ERROR_PRINTEROPEN"
            <<"ERROR_FILEOPERATION"
            <<"ERROR_SERIALPARAMETER"
            <<"ERROR_LONGRANGE"
            <<"ERROR_INTEGERRANGE"
            <<"ERROR_SLICESIZE"
            <<"ERROR_ARRAYLENGTH2D"
            <<"ERROR_EXPECTEDARRAY"
            <<"ERROR_VARNULL"
            <<"ERROR_VARCIRCULAR"
            <<"ERROR_IMAGERESOURCE"
            <<"ERROR_SOUNDRESOURCE"
            <<"ERROR_INVALIDRESOURCE"
            <<"ERROR_SOUNDFILE"
            <<"ERROR_SOUNDNOTSEEKABLE"
            <<"ERROR_SOUNDLENGTH"
            <<"ERROR_DOWNLOAD"
            <<"ERROR_EXPECTEDSOUND"
            <<"ERROR_TOOMANYSOUNDS"
            <<"ERROR_ENVELOPEODD"
            <<"ERROR_ENVELOPEMAX"
            <<"ERROR_HARMONICNUMBER"
            <<"ERROR_HARMONICLIST"
            <<"ERROR_WAVOBSOLETE"
            <<"ERROR_SOUNDFILEFORMAT"
            <<"ERROR_SOUNDERROR"
            <<"ERROR_ONEDIMENSIONAL"
            <<"ERROR_WAVEFORMLOGICAL"
            <<"ERROR_STRING2NOTE"
            <<"ERROR_ARRAYELEMENT"
            <<"ERROR_SETTINGSGETACCESS"
            <<"ERROR_SETTINGSSETACCESS"
            <<"ERROR_INVALIDPROGNAME"
            <<"ERROR_INVALIDKEYNAME"
            <<"ERROR_SETTINGMAXLEN"
            <<"ERROR_SETTINGMAXKEYS"
            <<"ERROR_REFNOTASSIGNED"
            <<"ERROR_NOTIMPLEMENTED"
            <<"ERROR_UNEXPECTEDRETURN"
            <<"WARNING_START"
            <<"WARNING_TYPECONV"
            <<"WARNING_VARNOTASSIGNED"
            <<"WARNING_LONGRANGE"
            <<"WARNING_INTEGERRANGE"
            <<"WARNING_SOUNDNOTSEEKABLE"
            <<"WARNING_SOUNDLENGTH"
            <<"WARNING_WAVOBSOLETE"
            <<"WARNING_SOUNDFILEFORMAT"
            <<"WARNING_SOUNDERROR"
            <<"WARNING_STRING2NOTE"
            <<"WARNING_ARRAYELEMENT"
            <<"WARNING_REFNOTASSIGNED"

            << "TYPE_ARRAY"
			<< "TYPE_FLOAT"
			<< "TYPE_INT"
			<< "TYPE_STRING"
			<< "TYPE_UNASSIGNED"
			<< "MOUSEBUTTON_CENTER"
			<< "MOUSEBUTTON_LEFT"
			<< "MOUSEBUTTON_NONE"
			<< "MOUSEBUTTON_RIGHT"
			<< "MOUSEBUTTON_DOUBLECLICK"
			<< "IMAGETYPE_BMP"
			<< "IMAGETYPE_JPG"
			<< "IMAGETYPE_PNG"
			<< "OSTYPE_ANDROID"
			<< "OSTYPE_LINUX"
			<< "OSTYPE_MACINTOSH"
			<< "OSTYPE_WINDOWS"
			<< "SLICE_ALL"
			<< "SLICE_PAINT"
			<< "SLICE_SPRITE"
			;
	for (QStringList::iterator it = constantPatterns.begin(); it != constantPatterns.end(); ++it ) {
		HighlightRule *rule = new HighlightRule;
		rule->pattern = QRegExp("\\b" + *it + "\\b", Qt::CaseInsensitive);
		rule->format  = m_constantFmt;
		m_standardRules.append(*rule);
	}
}

void EditSyntaxHighlighter::initQuotes() {
	m_quoteFmt.setForeground(Qt::magenta);

	HighlightRule *rule = new HighlightRule;
	rule->pattern = QRegExp("(\"[^\"]*\")|(\'[^\']*\')");
	rule->format = m_quoteFmt;
	m_standardRules.append(*rule);
}

void EditSyntaxHighlighter::initLabels() {
	m_labelFmt.setForeground(Qt::blue);

	HighlightRule *rule = new HighlightRule;
	rule->pattern = QRegExp("(?:^\\s*)([a-z0-9]+):", Qt::CaseInsensitive);
	rule->format = m_labelFmt;
	m_standardRules.append(*rule);
}

void EditSyntaxHighlighter::initNumbers() {
	m_numberFmt.setForeground(Qt::darkMagenta);

	HighlightRule *rule = new HighlightRule;
	rule->pattern = QRegExp("(\\b([0-9]*\\.?[0-9]+(e[-+]?[0-9]+)?)\\b)|(\\b0x[0-9a-f]+\\b)|(\\b0b[0-1]+\\b)|(\\b0o[0-7]+\\b)", Qt::CaseInsensitive);
	rule->format = m_numberFmt;
	m_standardRules.append(*rule);
}

void EditSyntaxHighlighter::initComments() {
	m_commentFmt.setForeground(Qt::darkGreen);
	m_commentFmt.setFontItalic(true);

	HighlightRule *rule;
	rule = new HighlightRule;
	rule->pattern = QRegExp("(\\bREM\\b.*$)|(#.*$)", Qt::CaseInsensitive);
	rule->format = m_commentFmt;
	m_standardRules.append(*rule);
}
