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
  : 	QSyntaxHighlighter(parent)
{
  initKeywords();
  initColors();
  initQuotes();
  initFunctions();	
  initComments();
}

void EditSyntaxHighlighter::highlightBlock(const QString &text)
{
  HighlightRule rule;

  VecHighlightRules::iterator sIt = m_standardRules.begin();
  VecHighlightRules::iterator sItEnd = m_standardRules.end();
  while (sIt != sItEnd) 
    {
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
  while (fIt != fItEnd)
    {
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

void EditSyntaxHighlighter::initKeywords()
{
  HighlightRule rule;
  
  m_keywordFmt.setForeground(Qt::darkBlue);
  QStringList keywordPatterns;
  
  keywordPatterns << "\\b[Pp][Rr][Ii][Nn][Tt]\\b"		// print
		  << "\\b[Gg][Oo][Tt][Oo]\\b"			// goto
		  << "\\b[Gg][Oo][Ss][Uu][Bb]\\b"		// gosub
		  << "\\b[Ii][Nn][Pp][Uu][Tt]\\b"		// input
		  << "\\b[Kk][Ee][Yy]\\b"			// key
		  << "\\b[Rr][Ee][Tt][Uu][Rr][Nn]\\b"	        // return
		  << "\\b[Ii][Ff]\\b"				// if
		  << "\\b[Tt][Hh][Ee][Nn]\\b"			// then
		  << "\\b[Dd][Ii][Mm]\\b"			// dim
		  << "\\b[Ee][Nn][Dd]\\b"			// end
		  << "\\b[Ff][Oo][Rr]\\b"			// for
		  << "\\b[Tt][Oo]\\b"				// to
		  << "\\b[Nn][Ee][Xx][Tt]\\b"			// next
		  << "\\b[Ss][Tt][Ee][Pp]\\b"			// step
		  << "\\b[Oo][Pp][Ee][Nn]\\b"			// open
		  << "\\b[Rr][Ee][Aa][Dd]\\b"			// read
		  << "\\b[Ww][Rr][Ii][Tt][Ee]\\b"		// write
		  << "\\b[Cc][Ll][Oo][Ss][Ee]\\b"		// close
		  << "\\b[Rr][Ee][Ss][Ee][Tt]\\b"		// reset
		  << "\\b[Pp][Ll][Oo][Tt]\\b"			// plot
		  << "\\b[Cc][Ii][Rr][Cc][Ll][Ee]\\b"	// circle
		  << "\\b[Rr][Ee][Cc][Tt]\\b"			// rect
		  << "\\b[Pp][Oo][Ll][Yy]\\b"			// poly
		  << "\\b[Ll][Ii][Nn][Ee]\\b"			// line
		  << "\\b[Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]\\b"	// fastgraphics
		  << "\\b[Rr][Ee][Ff][Rr][Ee][Ss][Hh]\\b"	// refresh
		  << "\\b[Cc][Ll][Ss]\\b"					// cls
		  << "\\b[Cc][Ll][Gg]\\b"					// clg
		  << "\\b[Cc][Oo][Ll][Oo][Rr]\\b"			// color
		  << "\\b[Cc][Oo][Ll][Oo][Uu][Rr]\\b"		// colour
		  << "\\b[Cc][Ll][Ee][Aa][Rr]\\b"			// clear
		  << "\\b[Ii][Nn][Tt]\\b"					// toint
		  << "\\b[Ss][Tt][Rr][Ii][Nn][Gg]\\b"		// tostring
		  << "\\b[Ll][Ee][Nn][Gg][Tt][Hh]\\b"		// length
		  << "\\b[Mm][Ii][Dd]\\b"					// mid
		  << "\\b[Ii][Nn][Ss][Tt][Rr]\\b"			// instr
		  << "\\b[Cc][Ee][Ii][Ll]\\b"				// ceil
		  << "\\b[Ff][Ll][Oo][Oo][Rr]\\b"			// floor
		  << "\\b[Aa][Bb][Ss]\\b"					// abs
		  << "\\b[Ss][Ii][Nn]\\b"					// sin
		  << "\\b[Cc][Oo][Ss]\\b"					// cos
		  << "\\b[Tt][Aa][Nn]\\b"					// tan
		  << "\\b[Rr][Aa][Nn][Dd]\\b"				// rand
		  << "\\b[Pp][Ii]\\b"						// pi
		  << "\\b[Aa][Nn][Dd]\\b"					// and
		  << "\\b[Oo][Rr]\\b"						// or
		  << "\\b[Xx][Oo][Rr]\\b"					// xor
		  << "\\b[Nn][Oo][Tt]\\b"					// not
		  << "\\b[Pp][Aa][Uu][Ss][Ee]\\b";		// pause
	
  for (QStringList::iterator it = keywordPatterns.begin(); it != keywordPatterns.end(); ++it)
    {
      rule.pattern = QRegExp(*it);
      rule.format  = m_keywordFmt;
      m_standardRules.append(rule);
    }
	
}

void EditSyntaxHighlighter::initColors()
{
  HighlightRule rule;

  m_colorFmt.setForeground(Qt::darkCyan);
  QStringList colorPatterns;
  
  colorPatterns << "\\b([Dd][Aa][Rr][Kk]){0,1}[Bb][Ll][Aa][Cc][Kk]\\b"			// black
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Ww][Hh][Ii][Tt][Ee]\\b"			// white
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Rr][Ee][Dd]\\b"					// red
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Gg][Rr][Ee][Ee][Nn]\\b"			// green
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Bb][Ll][Uu][Ee]\\b"				// blue
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Cc][Yy][Aa][Nn]\\b"				// cyan
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Pp][Uu][Rr][Pp][Ll][Ee]\\b"		// purple
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Yy][Ee][Ll][Ll][Oo][Ww]\\b"		// yellow
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Oo][Rr][Aa][Nn][Gg][Ee]\\b"		// orange
		<< "\\b([Dd][Aa][Rr][Kk]){0,1}[Gg][Rr][AaEe][Yy]\\b";			// gray
	
  for (QStringList::iterator it = colorPatterns.begin(); it != colorPatterns.end(); ++it )
    {
      rule.pattern = QRegExp(*it);
      rule.format  = m_colorFmt;
      m_standardRules.append(rule);
    }
	
}

void EditSyntaxHighlighter::initQuotes()
{
  HighlightRule rule;

  m_quoteFmt.setForeground(Qt::darkRed);
  rule.pattern = QRegExp("\".*\"");
  rule.format = m_quoteFmt;
  m_standardRules.append(rule);
}

void EditSyntaxHighlighter::initFunctions()
{
  HighlightRule rule;
	
  m_functionFmt.setForeground(Qt::blue);
  rule.pattern = QRegExp("\\b[A-Za-z0-9]+(?=\\:$)");
  rule.format = m_functionFmt;
  m_functionRules.append(rule);		
}

void EditSyntaxHighlighter::initComments()
{
  HighlightRule rule;
	
  m_commentFmt.setForeground(Qt::darkGreen);
  m_commentFmt.setFontItalic(true);
  rule.pattern = QRegExp("(([Rr][Ee][Mm][ ])|#)[^\n]*");
  rule.format = m_commentFmt;
  m_standardRules.append(rule);	
}
