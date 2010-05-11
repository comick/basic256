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
#if !defined(WIN32) || defined(__MINGW32__)
#include <unistd.h>
#endif 

using namespace std;

#include "MainWindow.h"

int main(int argc, char *argv[])
{
  QApplication qapp(argc, argv);
  char *lang = NULL;		// locale code passed with argument -l on command line
  bool ok;
  QString localecode;		// either lang or the system localle - stored on mainwin for help display

#if !defined(WIN32) || defined(__MINGW32__)
  int opt = getopt(argc, argv, "l:");
  while (opt != -1)
    {
      switch ((char) opt)
      {
	    case 'l':
        lang = optarg;
        break;
      default:
        break;
      }
      opt = getopt(argc, argv, "l:");
    }
#endif

if (lang) {
localecode = QString(lang);
} else {
localecode = QLocale::system().name();
}

  QTranslator qtTranslator;
  ok = qtTranslator.load("qt_" + localecode);
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
  mainwin->setWindowState(mainwin->windowState() & Qt::WindowMaximized);
  mainwin->resize(800,600);
  mainwin->setWindowTitle(QObject::tr("Untitled - BASIC-256"));
  mainwin->statusBar()->showMessage(QObject::tr("Ready."));
  mainwin->localecode = localecode;
  mainwin->show();

#ifndef WIN32
  // load initial file
  if (argv[optind]) {
    QString s = QString::fromAscii(argv[optind]);
    if (s.endsWith(".kbs")) {
        QFileInfo fi(s);
        if (fi.exists()) {
            mainwin->editor->loadFile(fi.absoluteFilePath());
        }
    }
  }
#else
  if (argc >= 1 && argv[1] != NULL)
  {
	QString s = QString::fromAscii(argv[1]);
    if (s.endsWith(".kbs")) {
        QFileInfo fi(s);
        if (fi.exists()) {
            mainwin->editor->loadFile(fi.absoluteFilePath());
        }
    }
  }
#endif

  return qapp.exec();
}
