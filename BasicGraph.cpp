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
#include <QMessageBox>

#include "BasicGraph.h"
#include "MainWindow.h"

QMutex keymutex;
int currentKey;

BasicGraph::BasicGraph(BasicOutput *o)
{
  image = NULL;
  imask = NULL;
  resize(GSIZE_MIN, GSIZE_MIN);
  output = o;
}

void 
BasicGraph::resize(int width, int height)
{
  if (image != NULL && width == image->width() && height == image->height())
    {
      return;
    }
  if (width > GWIDTH_MAX) { width = GWIDTH_MAX; }
  if (height > GHEIGHT_MAX) { height = GHEIGHT_MAX; }
  gwidth  = width;
  gheight = height;
  delete image;
  delete imask;
  imagedata = new uchar[sizeof(int) * width * height];
  maskdata  = new uchar[width * height];	
  image = new QImage(imagedata, width, height, QImage::Format_ARGB32);
  imask = new QImage(maskdata, width, height, QImage::Format_Mono);
}


void
BasicGraph::paintEvent(QPaintEvent *)
{
  QPainter p2(this);
  image->setAlphaChannel(*imask);
  p2.drawImage((width() - gwidth) / 2,
	       (height() - gheight) / 2,
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

bool BasicGraph::initActions(QMenu * vMenu, ToolBar * vToolBar)
{
	if ((NULL == vMenu) || (NULL == vToolBar))
	{
		return false;
	}

	QAction *copyAct = vMenu->addAction(QObject::tr("Copy"));
	QAction *printAct = vMenu->addAction(QObject::tr("Print"));

	vToolBar->addAction(copyAct);
	vToolBar->addAction(printAct);
	
	QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(slotCopy()));
	QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
		
	m_usesToolBar = true;
	m_usesMenu = true;

	return true;	
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
		if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted))
		{
			QPainter painter(&printer);
			QRect rect = painter.viewport();
			QSize size = image->size();
			size.scale(rect.size(), Qt::KeepAspectRatio);
			painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
			painter.setWindow(image->rect());
			painter.drawImage(0, 0, *image);
		}
		else
		{
			QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
		}		
	}	
}
