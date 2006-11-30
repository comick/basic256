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

#include "VariableWin.h"
#include <QHeaderView>

VariableWin::VariableWin (QWidget * parent) 
  :QDockWidget(QString(tr("Variable Watch")), parent)
{
  rows = 0;
  table = new QTableWidget(rows, 2, this);
  table->setColumnCount(2);
  table->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
  table->horizontalHeader()->setVisible(false);
  table->verticalHeader()->setVisible(false);
  setWidget(table);
}


void
VariableWin::addVar(QString name, QString value)
{
  QTableWidgetItem *nameItem = new QTableWidgetItem(name);
  QTableWidgetItem *valueItem = new QTableWidgetItem(value);
  for (unsigned int i = 0; i < rows; i++)
    {
      if (table->item(i, 0)->text() == name)
	{
	  table->setItem(i, 1, valueItem);
	  return;
	}
    }
 
  table->setRowCount(++rows);
  table->setItem(rows - 1, 0, nameItem);
  table->setItem(rows - 1, 1, valueItem);
}

void
VariableWin::clearTable()
{
  table->clear();
  table->setRowCount(0);
  rows = 0;
}
