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
	frominput = new QLineEdit(QString::null,this);
	frominput->setMaxLength(100);
	layout->addWidget(fromlabel,r,1,1,1);
	layout->addWidget(frominput,r,2,1,3);
	//
	r++;
	tolabel = new QLabel(tr("To:"),this);
	toinput = new QLineEdit(QString::null,this);
	toinput->setMaxLength(100);
	layout->addWidget(tolabel,r,1,1,1);
	layout->addWidget(toinput,r,2,1,3);
	//
	r++;
	casesenscheckbox = new QCheckBox(tr("Case Sensitive"),this);
	casesenscheckbox->setChecked(false);
	layout->addWidget(casesenscheckbox,r,2,1,3);
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

}

void ReplaceWin::clickCancelButton() {
	close();
}

void ReplaceWin::clickFindButton() {
	be->findString(frominput->text(), false, casesenscheckbox->isChecked());
}

void ReplaceWin::clickReplaceButton() {
	be->replaceString(frominput->text(), toinput->text(), casesenscheckbox->isChecked(), false);
}

void ReplaceWin::clickReplaceAllButton() {
	be->replaceString(frominput->text(), toinput->text(), casesenscheckbox->isChecked(), true);
}

void ReplaceWin::closeEvent(QCloseEvent *e) {
	// save current screen posision
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	settings.setValue(SETTINGSFINDPOS, pos());

}

