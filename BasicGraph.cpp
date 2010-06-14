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
#include <QMutex>
#include <QAction>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QClipboard>
#include <QMessageBox>
using namespace std;

#include "BasicGraph.h"
#include "MainWindow.h"

QMutex keymutex;
int currentKey;

BasicGraph::BasicGraph(BasicOutput *o)
{
  image = NULL;
  resize(GSIZE_MIN, GSIZE_MIN);
  output = o;
  setMinimumSize(gwidth, gheight);
}

void
BasicGraph::resize(int width, int height)
{
	if (image != NULL && width == image->width() && height == image->height()) {
		return;
    }
	if (width > GWIDTH_MAX) { width = GWIDTH_MAX; }
	if (height > GHEIGHT_MAX) { height = GHEIGHT_MAX; }
	gwidth  = width;
	gheight = height;
	delete image;
	imagedata = new uchar[sizeof(int) * width * height];
	image = new QImage(imagedata, width, height, QImage::Format_ARGB32);
	image->fill(Qt::color0);
	mouseX = 0;
	mouseY = 0;
	mouseB = 0;
	clickX = 0;
	clickY = 0;
	clickB = 0;
	setMouseTracking(true);
}


void
BasicGraph::paintEvent(QPaintEvent *)
{
  QPainter p2(this);
  gtop = (height() - gheight) / 2;
  gleft = (width() - gwidth) / 2;
  p2.drawImage(gleft, gtop, *image);
}


void 
BasicGraph::keyPressEvent(QKeyEvent *e)
{
  e->accept();
  
  keymutex.lock();
  currentKey = e->key();
  keymutex.unlock();
}

void BasicGraph::mouseMoveEvent(QMouseEvent *e) {
    if (e->x() >= (int) gleft && e->x() < (int) (gleft+gwidth) && e->y() >= (int) gtop && e->y() < (int) (gtop+gheight)) { 
		mouseX = e->x() - gleft;
		mouseY = e->y() - gtop;
		mouseB = e->buttons();
	}
}

void BasicGraph::mouseReleaseEvent(QMouseEvent *e) {
	// cascace call to mouse move so we record clicks real time like moves
	mouseMoveEvent(e);
}

void BasicGraph::mousePressEvent(QMouseEvent *e) {
    if (e->x() >= (int) gleft && e->x() < (int) (gleft+gwidth) && e->y() >= (int) gtop && e->y() < (int) (gtop+gheight)) { 
		clickX = mouseX = e->x() - gleft;
		clickY = mouseY = e->y() - gtop;
		clickB = mouseB = e->buttons();
	}
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