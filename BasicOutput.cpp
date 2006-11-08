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



using namespace std;
#include <iostream>
#include <QPainter>
#include <QTextCursor>

#include "BasicOutput.h"


BasicOutput::BasicOutput( ) : QTextEdit () 
{
  setFocusPolicy(Qt::StrongFocus);
  gettingInput = false;
}

void
BasicOutput::getInput()
{
  gettingInput = true;
  startPos = textCursor().position();
  setReadOnly(false);
  setFocus();
}

void
BasicOutput::keyPressEvent(QKeyEvent *e)
{
  e->accept();
  if (!gettingInput)
    {
      QTextEdit::keyPressEvent(e);
    }
  else
    {
      if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
	{
	  QTextCursor t(textCursor());
	  t.setPosition(startPos, QTextCursor::KeepAnchor);
	  emit(inputEntered(t.selectedText()));

	  insertPlainText("\n");
	  gettingInput = false;
	  setReadOnly(true);
	}
      else if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Left)
	{
	  QTextCursor t(textCursor());
	  t.movePosition(QTextCursor::PreviousCharacter);
	  if (t.position() >= startPos)
	    QTextEdit::keyPressEvent(e);
	}
      else if (e->key() == Qt::Key_Home || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_Up)
	{
	  QTextCursor t(textCursor());
	  t.setPosition(startPos);
	  setTextCursor(t);
	}
      else
	{
	  QTextEdit::keyPressEvent(e);
	}
    }
}

