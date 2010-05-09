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
#ifndef WIN32
#include <unistd.h>
#endif 

using namespace std;

#include "MainWindow.h"

int main(int argc, char *argv[])
{
  QApplication qapp(argc, argv);
  char *lang = NULL;

#ifndef WIN32
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
#ifndef WIN32
      kbTranslator.load("basic256_" + QString(lang), qApp->applicationDirPath() + "/Translations/");
#else
      kbTranslator.load("basic256_" + QString(lang), "/usr/share/basic256/");
#endif
    }
  else
    {
#ifndef WIN32
      kbTranslator.load("basic256_" + QLocale::system().name(), qApp->applicationDirPath() + "/Translations/");
#else
      kbTranslator.load("basic256_" + QLocale::system().name(), "/usr/share/basic256/");
#endif
    }
  qapp.installTranslator(&kbTranslator);

  MainWindow *mainwin = new MainWindow();
  mainwin->setObjectName( "mainwin" );
  mainwin->setWindowState(mainwin->windowState() & Qt::WindowMaximized);
  mainwin->resize(800,600);
  mainwin->setWindowTitle(QObject::tr("Untitled - BASIC-256"));
  mainwin->statusBar()->showMessage(QObject::tr("Ready."));
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
