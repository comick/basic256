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


using namespace std;
#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QStatusBar>
#include <QFile>
#include <unistd.h>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
  QApplication qapp(argc, argv);
  int opt = getopt(argc, argv, "l:");
  char *lang = NULL;

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

  QTranslator qtTranslator;
  if (lang)
    {
      qtTranslator.load("qt_" + QString(lang));
    }
  else
    {
      qtTranslator.load("qt_" + QLocale::system().name());
    }
  qapp.installTranslator(&qtTranslator);
  
  QTranslator kbTranslator;
  if (lang)
    {
      kbTranslator.load("basic256_" + QString(lang), qApp->applicationDirPath() + "/Translations/");
    }
  else
    {
      kbTranslator.load("basic256_" + QLocale::system().name(), qApp->applicationDirPath() + "/Translations/");
    }
  qapp.installTranslator(&kbTranslator);

  MainWindow *mainwin = new MainWindow();
	mainwin->setObjectName( "mainwin" );
	mainwin->setWindowState(mainwin->windowState() & Qt::WindowMaximized);
  
  mainwin->resize(800,600);
  mainwin->setWindowTitle(QObject::tr("Untitled - BASIC-256"));
  mainwin->statusBar()->showMessage(QObject::tr("Ready."));
  mainwin->show();

  // load initial file
  if (argv[optind]) {
    QString s = QString::fromAscii(argv[optind]);
    if (s.endsWith(".kbs")) {
        QFile f(s);
        if (f.exists()) {
            mainwin->editor->loadFile(s);
        }
    }
  }

  return qapp.exec();
}
