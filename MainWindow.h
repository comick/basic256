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


#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QMainWindow>
	#include <QtWidgets/QGridLayout>
	#include <QtWidgets/QAction>
	#include <QtWidgets/QMessageBox>
	#include <QtWidgets/QShortcut>
#else
	#include <QMainWindow>
	#include <QGridLayout>
	#include <QAction>
	#include <QMessageBox>
	#include <QShortcut>
#endif


#include "BasicWidget.h"
#include "BasicOutput.h"
#include "BasicEdit.h"
#include "BasicGraph.h"
#include "VariableWin.h"
#include "DocumentationWin.h"
#include "PreferencesWin.h"
#include "RunController.h"
#include "EditSyntaxHighlighter.h"
#include "Settings.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
  	MainWindow(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~MainWindow();
	void closeEvent(QCloseEvent *);
	void loadAndGoMode();
	QAction * runact;
	QAction * debugact;
	QAction * stepact;
	QAction * stopact;
	QAction *editWinVisibleAct;
	QAction *textWinVisibleAct;
	QAction *graphWinVisibleAct;
	QAction *graphGridVisibleAct;
	BasicWidget * editwinwgt;
	BasicWidget * outwinwgt;
	BasicWidget * graphwinwgt;
	BasicWidget * varwinwgt;

	RunController *rc;
	EditSyntaxHighlighter * editsyntax;

	QString localecode;

public slots:
  void updateStatusBar(QString);
  void updateWindowTitle(QString);

private:
	QMenu * filemenu;
	QAction * newact;
	QAction * openact;
	QAction * saveact;
	QAction * saveasact;
	QAction * printact;
	bool showRecentList;
	QAction *recentact[SETTINGSGROUPHISTN]; 
	QAction * exitact;

	QMenu * editmenu;
	QAction *undoact;
	QAction *redoact;
	QAction *cutact;
	QAction *copyact;
	QAction *pasteact;
	QAction *selectallact;
	QAction *findact;
	QShortcut *findagain;
	QAction *replaceact;
	QAction *beautifyact;
	QAction *prefact;

	QMenu *viewmenu;
	QAction *variableWinVisibleAct;

	QMenu *runmenu;

	// void pointer to the run controller
	// can't specify type because of circular reference
	//void *rcvoidpointer;		

private slots:
	void updateRecent();
	void about();
};

#endif
