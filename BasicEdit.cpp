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
#include <QTextBlock>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFontDialog>
#include <QFlags>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>

using namespace std;

#include "BasicEdit.h"
#include "LineNumberArea.h"
#include "Settings.h"

BasicEdit::BasicEdit(QMainWindow *mw) : QPlainTextEdit(mw)
{
	mainwin = mw;

    currentMaxLine = 10;
	currentLine = 1;
	startPos = textCursor().position();
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorMove()));
	codeChanged = false;
	
	lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void
BasicEdit::cursorMove()
{
	QTextCursor t(textCursor());
	mainwin->statusBar()->showMessage(tr("Line: ") + QString::number(t.blockNumber()+1) + tr(" Column: ") + QString::number(t.positionInBlock()));
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
	QPlainTextEdit::keyPressEvent(e);
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
			msgBox.setText(tr("The file ") + filename + tr(" already exists."));
			msgBox.setInformativeText(tr("Do you want to overwrite?"));
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
			QDir::setCurrent(fi.absolutePath());
			codeChanged = false;
			addFileToRecentList(filename);
		}
	}
}

void BasicEdit::addFileToRecentList(QString fn) {
	// keep list of recently open or saved files
	// put file name at position 0 on list
    SETTINGS;
	settings.beginGroup(SETTINGSGROUPHIST);
	// if program is at top then do nothing
	if (settings.value(QString::number(0), "").toString() != fn) {
		// find end of scootdown
		int e;
		for(e=1; e<SETTINGSGROUPHISTN && settings.value(QString::number(e), "").toString() != fn; e++) {}
		// scoot entries down
		for (int i=e; i>=1; i--) {
			settings.setValue(QString::number(i), settings.value(QString::number(i-1), ""));
		}
		settings.setValue(QString::number(0), fn);
	}
	settings.endGroup();

	// print out for debugging
	//settings.beginGroup(SETTINGSGROUPHIST);
	//for (int i=0; i<SETTINGSGROUPHISTN; i++) {
	//	printf("%i %s\n", i, settings.value(QString::number(i), "").toString().toUtf8().data());
	//}
	//settings.endGroup();
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

void BasicEdit::loadRecent0() { loadRecent(0); }
void BasicEdit::loadRecent1() { loadRecent(1); }
void BasicEdit::loadRecent2() { loadRecent(2); }
void BasicEdit::loadRecent3() { loadRecent(3); }
void BasicEdit::loadRecent4() { loadRecent(4); }
void BasicEdit::loadRecent5() { loadRecent(5); }
void BasicEdit::loadRecent6() { loadRecent(6); }
void BasicEdit::loadRecent7() { loadRecent(7); }
void BasicEdit::loadRecent8() { loadRecent(8); }

void
BasicEdit::loadRecent(int i)
{
    SETTINGS;
	settings.beginGroup(SETTINGSGROUPHIST);
	loadFile(settings.value(QString::number(i), "").toString());
	settings.endGroup();
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
			msgBox.setInformativeText(tr("Do you want to discard your changes?"));
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
			addFileToRecentList(s);
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
		} else if (line.contains(QRegExp("^[Ii][Ff]\\s.+\\s[Tt][Hh][Ee][Nn]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// if/then (NOTHING FOLLOWING) - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Ll][Ss][Ee]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// else - come out of block and start new block
			decreaseIndent = true;
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ii][Ff]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// end if - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Ww][Hh][Ii][Ll][Ee]\\s"))) {
			// while - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ww][Hh][Ii][Ll][Ee]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// endwhile - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]\\s"))) {
			// function - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// endfunction - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]\\s"))) {
			// function - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^[Ee][Nn][Dd]\\s*[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
			// endfunction - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^[Dd][Oo]\\s*((#|([Rr][Ee][Mm]\\s)).*)?$"))) {
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

void BasicEdit::findString(QString s, bool reverse, bool casesens)
{
	QTextDocument::FindFlags flag;
	if (reverse) flag |= QTextDocument::FindBackward;
	if (casesens) flag |= QTextDocument::FindCaseSensitively;
	if (!find(s, flag)) {
		QMessageBox msgBox;
		msgBox.setText(tr("String not found."));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
	}
}

void BasicEdit::replaceString(QString from, QString to, bool reverse, bool casesens, bool doall)
{
	bool doone = true;
	QTextDocument::FindFlags flag;
	if (reverse) flag |= QTextDocument::FindBackward;
	if (casesens) flag |= QTextDocument::FindCaseSensitively;
	while (doone) {
		if (from.compare(this->textCursor().selectedText(),(casesens ? Qt::CaseSensitive : Qt::CaseInsensitive))!=0) {
 			if (!find(from, flag)) {
				QMessageBox msgBox;
				msgBox.setText(tr("Replace completed."));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				msgBox.exec();
				doall = false;
			}
		} else {
			this->textCursor().clearSelection();
			this->textCursor().insertText(to);
			codeChanged = true;
		}
		doone = doall;
	}
}

QString BasicEdit::getCurrentWord()
{
	// get the cursor and find the current word on this line
	// anything but letters delimits
	QString w;
	QTextCursor t(textCursor());
	QTextBlock b(t.block());
	w = b.text();
	w = w.left(w.indexOf(QRegExp("[^a-zA-Z0-9]"),t.positionInBlock()));
	w = w.mid(w.lastIndexOf(QRegExp("[^a-zA-Z0-9]"))+1);
	return w;
}

int BasicEdit::lineNumberAreaWidth()
{
	int digits = 1;
	int max = qMax(1, blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}
	int space = 10 + fontMetrics().width(QLatin1Char('9')) * digits;
	return space;
}

void BasicEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void BasicEdit::updateLineNumberArea(const QRect &rect, int dy)
{
	if (dy) {
		lineNumberArea->scroll(0, dy);
	} else {
		lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
	}
 
	if (rect.contains(viewport()->rect())) {
		updateLineNumberAreaWidth(0);
	}
}



 void BasicEdit::highlightCurrentLine()
 {
     QList<QTextEdit::ExtraSelection> extraSelections;

     if (!isReadOnly()) {
         QTextEdit::ExtraSelection selection;

         QColor lineColor = QColor(Qt::yellow).lighter(175);

         selection.format.setBackground(lineColor);
         selection.format.setProperty(QTextFormat::FullWidthSelection, true);
         selection.cursor = textCursor();
         selection.cursor.clearSelection();
         extraSelections.append(selection);
     }

     setExtraSelections(extraSelections);
 }


void BasicEdit::resizeEvent(QResizeEvent *e)
{
	QPlainTextEdit::resizeEvent(e);
	QRect cr = contentsRect();
	lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void BasicEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	QTextCursor t(textCursor());

	QPainter painter(lineNumberArea);
	painter.fillRect(event->rect(), Qt::lightGray);

	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();

	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			QString number = QString::number(blockNumber + 1);
			if (blockNumber==t.blockNumber()) {
				painter.setPen(Qt::blue);
			} else {
				painter.setPen(Qt::black);
			}
			painter.drawText(5, top, lineNumberArea->width()-5, fontMetrics().height(), Qt::AlignRight, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + (int) blockBoundingRect(block).height();
		++blockNumber;
	}
}
