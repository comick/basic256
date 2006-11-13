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

#include "ToolBar.h"
#include "ViewWidgetIFace.h"
#include "BasicWidget.h"

BasicWidget::BasicWidget(QWidget * parent, Qt::WindowFlags f)
:	QWidget(parent, f)
,	m_viewWidget(NULL)
,	m_toolBar(NULL)
,	m_usesToolBar(false)
{
	m_toolBar = new ToolBar();
	m_layout = new QVBoxLayout();

	setLayout(m_layout);
}

BasicWidget::~BasicWidget()
{
	if (NULL != m_toolBar)
	{
		delete m_toolBar;
		m_toolBar = NULL;
	}		
}

bool BasicWidget::setViewWidget(QWidget * view)
{
	if (NULL == view)
	{
		return false;
	}
	
	m_viewWidget = view;
	m_layout->addWidget(m_viewWidget);
	
	ViewWidgetIFace * viewIFace = dynamic_cast< ViewWidgetIFace * >(view);
	if (NULL != viewIFace)	// Initialise view with tool bar.
	{
		if (viewIFace->initToolBar(m_toolBar))
		{
			m_layout->addWidget(m_toolBar);
			m_usesToolBar = true;
		}
		m_layout->addWidget(m_viewWidget, 1);
		return true;
	}
	
	return false;
}

void BasicWidget::slotShowToolBar(const bool vShow)
{
	if (!m_usesToolBar)
		return;
	
	m_toolBar->setShown(vShow);
}
