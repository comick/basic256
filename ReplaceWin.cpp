/** Copyright (C) 2010, J.M.Reneau
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

using namespace std;

#include "ReplaceWin.h"
#include "Settings.h"
#include "MainWindow.h"
#include "md5.h"

ReplaceWin::ReplaceWin (QWidget * parent)
{

	// parent must be basicedit
	be = (BasicEdit *) parent;

	// position where it was last on screen
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	move(settings.value(SETTINGSFINDPOS, QPoint(200, 200)).toPoint());
	setWindowTitle(tr("BASIC-256 Replace"));

	QGridLayout * layout = new QGridLayout();
	int r=0;

	fromlabel = new QLabel(tr("From:"),this);
	layout->addWidget(fromlabel,r,1,1,1);
	frominput = new QLineEdit(settings.value(SETTINGSREPLACEFROM, "").toString());
	frominput->setMaxLength(100);
	connect(frominput, SIGNAL(textChanged(QString)), this, SLOT (changeFromInput(QString)));
	layout->addWidget(frominput,r,2,1,3);
	//
	r++;
	tolabel = new QLabel(tr("To:"),this);
	layout->addWidget(tolabel,r,1,1,1);
	toinput = new QLineEdit(settings.value(SETTINGSREPLACETO, "").toString());
	toinput->setMaxLength(100);
	layout->addWidget(toinput,r,2,1,3);
	//
	r++;
	casecheckbox = new QCheckBox(tr("Case Sensitive"),this);
	casecheckbox->setChecked(settings.value(SETTINGSREPLACECASE, SETTINGSREPLACECASEDEFAULT).toBool());
	layout->addWidget(casecheckbox,r,2,1,3);
	//
	r++;
	cancelbutton = new QPushButton(tr("Cancel"), this);
	connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
	layout->addWidget(cancelbutton,r,1,1,1);
	replaceallbutton = new QPushButton(tr("Replace All"), this);
	connect(replaceallbutton, SIGNAL(clicked()), this, SLOT (clickReplaceAllButton()));
	layout->addWidget(replaceallbutton,r,2,1,1);
	replacebutton = new QPushButton(tr("Replace"), this);
	connect(replacebutton, SIGNAL(clicked()), this, SLOT (clickReplaceButton()));
	layout->addWidget(replacebutton,r,3,1,1);
	findbutton = new QPushButton(tr("Find"), this);
	connect(findbutton, SIGNAL(clicked()), this, SLOT (clickFindButton()));
	layout->addWidget(findbutton,r,4,1,1);
	//
	this->setLayout(layout);
	this->show();
	changeFromInput(frominput->text());
}

void ReplaceWin::changeFromInput(QString t) {
		replacebutton->setEnabled((t.length() != 0) && (t.compare(be->textCursor().selectedText(),(casecheckbox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive))==0));
		replaceallbutton->setEnabled(t.length() != 0);
		findbutton->setEnabled(t.length() != 0);
}

void ReplaceWin::clickCancelButton() {
	close();
}

void ReplaceWin::clickFindButton() {
	saveSettings();
	be->findString(frominput->text(), false, casecheckbox->isChecked());
	changeFromInput(frominput->text());
}

void ReplaceWin::clickReplaceButton() {
	saveSettings();
	be->replaceString(frominput->text(), toinput->text(), casecheckbox->isChecked(), false);
	changeFromInput(frominput->text());
}

void ReplaceWin::clickReplaceAllButton() {
	saveSettings();
	be->replaceString(frominput->text(), toinput->text(), casecheckbox->isChecked(), true);
}

void ReplaceWin::closeEvent(QCloseEvent *e) {
	saveSettings();
}

void ReplaceWin::saveSettings() {
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	settings.setValue(SETTINGSREPLACEPOS, pos());
	settings.setValue(SETTINGSREPLACEFROM, frominput->text());
	settings.setValue(SETTINGSREPLACETO, toinput->text());
	settings.setValue(SETTINGSREPLACECASE, casecheckbox->isChecked());
}

