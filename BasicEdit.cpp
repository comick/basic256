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
#include <QScrollBar>
#include <QTextCursor>
#include <QTextBlock>
#include <QFlags>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>

#include <QtWidgets/QFontDialog>

using namespace std;

#include "MainWindow.h"
#include "BasicEdit.h"
#include "LineNumberArea.h"
#include "Settings.h"

extern MainWindow * mainwin;

BasicEdit::BasicEdit() {

	this->setInputMethodHints(Qt::ImhNoPredictiveText);
	currentMaxLine = 10;
	currentLine = 1;
	startPos = this->textCursor().position();
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorMove()));
	codeChanged = false;

	breakPoints = new QList<int>;
	lineNumberArea = new LineNumberArea(this);
	
	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
	connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

	updateLineNumberAreaWidth(0);
	highlightCurrentLine();
}


BasicEdit::~BasicEdit() {
	if (breakPoints) {
		delete breakPoints;
		breakPoints = NULL;
	}
	if (lineNumberArea) {
		delete lineNumberArea;
		lineNumberArea = NULL;
	}
}

void BasicEdit::setFont(QFont f) {
	// set the font and the tab stop at EDITOR_TAB_WIDTH spaces
	QPlainTextEdit::setFont(f);
	QFontMetrics metrics(f);
	setTabStopWidth(metrics.width(" ")*EDITOR_TAB_WIDTH);
}


void
BasicEdit::cursorMove() {
	QTextCursor t(textCursor());
	emit(changeStatusBar(tr("Line: ") + QString::number(t.blockNumber()+1)
		+ tr(" Character: ") + QString::number(t.positionInBlock())));
}

void
BasicEdit::seekLine(int newLine) {
	// go to a line number and set
	// the text cursor
	//
	// code should be proximal in that it should be closest to look at the curent
	// position than to go and search the entire program from the top
	QTextCursor t = textCursor();
	int line = t.blockNumber()+1;	// current line number for the block
	// go back or forward to the line from the current position
	if (line==newLine) return;
	if (line<newLine) {
		// advance forward
		while (line < newLine && t.movePosition(QTextCursor::NextBlock)) {
			line++;
		}
	} else {
		// go back to find the line
		while (line > newLine && t.movePosition(QTextCursor::PreviousBlock)) {
			line--;
		}
	}
	setTextCursor(t);
}

void BasicEdit::slotWhitespace(bool checked) {
	// toggle the display of whitespace characters
	// http://www.qtcentre.org/threads/27245-Printing-white-spaces-in-QPlainTextEdit-the-QtCreator-way
	QTextOption option = document()->defaultTextOption();
	if (checked) {
		option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
	} else {
		option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
	}
	option.setFlags(option.flags() | QTextOption::AddSpaceForLineAndParagraphSeparators);
	document()->setDefaultTextOption(option);
}

void
BasicEdit::goToLine(int newLine) {
	seekLine(newLine);
	setFocus();
}


void
BasicEdit::keyPressEvent(QKeyEvent *e) {
	e->accept();
	codeChanged = true;
	//Autoindent new line as previus one
	if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter){
		QPlainTextEdit::keyPressEvent(e);
		QTextCursor cur = textCursor();
		cur.movePosition(QTextCursor::PreviousBlock);
		cur.movePosition(QTextCursor::StartOfBlock);
		cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		QString str = cur.selectedText();
		QRegExp rx("^([\\t ]+)");
		if(str.indexOf(rx) >= 0)
			textCursor().insertText(rx.cap(1));
	}else if(e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier){
		if(!indentSelection())
			QPlainTextEdit::keyPressEvent(e);
	}else if((e->key() == Qt::Key_Tab && e->modifiers() & Qt::ShiftModifier) || e->key() == Qt::Key_Backtab){
		unindentSelection();
	}else{
		QPlainTextEdit::keyPressEvent(e);
	}
}

void
BasicEdit::newProgram() {
	bool donew = true;
	if (codeChanged) {
		donew = ( QMessageBox::Yes == QMessageBox::warning(this, tr("New Program?"),
			tr("Are you sure you want to completely clear this program and start a new one?"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No));
	}
	if (donew) {
		clear();
		clearBreakPoints();
		emit(changeWindowTitle(tr("Untitled - BASIC-256")));
		filename = "";
		codeChanged = false;
	}
}


void BasicEdit::saveFile(bool overwrite) {
	// BE SURE TO SET filename PROPERTY FIRST
	// or set it to '' to prompt for a new file name
	if (filename == "") {
		filename = QFileDialog::getSaveFileName(this, tr("Save file as"), ".", tr("BASIC-256 File ") + "(*.kbs);;" + tr("Any File ")  + "(*.*)");
	}

	if (filename != "") {
		QRegExp rx("\\.[^\\/]*$");
		if (rx.indexIn(filename) == -1) {
			filename += ".kbs";
		}
		QFile f(filename);
		bool dooverwrite = true;
		if (!overwrite && f.exists()) {
			dooverwrite = ( QMessageBox::Yes == QMessageBox::warning(this, tr("The file ") + filename + tr(" already exists."),
				tr("Do you want to overwrite?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No));
		}
		if (dooverwrite) {
			f.open(QIODevice::WriteOnly | QIODevice::Truncate);
			f.write(this->document()->toPlainText().toUtf8());
			f.close();
			QFileInfo fi(f);
			emit(changeWindowTitle(fi.fileName() + tr(" - BASIC-256")));
			QDir::setCurrent(fi.absolutePath());
			codeChanged = false;
			addFileToRecentList(filename);
		}
	}
}

void
BasicEdit::saveProgram() {
    saveFile(false);
}

void
BasicEdit::saveAsProgram() {
    QString tempfilename = QFileDialog::getSaveFileName(this, tr("Save file as"), ".", tr("BASIC-256 File ")+ "(*.kbs);;" + tr("Any File ")+ "(*.*)");
    if (tempfilename != "") {
        filename = tempfilename;
        saveFile(false);
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
BasicEdit::loadProgram() {
    QString s = QFileDialog::getOpenFileName(this, tr("Open a file"), ".", tr("BASIC-256 file ") + "(*.kbs);;" + tr("Any File ") + "(*.*)");
    loadFile(s);
}

void BasicEdit::loadRecent0() {
    loadRecent(0);
}
void BasicEdit::loadRecent1() {
    loadRecent(1);
}
void BasicEdit::loadRecent2() {
    loadRecent(2);
}
void BasicEdit::loadRecent3() {
    loadRecent(3);
}
void BasicEdit::loadRecent4() {
    loadRecent(4);
}
void BasicEdit::loadRecent5() {
    loadRecent(5);
}
void BasicEdit::loadRecent6() {
    loadRecent(6);
}
void BasicEdit::loadRecent7() {
    loadRecent(7);
}
void BasicEdit::loadRecent8() {
    loadRecent(8);
}

void
BasicEdit::loadRecent(int i) {
    SETTINGS;
    settings.beginGroup(SETTINGSGROUPHIST);
    loadFile(settings.value(QString::number(i), "").toString());
    settings.endGroup();
}

void
BasicEdit::loadFile(QString s) {
	if (s != NULL) {
		bool doload = true;
		if (codeChanged) {
			doload = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Program modifications have not been saved."),
				tr("Do you want to discard your changes?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No));
		}
		if (doload) {
			if (QFile::exists(s)) {
				QFile f(s);
				if (f.open(QIODevice::ReadOnly)) {
					QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
					QByteArray ba = f.readAll();
					this->setPlainText(QString::fromUtf8(ba.data()));
					f.close();
					filename = s;
					QFileInfo fi(f);
					emit(changeWindowTitle(fi.fileName() + tr(" - BASIC-256")));
					QDir::setCurrent(fi.absolutePath());
					codeChanged = false;
					clearBreakPoints();
					addFileToRecentList(s);
					QApplication::restoreOverrideCursor();
				} else {
					QMessageBox::warning(this, tr("File \"")+s+tr("\"."),
						tr("Unable to open program file."), QMessageBox::Ok, QMessageBox::Ok);
				}
			} else {
				QMessageBox::warning(this, tr("File \"")+s+tr("\"."),
					tr("Program file does not exist."), QMessageBox::Ok, QMessageBox::Ok);
			}
		}
	}

}


void BasicEdit::slotPrint() {
#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else
    QTextDocument *document = this->document();
    QPrinter printer;

    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Code"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted)) {

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            document->print(&printer);
            QApplication::restoreOverrideCursor();
        } else {
            QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }

    }
    
    delete dialog;
#endif
}



void BasicEdit::beautifyProgram() {
	QString program;
	QStringList lines;
	int indent = 0;
	bool indentThisLine = true;
	bool increaseIndent = false;
	bool decreaseIndent = false;
	bool increaseIndentDouble = false;
	bool decreaseIndentDouble = false;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	program = this->document()->toPlainText();
	lines = program.split(QRegExp("\\n"));
	for (int i = 0; i < lines.size(); i++) {
		QString line = lines.at(i);
		line = line.trimmed();
		if (line.contains(QRegExp("^\\S+[:]"))) {
			// label - one line no indent
			indentThisLine = false;
		} else if (line.contains(QRegExp("^for\\s", Qt::CaseInsensitive))) {
			// for - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^next\\s", Qt::CaseInsensitive))) {
			// next - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^if\\s.+\\sthen\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// if/then (NOTHING FOLLOWING) - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^else\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// else - come out of block and start new block
			decreaseIndent = true;
			increaseIndent = true;
		} else if (line.contains(QRegExp("^end\\s*if\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// end if - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^while\\s", Qt::CaseInsensitive))) {
			// while - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^end\\s*while\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// endwhile - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^function\\s", Qt::CaseInsensitive))) {
			// function - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^end\\s*function\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// endfunction - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^subroutine\\s", Qt::CaseInsensitive))) {
			// function - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^end\\s*subroutine\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// endfunction - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^do\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// do - indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^until\\s", Qt::CaseInsensitive))) {
			// until - come out of block
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^try\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// try indent next (block of code)
			increaseIndent = true;
		} else if (line.contains(QRegExp("^catch\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// catch - come out of block and start new block
			decreaseIndent = true;
			increaseIndent = true;
		} else if (line.contains(QRegExp("^end\\s*try\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// end try - come out of block - reduce indent
			decreaseIndent = true;
		} else if (line.contains(QRegExp("^begin\\s*case\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// begin case double indent next (block of code)
			increaseIndentDouble = true;
		} else if (line.contains(QRegExp("^end\\s*case\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// end case double reduce
			decreaseIndentDouble = true;
		} else if (line.contains(QRegExp("^case\\s.+\\s*((#|(rem\\s)).*)?$", Qt::CaseInsensitive))) {
			// case expression - indent one line
			decreaseIndent = true;
			increaseIndent = true;
		}
		//
		if (decreaseIndent) {
			indent--;
			if (indent<0) indent=0;
			decreaseIndent = false;
		}
		if (decreaseIndentDouble) {
			indent-=2;
			if (indent<0) indent=0;
			decreaseIndentDouble = false;
		}
		if (indentThisLine) {
			line = QString(indent, QChar('\t')) + line;
		} else {
			indentThisLine = true;
		}
		if (increaseIndent) {
			indent++;
			increaseIndent = false;
		}
		if (increaseIndentDouble) {
			indent+=2;
			increaseIndentDouble = false;
		}
		//
		lines.replace(i, line);
	}
	this->setPlainText(lines.join("\n"));
	codeChanged = true;
	QApplication::restoreOverrideCursor();
}

void BasicEdit::findString(QString s, bool reverse, bool casesens, bool words)
{
	if(s.length()==0) return;
	QTextDocument::FindFlags flag;
	if (reverse) flag |= QTextDocument::FindBackward;
	if (casesens) flag |= QTextDocument::FindCaseSensitively;
	if (words) flag |= QTextDocument::FindWholeWords;

	QTextCursor cursor = this->textCursor();
	// here we save the cursor position and the verticalScrollBar value
	QTextCursor cursorSaved = cursor;
	int scroll = verticalScrollBar()->value();

	if (!find(s, flag))
	{
		//nothing is found | jump to start/end
		cursor.movePosition(reverse?QTextCursor::End:QTextCursor::Start);
		setTextCursor(cursor);

		if (!find(s, flag))
		{
			// word not found : we set the cursor back to its initial position and restore verticalScrollBar value
			setTextCursor(cursorSaved);
			verticalScrollBar()->setValue(scroll);
			QMessageBox::information(this, tr("BASIC-256"), tr("String not found."), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

void BasicEdit::replaceString(QString from, QString to, bool reverse, bool casesens, bool words, bool doall) {
	if(from.length()==0) return;

	// create common flags for replace or replace all
	QTextDocument::FindFlags flag;
	if (casesens) flag |= QTextDocument::FindCaseSensitively;
	if (words) flag |= QTextDocument::FindWholeWords;

	// get a COPY of the cursor on the current text
	QTextCursor cursor = this->textCursor();
	
	// save the cursor position and the verticalScrollBar value
	int position = cursor.position();
	int scroll = verticalScrollBar()->value();

	// Replace one time.
	if(!doall){
		if (reverse) flag |= QTextDocument::FindBackward;

		//Replace if text is selected - use the cursor from the last find not the copy
		if (from.compare(cursor.selectedText(),(casesens ? Qt::CaseSensitive : Qt::CaseInsensitive))==0){
			cursor.insertText(to);
			setTextCursor(cursor);
			codeChanged = true;
		}

		//Make a search
		if (!find(from, flag))
		{
			//nothing is found | jump to start/end
			cursor.movePosition(reverse?QTextCursor::End:QTextCursor::Start);
			setTextCursor(cursor);

			if (!find(from, flag))
			{
				// word not found : we set the cursor back to its initial position and restore verticalScrollBar value
				cursor.setPosition(position);
				setTextCursor(cursor);
				verticalScrollBar()->setValue(scroll);
				QMessageBox::information(this, tr("BASIC-256"), tr("String not found."), QMessageBox::Ok, QMessageBox::Ok);
			}
		}

	//Replace all
	}else{
		int n = 0;
		cursor.movePosition(QTextCursor::Start);
		setTextCursor(cursor);
		while (find(from, flag)){
			if (textCursor().hasSelection()){
				textCursor().insertText(to);
				codeChanged = true;
				n++;
			}
		}
		// set the cursor back to its initial position and restore verticalScrollBar value
		cursor.setPosition(position);
		setTextCursor(cursor);
		verticalScrollBar()->setValue(scroll);
		if(n==0)
			QMessageBox::information(this, tr("BASIC-256"), tr("String not found."), QMessageBox::Ok, QMessageBox::Ok);
		else
			QMessageBox::information(this, tr("BASIC-256"), tr("Replace completed.") + "\n" + QString::number(n) + " " + tr("occurrence(s) were replaced."), QMessageBox::Ok, QMessageBox::Ok);
	}
}

QString BasicEdit::getCurrentWord() {
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

int BasicEdit::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().width(QLatin1Char('9')) * digits;
    return space;
}

void BasicEdit::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void BasicEdit::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}



void BasicEdit::highlightCurrentLine() {
	QList<QTextEdit::ExtraSelection> extraSelections;
	QTextEdit::ExtraSelection selection;
	QColor lineColor;
	
	if (isReadOnly()) {
		lineColor = QColor(Qt::red).lighter(175);
	} else {
		lineColor = QColor(Qt::yellow).lighter(175);
	}

	selection.format.setBackground(lineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = textCursor();
	selection.cursor.clearSelection();
	extraSelections.append(selection);

	setExtraSelections(extraSelections);
}


void BasicEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void BasicEdit::lineNumberAreaPaintEvent(QPaintEvent *event) {
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
            // Draw breakpoints
            if (breakPoints->contains(blockNumber)) {
                    painter.setBrush(Qt::red);
                    painter.setPen(Qt::red);
                    int w = lineNumberArea->width();
                    int bh = blockBoundingRect(block).height();
                    int fh = fontMetrics().height();
                    painter.drawEllipse((w-(fh-6))/2, top+(bh-(fh-6))/2, fh-6, fh-6);
			}
            // draw text
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

void BasicEdit::lineNumberAreaMouseClickEvent(QMouseEvent *event) {
	// based on mouse click - set the breakpoint in the map and highlight the line
	int line;
	int bottom=0;
	QTextBlock block = firstVisibleBlock();
	line = block.blockNumber();
	// line 0 ... (n-1) of what was clicked
	while(block.isValid()) {
		bottom += blockBoundingRect(block).height();
		if (event->y() < bottom) {
            if (breakPoints->contains(line)) {
				breakPoints->removeAt(breakPoints->indexOf(line,0));
			} else {
				breakPoints->append(line);
			}
			lineNumberArea->repaint();
			return;
		}
		block = block.next();
		line++;
	}
}

void BasicEdit::clearBreakPoints() {
	// remove all breakpoints from map
	breakPoints->clear();
	lineNumberArea->repaint();
}


int BasicEdit::indentSelection() {
	QTextCursor cur = textCursor();
	if(!cur.hasSelection())
			return false;
	int a = cur.anchor();
	int p = cur.position();
	int start = (a<=p?a:p);
	int end = (a>p?a:p);

	cur.beginEditBlock();
	cur.setPosition(end);
	int eblock = cur.block().blockNumber();
	cur.setPosition(start);
	int sblock = cur.block().blockNumber();

	for(int i = sblock; i <= eblock; i++)
	{
		cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
		cur.insertText("\t");
		cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
	}
	cur.endEditBlock();
return true;
}


void BasicEdit::unindentSelection() {
	QTextCursor cur = textCursor();
	int a = cur.anchor();
	int p = cur.position();
	int start = (a<=p?a:p);
	int end = (a>p?a:p);

	cur.beginEditBlock();
	cur.setPosition(end);
	int eblock = cur.block().blockNumber();
	cur.setPosition(start);
	int sblock = cur.block().blockNumber();
	QString s;

	for(int i = sblock; i <= eblock; i++)
	{
		cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
		cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
		s = cur.selectedText();
		if(!s.isEmpty()){
			if(s.startsWith("    ") || s.startsWith("   \t")){
				cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
				cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 4);
				cur.removeSelectedText();
			}else if(s.startsWith("   ") || s.startsWith("  \t")){
				cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
				cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 3);
				cur.removeSelectedText();
			}else if(s.startsWith("  ") || s.startsWith(" \t")){
				cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
				cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);
				cur.removeSelectedText();
			}else if(s.startsWith(" ") || s.startsWith("\t")){
				cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
				cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
				cur.removeSelectedText();
			}
		}
		cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
	}
	cur.endEditBlock();
}


