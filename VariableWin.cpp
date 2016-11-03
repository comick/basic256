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



#include <qglobal.h>

#include <QtWidgets/QHeaderView>
#include <QMutex>
#include <QWaitCondition>

//#include "VariableWin.h"
#include "DataElement.h"
#include "MainWindow.h"

extern MainWindow * mainwin;

extern QMutex* mymutex;
extern QWaitCondition* waitCond;

extern "C" {
    extern char *symtable[];
    extern int numsyms;
}

VariableWin::VariableWin () {
	setColumnCount(3);
	setHeaderLabels(QStringList() << tr("Level - Name") << tr("Ty") << tr("Value"));
	resizeColumnToContents(1);
	sortByColumn(0,Qt::AscendingOrder);
	setSortingEnabled(true);
	convert = new Convert(NULL, mainwin->locale);
}

VariableWin::~VariableWin() {
	delete(convert);
}

void VariableWin::setTypeAndValue(QTreeWidgetItem *r, DataElement *d) {
	// set thr type and value columns (2 and 3)
	if (r) {
		if (d!=NULL && d->type!=T_UNASSIGNED) {
			if (d->type==T_STRING) r->setText(1,"S");
			if (d->type==T_INT) r->setText(1,"I");
			if (d->type==T_FLOAT) r->setText(1,"F");
			r->setText(2, convert->getString(d));
		} else {
			r->setText(1, tr("?"));
			r->setText(2, tr(""));
		}
	}

}


void VariableWin::removeArrayEntries(QString name) {
	// remove old entries when a variable becomes an array or not
	// "ref - VAR[" - call this private when dimming an array
	// or setting a previous array to a non array
	QList<QTreeWidgetItem *> items = findItems(name + "[", Qt::MatchStartsWith | Qt::MatchRecursive, 0);
	for (int n=items.size()-1; n>=0; n--) {
		delete items[n];
	}
}

void VariableWin::varWinAssign(Variables **variables, int varnum, int x, int y) {
	QTreeWidgetItem *rowItem = NULL;
	QString name;
	VariableInfo *vi;
	DataElement *d;
	
    mymutex->lock();
    if(*variables){
        vi = (*variables)->getInfo(varnum);
        name = QString::number(vi->level) + " - " + symtable[vi->varnum];
        delete vi;
        d = (*variables)->arraygetdata(varnum,x,y);

        // find either name[x] or name[x,y]
        QString tname;
        tname = name + "[" + QString::number(x) + "]";
        QList<QTreeWidgetItem *> list = findItems(tname, Qt::MatchExactly | Qt::MatchRecursive, 0);
        if (list.size()>0) {
            rowItem = list[0];
        } else {
            tname = name + "[" + QString::number(x) + "," + QString::number(y) + "]";
            QList<QTreeWidgetItem *> list = findItems(tname, Qt::MatchExactly | Qt::MatchRecursive, 0);
            if (list.size()>0) rowItem = list[0];
        }
        // set the data and data type
        setTypeAndValue(rowItem, d);
    }
    waitCond->wakeAll();
    mymutex->unlock();
}

void VariableWin::varWinAssign(Variables **variables, int varnum) {
	// simple variable assignmemnt
	QTreeWidgetItem *rowItem;
	QString name;
	VariableInfo *vi;
	DataElement *d;
	
    mymutex->lock();
    if(*variables){
        vi = (*variables)->getInfo(varnum);
        name = QString::number(vi->level) + " - " + symtable[vi->varnum];
        delete vi;
        d = (*variables)->getdata(varnum);

        // not an array element - just add or replace name
        QList<QTreeWidgetItem *> list = findItems(name, Qt::MatchExactly | Qt::MatchRecursive, 0);
        if (list.size() > 0) {
            rowItem = list[0];
            if (rowItem->data(1,0)=="A") removeArrayEntries(name);
        } else {
            // add new element for a simple variable
            rowItem = new QTreeWidgetItem();
            rowItem->setText(0, name);
            addTopLevelItem(rowItem);
        }
        // display the data type and the data
        setTypeAndValue(rowItem, d);
    }
    waitCond->wakeAll();
    mymutex->unlock();
}



void VariableWin::varWinDimArray(Variables **variables, int varnum, int arraylenx, int arrayleny) {
	QTreeWidgetItem *rowItem;
	QString name;
	VariableInfo *vi;
	DataElement *d;

    mymutex->lock();
    if(*variables){
        vi = (*variables)->getInfo(varnum);
        name = QString::number(vi->level) + " - " + symtable[vi->varnum];
        delete vi;

        // create the top level for the new array
        // see if element is on the list and change value or add
        QList<QTreeWidgetItem *> list = findItems(name, Qt::MatchExactly | Qt::MatchRecursive, 0);

        if (list.size() > 0) {
            // get existing element
            rowItem = list[0];
            if (rowItem->data(1,0)=="A") removeArrayEntries(name);
        } else {
            // add new element
            rowItem = new QTreeWidgetItem();
            rowItem->setText(0, name);
            addTopLevelItem(rowItem);
        }
        rowItem->setText(1,  "A");
        rowItem->setText(2,  QString::number(arraylenx) + (arrayleny > 1?"," + QString::number(arrayleny):""));
        // add place holders for the array elements as children for a new array
        if (arrayleny <= 1) {
            // 1d array
            for(int x=0; x<arraylenx; x++) {
                QTreeWidgetItem *childItem = new QTreeWidgetItem();
                childItem->setText(0, name + "[" + QString::number(x) + "]");
                d = (*variables)->arraygetdata(varnum,x,0);
                setTypeAndValue(childItem, d);
                rowItem->addChild(childItem);
            }
        } else {
            // 2d array
            for(int x=0; x<arraylenx; x++) {
                for(int y=0; y<arrayleny; y++) {
                    QTreeWidgetItem *childItem = new QTreeWidgetItem();
                    childItem->setText(0, name + "[" + QString::number(x) + "," + QString::number(y) + "]");
                    d = (*variables)->arraygetdata(varnum,x,y);
                    setTypeAndValue(childItem, d);
                    rowItem->addChild(childItem);
                }
            }
        }
    }
    waitCond->wakeAll();
    mymutex->unlock();
}


void VariableWin::varWinDropLevel(int level) {
	// when we return from a subroutine or function delete its variables
	mymutex->lock();

	QString name;
	name = QString::number(level) + " ";
	QList<QTreeWidgetItem *> items = findItems(name, Qt::MatchStartsWith | Qt::MatchRecursive, 0);
	for (int n=items.size()-1; n>=0; n--) {
		delete items[n];
	}
	//rowItem = new QTreeWidgetItem();
	//rowItem->setText(0, name);
	//rowItem->setText(1, "X");
	//addTopLevelItem(rowItem);
	repaint();
		
	waitCond->wakeAll();
	mymutex->unlock();
}
