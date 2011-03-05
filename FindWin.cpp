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

#include "FindWin.h"
#include "Settings.h"
#include "MainWindow.h"
#include "md5.h"

FindWin::FindWin (QWidget * parent)
{

	// parent must be basicedit
	be = (BasicEdit *) parent;

	// position where it was last on screen
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	move(settings.value(SETTINGSFINDPOS, QPoint(200, 200)).toPoint());
	setWindowTitle(tr("BASIC-256 Find"));

	QGridLayout * layout = new QGridLayout();
	int r=0;

	searchforlabel = new QLabel(tr("Search For:"),this);
	layout->addWidget(searchforlabel,r,1,1,1);
	searchforinput = new QLineEdit(settings.value(SETTINGSFINDSTRING, "").toString());
	searchforinput->setMaxLength(100);
	connect(searchforinput, SIGNAL(textChanged(QString)), this, SLOT (changeSearchForInput(QString)));
	layout->addWidget(searchforinput,r,2,1,2);
	//
	r++;
	casecheckbox = new QCheckBox(tr("Case Sensitive"),this);
	casecheckbox->setChecked(settings.value(SETTINGSFINDCASE, SETTINGSFINDCASEDEFAULT).toBool());
	layout->addWidget(casecheckbox,r,2,1,2);
	//
	r++;
	backcheckbox = new QCheckBox(tr("Search Backwards"),this);
	backcheckbox->setChecked(settings.value(SETTINGSFINDBACK, SETTINGSFINDBACKDEFAULT).toBool());
	layout->addWidget(backcheckbox,r,2,1,2);
	//
	r++;
	cancelbutton = new QPushButton(tr("Close"), this);
	connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
	layout->addWidget(cancelbutton,r,2,1,1);
	findbutton = new QPushButton(tr("Find"), this);
	connect(findbutton, SIGNAL(clicked()), this, SLOT (clickFindButton()));
	layout->addWidget(findbutton,r,3,1,1);
	
	this->setLayout(layout);
	this->show();
	changeSearchForInput(searchforinput->text());

}

void FindWin::changeSearchForInput(QString t) {
		findbutton->setEnabled(t.length() != 0);
}

void FindWin::clickCancelButton() {
	close();
}

void FindWin::clickFindButton() {
	saveSettings();	
	be->findString(searchforinput->text(), backcheckbox->isChecked(), casecheckbox->isChecked());
}

void FindWin::closeEvent(QCloseEvent *e) {
	saveSettings();	
}

void FindWin::saveSettings() {
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	settings.setValue(SETTINGSFINDPOS, pos());
	settings.setValue(SETTINGSFINDSTRING, searchforinput->text());
	settings.setValue(SETTINGSFINDCASE, casecheckbox->isChecked());
	settings.setValue(SETTINGSFINDBACK, backcheckbox->isChecked());
}

