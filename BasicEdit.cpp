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
#include <QTextCursor>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QPrinter>
#include <QPrintDialog>
#include <QMessageBox>

#include "BasicEdit.h"

BasicEdit::BasicEdit(QMainWindow *mw)
{
  QFont f;
  mainwin = mw;
  f.setFamily("Sans");
  f.setFixedPitch(true);
  f.setPointSize(10);
  setFont(f);
  currentMaxLine = 10;
  currentLine = 1;
  startPos = textCursor().position();
  setAcceptRichText(false);
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorMove()));
}

void
BasicEdit::cursorMove()
{
  QTextCursor t(textCursor());
  t.movePosition(QTextCursor::StartOfBlock);
  int line = 1;
  while (t.movePosition(QTextCursor::PreviousBlock))
    {
      line++;
    }
  currentLine = line;
  mainwin->statusBar()->showMessage(tr("Line: ") + QString::number(currentLine));
}

void
BasicEdit::highlightLine(int hline)
{
  QTextCursor t(textCursor());
  t.setPosition(0);
  int line = 1;
  while (line < hline)
    { 
      t.movePosition(QTextCursor::NextBlock);
      line++;
    }
  t.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor, 1);
  setTextCursor(t);
}

void
BasicEdit::goToLine(int newLine)
{
  QTextCursor t(textCursor());
  t.setPosition(0);
  int line = 1;
  while (line < newLine && t.movePosition(QTextCursor::NextBlock))
    {
      line++;
    }
  setTextCursor(t);
  setFocus();
}


void
BasicEdit::keyPressEvent(QKeyEvent *e)
{
  e->accept();
  codeChanged = true;
  QTextEdit::keyPressEvent(e);
}

void
BasicEdit::newProgram()
{
  int answer = QMessageBox::question(this, tr("New Program?"), tr("Are you sure you want to completely clear this program and start a new one?"), tr("Yes"), tr("Cancel"));
  
  if (answer == 0)
    {
      clear();
      mainwin->setWindowTitle(tr("Untitled - BASIC-256"));
    }
}


void
BasicEdit::saveProgram()
{
  if (filename == "")
    {
      filename = QFileDialog::getSaveFileName(this, tr("Save file as"), ".", tr("BASIC-256 File ") + "(*.kbs);;" + tr("Any File ")  + "(*.*)");
    }

  if (filename != "")
    {
      QRegExp rx("\\.[^\\/]*$");
      if (rx.indexIn(filename) == -1)
	{
	  filename += ".kbs";
	  //FIXME need to test for existence here
	}
      QFile f(filename);
      f.open(QIODevice::WriteOnly | QIODevice::Truncate);
      f.write(this->document()->toPlainText().toAscii());
      f.close();
      QFileInfo fi(f);
      mainwin->setWindowTitle(fi.fileName() + tr(" - BASIC-256"));
    }
}

void
BasicEdit::saveAsProgram()
{
      QString tempfilename = QFileDialog::getSaveFileName(this, "Save file as", ".", "BASIC-256 File (*.kbs);;Any File (*.*)");
      
      if (tempfilename != "")
	{
	  filename = tempfilename;
	  saveProgram();
	}
}

void
BasicEdit::loadProgram()
{
  QString s = QFileDialog::getOpenFileName(this, tr("Open a file"), ".", tr("BASIC-256 file ") + "(*.kbs);;" + tr("Any File ") + "(*.*)");
    
    if (s != NULL)
      {
	QFile f(s);
	f.open(QIODevice::ReadOnly);
	QByteArray ba = f.readAll();
	this->setPlainText(ba.data());
	f.close();
	filename = s;
	QFileInfo fi(f);
	mainwin->setWindowTitle(fi.fileName() + tr(" - BASIC-256"));
	QDir::setCurrent(fi.absolutePath());
      }
}

void BasicEdit::slotPrint()
{
	QTextDocument *document = this->document();
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Code"));
	
	if (dialog->exec() == QDialog::Accepted)
	{
		if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted))
		{
        	document->print(&printer);
		}
		else
		{
			QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
		}
	}
}
