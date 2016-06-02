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

#include <QtWidgets/QAction>
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
    gettingInput = false;

}

BasicOutput::~BasicOutput( ) {
    // destructor for basic output
}

void
BasicOutput::getInput() {
    gettingInput = true;
    startPos = textCursor().position();
    setReadOnly(false);
    setFocus();
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
		mymutex->unlock();
    } else {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            QTextCursor t(textCursor());
            t.setPosition(startPos, QTextCursor::KeepAnchor);
            emit(inputEntered(t.selectedText())); // send the string back to the interperter and run controller
            insertPlainText("\n");
            gettingInput = false;
            setReadOnly(true);
       } else if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Left) {
            QTextCursor t(textCursor());
            t.movePosition(QTextCursor::PreviousCharacter);
            if (t.position() >= startPos)
                QTextEdit::keyPressEvent(e);
        } else if (e->key() == Qt::Key_Home || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_Up) {
            QTextCursor t(textCursor());
            t.setPosition(startPos);
            setTextCursor(t);
        } else {
            QTextEdit::keyPressEvent(e);
        }
    }
}

void BasicOutput::keyReleaseEvent(QKeyEvent *e) {
	e->accept();
	if (!gettingInput) {
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
		mymutex->unlock();
	}
}


bool BasicOutput::initActions(QMenu * vMenu, ToolBar * vToolBar) {
    if ((NULL == vMenu) || (NULL == vToolBar)) {
        return false;
    }

    QAction *copyAct = vMenu->addAction(QObject::tr("Copy"));
    QAction *pasteAct = vMenu->addAction(QObject::tr("Paste"));
    QAction *printAct = vMenu->addAction(QObject::tr("Print"));

    vToolBar->addAction(copyAct);
    vToolBar->addAction(pasteAct);
    vToolBar->addAction(printAct);

    QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));
    QObject::connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
    QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));

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
