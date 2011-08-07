/** Copyright (C) 2006, Ian Paul Larsen.
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

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QStatusBar>
#include <QFileInfo>
#include <locale.h>
#if !defined(WIN32) || defined(__MINGW32__)
#include <unistd.h>
#endif 

using namespace std;

#include "MainWindow.h"

int main(int argc, char *argv[])
{
	QApplication qapp(argc, argv);
	char *lang = NULL;		// locale code passed with argument -l on command line
	bool loadandgo = false;		// if -r option then run code in loadandgo mode
	bool ok;
	QString localecode;		// either lang or the system localle - stored on mainwin for help display

	#if !defined(WIN32) || defined(__MINGW32__)

		while (true)
		{
			int opt = getopt(argc, argv, "rl:");
			if (opt == -1) break;

			switch ((char) opt)
			{
				case 'l':
					lang = optarg;
					break;
				case 'r':
					loadandgo = true;
					break;
				default:
					break;
			}
		}
	#endif

	if (lang) {
		localecode = QString(lang);
	} else {
		localecode = QLocale::system().name();
	}

	QTranslator qtTranslator;
	#ifdef WIN32
		ok = qtTranslator.load("qt_" + localecode);
	#else
		ok = qtTranslator.load("qt_" + localecode, "/usr/share/qt4/translations/");
	#endif
	qapp.installTranslator(&qtTranslator);

	QTranslator kbTranslator;
	#ifdef WIN32
		ok = kbTranslator.load("basic256_" + localecode, qApp->applicationDirPath() + "/Translations/");
	#else
		ok = kbTranslator.load("basic256_" + localecode, "/usr/share/basic256/");
	#endif
	qapp.installTranslator(&kbTranslator);

	MainWindow *mainwin = new MainWindow();

	mainwin->setObjectName( "mainwin" );
	//mainwin->setWindowState(mainwin->windowState() & Qt::WindowMaximized);
	//mainwin->resize(800,600);
	mainwin->setWindowTitle(QObject::tr("Untitled - BASIC-256"));
	mainwin->statusBar()->showMessage(QObject::tr("Ready."));
	mainwin->localecode = localecode;
	mainwin->show();

	#if !defined(WIN32) || defined(__MINGW32__)
		// load initial file -- POSIX style
		if (optind < argc) {
			// printf("extra arg %s\n",argv[optind]);
			QString s = QString::fromUtf8(argv[optind]);
			if (s.endsWith(".kbs")) {
				QFileInfo fi(s);
				if (fi.exists()) {
					mainwin->editor->loadFile(fi.absoluteFilePath());
				}
			}
		}
	#else
		// load initial file -- Win style
		if (argc >= 1 && argv[1] != NULL)
		{
			QString s = QString::fromUtf8(argv[1]);
			if (s.endsWith(".kbs")) {
				QFileInfo fi(s);
				if (fi.exists()) {
					mainwin->editor->loadFile(fi.absoluteFilePath());
				}
			}
		}
	#endif

	setlocale(LC_ALL,"C");

	if (loadandgo) mainwin->loadAndGoMode();

	int returnval = qapp.exec();

	mainwin->~MainWindow();
 	return returnval;
}
