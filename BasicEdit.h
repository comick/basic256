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


#ifndef __BASICEDIT_H
#define __BASICEDIT_H


#include <QApplication>
#include <QTextEdit>
#include <QMainWindow>
#include <QKeyEvent>

#include "ViewWidgetIFace.h"

class BasicEdit : public QTextEdit, public ViewWidgetIFace
{
  Q_OBJECT;
 public:
  BasicEdit(QMainWindow *);
  void loadFile(QString);
  bool codeChanged;

 public slots:
  void newProgram();
  void saveProgram();
  void saveAsProgram();
  void loadProgram();
  void cursorMove();
  void goToLine(int);
  void highlightLine(int);
  void slotPrint();
  void beautifyProgram();
  void fontSmall();
  void fontMedium();
  void fontLarge();
  void fontHuge();
  void loadRecent0();
  void loadRecent1();
  void loadRecent2();
  void loadRecent3();
  void loadRecent4();
  void loadRecent5();
  void loadRecent6();
  void loadRecent7();
  void loadRecent8();

 
 protected:
  void keyPressEvent(QKeyEvent *);

 private:
  QMainWindow *mainwin;
  int currentMaxLine;
  int currentLine;
  int startPos;
  int linePos;
  QString filename;
  void changeFontSize(unsigned int);
  void addFileToRecentList(QString);
 void loadRecent(int);

};


#endif
