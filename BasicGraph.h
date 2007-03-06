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


#ifndef __BASICGRAPH_H
#define __BASICGRAPH_H

#include <QWidget>
#include <QPainter>
#include <QKeyEvent>

#include "ViewWidgetIFace.h"
#include "BasicOutput.h"

class BasicGraph : public QWidget, public ViewWidgetIFace
{
  Q_OBJECT;
 public:
  BasicGraph(BasicOutput *, unsigned int gsize);
  QImage *image;
  QImage *imask;

 	bool initActions(QMenu *, ToolBar *);
 
 public slots:
	void slotCopy();
	void slotPrint();
 
 protected:
  void paintEvent(QPaintEvent *);
  void keyPressEvent(QKeyEvent *);
  
 private:
  uchar *imagedata;
  uchar *maskdata;
  BasicOutput *output;
  unsigned int gsize;
};


#endif
