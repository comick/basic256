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
#include <QMutex>
#include <QAction>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QClipboard>

#include "BasicGraph.h"

QMutex keymutex;
int currentKey;

BasicGraph::BasicGraph(BasicOutput *o)
{
  image = new QImage(imagedata, 300, 300, QImage::Format_ARGB32);
  imask = new QImage(maskdata, 300, 300, QImage::Format_Mono);
  output = o;
}

void
BasicGraph::paintEvent(QPaintEvent *)
{
  QPainter p2(this);
  image->setAlphaChannel(*imask);
  p2.drawImage((width() - 300) / 2,
	       (height() - 300) / 2,
	       *image);
}


void 
BasicGraph::keyPressEvent(QKeyEvent *e)
{
  e->accept();
  
  keymutex.lock();
  currentKey = e->key();
  keymutex.unlock();
}

bool BasicGraph::initToolBar(ToolBar * vToolBar)
{
	// To switch on the toolbar comment the following line.
	return ViewWidgetIFace::initToolBar(vToolBar);
	
	// Add some buttons to the toolbar.
	if (NULL != vToolBar)
	{
		QAction *copyAct = vToolBar->addAction(QObject::tr("Copy"));
		QAction *printAct = vToolBar->addAction(QObject::tr("Print"));
		
		QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(slotCopy()));
		QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
		
		return true;
	}
	
	return false;	
}

void BasicGraph::slotCopy()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setImage(*image);
}

void BasicGraph::slotPrint()
{
	QPrinter printer(QPrinter::HighResolution);
	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(QObject::tr("Print Graphics Output"));
	
	if (dialog->exec() == QDialog::Accepted) 
	{
		QPainter painter(&printer);
		QRect rect = painter.viewport();
		QSize size = image->size();
		size.scale(rect.size(), Qt::KeepAspectRatio);
		painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
		painter.setWindow(image->rect());
		painter.drawImage(0, 0, *image);
	}	
}
