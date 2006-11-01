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
      mainwin->setWindowTitle(tr("Untitled - KidBASIC"));
    }
}


void
BasicEdit::saveProgram()
{
  if (filename == "")
    {
      filename = QFileDialog::getSaveFileName(this, tr("Save file as"), ".", tr("KidBASIC File ") + "(*.kbs);;" + tr("Any File ")  + "(*.*)");
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
      mainwin->setWindowTitle(fi.fileName() + tr(" - KidBASIC"));
    }
}

void
BasicEdit::saveAsProgram()
{
      QString tempfilename = QFileDialog::getSaveFileName(this, "Save file as", ".", "KidBASIC File (*.kbs);;Any File (*.*)");
      
      if (tempfilename != "")
	{
	  filename = tempfilename;
	  saveProgram();
	}
}

void
BasicEdit::loadProgram()
{
  QString s = QFileDialog::getOpenFileName(this, tr("Open a file"), ".", tr("KidBASIC file ") + "(*.kbs);;" + tr("Any File ") + "(*.*)");
    
    if (s != NULL)
      {
	QFile f(s);
	f.open(QIODevice::ReadOnly);
	QByteArray ba = f.readAll();
	this->setPlainText(ba.data());
	f.close();
	filename = s;
	QFileInfo fi(f);
	mainwin->setWindowTitle(fi.fileName() + tr(" - KidBASIC"));
	QDir::setCurrent(fi.absolutePath());
      }
}




/*
void
BasicEdit::renumber()
{
  QMap<unsigned int, char *> programMap;
  QMap<unsigned int, unsigned int> gotoMap;
  char *line;
  char *program = strdup(document()->toPlainText().toAscii().data());
  char *fixedcode = (char *) malloc(strlen(program) + 10000);
  unsigned int maxlineno = 0;

  fixedcode[0] = 0;

  line = strtok(program, "\n");
  while (line)
    {
      unsigned int linenum = atoi(line);
      if (linenum > maxlineno) 
	{
	  maxlineno = linenum;
	}
      programMap[linenum] = line;
      line = strtok(NULL, "\n");
    }

  unsigned int count = 0;
  unsigned int prettycount = 10;
  while (count <= maxlineno)
    {
      if (programMap.value(count))
	{
	  unsigned int oldline;
	  char buffer[12];
	  char *temp = programMap.value(count);
	  while (*temp == ' ') temp++;
	  oldline = atoi(temp);
	  while (*temp >= '0' && *temp <= '9') temp++;

	  gotoMap[oldline] = prettycount;
	  sprintf(buffer, "%d", prettycount);
	  prettycount += 10;

	  strcat(fixedcode, buffer);
	  strcat(fixedcode, temp);
	  strcat(fixedcode, "\n");
	}
      count++;
    }

  setPlainText(fixedcode);
  free(program);
  free(fixedcode);

  QMapIterator<unsigned int, unsigned int> i(gotoMap);
  QString ian = document()->toPlainText();
  QRegExp rx1("[Gg][Oo][Tt][Oo]\\s+(\\d+)(\\s+|\\n)");
  ian.replace(rx1, "goto \\1X\\2");
  QRegExp rx2("[Gg][Oo][Ss][Uu][Bb]\\s+(\\d+)(\\s+|\\n)");
  ian.replace(rx2, "gosub \\1X\\2");
  while (i.hasNext())
    {
      i.next();
      QString oldl = QString::number(i.key());
      QString newl = QString::number(i.value());

      QRegExp rx1("[Gg][Oo][Tt][Oo]\\s+" + oldl + "X(\\s+|\\n)");
      ian.replace(rx1, "goto " + newl + "\\1");
      QRegExp rx2("[Gg][Oo][Ss][Uu][Bb]\\s+" + oldl + "X(\\s+|\\n)");
      ian.replace(rx2, "gosub " + newl + "\\1");
    }
  setPlainText(ian);
}
*/


