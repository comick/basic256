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



#if QT_VERSION >= 0x05000000
	#include <QtWidgets/QDockWidget>
	#include <QtWidgets/QTreeWidget>
#else
	#include <QDockWidget>
	#include <QTreeWidget>
#endif

#include "ViewWidgetIFace.h"

class VariableWin : public QTreeWidget, public ViewWidgetIFace
{
  Q_OBJECT;
 public:
  VariableWin();
    
 public slots:
  void varAssignment(int recurse, QString name, QString value, int arraylenx, int arrayleny);
  
 private:

};
