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

#include "DocumentationWin.h"
#include "MainWindow.h"

DocumentationWin::DocumentationWin (QWidget * parent)
		:QDialog(parent)
{
	//QString localecode = ((MainWindow *) parent)->localecode;
	
	// position where it was last on screen
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	resize(settings.value(SETTINGSDOCSIZE, QSize(700, 500)).toSize());
	move(settings.value(SETTINGSDOCPOS, QPoint(150, 150)).toPoint());
	
	docs = new QWebView();

	toolbar = new QToolBar(tr("Help Navigation"));
    toolbar->addAction(docs->pageAction(QWebPage::Back));
    toolbar->addAction(docs->pageAction(QWebPage::Forward));
	//
	layout = new QVBoxLayout;
    layout->addWidget(toolbar);
    layout->addWidget(docs);

    this->setLayout(layout);
    this->show();
	
	docs->load(QString("http://doc.basic256.org"));
	docs->show();
	
}

void DocumentationWin::closeEvent(QCloseEvent *e) {
     // save current screen posision
     QSettings settings(SETTINGSORG, SETTINGSAPP);
     settings.setValue(SETTINGSDOCSIZE, size());
     settings.setValue(SETTINGSDOCPOS, pos());

}

