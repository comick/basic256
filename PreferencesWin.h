/** Copyright (C) 2010, J.M.Reneau.
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
#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QMessageBox>
	#include <QtWidgets/QWidget>
	#include <QtWidgets/QDialog>
	#include <QtWidgets/QGridLayout>
	#include <QtWidgets/QToolBar>
	#include <QtWidgets/QLabel>
	#include <QtWidgets/QLineEdit>
	#include <QtWidgets/QCheckBox>
	#include <QtWidgets/QComboBox>
	#include <QtWidgets/QPushButton>
	#include <QtWidgets/QAction>
#else
	#include <QMessageBox>
	#include <QWidget>
	#include <QDialog>
	#include <QGridLayout>
	#include <QToolBar>
	#include <QLabel>
	#include <QLineEdit>
	#include <QCheckBox>
	#include <QComboBox>
	#include <QPushButton>
	#include <QAction>
#endif

#ifndef PREFERENCESWINH

#define PREFERENCESWINH

class PreferencesWin : public QDialog
{
  Q_OBJECT

 public:
	PreferencesWin(QWidget * parent);
	void closeEvent(QCloseEvent *);

private slots:
	void clickCancelButton();
	void clickSaveButton();

  
private:
	QLabel *passwordlabel;
	QLineEdit *passwordinput;
	QCheckBox *systemcheckbox;
	QCheckBox *settingcheckbox;
	QCheckBox *portcheckbox;
	QCheckBox *warningscheckbox;
	QLabel *voicelabel;
	QComboBox *voicecombo;

	QPushButton *cancelbutton;
	QPushButton *savebutton;

};

#endif
