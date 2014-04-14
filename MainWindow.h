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

#include <qglobal.h>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QScrollArea>

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
#include "DockWidget.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
  	MainWindow(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~MainWindow();
	void closeEvent(QCloseEvent *);
	void loadAndGoMode();

	// Main IU Widgets
	BasicWidget * editwinwgt;
	BasicWidget * outwinwgt;
	BasicWidget * graphwinwgt;
	BasicWidget * varwinwgt;

	// file menu and choices
	QMenu * filemenu;
	QAction * newact;
	QAction * openact;
	QAction * saveact;
	QAction * saveasact;
	QAction * printact;
	bool showRecentList;
	QAction *recentact[SETTINGSGROUPHISTN]; 
	QAction * exitact;

	// edit menu and choices
	QMenu * editmenu;
	QAction *undoact;
	QAction *redoact;
	QAction *cutact;
	QAction *copyact;
	QAction *pasteact;
	QAction *selectallact;
	QAction *findact;
	QShortcut* findagain1;
	QShortcut* findagain2;
	QAction *replaceact;
	QAction *beautifyact;
	QAction *prefact;

	// view menu
	QMenu *viewmenu;
	QAction *editWinVisibleAct;
	QAction *textWinVisibleAct;
	QAction *graphWinVisibleAct;
	QAction *variableWinVisibleAct;
	QAction *graphGridVisibleAct;
	QAction *fontact;

	// run menu
	QMenu *runmenu;
	QAction * runact;
	QAction * debugact;
	QAction * stepact;
	QAction * stopact;

	RunController *rc;
    EditSyntaxHighlighter * editsyntax;

	QString localecode;

public slots:
  void updateStatusBar(QString);
  void updateWindowTitle(QString);

private:

	DockWidget * outdock;
	DockWidget * graphdock;
	QScrollArea * graphscroll;
	DockWidget * vardock;

	QToolBar *maintbar;

	// void pointer to the run controller
	// can't specify type because of circular reference
	//void *rcvoidpointer;		

private slots:
	void updateRecent();
	void about();
};

#endif
