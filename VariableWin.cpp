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

#include "VariableWin.h"
#include <QHeaderView>

VariableWin::VariableWin (QWidget * parent) 
  :QDockWidget(QString(tr("Variable Watch")), parent)
{
  rows = 0;
  table = new QTreeWidget(this);
  table->setColumnCount(2);
  table->setHeaderLabels(QStringList() << tr("Name") << tr("Value"));
  setWidget(table);
}



void
VariableWin::addVar(QString name, QString value, int arraylen)
{
  QTreeWidgetItem *rowItem = new QTreeWidgetItem();

  if (value != NULL && arraylen > -1)
    {
      name = name + "[" + QString::number(arraylen) + "]";
    }
  else if (value == NULL)
    {
      value = tr("<array ") + QString::number(arraylen) + ">";
    }
  rowItem->setText(0, name);
  rowItem->setText(1, value);

  QList<QTreeWidgetItem *> list = table->findItems(name, Qt::MatchExactly | Qt::MatchRecursive, 0);

  if (list.size() > 0)
    {
      list[0]->setText(1, value);
      return;
    }
  
 
  table->insertTopLevelItem(rows, rowItem);
  if (arraylen > 0)
    {
      for (int i = 0; i < arraylen; i++)
	{
	  QTreeWidgetItem *temp = new QTreeWidgetItem(rowItem);
	  temp->setText(0, name + "[" + QString::number(i) + "]");
	  temp->setText(1, "");
	}
    }
  rows++;
}



void
VariableWin::clearTable()
{
  table->clear();
  rows = 0;
}
