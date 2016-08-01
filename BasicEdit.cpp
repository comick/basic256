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
#include "Constants.h"

extern MainWindow * mainwin;

BasicEdit::BasicEdit() {
	this->setInputMethodHints(Qt::ImhNoPredictiveText);
	currentLine = 1;
	runState = RUNSTATESTOP;
    startPos = this->textCursor().position();
    rightClickBlockNumber = -1;
    breakPoints = new QList<int>;
    lineNumberArea = new LineNumberArea(this);
	
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorMove()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(this, SIGNAL(modificationChanged(bool)), this, SLOT(updateTitle()));

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
    //Autoindent new line as previous one
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
    if (document()->isModified()) {
		donew = ( QMessageBox::Yes == QMessageBox::warning(this, tr("New Program"),
			tr("Are you sure you want to completely clear this program and start a new one?"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No));
	}
	if (donew) {
        clearBreakPoints();
        filename = "";
        clear();
        document()->setModified(false);
        setWindowTitle(tr("Untitled"));
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
			dooverwrite = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Save File"),
				tr("The file ") + filename + tr(" already exists.")+ "\n" +tr("Do you want to overwrite?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No));
		}
		if (dooverwrite) {
			f.open(QIODevice::WriteOnly | QIODevice::Truncate);
			f.write(this->document()->toPlainText().toUtf8());
			f.close();
			QFileInfo fi(f);
            document()->setModified(false);
            setWindowTitle(fi.fileName());
			QDir::setCurrent(fi.absolutePath());
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
        if (document()->isModified()) {
			doload = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Load File"),
				tr("Program modifications have not been saved.")+ "\n" + tr("Do you want to discard your changes?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No));
		}
		if (doload) {
			if (QFile::exists(s)) {
				QFile f(s);
				if (f.open(QIODevice::ReadOnly)) {
                    QFileInfo fi(f);
                    QMimeDatabase db;
                    QMimeType mime = db.mimeTypeForFile(fi);
                    // Get user confirmation for non-text files
                    //Remember that empty ".kbs" files are detected as non-text files
                    if (!(mime.inherits("text/plain") || (fi.fileName().endsWith(".kbs",Qt::CaseInsensitive) && fi.size()==0))) {
                        doload = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Load File"),
                            tr("It does not seem to be a text file.")+ "\n" + tr("Load it anyway?"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No));
                    }
                    if (doload) {
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        QByteArray ba = f.readAll();
                        this->setPlainText(QString::fromUtf8(ba.data()));
                        filename = s;
                        document()->setModified(false);
                        setWindowTitle(fi.fileName());
                        QDir::setCurrent(fi.absolutePath());
                        clearBreakPoints();
                        addFileToRecentList(s);
                        QApplication::restoreOverrideCursor();
                    }
                    f.close();
                } else {
					QMessageBox::warning(this, tr("Load File"),
						tr("Unable to open program file \"")+s+tr("\".\nFile permissions problem or file open by another process."),
						QMessageBox::Ok, QMessageBox::Ok);
				}
			} else {
				QMessageBox::warning(this, tr("Load File"),
					tr("Program file does not exist. \"")+s+tr("\"."),
					QMessageBox::Ok, QMessageBox::Ok);
			}
		}
	}

}


void BasicEdit::slotPrint() {
#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print"),
		QObject::tr("Printing is not supported in this platform at this time."));
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
            QMessageBox::warning(this, QObject::tr("Print"),
				QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
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
        if(line.isEmpty()){
            // label - empty line no indent
            indentThisLine = false;
        } else if (line.contains(QRegExp("^\\S+[:]"))) {
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
    //this->setPlainText(lines.join("\n"));
    QTextCursor cursor = this->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.insertText(lines.join("\n"));
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
        setUpdatesEnabled(false);
        cursor.movePosition(reverse?QTextCursor::End:QTextCursor::Start);
        setTextCursor(cursor);

        if (!find(s, flag))
        {
            // word not found : we set the cursor back to its initial position and restore verticalScrollBar value
			setTextCursor(cursorSaved);
            verticalScrollBar()->setValue(scroll);
            setUpdatesEnabled(true);
            QMessageBox::information(this, tr("Find"),
                tr("String not found."),
                QMessageBox::Ok, QMessageBox::Ok);
        }else{
            //start from current screen position -> no scroll or scroll only as needed
            //if next search is in the same view
            verticalScrollBar()->setValue(scroll);
            // we can use ensureCursorVisible() but we want to make sure that entire selection is visible
            cursor = this->textCursor();
            cursorSaved = cursor;
            cursor.setPosition(cursor.selectionStart());
            setTextCursor(cursor);
            setTextCursor(cursorSaved);
            setUpdatesEnabled(true);
        }
    }
}

void BasicEdit::replaceString(QString from, QString to, bool reverse, bool casesens, bool words, bool doall) {
	if(from.length()==0) return;

	// Replace one time.
	if(!doall){
		//Replace if text is selected - use the cursor from the last find not the copy
        QTextCursor cursor = this->textCursor();
        if (from.compare(cursor.selectedText(),(casesens ? Qt::CaseSensitive : Qt::CaseInsensitive))==0){
			cursor.insertText(to);
		}

		//Make a search
        findString(from, reverse, casesens, words);

    //Replace all
    }else{
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        setUpdatesEnabled(false);
        QTextCursor cursor = this->textCursor();
        QTextCursor cursorSaved = cursor;
        int scroll = verticalScrollBar()->value();
        cursor.beginEditBlock();
        int n = 0;
		cursor.movePosition(QTextCursor::Start);
		setTextCursor(cursor);
        QTextDocument::FindFlags flag;
        if (casesens) flag |= QTextDocument::FindCaseSensitively;
        if (words) flag |= QTextDocument::FindWholeWords;
		while (find(from, flag)){
			if (textCursor().hasSelection()){
				textCursor().insertText(to);
				n++;
			}
		}
		// set the cursor back to its initial position and restore verticalScrollBar value
        setTextCursor(cursorSaved);
        verticalScrollBar()->setValue(scroll);
        cursor.endEditBlock();
        setUpdatesEnabled(true);
        QApplication::restoreOverrideCursor();
        if(n==0)
			QMessageBox::information(this, tr("Replace"),
				tr("String not found."),
				QMessageBox::Ok, QMessageBox::Ok);
		else
            QMessageBox::information(this, tr("Replace"),
				tr("Replace completed.") + "\n" + QString::number(n) + " " + tr("occurrence(s) were replaced."),
				QMessageBox::Ok, QMessageBox::Ok);
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
    //update setViewportMargins only when lineNumberAreaWidth is changed - save CPU
    //(this signal is emitted even when user scroll up or scroll down code)
    static int last=-1;
    int w = lineNumberAreaWidth();
    if(w!=last){
        last=w;
        setViewportMargins(w, 0, 0, 0);
    }
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
    QTextEdit::ExtraSelection blockSelection;
    QTextEdit::ExtraSelection selection;
    QColor lineColor;
    QColor blockColor;
	
	if (runState==RUNSTATERUN || runState==RUNSTATESTOPING || (runState==RUNSTATESTOP && isReadOnly())) {
		// no highlighting when running, stopping, or in -r mode
		setExtraSelections(extraSelections);
		return;
	}
	if (runState==RUNSTATEDEBUG) {
		// if we are waiting to execute in debug mode
        lineColor = QColor(Qt::red).lighter(175);
        blockColor = QColor(Qt::red).lighter(190);
	}
	if (runState==RUNSTATERUNDEBUG) {
		// if we are executing in debug mode
        lineColor = QColor(Qt::green).lighter(175);
        blockColor = QColor(Qt::green).lighter(190);
	}
	if (runState==RUNSTATESTOP) {
		// in edit mode
        lineColor = QColor(Qt::yellow).lighter(165);
        blockColor = QColor(Qt::yellow).lighter(190);
	}

    blockSelection.format.setBackground(blockColor);
    blockSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
    blockSelection.cursor = textCursor();
    blockSelection.cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::MoveAnchor);
    blockSelection.cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
    extraSelections.append(blockSelection);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);


    //mark brackets if we are editing
	if (runState==RUNSTATESTOP && !isReadOnly()) {
        QTextCursor cur = textCursor();
        int pos = cur.position();
        cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
        cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        int posDoc = cur.position();
        pos = pos - posDoc -1;
        QString s = cur.selectedText(); //current line as QString
        int length = s.length();
        QChar cPrev, cNext;


        if(length>1){
            QChar open;
            QChar close;
            QChar c;
            bool lookBefore = false;
            bool lookAfter = false;
            int quote=0;
            int doubleQuote=0;
            int i = 0;
            const QColor bracketColor = QColor(Qt::green).lighter(165);
            QTextEdit::ExtraSelection selectionBracket;

            if(pos>0){
                cPrev = s.at(pos);
                if(cPrev==')'){
                    open = '(';
                    lookBefore = true;
                }else if(cPrev==']'){
                    open = '[';
                    lookBefore = true;
                }else if(cPrev=='}'){
                    open = '{';
                    lookBefore = true;
                }
            }

            if(pos<length-1){
                cNext = s.at(pos+1);
                if(cNext=='('){
                    close = ')';
                    lookAfter = true;
                }else if(cNext=='['){
                    close = ']';
                    lookAfter = true;
                }else if(cNext=='{'){
                    close = '}';
                    lookAfter = true;
                }
            }

            if(lookBefore){
                //check for open bracket
                QList<int> list;
                for(i=0;i<pos;i++){
                    c=s.at(i);
                    if(c=='\"' && quote==0){
                        doubleQuote^=1;
                    }else if(c=='\'' && doubleQuote==0){
                        quote^=1;
                    }else if(quote==0 && doubleQuote==0){
                        if(c==open){
                            list.append(i);
                        }else if(c==cPrev){
                            if(!list.isEmpty())
                                list.removeLast();
                        }
                    }
                }
                if(quote==0 && doubleQuote==0 && !list.isEmpty()){
                    //mark current bracket and the corresponding bracket
                    selectionBracket.format.setBackground(bracketColor);
                    selectionBracket.cursor = textCursor();
                    selectionBracket.cursor.setPosition(pos+posDoc+1);
                    selectionBracket.cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor);
                    extraSelections.append(selectionBracket);
                    selectionBracket.cursor.setPosition(list.last()+posDoc+1);
                    selectionBracket.cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor);
                    extraSelections.append(selectionBracket);
                }
            }else if(lookAfter){
                //else check only for quotes if lookAfter
                for(i=0;i<pos;i++){
                    c=s.at(i);
                    if(c=='\"' && quote==0){
                        doubleQuote^=1;
                    }else if(c=='\'' && doubleQuote==0){
                        quote^=1;
                    }
                }
            }

            if(lookAfter && quote==0 && doubleQuote==0){
                //check for bracket if current bracket is not quoted
                int count = 1;
                for(i=pos+2;i<length;i++){
                    c=s.at(i);
                    if(c=='\"' && quote==0){
                        doubleQuote^=1;
                    }else if(c=='\'' && doubleQuote==0){
                        quote^=1;
                    }else if(quote==0 && doubleQuote==0){
                        if(c==cNext){
                            count++;
                        }else if(c==close){
                            count--;
                            if(count==0){
                                //mark current bracket and the corresponding bracket
                                selectionBracket.format.setBackground(bracketColor);
                                selectionBracket.cursor = textCursor();
                                selectionBracket.cursor.setPosition(pos+posDoc+1);
                                selectionBracket.cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);
                                extraSelections.append(selectionBracket);
                                selectionBracket.cursor.setPosition(i+posDoc+1);
                                selectionBracket.cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor);
                                extraSelections.append(selectionBracket);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    setExtraSelections(extraSelections);
}


void BasicEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


void BasicEdit::lineNumberAreaPaintEvent(QPaintEvent *event) {
    //We need lastBlockNumber to adjust Qt bug (see below)
    QTextCursor t(textCursor());
    int currentBlockNumber = t.blockNumber();
    t.movePosition(QTextCursor::End);
    int lastBlockNumber = t.block().blockNumber();

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    int y = -1;

    if(lineNumberArea->underMouse())
        y = lineNumberArea->mapFromGlobal(QCursor::pos()).y();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom > event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            //Draw lighter background for current paragraph
            //For this to work (and to correctly change the color of current paragraph number) highlightCurrentLine()
            //must set a specialSelection for entire current block. If not, QPaintEvent will update only
            //a small portion of lineNumberAreaPaint (just current line, not entire paragraph), acording with Automatic Clipping.
            //Also there is a bug with last block. See http://stackoverflow.com/questions/37890906/qt-linenumberarea-example-blockboundingrectlastblock
            //or http://www.forum.crossplatform.ru/index.php?showtopic=3963
            if (blockNumber==currentBlockNumber)
                painter.fillRect(0,top,lineNumberArea->width(),bottom-top-(blockNumber==lastBlockNumber?3:0), QColor(Qt::lightGray).lighter(110));
            else if(top <= y && bottom > y && y>=0 && runState != RUNSTATERUN)
                painter.fillRect(0,top,lineNumberArea->width(),bottom-top-(blockNumber==lastBlockNumber?3:0), QColor(Qt::lightGray).lighter(104));

            // Draw breakpoints
            if (block.userState()==STATEBREAKPOINT) {
                    painter.setBrush(Qt::red);
                    painter.setPen(Qt::red);
                    int w = lineNumberArea->width();
                    int bh = blockBoundingRect(block).height();
                    int fh = fontMetrics().height();
                    painter.drawEllipse((w-(fh-6))/2, top+(bh-(fh-6))/2, fh-6, fh-6);
            }
            // draw text
            if (blockNumber==currentBlockNumber) {
                painter.setPen(Qt::blue);
            } else {
                painter.setPen(Qt::black);
            }
            painter.drawText(0, top, lineNumberArea->width()-5, fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}


void BasicEdit::lineNumberAreaMouseWheelEvent(QWheelEvent *event) {
    QPlainTextEdit::wheelEvent(event);
}


//Breakpoints are stick to each paragraph by using setUserState to avoid messing up while delete/edit/drop/paste large sections of text.
//breakPoints list is also needed by interpretor. Before running in debug mode breakPoints list is update by calling updateBreakPointsList().
//Because setting/removing breakpoints is enabled in debug mode, we set/unset breakpoints in breakPoints list too.
void BasicEdit::lineNumberAreaMouseClickEvent(QMouseEvent *event) {
    if (runState == RUNSTATERUN)
        return;
    // based on mouse click - set the breakpoint in the map/block and highlight the line
	int line;
	QTextBlock block = firstVisibleBlock();
    int bottom = (int) blockBoundingGeometry(block).translated(contentOffset()).top(); //bottom from previous block
    line = block.blockNumber();
	// line 0 ... (n-1) of what was clicked
	while(block.isValid()) {
		bottom += blockBoundingRect(block).height();
		if (event->y() < bottom) {
            if(event->button() == Qt::LeftButton){
                //keep breakPoints list update for debug running mode
                if(block.userState()==STATEBREAKPOINT){
                    block.setUserState(STATECLEAR);
                    if (breakPoints->contains(line))
                        breakPoints->removeAt(breakPoints->indexOf(line,0));
                }else{
                    block.setUserState(STATEBREAKPOINT);
                    breakPoints->append(line);
                }
                lineNumberArea->repaint();
            }else if(event->button() == Qt::RightButton){
                rightClickBlockNumber = line;
                QMenu contextMenu(this);
                if(block.userState()==STATEBREAKPOINT)
                    contextMenu.addAction ( tr("Remove breakpoint from line") + " " + QString::number(line+1), this , SLOT (toggleBreakPoint()) );
                else
                    contextMenu.addAction ( tr("Set breakpoint at line") + " " + QString::number(line+1), this , SLOT (toggleBreakPoint()) );
                contextMenu.addAction ( tr("Clear all breakpoints") , this , SLOT (clearBreakPoints()) );
                contextMenu.exec (event->globalPos());
            }
            return;
        }
		block = block.next();
		line++;
	}
    QMenu contextMenu(this);
    contextMenu.addAction ( tr("Clear all breakpoints") , this , SLOT (clearBreakPoints()) );
    contextMenu.exec (event->globalPos());
}

void BasicEdit::toggleBreakPoint(){
    if(rightClickBlockNumber<0)
        return;
    QTextBlock block = firstVisibleBlock();
    int n = block.blockNumber();
    if(n<rightClickBlockNumber){
        for(;n<rightClickBlockNumber;n++){
            block = block.next();
        }
    }else if(n>rightClickBlockNumber){
        for(;n>rightClickBlockNumber;n--){
            block = block.previous();
        }
    }
    if(block.isValid()){
        if(block.blockNumber()==rightClickBlockNumber){
            //keep breakPoints list update for debug running mode
            if(block.userState()==STATEBREAKPOINT){
                block.setUserState(STATECLEAR);
                if (breakPoints->contains(rightClickBlockNumber))
                    breakPoints->removeAt(breakPoints->indexOf(rightClickBlockNumber,0));
            }else{
                block.setUserState(STATEBREAKPOINT);
                breakPoints->append(rightClickBlockNumber);
            }
            lineNumberArea->repaint();
        }
    }
    rightClickBlockNumber=-1;
}

void BasicEdit::clearBreakPoints() {
    // remove all breakpoints
    QTextBlock b(document()->firstBlock());
    while (b.isValid()){
        b.setUserState(STATECLEAR);
        b = b.next();
    }
    breakPoints->clear();
    lineNumberArea->repaint();
}


void BasicEdit::updateBreakPointsList() {
    breakPoints->clear();
    QTextBlock b(document()->firstBlock());
    while (b.isValid()){
        if(b.userState()==STATEBREAKPOINT)
            breakPoints->append(b.blockNumber());
        b = b.next();
    }
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

void BasicEdit::setWindowTitle(QString title){
    winTitle = title;
    updateTitle();
}

void BasicEdit::updateTitle(){
    emit(changeWindowTitle((document()->isModified()?"*":"") + winTitle + tr(" - BASIC-256")));
}

void BasicEdit::dragEnterEvent(QDragEnterEvent *event){
    if (event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}


void BasicEdit::dropEvent(QDropEvent *event){
    if(event->mimeData()->hasFormat("text/uri-list")){
        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.isEmpty())
            return;
        QString fileName = urls.first().toLocalFile();
        if (fileName.isEmpty())
            return;
        loadFile(fileName);
    }else if (event->mimeData()->hasFormat("text/plain")){
        event->acceptProposedAction();
    }
}
