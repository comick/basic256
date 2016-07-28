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


#ifndef __BASICEDIT_H
#define __BASICEDIT_H

#include <qglobal.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QMainWindow>
#include <QKeyEvent>
#include <QList>
#include <QMimeData>

#include <QMimeDatabase>

#include "ViewWidgetIFace.h"

class BasicEdit : public QPlainTextEdit, public ViewWidgetIFace
{
	Q_OBJECT

	public:
		BasicEdit();
		~BasicEdit();

		void loadFile(QString);
		void saveFile(bool);
		void findString(QString, bool, bool, bool);
        void replaceString(QString, QString, bool, bool, bool, bool);
		QString getCurrentWord();
		void lineNumberAreaPaintEvent(QPaintEvent *event);
        void lineNumberAreaMouseClickEvent(QMouseEvent *event);
        void lineNumberAreaMouseWheelEvent(QWheelEvent *event);
		int lineNumberAreaWidth();
        QList<int> *breakPoints;
        void updateBreakPointsList();
		void setFont(QFont);
        int runState; //0 - stop, 1-run, 2-debug
        QString winTitle;
        void setWindowTitle(QString title);
        void dropEvent(QDropEvent *event);
        void dragEnterEvent(QDragEnterEvent *event);


	public slots:
		void newProgram();
		void saveProgram();
		void saveAsProgram();
		void loadProgram();
		void cursorMove();
		void goToLine(int);
		void seekLine(int);
		void slotPrint();
		void beautifyProgram();
		void slotWhitespace(bool);
		void loadRecent0();
		void loadRecent1();
		void loadRecent2();
		void loadRecent3();
		void loadRecent4();
		void loadRecent5();
		void loadRecent6();
		void loadRecent7();
		void loadRecent8();
		void highlightCurrentLine();
		int  indentSelection();
		void unindentSelection();
        void clearBreakPoints();
        void toggleBreakPoint();


	signals:
		void changeStatusBar(QString);
		void changeWindowTitle(QString);

	protected:
		void keyPressEvent(QKeyEvent *);
        void resizeEvent(QResizeEvent *event);

	private:
		const int STATECLEAR = -1;
		const int STATEBREAKPOINT = 1;
		QMainWindow *mainwin;
		int currentLine;
		int startPos;
		int linePos;
		QString filename;
		void addFileToRecentList(QString);
		void loadRecent(int);
		QWidget *lineNumberArea;
        int rightClickBlockNumber;

	private slots:
		void updateLineNumberAreaWidth(int newBlockCount);
		void updateLineNumberArea(const QRect &, int);
        void updateTitle();
};


#endif
