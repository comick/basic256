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

#ifndef DISABLESYNTAXHIGHLIGHT

EditSyntaxHighlighter::EditSyntaxHighlighter(QTextDocument *parent)
    : 	QSyntaxHighlighter(parent) {
    initKeywords();
    initColors();
    initFunctions();
    initComments();
    initQuotes();
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
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
        ++sIt;
    }

    // Now do the functions, checking for calls to any matches.
    VecHighlightRules::iterator fIt = m_functionRules.begin();
    VecHighlightRules::iterator fItEnd = m_functionRules.end();
    while (fIt != fItEnd) {
        rule = (*fIt);
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
        ++fIt;
    }
}

void EditSyntaxHighlighter::initKeywords() {
    HighlightRule rule;

    m_keywordFmt.setForeground(Qt::darkBlue);
    m_keywordFmt.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;

    keywordPatterns
            << "\\b[Aa][Bb][Ss]\\b"												// abs
            << "\\b[Aa][Cc][Oo][Ss]\\b"											// acos
            << "\\b[Aa][Ll][Ee][Rr][Tt]\\b"										// alert
            << "\\b[Aa][Nn][Dd]\\b"												// and
            << "\\b[Aa][Rr][Cc]\\b"												// arc
            << "\\b[Aa][Ss][Cc]\\b"												// asc
            << "\\b[Aa][Ss][Ii][Nn]\\b"											// asin
            << "\\b[Aa][Tt][Aa][Nn]\\b"											// atan
            << "\\b[Bb][Ee][Gg][Ii][Nn][ \t]*[Cc][Aa][Ss][Ee]\\b"				// begincase
            << "\\b[Cc][Aa][Ll][Ll]\\b"											// call
            << "\\b[Cc][Aa][Ss][Ee]\\b"											// case
            << "\\b[Cc][Aa][Tt][Cc][Hh]\\b"										// catch
            << "\\b[Cc][Ee][Ii][Ll]\\b"											// ceil
            << "\\b[Cc][Hh][Aa][Nn][Gg][Ee][Dd][Ii][Rr]\\b"						// changedir
            << "\\b[Cc][Hh][Oo][Rr][Dd]\\b"										// chord
            << "\\b[Cc][Hh][Rr]\\b"												// chr
            << "\\b[Cc][Ii][Rr][Cc][Ll][Ee]\\b"									// circle
            << "\\b[Cc][Ll][Ii][Cc][Kk][Bb]\\b"									// clickb
            << "\\b[Cc][Ll][Ii][Cc][Kk][Cc][Ll][Ee][Aa][Rr]\\b"					// clickclear
            << "\\b[Cc][Ll][Ii][Cc][Kk][Xx]\\b"									// clickx
            << "\\b[Cc][Ll][Ii][Cc][Kk][Yy]\\b"									// clicky
            << "\\b[Cc][Ll][Gg]\\b"												// clg
            << "\\b[Cc][Ll][Oo][Ss][Ee]\\b"										// close
            << "\\b[Cc][Ll][Ss]\\b"												// cls
            << "\\b[Cc][Oo][Ll][Oo][Rr]\\b"										// color
            << "\\b[Cc][Oo][Ll][Oo][Uu][Rr]\\b"									// colour
            << "\\b[Cc][Oo][Nn][Ff][Ii][Rr][Mm]\\b"								// confirm
            << "\\b[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Dd][Oo]\\b"			// continuedo
            << "\\b[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ff][Oo][Rr]\\b"		// continuefor
            << "\\b[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ww][Hh][Ii][Ll][Ee]\\b"	// continuewhile
            << "\\b[Cc][Oo][Ss]\\b"												// cos
            << "\\b[Cc][Oo][Uu][Nn][Tt]\\b"										// count
            << "\\b[Cc][Oo][Uu][Nn][Tt][Xx]\\b"									// countx
            << "\\b[Cc][Uu][Rr][Rr][Ee][Nn][Tt][Dd][Ii][Rr]\\b"					// currentdir
            << "\\b[Dd][Aa][Yy]\\b"												// day
            << "\\b[Dd][Bb][Cc][Ll][Oo][Ss][Ee]\\b"								// dbclose
            << "\\b[Dd][Bb][Cc][Ll][Oo][Ss][Ee][Ss][Ee][Tt]\\b"					// dbcloseset
            << "\\b[Dd][Bb][Ee][Xx][Ee][Cc][Uu][Tt][Ee]\\b"						// dbexecute
            << "\\b[Dd][Bb][Ff][Ll][Oo][Aa][Tt]\\b"								// dbfloat
            << "\\b[Dd][Bb][Ii][Nn][Tt]\\b"										// dbint
            << "\\b[Dd][Bb][Nn][Uu][Ll][Ll]\\b"									// dbnull
            << "\\b[Dd][Bb][Oo][Pp][Ee][Nn]\\b"									// dbopen
            << "\\b[Dd][Bb][Oo][Pp][Ee][Nn][Ss][Ee][Tt]\\b"						// dbopenset
            << "\\b[Dd][Bb][Rr][Oo][Ww]\\b"										// dbrow
            << "\\b[Dd][Bb][Ss][Tt][Rr][Ii][Nn][Gg]\\b"							// dbstring
            << "\\b[Dd][Ee][Gg][Rr][Ee][Ee][Ss]\\b"								// degrees
            << "\\b[Dd][Ee][Bb][Uu][Gg][Ii][Nn][Ff][Oo]\\b"						// debuginfo
            << "\\b[Dd][Ii][Mm]\\b"												// dim
            << "\\b[Dd][Ii][Rr]\\b"												// dir
            << "\\b[Dd][Oo]\\b"													// do
            << "\\b[Ee][Dd][Ii][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]\\b"   			// editvisible
            << "\\b[Ee][Ll][Ss][Ee]\\b"											// else
            << "\\b[Ee][Nn][Dd]\\b"												// end
            << "\\b[Ee][Nn][Dd][ \t]*[Cc][Aa][Ss][Ee]\\b"						// endcase
            << "\\b[Ee][Nn][Dd][ \t]*[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]\\b"		// endfunction
            << "\\b[Ee][Nn][Dd][ \t]*[Ii][Ff]\\b"								// endif
            << "\\b[Ee][Nn][Dd][ \t]*[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]\\b"	// endsubroutine
            << "\\b[Ee][Nn][Dd][ \t]*[Tt][Rr][Yy]\\b"							// endtry
            << "\\b[Ee][Nn][Dd][ \t]*[Ww][Hh][Ii][Ll][Ee]\\b"					// endwhile
            << "\\b[Ee][Oo][Ff]\\b"												// eof
            << "\\b[Ee][Xx][Ii][Ss][Tt][Ss]\\b"									// exists
            << "\\b[Ee][Xx][Ii][Tt][ \t]*[Dd][Dd]\\b"							// exitdo
            << "\\b[Ee][Xx][Ii][Tt][ \t]*[Ff][Oo][Rr]\\b"						// exitfor
            << "\\b[Ee][Xx][Ii][Tt][ \t]*[Ww][Hh][Ii][Ll][Ee]\\b"				// exitwhile
            << "\\b[Ee][Xx][Pp]\\b"												// exp
            << "\\b[Ee][Xx][Pp][Ll][Oo][Dd][Ee]\\b"								// explode
            << "\\b[Ee][Xx][Pp][Ll][Oo][Dd][Ee][Xx]\\b"							// explodex
            << "\\b[Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]\\b"			// fastgraphics
            << "\\b[Ff][Ll][Oo][Aa][Tt]\\b"										// float
            << "\\b[Ff][Ll][Oo][Oo][Rr]\\b"										// floor
            << "\\b[Ff][Oo][Nn][Tt]\\b"											// font
            << "\\b[Ff][Oo][Rr]\\b"												// for
            << "\\b[Ff][Rr][Ee][Ee][Dd][Bb]\\b"									// freedb
            << "\\b[Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]\\b"						// freedbset
            << "\\b[Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]\\b"							// freefile
            << "\\b[Ff][Rr][Ee][Ee][Nn][Ee][Tt]\\b"								// freenet
            << "\\b[Ff][Rr][Oo][Mm][Bb][Ii][Nn][Aa][Rr][Yy]\\b"					// frombinary
            << "\\b[Ff][Rr][Oo][Mm][Hh][Ee][Xx]\\b"								// fromhex
            << "\\b[Ff][Rr][Oo][Mm][Oo][Cc][Tt][Aa][Ll]\\b"						// fromoctal
            << "\\b[Ff][Rr][Oo][Mm][Rr][Aa][Dd][Ii][Xx]\\b"						// fromradix
            << "\\b[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]\\b"							// function
            << "\\b[Gg][Ee][Tt][Bb][Rr][Uu][Ss][Hh][Cc][Oo][Ll][Oo][Rr]\\b"		// getbrushcolor
            << "\\b[Gg][Ee][Tt][Cc][Oo][Ll][Oo][Rr]\\b"							// getcolor
            << "\\b[Gg][Ee][Tt][Pp][Ee][Nn][Ww][Ii][Dd][Hh][Tt]\\b"				// getpenwidth
            << "\\b[Gg][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]\\b"					// getsetting
            << "\\b[Gg][Ee][Tt][Ss][Ll][Ii][Cc][Ee]\\b"							// getslice
            << "\\b[Gg][Ll][Oo][Bb][Aa][Ll]\\b"									// global
            << "\\b[Gg][Oo][Ss][Uu][Bb]\\b"										// gosub
            << "\\b[Gg][Oo][Tt][Oo]\\b"											// goto
            << "\\b[Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]\\b"				// graphheignt
            << "\\b[Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]\\b"   					// graphsize
            << "\\b[Gg][Rr][Aa][Pp][Hh][Vv][Ii][Ss][Ii][Bb][Ll][Ee]\\b"   		// graphvisible
            << "\\b[Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]\\b"   				// graphwidth
            << "\\b[Hh][Oo][Uu][Rr]\\b"											// hour
            << "\\b[Ii][Ff]\\b"													// if
            << "\\b[Ii][Mm][Gg][Ll][Oo][Aa][Dd]\\b"								// imgload
            << "\\b[Ii][Mm][Gg][Ss][Aa][Vv][Ee]\\b"								// imgsave
            << "\\b[Ii][Mm][Pp][Ll][Oo][Dd][Ee]\\b"								// implode
            << "\\b[Ii][Nn][Cc][Ll][Uu][Dd][Ee]\\b"								// include
            << "\\b[Ii][Nn][Pp][Uu][Tt]\\b"										// input
            << "\\b[Ii][Nn][Ss][Tt][Rr]\\b"										// instr
            << "\\b[Ii][Nn][Ss][Tt][Rr][Xx]\\b"									// instrx
            << "\\b[Ii][Nn][Tt]\\b"												// int
            << "\\b[Kk][Ee][Yy]\\b"												// key
            << "\\b[Kk][Ii][Ll][Ll]\\b"											// kill
            << "\\b[Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr]\\b"						// lasterror
            << "\\b[Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ee][Xx][Tt][Rr][Aa]\\b"	// lasterrorextra
            << "\\b[Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ll][Ii][Nn][Ee]\\b"		// lasterrorline
            << "\\b[Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Mm][Ee][Ss][Ss][Aa][Gg][Ee]\\b"// lasterrormessage
            << "\\b[Ll][Ee][Ff][Tt]\\b"											// left
            << "\\b[Ll][Ee][Tt]\\b"												// let
            << "\\b[Ll][Ee][Nn][Gg][Tt][Hh]\\b"									// length
            << "\\b[Ll][Ii][Nn][Ee]\\b"											// line
            << "\\b[Ll][Oo][Gg]\\b"												// log
            << "\\b[Ll][Oo][Gg][1][0]\\b"										// log10
            << "\\b[Ll][Oo][Ww][Ee][Rr]\\b"										// lower
            << "\\b[Mm][Dd][5]\\b"												// md5
            << "\\b[Mm][Ii][Dd]\\b"												// mid
            << "\\b[Mm][Ii][Nn][Uu][Tt][Ee]\\b"									// minute
            << "\\b[Mm][Oo][Nn][Tt][Hh]\\b"										// month
            << "\\b[Mm][Oo][Uu][Ss][Ee][Bb]\\b"									// mouseb
            << "\\b[Mm][Oo][Uu][Ss][Ee][Xx]\\b"									// mousex
            << "\\b[Mm][Oo][Uu][Ss][Ee][Yy]\\b"									// mousey
            << "\\b[Mm][Ss][Ee][Cc]\\b"											// msec
            << "\\b[Nn][Ee][Tt][Aa][Dd][Dd][Rr][Ee][Ss][Ss]\\b"					// netaddress
            << "\\b[Nn][Ee][Tt][Cc][Ll][Oo][Ss][Ee]\\b"							// netclose
            << "\\b[Nn][Ee][Tt][Cc][Oo][Nn][Nn][Ee][Cc][Tt]\\b"					// netconnect
            << "\\b[Nn][Ee][Tt][Dd][Aa][Tt][Aa]\\b"								// netdata
            << "\\b[Nn][Ee][Tt][Ll][Ii][Ss][Tt][Ee][Nn]\\b"						// netlisten
            << "\\b[Nn][Ee][Tt][Rr][Ee][Aa][Dd]\\b"								// netread
            << "\\b[Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]\\b"							// netwrite
            << "\\b[Nn][Ee][Xx][Tt]\\b"											// next
            << "\\b[Nn][Oo][Tt]\\b"												// not
            << "\\b[Oo][Ff][Ff][Ee][Rr][Rr][Oo][Rr]\\b"							// offerror
            << "\\b[Oo][Nn][Ee][Rr][Rr][Oo][Rr]\\b"								// onerror
            << "\\b[Oo][Pp][Ee][Nn]\\b"											// open
            << "\\b[Oo][Pp][Ee][Nn][Bb]\\b"										// openb
            << "\\b[Oo][Rr]\\b"													// or
            << "\\b[Oo][Ss][Tt][Yy][Pp][Ee]\\b"									// ostype
            << "\\b[Oo][Uu][Tt][Pp][Uu][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]\\b"		// outputvisible
            << "\\b[Pp][Aa][Uu][Ss][Ee]\\b"										// pause
            << "\\b[Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]\\b"							// penwidth
            << "\\b[Pp][Ii][Ee]\\b"												// pie
            << "\\b[Pp][Ii][Xx][Ee][Ll]\\b"										// pixel
            << "\\b[Pp][Ll][Oo][Tt]\\b"											// plot
            << "\\b[Pp][Oo][Ll][Yy]\\b"											// poly
            << "\\b[Pp][Oo][Rr][Tt][Ii][Nn]\\b"									// portin
            << "\\b[Pp][Oo][Rr][Tt][Oo][Uu][Tt]\\b"								// portout
            << "\\b[Pp][Rr][Ii][Nn][Tt]\\b"										// print
            << "\\b[Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]?[Cc][Aa][Nn][Cc][Ee][Ll]\\b"	// printercancel
            << "\\b[Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]?[Oo][Ff][Ff]\\b"			// printeroff
            << "\\b[Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]?[Oo][Nn]\\b"				// printeron
            << "\\b[Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]?[Pp][Aa][Gg][Ee]\\b"		// printerpage
            << "\\b[Pp][Rr][Oo][Mm][Pp][Tt]\\b"									// prompt
            << "\\b[Pp][Uu][Tt][Ss][Ll][Ii][Cc][Ee]\\b"							// putslice
            << "\\b[Rr][Aa][Dd][Ii][Aa][Nn][Ss]\\b"								// radians
            << "\\b[Rr][Aa][Nn][Dd]\\b"											// rand
            << "\\b[Rr][Ee][Aa][Dd]\\b"											// read
            << "\\b[Rr][Ee][Aa][Dd][Bb][Yy][Tt][Ee]\\b"							// readbyte
            << "\\b[Rr][Ee][Aa][Dd][Ll][Ii][Nn][Ee]\\b"							// readline
            << "\\b[Rr][Ee][Cc][Tt]\\b"											// rect
            << "\\b[Rr][Ee][Dd][Ii][Mm]\\b"										// redim
            << "\\b[Rr][Ee][Ff][Rr][Ee][Ss][Hh]\\b"								// refresh
            << "\\b[Rr][Ee][Pp][Ll][Aa][Cc][Ee]\\b"								// replace
            << "\\b[Rr][Ee][Pp][Ll][Aa][Cc][Ee][Xx]\\b"							// replacex
            << "\\b[Rr][Ee][Ss][Ee][Tt]\\b"										// reset
            << "\\b[Rr][Ee][Tt][Uu][Rr][Nn]\\b"	        						// return
            << "\\b[Rr][Gg][Bb]\\b"												// rgb
            << "\\b[Rr][Ii][Gg][Hh][Tt]\\b"										// right
            << "\\b[Ss][Aa][Yy]\\b"												// say
            << "\\b[Ss][Ee][Cc][Oo][Nn][Dd]\\b"									// second
            << "\\b[Ss][Ee][Ee][Kk]\\b"											// seek
            << "\\b[Ss][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]\\b"					// setsetting
            << "\\b[Ss][Ii][Nn]\\b"												// sin
            << "\\b[Ss][Ii][Zz][Ee]\\b"											// size
            << "\\b[Ss][Oo][Uu][Nn][Dd]\\b"										// sound
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Cc][Oo][Ll][Ll][Ii][Dd][Ee]\\b"		// spritedcollide
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Dd][Ii][Mm]\\b"						// spritedim
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Hh]\\b"								// spriteh
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Hh][Ii][Dd][Ee]\\b"					// spritehide
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Ll][Oo][Aa][Dd]\\b"					// spriteload
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Mm][Oo][Vv][Ee]\\b"					// spritemove
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Pp][Ll][Aa][Cc][Ee]\\b"				// spriteplace
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Pp][Oo][Ll][Yy]\\b"					// spritepoly
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Rr]\\b"								// spriter
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Ss]\\b"								// sprites
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Ss][Hh][Oo][Ww]\\b"					// spriteshow
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]\\b"				// spriteslice
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Vv]\\b"								// spritev
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Ww]\\b"								// spritew
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Xx]\\b"								// spritex
            << "\\b[Ss][Pp][Rr][Ii][Tt][Ee][Yy]\\b"								// spritey
            << "\\b[Ss][Qq][Rr]\\b"												// sqr
            << "\\b[Ss][Qq][Rr][Tt]\\b"											// sqrt
            << "\\b[Ss][Tt][Aa][Mm][Pp]\\b"										// stamp
            << "\\b[Ss][Tt][Ee][Pp]\\b"											// step
            << "\\b[Ss][Tt][Rr][Ii][Nn][Gg]\\b"									// string
            << "\\b[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]\\b"					// subroutine
            << "\\b[Ss][Yy][Ss][Tt][Ee][Mm]\\b"									// system
            << "\\b[Tt][Aa][Nn]\\b"												// tan
            << "\\b[Tt][Ee][Xx][Tt]\\b"											// text
            << "\\b[Tt][Ee][Xx][Tt][Hh][Ee][Ii][Gg][Hh][Tt]\\b"					// textwidth
            << "\\b[Tt][Ee][Xx][Tt][Ww][Ii][Dd][Tt][Hh]\\b"						// textheight
            << "\\b[Tt][Hh][Ee][Nn]\\b"											// then
            << "\\b[Tt][Hh][Rr][Oo][Ww][Ee][Rr][Rr][Oo][Rr]\\b"					// throwerror
            << "\\b[Tt][Oo]\\b"													// to
            << "\\b[Tt][Oo][Bb][Ii][Nn][Aa][Rr][Yy]\\b"							// tobinary
            << "\\b[Tt][Oo][Hh][Ee][Xx]\\b"										// tohex
            << "\\b[Tt][Oo][Oc][Cc][Tt][Aa][Ll]\\b"								// tooctal
            << "\\b[Tt][Oo][Rr][Aa][Dd][Ii][Xx]\\b"								// toradix
            << "\\b[Tt][Rr][Yy]\\b"												// try
            << "\\b[Uu][Nn][Tt][Ii][Ll]\\b"										// until
            << "\\b[Uu][Pp][Pp][Ee][Rr]\\b"										// upper
            << "\\b[Vv][Ee][Rr][Ss][Ii][Oo][Nn]\\b"								// version
            << "\\b[Vv][Oo][Ll][Uu][Mm][Ee]\\b"									// volume
            << "\\b[Ww][Aa][Vv][Ll][Ee][Nn][Gg][Tt][Hh]\\b"						// wavlength
            << "\\b[Ww][Aa][Vv][Pp][Aa][Uu][Ss][Ee]\\b"							// wavpause
            << "\\b[Ww][Aa][Vv][Pp][Oo][Ss]\\b"									// wavpos
            << "\\b[Ww][Aa][Vv][Pp][Ll][Aa][Yy]\\b"								// wavplay
            << "\\b[Ww][Aa][Vv][Ss][Ee][Ee][Kk]\\b"								// wavseek
            << "\\b[Ww][Aa][Vv][Ss][Tt][Aa][Tt][Ee]\\b"							// wavstate
            << "\\b[Ww][Aa][Vv][Ss][Tt][Oo][Pp]\\b"								// wavstop
            << "\\b[Ww][Aa][Vv][Ww][Aa][Ii][Tt]\\b"								// wavwait
            << "\\b[Ww][Hh][Ii][Ll][Ee]\\b"										// while
            << "\\b[Ww][Rr][Ii][Tt][Ee]\\b"										// write
            << "\\b[Ww][Rr][Ii][Tt][Ee][Bb][Yy][Tt][Ee]\\b"						// writebyte
            << "\\b[Ww][Rr][Ii][Tt][Ee][Ll][Ii][Nn][Ee]\\b"						// writeline
            << "\\b[Xx][Oo][Rr]\\b"												// xor
            << "\\b[Yy][Ee][Aa][Rr]\\b"											// year
            ;
    for (QStringList::iterator it = keywordPatterns.begin(); it != keywordPatterns.end(); ++it) {
        rule.pattern = QRegExp(*it);
        rule.format  = m_keywordFmt;
        m_standardRules.append(rule);
    }

}

void EditSyntaxHighlighter::initColors() {
    HighlightRule rule;

    m_colorFmt.setForeground(Qt::darkCyan);
    QStringList colorPatterns;

    colorPatterns
            << "\\b[Pp][Ii]\\b"											// pi
            << "\\b[Tt][Rr][Uu][Ee]\\b"									// true
            << "\\b[Ff][Aa][Ll][Ss][Ee]\\b"								// false
            << "\\b[Cc][Ll][Ee][Aa][Rr]\\b"								// clear
            << "\\b[Bb][Ll][Aa][Cc][Kk]\\b"								// black
            << "\\b[Ww][Hh][Ii][Tt][Ee]\\b"								// white
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Rr][Ee][Dd]\\b"				// red and darkred
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Gg][Rr][Ee][Ee][Nn]\\b"		// green and darkgreen
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Bb][Ll][Uu][Ee]\\b"			// blue and darkblue
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Cc][Yy][Aa][Nn]\\b"			// cyan and darkcyan
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Pp][Uu][Rr][Pp][Ll][Ee]\\b"	// purple and darkpurple
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Yy][Ee][Ll][Ll][Oo][Ww]\\b"	// yellow and darkyellow
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Oo][Rr][Aa][Nn][Gg][Ee]\\b"	// orange and darkorange
            << "\\b([Dd][Aa][Rr][Kk]){0,1}[Gg][Rr][AaEe][Yy]\\b"		// gray and darkgrey
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Nn][Ee]\\b"		// ERROR_NONE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Rr][1]\\b"		// ERROR_FOR1
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Rr][2]\\b"		// ERROR_FOR2
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_FILENUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Oo][Pp][Ee][Nn]\\b"		// ERROR_FILEOPEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Oo][Tt][Oo][Pp][Ee][Nn]\\b"		// ERROR_FILENOTOPEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Ww][Rr][Ii][Tt][Ee]\\b"		// ERROR_FILEWRITE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Rr][Ee][Ss][Ee][Tt]\\b"		// ERROR_FILERESET
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ll][Aa][Rr][Gg][Ee]\\b"		// ERROR_ARRAYSIZELARGE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ss][Mm][Aa][Ll][Ll]\\b"		// ERROR_ARRAYSIZESMALL
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Ss][Uu][Cc][Hh][Vv][Aa][Rr][Ii][Aa][Bb][Ll][Ee]\\b"		// ERROR_NOSUCHVARIABLE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx]\\b"		// ERROR_ARRAYINDEX
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Nn][Ee][Gg][Ll][Ee][Nn]\\b"		// ERROR_STRNEGLEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ss][Tt][Aa][Rr][Tt]\\b"		// ERROR_STRSTART
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ee][Nn][Dd]\\b"		// ERROR_STREND
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Nn][Nn][Uu][Mm][Ee][Rr][Ii][Cc]\\b"		// ERROR_NONNUMERIC
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Rr][Gg][Bb]\\b"		// ERROR_RGB
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Uu][Tt][Bb][Ii][Tt][Ff][Oo][Rr][Mm][Aa][Tt]\\b"		// ERROR_PUTBITFORMAT
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Oo][Ll][Yy][Aa][Rr][Rr][Aa][Yy]\\b"		// ERROR_POLYARRAY
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Oo][Ll][Yy][Pp][Oo][Ii][Nn][Tt][Ss]\\b"		// ERROR_POLYPOINTS
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ff][Ii][Ll][Ee]\\b"		// ERROR_IMAGEFILE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_SPRITENUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Aa]\\b"		// ERROR_SPRITENA
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]\\b"		// ERROR_SPRITESLICE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Ll][Dd][Ee][Rr]\\b"		// ERROR_FOLDER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Ff][Ii][Nn][Ii][Tt][Yy]\\b"		// ERROR_INFINITY
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Oo][Pp][Ee][Nn]\\b"		// ERROR_DBOPEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Qq][Uu][Ee][Rr][Yy]\\b"		// ERROR_DBQUERY
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Oo][Pp][Ee][Nn]\\b"		// ERROR_DBNOTOPEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Ll][Nn][Oo]\\b"		// ERROR_DBCOLNO
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt]\\b"		// ERROR_DBNOTSET
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk]\\b"		// ERROR_NETSOCK
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Hh][Oo][Ss][Tt]\\b"		// ERROR_NETHOST
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Cc][Oo][Nn][Nn]\\b"		// ERROR_NETCONN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Rr][Ee][Aa][Dd]\\b"		// ERROR_NETREAD
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Nn][Oo][Nn][Ee]\\b"		// ERROR_NETNONE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]\\b"		// ERROR_NETWRITE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Oo][Pp][Tt]\\b"		// ERROR_NETSOCKOPT
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Bb][Ii][Nn][Dd]\\b"		// ERROR_NETBIND
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Aa][Cc][Cc][Ee][Pp][Tt]\\b"		// ERROR_NETACCEPT
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_NETSOCKNUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Rr][Mm][Ii][Ss][Ss][Ii][Oo][Nn]\\b"		// ERROR_PERMISSION
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Aa][Vv][Ee][Tt][Yy][Pp][Ee]\\b"		// ERROR_IMAGESAVETYPE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Ii][Vv][Zz][Ee][Rr][Oo]\\b"		// ERROR_DIVZERO
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Bb][Yy][Rr][Ee][Ff]\\b"		// ERROR_BYREF
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Bb][Yy][Rr][Ee][Ff][Tt][Yy][Pp][Ee]\\b"		// ERROR_BYREFTYPE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]\\b"		// ERROR_FREEFILE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Nn][Ee][Tt]\\b"		// ERROR_FREENET
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb]\\b"		// ERROR_FREEDB
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Nn][Nn][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_DBCONNNUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]\\b"		// ERROR_FREEDBSET
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Ss][Ee][Tt][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_DBSETNUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt][Rr][Oo][Ww]\\b"		// ERROR_DBNOTSETROW
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]\\b"		// ERROR_PENWIDTH
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Cc][Oo][Ll][Oo][Rr][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_COLORNUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx][Mm][Ii][Ss][Ss][Ii][Nn][Gg]\\b"		// ERROR_ARRAYINDEXMISSING
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Cc][Aa][Ll][Ee]\\b"		// ERROR_IMAGESCALE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Nn][Tt][Ss][Ii][Zz][Ee]\\b"		// ERROR_FONTSIZE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Nn][Tt][Ww][Ee][Ii][Gg][Hh][Tt]\\b"		// ERROR_FONTWEIGHT
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx][Ss][Tt][Rr][Ii][Nn][Gg]\\b"		// ERROR_RADIXSTRING
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx]\\b"		// ERROR_RADIX
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ll][Oo][Gg][Rr][Aa][Nn][Gg][Ee]\\b"		// ERROR_LOGRANGE
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][Mm][Aa][Xx][Ll][Ee][Nn]\\b"		// ERROR_STRINGMAXLEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Tt][Aa][Nn][Uu][Mm][Bb][Ee][Rr]\\b"		// ERROR_NOTANUMBER
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Nn]\\b"		// ERROR_PRINTERNOTON
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Ff][Ff]\\b"		// ERROR_PRINTERNOTOFF
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Oo][Pp][Ee][Nn]\\b"		// ERROR_PRINTEROPEN
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Ff][Ii][Ll][Ee][Ff][Oo][Rr][Mm][Aa][Tt]\\b"		// ERROR_WAVFILEFORMAT
            << "\\b[Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Nn][Oo][Tt][Oo][Pp][Ee][Nn]\\b"		// ERROR_WAVNOTOPEN
            
            
            ;
    for (QStringList::iterator it = colorPatterns.begin(); it != colorPatterns.end(); ++it ) {
        rule.pattern = QRegExp(*it);
        rule.format  = m_colorFmt;
        m_standardRules.append(rule);
    }

}

void EditSyntaxHighlighter::initQuotes() {
    HighlightRule rule;

    m_quoteFmt.setForeground(Qt::darkRed);
    rule.pattern = QRegExp("(\"[^\"]*\")|(\'[^\']*\')");
    rule.format = m_quoteFmt;
    m_standardRules.append(rule);
}

void EditSyntaxHighlighter::initFunctions() {
    HighlightRule rule;

    m_functionFmt.setForeground(Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9]+(?=\\:$)");
    rule.format = m_functionFmt;
    m_functionRules.append(rule);
}

void EditSyntaxHighlighter::initComments() {
    HighlightRule rule;

    m_commentFmt.setForeground(Qt::darkGreen);
    m_commentFmt.setFontItalic(true);
    rule.pattern = QRegExp("(([Rr][Ee][Mm][ :\t].+)|([Rr][Ee][Mm][ :\t]?)|#.*)$");
    rule.format = m_commentFmt;
    m_standardRules.append(rule);
}

#endif  // DISABLESYNTAXHIGHLIGHT
