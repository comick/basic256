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



#include <iostream>
#include <QTextCursor>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QPrinter>
#include <QPrintDialog>
#include <QMessageBox>
using namespace std;

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
	codeChanged = false;
}

void
BasicEdit::fontSmall()
{
	changeFontSize(8);
}
void
BasicEdit::fontMedium()
{
	changeFontSize(10);
}
void
BasicEdit::fontLarge()
{
	changeFontSize(12);
}
void
BasicEdit::fontHuge()
{
	changeFontSize(15);
}

void
BasicEdit::changeFontSize(unsigned int pointSize)
{
	QFont f;
	f.setFamily("Sans");
	f.setFixedPitch(true);
	f.setPointSize(pointSize);
	setFont(f);
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
	bool donew = true;
	if (codeChanged) {
		QMessageBox msgBox;
		msgBox.setText(tr("New Program?"));
		msgBox.setInformativeText(tr("Are you sure you want to completely clear this program and start a new one?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		donew = (msgBox.exec() == QMessageBox::Yes);
	}
	if (donew)
	{
		clear();
		mainwin->setWindowTitle(tr("Untitled - BASIC-256"));
		filename = "";
		codeChanged = false;
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
		}
		QFile f(filename);
		bool dooverwrite = true;
		if (f.exists()) {
			QMessageBox msgBox;
			msgBox.setText(tr("The file ") + filename + tr(" exists on file."));
			msgBox.setInformativeText(tr("Do you want wish to overwrite?"));
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			msgBox.setDefaultButton(QMessageBox::Yes);
			dooverwrite = (msgBox.exec() == QMessageBox::Yes);
		}
		if (dooverwrite) {
			f.open(QIODevice::WriteOnly | QIODevice::Truncate);
			f.write(this->document()->toPlainText().toUtf8());
			f.close();
			QFileInfo fi(f);
			mainwin->setWindowTitle(fi.fileName() + tr(" - BASIC-256"));
			codeChanged = false;
		}
	}
}

void
BasicEdit::saveAsProgram()
{
	QString tempfilename = QFileDialog::getSaveFileName(this, tr("Save file as"), ".", tr("BASIC-256 File ")+ "(*.kbs);;" + tr("Any File ")+ "(*.*)");

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
	loadFile(s);
}

void
BasicEdit::loadFile(QString s)
{
	if (s != NULL)
	{
		bool doload = true;
		if (codeChanged) {
			QMessageBox msgBox;
			msgBox.setText(tr("Program modifications have not been saved."));
			msgBox.setInformativeText(tr("Do you want wish to discard your changes?"));
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			msgBox.setDefaultButton(QMessageBox::Yes);
			doload = (msgBox.exec() == QMessageBox::Yes);
		}
		if (doload) {
			QFile f(s);
			f.open(QIODevice::ReadOnly);
			QByteArray ba = f.readAll();
			this->setPlainText(QString::fromUtf8(ba.data()));
			f.close();
			filename = s;
			QFileInfo fi(f);
			mainwin->setWindowTitle(fi.fileName() + tr(" - BASIC-256"));
			QDir::setCurrent(fi.absolutePath());
			codeChanged = false;
		}
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



void BasicEdit::beautifyProgram()
{
	QString program;
	QStringList lines;
	const int TAB = 3;
	int indent = 0;
	bool indentThisLine = true;
	bool increaseIndent = false;
	bool decreaseIndent = false;
	program = this->document()->toPlainText();
	lines = program.split(QRegExp("\\n"));
	for (int i = 0; i < lines.size(); i++) {
		QString line = lines.at(i);
		line = line.trimmed();
		if (line.contains(QRegExp("^\\S+[:]"))) {
			// label - one line no indent
			indentThisLine = false;
		} else if (line.contains(QRegExp("^[Ff][Oo][Rr]\\s"))) {
			// for - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Nn][Ee][Xx][Tt]\\s"))) {
			// next - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Ii][Ff]\\s.+\\s[Tt][Hh][Ee][Nn]$"))) {
			// if/then (NOTHING FOLLOWING) - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Ll][Ss][Ee]$"))) {
			// else - come out of block and start new block
			decreaseIndent = true;
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ii][Ff]$"))) {
			// end if - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Ww][Hh][Ii][Ll][Ee]\\s"))) {
			// while - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ww][Hh][Ii][Ll][Ee]$"))) {
			// endwhile - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Dd][Oo]$"))) {
			// do - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Uu][Nn][Tt][Ii][Ll]\\s"))) {
			// until - come out of block
			decreaseIndent = true;
		}
		//
		if (decreaseIndent) {
			indent -= TAB;
			if (indent<0) indent=0;
			decreaseIndent = false;
		}
		if (indentThisLine) {
			line = QString(indent, QChar(' ')) + line;
		} else {
			indentThisLine = true;
		}
		if (increaseIndent) {
			indent += TAB;
			increaseIndent = false;
		}
		//
		lines.replace(i, line);
	}
	this->setPlainText(lines.join("\n"));
	codeChanged = true;
}


