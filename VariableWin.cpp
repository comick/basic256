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

#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QHeaderView>
#else
	#include <QHeaderView>
#endif

//#include "VariableWin.h"
#include "MainWindow.h"

extern MainWindow * mainwin;

VariableWin::VariableWin () 
{
  setColumnCount(2);
  setHeaderLabels(QStringList() << tr("Level - Name") << tr("Value"));
  sortByColumn(0,Qt::AscendingOrder);
  setSortingEnabled(true);
}

void
VariableWin::addVar(int recurse, QString name, QString value, int arraylenx, int arrayleny)
{
    // pass -1 for a normal variable
    // value is NULL for a new array
	QTreeWidgetItem *rowItem;
	bool newArrayFlag = (value==NULL);
	
if (name!="") {
	name = QString::number(recurse) + " - " + name;
		
	// remove old entries when a variable becomes an array or not
	if (arraylenx==-1||value==NULL) {
		// delete everything begining with name[
		QList<QTreeWidgetItem *> items = findItems(name + "[", Qt::MatchStartsWith | Qt::MatchRecursive, 0);
		for (int n=items.size()-1; n>=0; n--) {
			delete items[n];
		}
	}

	// fix the name for an array element of the dim of a new array
	if (!newArrayFlag && arraylenx > -1)
	{
		// if we are acessing an array element then change name to full []
		name = name + "[" + QString::number(arraylenx);
		if (arrayleny > -1) {
			name = name + "," + QString::number(arrayleny);
		}
		name = name + "]";
	}
	else if (newArrayFlag)
	{
		value = tr("<array ") + QString::number(arraylenx);
		if (arrayleny > -1) {
			value = value + "," + QString::number(arrayleny);
		}
		value = value + ">";
	}
	
	// see if element is on the list and change value or add
	QList<QTreeWidgetItem *> list = findItems(name, Qt::MatchExactly | Qt::MatchRecursive, 0);
	
	if (list.size() > 0) {
		// get existing element
		rowItem = list[0];
	} else {
		// add new element
		rowItem = new QTreeWidgetItem();
		rowItem->setText(0, name);
		addTopLevelItem(rowItem);
	}
    rowItem->setText(1, value);

	// add place holders for the array elements as children for a new array
    if (newArrayFlag)
	{
		if (arrayleny == -1) {
			// 1d array
			for(int x=0; x<arraylenx; x++) {
				QTreeWidgetItem *childItem = new QTreeWidgetItem();
				childItem->setText(0, name + "[" + QString::number(x) + "]");
				childItem->setText(1, tr("<unassigned>"));
				rowItem->addChild(childItem);
			}
		} else {
			// 2d array
			for(int x=0; x<arraylenx; x++) {
				for(int y=0; y<arrayleny; y++) {
					QTreeWidgetItem *childItem = new QTreeWidgetItem();
					childItem->setText(0, name + "[" + QString::number(x) + "," + QString::number(y) + "]");
					childItem->setText(1, tr("<>unassigned>"));
					rowItem->addChild(childItem);
				}
			}
		}
	}
} else {
	// when we return from a subroutine or function delete its variables
	QList<QTreeWidgetItem *> items = findItems(QString::number(recurse) + " ", Qt::MatchStartsWith | Qt::MatchRecursive, 0);
	for (int n=items.size()-1; n>=0; n--) {
		delete items[n];
	}
}
		
}
