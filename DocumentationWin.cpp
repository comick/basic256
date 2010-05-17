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
	bool lookharder = true;
	
	QStringList searchpaths;
	searchpaths << "./help/" << "/usr/share/basic256/help/";
	
	resize(700,500);
	
	docs = new QWebView();

	toolbar = new QToolBar(tr("Help Navigation"));
    toolbar->addAction(docs->pageAction(QWebPage::Back));
    toolbar->addAction(docs->pageAction(QWebPage::Forward));
	
	layout = new QVBoxLayout;
     layout->addWidget(toolbar);
     layout->addWidget(docs);

     this->setLayout(layout);
     this->show();
	
	// set lookharder to false once we find a good candidate html file
	for ( QStringList::Iterator path = searchpaths.begin(); path != searchpaths.end() && lookharder; ++path ) {
		helpfile = *path + "help_" + localecode + ".html";
		if (QFile::exists(helpfile)) {
			lookharder = false;
		}
		else
		{
			// fall back to just the first two letters
			helpfile = *path + "help_" + localecode.left(2) + ".html";
			if (QFile::exists(helpfile)) {
				lookharder = false;
			}
			else
			{
				// fall back to english
				helpfile = *path + "help_en.html";
				if (QFile::exists(helpfile)) {
						lookharder = false;
				}
			}
		}
    }

	if (lookharder) {
		// unable to find help
		docs->setHtml(QString("<html><body><h1>help folder and help items missing.</h1></body></html>"));
	}
	else
	{
		docs->load(helpfile);
	}
	docs->show();
	
}



