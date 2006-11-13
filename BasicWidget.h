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


#ifndef __BASICWIDGET_H
#define __BASICWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

class ToolBar;

class BasicWidget : public QWidget
{
  	Q_OBJECT;
public:
  	BasicWidget(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~BasicWidget();
	
	bool setViewWidget(QWidget * view);
	bool usesToolBar() { return m_usesToolBar; }

public slots:
	void slotShowToolBar(const bool);

private:
	bool m_usesToolBar;

	QWidget *	m_viewWidget;
	ToolBar *	m_toolBar;
	QVBoxLayout *	m_layout;
};

#endif
