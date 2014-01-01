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

#include <QClipboard>
#include <QMutex>
#include <QPainter>

#if QT_VERSION >= 0x050000
    #include <QtPrintSupport/QPrintDialog>
    #include <QtPrintSupport/QPrinter>
    #include <QtWidgets/QAction>
    #include <QtWidgets/QApplication>
    #include <QtWidgets/QMessageBox>
#else
    #include <QAction>
    #include <QApplication>
    #include <QMessageBox>
    #include <QPrintDialog>
    #include <QPrinter>
#endif

#include "BasicGraph.h"

extern QMutex *mymutex;
extern int currentKey;

BasicGraph::BasicGraph()
{
  image = NULL;
  resize(GSIZE_INITIAL_WIDTH, GSIZE_INITIAL_HEIGHT);
  setMinimumSize(gwidth, gheight);
  gridlines = false;
}

void
BasicGraph::resize(int width, int height)
{
	if (image != NULL && width == image->width() && height == image->height()) {
		return;
    }
	gwidth  = width;
	gheight = height;
	delete image;
	image = new QImage(width, height, QImage::Format_ARGB32);

	// clear the new image
    QPainter ian(image);
	ian.setPen(QColor(0,0,0,0));
	ian.setBrush(QColor(0,0,0,0));
	ian.setCompositionMode(QPainter::CompositionMode_Clear);
	ian.drawRect(0, 0, width, height);
	ian.end();

	mouseX = 0;
	mouseY = 0;
	mouseB = 0;
	clickX = 0;
	clickY = 0;
	clickB = 0;
	setMouseTracking(true);
}

bool BasicGraph::isVisibleGridLines() {
	return gridlines;
}

void
BasicGraph::paintEvent(QPaintEvent *)
{
	unsigned int tx, ty;
  
	QPainter p2(this);
	gtop = (height() - gheight) / 2;
	gleft = (width() - gwidth) / 2;
	
	if (gridlines) {
		p2.setPen(QColor(128,128,128,255));
		for(tx=0; tx<gwidth; tx=tx+10) {
			if (tx%100==0) {
				p2.setPen(QColor(64,64,64,255));
			} else {
				p2.setPen(QColor(128,128,128,255));
			}
			p2.drawLine(tx+gleft, gtop, tx+gleft, gheight+gtop);
		}

		for(ty=0; ty<gheight; ty=ty+10) {
			if (ty%100==0) {
				p2.setPen(QColor(64,64,64,255));
			} else {
				p2.setPen(QColor(128,128,128,255));
			}
			p2.drawLine(gleft, ty+gtop, gwidth+gleft, ty+gtop);
		}
		
		p2.setPen(QColor(64,64,64,255));
		p2.setFont(QFont("Sans", 6, 100));
		char buffer[64];
		for(tx=0; tx<gwidth; tx=tx+100) {
			for(ty=0; ty<gheight; ty=ty+100) {
				sprintf(buffer, "%u,%u", tx, ty);
				p2.drawText(gleft+tx+2, gtop+ty+(QFontMetrics(p2.font()).ascent())+1, buffer);
			}
		}
	}
	
	p2.drawImage(gleft, gtop, *image);
}


void 
BasicGraph::keyPressEvent(QKeyEvent *e)
{
  e->accept();
  
  mymutex->lock();
  currentKey = e->key();
  mymutex->unlock();
}

void BasicGraph::mouseMoveEvent(QMouseEvent *e) {
    if (e->x() >= (int) gleft && e->x() < (int) (gleft+gwidth) && e->y() >= (int) gtop && e->y() < (int) (gtop+gheight)) { 
		mouseX = e->x() - gleft;
		mouseY = e->y() - gtop;
		mouseB = e->buttons();
	}
}

void BasicGraph::mouseReleaseEvent(QMouseEvent *e) {
	// cascade call to mouse move so we record clicks real time like moves
	mouseMoveEvent(e);
}

void BasicGraph::mousePressEvent(QMouseEvent *e) {
    if (e->x() >= (int) gleft && e->x() < (int) (gleft+gwidth) && e->y() >= (int) gtop && e->y() < (int) (gtop+gheight)) { 
		clickX = mouseX = e->x() - gleft;
		clickY = mouseY = e->y() - gtop;
		clickB = mouseB = e->buttons();
	}
    setFocus();
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

void BasicGraph::slotGridLines(bool visible)
{
	gridlines = visible;
	update();
}

void BasicGraph::slotCopy()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setImage(*image);
}

void BasicGraph::slotPrint()
{

#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else

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
#endif

}
