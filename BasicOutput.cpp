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

#include <QPainter>
#include <QTextCursor>
#include <QMutex>
#include <QClipboard>
#include <QMimeData>

#include <QtWidgets/QAction>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include "Settings.h"
#include "BasicOutput.h"

extern QMutex *mymutex;

extern int lastKey;
extern std::list<int> pressedKeys;

BasicOutput::BasicOutput( ) : QTextEdit () {
	setInputMethodHints(Qt::ImhNoPredictiveText);
	setFocusPolicy(Qt::StrongFocus);
	setAcceptRichText(false);
	setUndoRedoEnabled(false);
	gettingInput = false;
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorChanged()));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(cursorChanged()));
}

BasicOutput::~BasicOutput( ) {
    // destructor for basic output
}

void
BasicOutput::getInput() {
    // move cursor to the end of the existing text and start input
	gettingInput = true;
	emit(mainWindowsVisible(2,true));
	QTextCursor t(textCursor());
	t.movePosition(QTextCursor::End);
	setTextCursor(t);
	startPos = t.position();
	setReadOnly(false);
	setFocus();
    updatePasteButton();
}

void BasicOutput::stopInput() {
	gettingInput = false;
	setReadOnly(true);
    updatePasteButton();
}


void BasicOutput::keyPressEvent(QKeyEvent *e) {
    e->accept();
    if (!gettingInput) {
		mymutex->lock();
		lastKey = e->key();
		pressedKeys.push_front(lastKey);
		if( e->modifiers() & Qt::ShiftModifier )
		{
				pressedKeys.push_front(Qt::Key_Shift);
		}else{
				pressedKeys.remove(Qt::Key_Shift);
		}
		if( e->modifiers() & Qt::ControlModifier )
		{
				pressedKeys.push_front(Qt::Key_Control);
		}else{
				pressedKeys.remove(Qt::Key_Control);
		}
		if( e->modifiers() & Qt::AltModifier )
		{
				pressedKeys.push_front(Qt::Key_Alt);
		}else{
				pressedKeys.remove(Qt::Key_Alt);
		}
		if( e->modifiers() & Qt::MetaModifier )
		{
				pressedKeys.push_front(Qt::Key_Meta);
		}else{
				pressedKeys.remove(Qt::Key_Meta);
		}
        QTextEdit::keyPressEvent(e);
		mymutex->unlock();
    } else {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            QTextCursor t(textCursor());
            t.setPosition(startPos, QTextCursor::KeepAnchor);
            emit(inputEntered(t.selectedText())); // send the string back to the interperter and run controller
            insertPlainText("\n");
            stopInput();
       } else if (e->key() == Qt::Key_Backspace) {
            QTextCursor t(textCursor());
            t.movePosition(QTextCursor::PreviousCharacter);
            if (t.position() >= startPos)
                QTextEdit::keyPressEvent(e);
        } else {
            QTextEdit::keyPressEvent(e);
        }
    }
}


void BasicOutput::keyReleaseEvent(QKeyEvent *e) {
	e->accept();
	if (!gettingInput) {
		mymutex->lock();
		if(!e->isAutoRepeat())pressedKeys.remove(e->key());
		if( e->modifiers() & Qt::ShiftModifier )
		{
				pressedKeys.push_front(Qt::Key_Shift);
		}else{
				pressedKeys.remove(Qt::Key_Shift);
		}
		if( e->modifiers() & Qt::ControlModifier )
		{
				pressedKeys.push_front(Qt::Key_Control);
		}else{
				pressedKeys.remove(Qt::Key_Control);
		}
		if( e->modifiers() & Qt::AltModifier )
		{
				pressedKeys.push_front(Qt::Key_Alt);
		}else{
				pressedKeys.remove(Qt::Key_Alt);
		}
		if( e->modifiers() & Qt::MetaModifier )
		{
				pressedKeys.push_front(Qt::Key_Meta);
		}else{
				pressedKeys.remove(Qt::Key_Meta);
		}
        QTextEdit::keyReleaseEvent(e);
		mymutex->unlock();
	}
}


bool BasicOutput::initActions(QMenu * vMenu, QToolBar * vToolBar) {
	if ((NULL == vMenu) || (NULL == vToolBar)) {
		return false;
	}

	vToolBar->setObjectName("outtoolbar");


    copyAct = vMenu->addAction(QObject::tr("Copy"));
    copyAct->setShortcutContext(Qt::WidgetShortcut);
    copyAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Copy));
    copyAct->setEnabled(false);
    pasteAct = vMenu->addAction(QObject::tr("Paste"));
    pasteAct->setShortcutContext(Qt::WidgetShortcut);
    pasteAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Paste));
    pasteAct->setEnabled(false);
    printAct = vMenu->addAction(QObject::tr("Print"));
    printAct->setShortcutContext(Qt::WidgetShortcut);
    printAct->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));

	vToolBar->addAction(copyAct);
	vToolBar->addAction(pasteAct);
	vToolBar->addAction(printAct);

	QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));
	QObject::connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
	QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
    QObject::connect(this, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updatePasteButton()));

	m_usesToolBar = true;
	m_usesMenu = true;

	return true;
}

void BasicOutput::slotPrint() {
#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else
    QTextDocument *document = this->document();
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Text Output"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted))	{
            document->print(&printer);
        } else {
            QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }
    }
#endif
}


//User can navigate or select text only in permitted area when BASIC-256 wait for input
void BasicOutput::cursorChanged() {
	if (gettingInput) {
		QTextCursor t(textCursor());
		int position=t.position();
		int anchor=t.anchor();
		if (position < startPos || anchor < startPos) {
			if (position < startPos) position = startPos;
			if (anchor < startPos) anchor = startPos;
			t.setPosition(anchor, QTextCursor::MoveAnchor);
			t.setPosition(position, QTextCursor::KeepAnchor);
			setTextCursor(t);
		}
	}
}


// Ensure that drag and drop is allowed only in permitted area when BASIC-256 wait for input
void BasicOutput::dragEnterEvent(QDragEnterEvent *e){
	if (e->mimeData()->hasFormat("text/plain") && gettingInput && !isReadOnly()  )
		e->acceptProposedAction();
}

void BasicOutput::dragMoveEvent (QDragMoveEvent *event){
	QTextCursor t = cursorForPosition(event->pos());
	if (t.position() >= startPos){
		event->acceptProposedAction();
		QDragMoveEvent move(event->pos(),event->dropAction(), event->mimeData(), event->mouseButtons(),
			event->keyboardModifiers(), event->type());
		QTextEdit::dragMoveEvent(&move); // Call the parent function (show cursor and keep selection)
	} else {
		event->ignore();
	}
}


//Ensure that drang and drop operation or paste operation will add only first line of the copied text.
void BasicOutput::insertFromMimeData(const QMimeData* source)
{
	if (source->hasText()) {
		QString s = source->text();
		QStringList l = s.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
		textCursor().insertText(l.at(0));
		setFocus();
	}
}

void BasicOutput::updatePasteButton(){
     pasteAct->setEnabled(this->canPaste());
}

