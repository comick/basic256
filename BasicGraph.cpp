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
#include <unordered_set>

#include <QClipboard>
#include <QMutex>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QDockWidget>

#include "BasicWidget.h"
#include "BasicGraph.h"
#include "Constants.h"

extern QMutex *mymutex;
extern int lastKey;
extern std::unordered_set<int> pressedKeys;


BasicGraph::BasicGraph() {
    image = NULL;
    gridlinesimage = NULL;
    displayedimage = NULL;
    spritesimage = NULL;
    sprites_clip_region = QRegion(0,0,0,0);
    gridlines = false;
    draw_sprites_flag = false;
    gwidth = 0;
    gheight = 0;
    setFocusPolicy(Qt::StrongFocus);
    resize(GSIZE_INITIAL_WIDTH, GSIZE_INITIAL_HEIGHT);
    setMouseTracking(true);
}

BasicGraph::~BasicGraph() {
    if (image) {
        delete image;
        image = NULL;
    }
    if (gridlinesimage) {
        delete gridlinesimage;
        gridlinesimage = NULL;
    }
    if (displayedimage) {
        delete displayedimage;
        displayedimage = NULL;
    }
    if (spritesimage) {
        delete spritesimage;
        spritesimage = NULL;
    }
}

void BasicGraph::resize(int width, int height) {
    if (width == gwidth && height == gheight) return;
    gwidth = width;
    gheight = height;

    // delete the old image and then create a new one the right size
    if(image){
        delete image;
        image = NULL;
    }
    image = new QImage(width, height, QImage::Format_ARGB32);
    image->fill(Qt::transparent);

    // delete displayed image and then create a new one the right size
    if(displayedimage){
        delete displayedimage;
        displayedimage = NULL;
    }
    displayedimage = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    displayedimage->fill(Qt::transparent);


    // delete sprites image and then create a new one the right size
    if(spritesimage){
        delete spritesimage;
        spritesimage = NULL;
    }
    spritesimage = new QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    spritesimage->fill(Qt::transparent);


    // delete the old gridlinesimage if exist and draw new one if required
    if(gridlinesimage){
        delete gridlinesimage;
        gridlinesimage = NULL;
    }
    if (gridlines)
        drawGridLines();


    mouseX = 0;
    mouseY = 0;
    mouseB = 0;
    clickX = 0;
    clickY = 0;
    clickB = 0;

    setFixedSize(width, height);

    //force resize
    //if graph window is floating, then also resize window
    //graph-scroll-basicwidget-dock
    if(parentWidget()){
        QDockWidget * dock = (QDockWidget *) parentWidget()->parentWidget()->parentWidget()->parentWidget();
        if(dock->isFloating()){
            QScrollArea * scroll = (QScrollArea *)parentWidget()->parentWidget();
            if (scroll) {
                scroll->setMinimumSize(width,height);
                QWidget *basicwidget = parentWidget()->parentWidget()->parentWidget();
                basicwidget->adjustSize();
                dock->adjustSize();
                scroll->setMinimumSize(0,0);
            }
        }
    }
}

bool BasicGraph::isVisibleGridLines() {
    return gridlines;
}

void BasicGraph::paintEvent(QPaintEvent *e) {
    QPainter painter(this);
    painter.drawImage(e->rect(), *displayedimage, e->rect());
    if (gridlines) {
        if(!gridlinesimage) drawGridLines();
        painter.drawImage(e->rect(), *gridlinesimage, e->rect());
    }
}


void BasicGraph::keyPressEvent(QKeyEvent *e) {
    e->accept();
    mymutex->lock();
    lastKey = e->key();
    if(!e->isAutoRepeat()){
        // Note that if the event is a multiple-key compressed event that is partly due to auto-repeat,
        // isAutoRepeat() function could return either true or false indeterminately.
        if( e->modifiers() & Qt::ShiftModifier )
        {
                pressedKeys.insert(Qt::Key_Shift);
        }else{
                pressedKeys.erase(Qt::Key_Shift);
        }
        if( e->modifiers() & Qt::ControlModifier )
        {
                pressedKeys.insert(Qt::Key_Control);
        }else{
                pressedKeys.erase(Qt::Key_Control);
        }
        if( e->modifiers() & Qt::AltModifier )
        {
                pressedKeys.insert(Qt::Key_Alt);
        }else{
                pressedKeys.erase(Qt::Key_Alt);
        }
        if( e->modifiers() & Qt::MetaModifier )
        {
                pressedKeys.insert(Qt::Key_Meta);
        }else{
                pressedKeys.erase(Qt::Key_Meta);
        }
        pressedKeys.insert(e->key());
    }
    mymutex->unlock();
}

void BasicGraph::keyReleaseEvent(QKeyEvent *e) {
    e->accept();
    mymutex->lock();
    if(!e->isAutoRepeat())pressedKeys.erase(e->key());
    if( e->modifiers() & Qt::ShiftModifier )
    {
            pressedKeys.insert(Qt::Key_Shift);
    }else{
            pressedKeys.erase(Qt::Key_Shift);
    }
    if( e->modifiers() & Qt::ControlModifier )
    {
            pressedKeys.insert(Qt::Key_Control);
    }else{
            pressedKeys.erase(Qt::Key_Control);
    }
    if( e->modifiers() & Qt::AltModifier )
    {
            pressedKeys.insert(Qt::Key_Alt);
    }else{
            pressedKeys.erase(Qt::Key_Alt);
    }
    if( e->modifiers() & Qt::MetaModifier )
    {
            pressedKeys.insert(Qt::Key_Meta);
    }else{
            pressedKeys.erase(Qt::Key_Meta);
    }
    mymutex->unlock();
}

void BasicGraph::focusOutEvent(QFocusEvent* ){
    //clear pressed keys list when lose focus to avoid detecting still pressed keys
    pressedKeys.clear();
}


void BasicGraph::mouseMoveEvent(QMouseEvent *e) {
	static int c = Qt::ArrowCursor;
    if (e->x() >= 0 && e->x() < gwidth && e->y() >= 0 && e->y() < gheight) {
        mouseX = e->x();
        mouseY = e->y();
		mouseB = e->buttons();

		if(gridlines){
			this->setToolTip(QString::number( mouseX ) + ", " + QString::number( mouseY ));

			if(c!=Qt::CrossCursor){
				this->setCursor(Qt::CrossCursor);
				c=Qt::CrossCursor;
			}
		}else{
			if(c!=Qt::ArrowCursor){
				this->setCursor(Qt::ArrowCursor);
				this->setToolTip("");
				c=Qt::ArrowCursor;
			}
		}
	}else{
		if(c!=Qt::ArrowCursor){ //if leave the image area, then do that only once
			this->setToolTip("");
			this->setCursor(Qt::ArrowCursor);
			c=Qt::ArrowCursor;
		}
	}
}

void BasicGraph::mouseReleaseEvent(QMouseEvent *e) {
    // cascade call to mouse move so we record clicks real time like moves
    mouseMoveEvent(e);
}

void BasicGraph::mousePressEvent(QMouseEvent *e) {
    if (e->x() >= 0 && e->x() < gwidth && e->y() >= 0 && e->y() < gheight) {
        clickX = mouseX = e->x();
        clickY = mouseY = e->y();
		clickB = e->button();
		mouseB = e->buttons();
	}
}

bool BasicGraph::initActions(QMenu * vMenu, QToolBar * vToolBar) {
	if ((NULL == vMenu) || (NULL == vToolBar)) {
		return false;
	}

	vToolBar->setObjectName("graphtoolbar");

    copyAct = vMenu->addAction(QObject::tr("Copy"));
    printAct = vMenu->addAction(QObject::tr("Print"));
    clearAct = vMenu->addAction(QObject::tr("Clear"));

	vToolBar->addAction(copyAct);
	vToolBar->addAction(printAct);
    vToolBar->addAction(clearAct);

	QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(slotCopy()));
	QObject::connect(printAct, SIGNAL(triggered()), this, SLOT(slotPrint()));
    QObject::connect(clearAct, SIGNAL(triggered()), this, SLOT(slotClear()));

	m_usesToolBar = true;
	m_usesMenu = true;

	return true;
}

void BasicGraph::slotGridLines(bool visible) {
	gridlines = visible;
	update();
}

void BasicGraph::slotCopy() {
	QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(*displayedimage);
	QApplication::processEvents();
}

void BasicGraph::slotPrint() {

#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Printing is not supported in this platform at this time."));
#else

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print Graphics Output"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted)) {
            QPainter painter(&printer);
            QRect rect = painter.viewport();
            QSize size = displayedimage->size();
            size.scale(rect.size(), Qt::KeepAspectRatio);
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(displayedimage->rect());
            painter.drawImage(0, 0, *displayedimage);
        } else {
            QMessageBox::warning(this, QObject::tr("Print Error"), QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }
    }
#endif

}


void BasicGraph::drawGridLines(){
    gridlinesimage = new QImage(gwidth, gheight, QImage::Format_ARGB32_Premultiplied);
    gridlinesimage->fill(Qt::transparent);
    int tx, ty;

    QPainter painter(gridlinesimage);

    const QColor darkColor = QColor(64,64,64,255);
    const QColor lightColor = QColor(128,128,128,255);
    painter.setFont(QFont("Sans", 6, 100));

    painter.setPen(lightColor);
    for(tx=10; tx<gwidth; tx=tx+10) if (tx%100!=0) painter.drawLine(tx, 0, tx, gheight);
    for(ty=10; ty<gheight; ty=ty+10) if (ty%100!=0) painter.drawLine(0, ty, gwidth, ty);
    painter.setPen(darkColor);
    for(tx=0; tx<gwidth; tx=tx+100) painter.drawLine(tx, 0, tx, gheight);
    for(ty=0; ty<gheight; ty=ty+100) painter.drawLine(0, ty, gwidth, ty);

    const int fontAscent = (QFontMetrics(painter.font()).ascent())+1;
    char buffer[64];
    for(tx=0; tx<gwidth; tx=tx+100) {
        for(ty=0; ty<gheight; ty=ty+100) {
            sprintf(buffer, "%u,%u", tx, ty);
            painter.drawText(tx+2, ty+fontAscent, buffer);
        }
    }
}

void BasicGraph::updateScreenImage(){
    if(draw_sprites_flag){
        QImage tmp = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QRectF target(0.0, 0.0, tmp.width(), tmp.height() );
        QPainter painter;
        painter.begin(&tmp);
        painter.setClipRegion(sprites_clip_region);
        painter.drawImage(target, *spritesimage);
        painter.end();
        displayedimage->swap(tmp);
    }else{
        *displayedimage = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
}

void BasicGraph::mouseDoubleClickEvent(QMouseEvent * e){
    if (e->x() >= 0 && e->x() < gwidth && e->y() >= 0 && e->y() < gheight) {
        clickX = mouseX = e->x();
        clickY = mouseY = e->y();
        clickB = e->button() | MOUSEBUTTON_DOUBLECLICK; //set doubleclick flag
        mouseB = e->buttons();
    }
}

void BasicGraph::slotClear(){
    displayedimage->fill(Qt::transparent);
    repaint();
}
