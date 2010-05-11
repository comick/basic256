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
	QString localecode = ((MainWindow *) parent)->localecode;
	QString helpfile;

	resize(700,500);

	docs = new QTextBrowser( this );

	docs->setSearchPaths(QStringList() << "./help" << "/usr/share/basic256/help");

	helpfile = "help_" + localecode + ".html";
	docs->setSource(QUrl(QString(helpfile)));
	if (docs->toPlainText().isEmpty()) {
		// fall back to just the first two letters
		helpfile = "help_" + localecode.left(2) + ".html";
		docs->setSource(QUrl(QString(helpfile)));
		if (docs->toPlainText().isEmpty()) {
			// fall back to english
			helpfile = "help_en.html";
			docs->setSource(QUrl(QString(helpfile)));
			if (docs->toPlainText().isEmpty()) {
				docs->setHtml(QString("<html><body><h1>help folder and help items missing.</h1></body></html>"));
			}
		}
	}
}

void DocumentationWin::resizeEvent(QResizeEvent *e) {
	docs->resize(size());
}



