/** Copyright (C) 2011, J.M.Reneau.
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



#include <QObject>
#include <QIcon>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QAction>
#include <QtWidgets/QShortcut>

#include "BasicEdit.h"

#ifndef REPLACEWINH

#define REPLACEWINH

class ReplaceWin : public QDialog
{
  Q_OBJECT

public:
	ReplaceWin();
	void closeEvent(QCloseEvent *);
	void setReplaceMode(bool);
	void findAgain();

private slots:
	void changeFromInput(QString);
	void clickCancelButton();
	void clickFindButton();
	void clickReplaceButton();
	void clickReplaceAllButton();
  
private:
	void saveSettings();
	QLabel *fromlabel;
	QLineEdit *frominput;
	QLabel *tolabel;
	QLineEdit *toinput;
	QCheckBox *backcheckbox;
    QCheckBox *casecheckbox;
    QCheckBox *wordscheckbox;
    QPushButton *cancelbutton;
	QPushButton *findbutton;
	QAction *findagain;
	QPushButton *replacebutton;
	QPushButton *replaceallbutton;
	bool replaceMode;

};

#endif
