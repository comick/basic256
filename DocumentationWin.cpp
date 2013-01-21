/** Copyright (C) 2010, J.M.Reneau, S.W.Irupin
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with this program; if not, write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

 
 
#include <iostream>
using namespace std;

#include "DocumentationWin.h"
#include "MainWindow.h"

DocumentationWin::DocumentationWin (QWidget * parent)
		:QDialog(parent)
{
	localecode = ((MainWindow *) parent)->localecode;
	indexfile = "start.html";

	// position where it was last on screen
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	resize(settings.value(SETTINGSDOCSIZE, QSize(700, 500)).toSize());
	move(settings.value(SETTINGSDOCPOS, QPoint(150, 150)).toPoint());

	docs = new QTextBrowser( this );
	toolbar = new QToolBar( this );

	QAction *backward = new QAction(QIcon(":images/backward.png"), tr("&Back"), this);
	connect(backward, SIGNAL(triggered()), docs, SLOT(backward()));
	connect(docs, SIGNAL(backwardAvailable(bool)), backward, SLOT(setEnabled(bool)));
	toolbar->addAction(backward);

	QAction *forward = new QAction(QIcon(":images/forward.png"), tr("&Forward"), this);
	connect(forward, SIGNAL(triggered()), docs, SLOT(forward()));
	connect(docs, SIGNAL(forwardAvailable(bool)), forward, SLOT(setEnabled(bool)));
	toolbar->addAction(forward);

	QAction *home = new QAction(QIcon(":images/home.png"), tr("&Home"), this);
	connect(home, SIGNAL(triggered()), docs, SLOT(home()));
	toolbar->addAction(home);

	toolbar->addSeparator();

	QAction *exit = new QAction(QIcon(":images/exit.png"), tr("&Exit"), this);
	connect(exit, SIGNAL(triggered()), this, SLOT(close()));
	toolbar->addAction(exit);

	layout = new QVBoxLayout;
	layout->addWidget(toolbar);
	layout->addWidget(docs);

	this->setLayout(layout);
	this->setWindowTitle(QObject::tr("BASIC-256 Reference"));
	this->show();

	docs->setSearchPaths(QStringList()
		<<	QApplication::applicationDirPath() + "/help/"
		<<	"/usr/share/basic256/help/"
		<<	"./"
		<<	QApplication::applicationDirPath() + "/../wikihelp/help/"
		<<	QApplication::applicationDirPath() + "/wikihelp/help/"
		 );

}

void DocumentationWin::go(QString word) {
	
	if (word == "") {
		docs->setSource(QUrl(indexfile));
		if(docs->toPlainText().length() == 0) {
			QMessageBox msgBox;
			msgBox.setText(tr("Off-line help does not appear to be installed.  Please use on-line help."));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
	} else {
		QString u = localecode.left(2) + "_" + word + ".html";
		docs->setSource(QUrl(u.toLower()));
		if(docs->toPlainText().length() == 0) {
			go("");
			QMessageBox msgBox;
			msgBox.setText(tr("Contextual help for the word '") + word + tr("' is not available."));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
	}
}


void DocumentationWin::resizeEvent(QResizeEvent *e) {
	this->resize(size());
}

void DocumentationWin::closeEvent(QCloseEvent *e) {
	QSettings settings(SETTINGSORG, SETTINGSAPP);
	settings.setValue(SETTINGSDOCSIZE, size());
	settings.setValue(SETTINGSDOCPOS, pos());
}
