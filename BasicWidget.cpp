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

#include <qglobal.h>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QMenu>
#else
#include <QMenu>
#endif

#include "ToolBar.h"
#include "ViewWidgetIFace.h"
#include "BasicWidget.h"

BasicWidget::BasicWidget(const QString & title, QWidget * parent, Qt::WindowFlags f)
    :	QWidget(parent, f)
    ,	m_viewWidget(NULL)
    ,	m_toolBar(NULL)
    ,	m_menu(NULL) {
    m_toolBar = new ToolBar();
    m_menu = new QMenu(title);
    m_layout = new QVBoxLayout();

    setLayout(m_layout);
}

BasicWidget::~BasicWidget() {
    if (NULL != m_toolBar) {
        delete m_toolBar;
        m_toolBar = NULL;
    }
}

bool BasicWidget::setViewWidget(QWidget * view) {
    if (NULL == view) {
        return false;
    }

    m_viewWidget = dynamic_cast< ViewWidgetIFace * >(view);

    if (NULL != m_viewWidget) {
        if (m_viewWidget->initActions(m_menu, m_toolBar)) {
            if (!m_viewWidget->usesToolBar()) {
                delete m_toolBar;
                m_toolBar = NULL;
            } else {
                m_layout->addWidget(m_toolBar);
            }
            if (!m_viewWidget->usesMenu()) {
                delete m_menu;
                m_menu = NULL;
            }
        }

        m_layout->addWidget(view, 1);

        return true;
    }

    return false;
}

void BasicWidget::slotShowToolBar(const bool vShow) {
    if (NULL == m_toolBar)
        return;

    m_toolBar->setVisible(vShow);
}

bool BasicWidget::isVisibleToolBar() {
    if (NULL == m_viewWidget) return false;
    if (NULL == m_toolBar) return false;
    return m_toolBar->isVisible();
}

bool BasicWidget::usesToolBar() {
    if (NULL != m_viewWidget) {
        return m_viewWidget->usesToolBar();
    }

    return false;
}

bool BasicWidget::usesMenu() {
    if (NULL != m_viewWidget) {
        return m_viewWidget->usesMenu();
    }

    return false;
}
