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
	searchforinput = new QLineEdit(QString::null,this);
	searchforinput->setMaxLength(100);
	layout->addWidget(searchforlabel,r,1,1,1);
	layout->addWidget(searchforinput,r,2,1,2);
	//
	//
	r++;
	cancelbutton = new QPushButton(tr("Cancel"), this);
	connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
	forwardbutton = new QPushButton(tr("Search Forward"), this);
	connect(forwardbutton, SIGNAL(clicked()), this, SLOT (clickForwardButton()));
	backbutton = new QPushButton(tr("Search Backwards"), this);
	connect(backbutton, SIGNAL(clicked()), this, SLOT (clickBackButton()));
	layout->addWidget(cancelbutton,r,1,1,1);
	layout->addWidget(forwardbutton,r,2,1,1);
	layout->addWidget(backbutton,r,3,1,1);
	
	this->setLayout(layout);
	this->show();

}

void FindWin::clickCancelButton() {
	close();
}

void FindWin::clickForwardButton() {
	be->findString(searchforinput->text(),0);
}

void FindWin::clickBackButton() {
	be->findString(searchforinput->text(),1);
}

void FindWin::closeEvent(QCloseEvent *e) {
	// save current screen posision
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	settings.setValue(SETTINGSFINDPOS, pos());

}

