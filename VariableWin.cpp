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

using namespace std;

#include <qglobal.h>

#include <QtWidgets/QHeaderView>

//#include "VariableWin.h"
#include "DataElement.h"
#include "MainWindow.h"

extern MainWindow * mainwin;

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

void VariableWin::varWinAssign(Variables* variables, int varnum, int x, int y) {

	QTreeWidgetItem *rowItem = NULL;
	Convert *convert = new Convert(NULL);
	QString name;
	VariableInfo *vi;
	DataElement *d;
	
	vi = variables->getInfo(varnum);
	name = QString::number(vi->level) + " - " + symtable[vi->varnum];
	d = variables->arraygetdata(varnum,x,y);

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
	if (rowItem) {
		if (d!=NULL && d->type!=T_UNASSIGNED) {
			if (d->type==T_STRING) rowItem->setText(1,"S");
			if (d->type==T_INT) rowItem->setText(1,"I");
			if (d->type==T_FLOAT) rowItem->setText(1,"F");
			rowItem->setText(2, convert->getString(d));
		} else {
			rowItem->setText(1, tr("?"));
			rowItem->setText(2, tr(""));
		}
	}
}

void VariableWin::varWinAssign(Variables* variables, int varnum) {
	// simple variable assignmemnt
	
	QTreeWidgetItem *rowItem;
	Convert *convert = new Convert(NULL);
	QString name;
	VariableInfo *vi;
	DataElement *d;
	
	vi = variables->getInfo(varnum);
	name = QString::number(vi->level) + " - " + symtable[vi->varnum];
	d = variables->getdata(varnum);

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
	if (d!=NULL && d->type!=T_UNASSIGNED) {
		if (d->type==T_STRING) rowItem->setText(1,"S");
		if (d->type==T_INT) rowItem->setText(1,"I");
		if (d->type==T_FLOAT) rowItem->setText(1,"F");
		rowItem->setText(2, convert->getString(d));
	} else {
		rowItem->setText(1, tr("?"));
		rowItem->setText(2, tr(""));
	}
}



void VariableWin::varWinDimArray(Variables* variables, int varnum, int arraylenx, int arrayleny) {

	QTreeWidgetItem *rowItem;
	QString name;
	VariableInfo *vi;

	vi = variables->getInfo(varnum);
	name = QString::number(vi->level) + " - " + symtable[vi->varnum];

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
			childItem->setText(1, "?");
			childItem->setText(2, "");
			rowItem->addChild(childItem);
		}
	} else {
		// 2d array
		for(int x=0; x<arraylenx; x++) {
			for(int y=0; y<arrayleny; y++) {
				QTreeWidgetItem *childItem = new QTreeWidgetItem();
				childItem->setText(0, name + "[" + QString::number(x) + "," + QString::number(y) + "]");
				childItem->setText(1, "?");
				childItem->setText(2, "");
				rowItem->addChild(childItem);
			}
		}
	}
}


void VariableWin::varWinDropLevel(int level) {
	// when we return from a subroutine or function delete its variables
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
}
