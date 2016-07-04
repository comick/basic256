/** Copyright (C) 2010, J.M.Reneau, Florin Oprea.
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

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFlags>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#ifndef DOCUMENTATIONWINH

#define DOCUMENTATIONWINH

class DocumentationWin : public QDialog
{
  Q_OBJECT
  public:
	DocumentationWin(QWidget * parent);
    void resizeEvent(QResizeEvent *e);
    void closeEvent(QCloseEvent *);
    void go(QString);

  protected:
    virtual void keyPressEvent(QKeyEvent *);

  public slots:

  private slots:
    void hijackAnchorClicked(QUrl);
    void slotPrintHelp();
    void searchTextChanged();
    void clickFindNext();
    void clickFindPrev();
    void clickCloseFind();
    void newSource(const QUrl url);
    void searchFocus();
    void userSelectLanguage(const QString s);

  private:
    bool helpFileExists(QString check);
    void setLanguageAlternatives(QString s);
    bool setBestSourceForHelp(QString s);
    void findWordInHelp(bool verbose, bool reverse = false);
    void highlight();
    int occurrences;
    QString indexfile;
    QString localecode;
    QString defaultlocale;
    QVBoxLayout* layout;
	QToolBar* toolbar;
	QTextBrowser* docs;
    QBoxLayout* footer;
    QLabel *searchlabel;
    QLabel *resultslabel;
    QLineEdit *searchinput;
    QToolButton *findprev;
    QToolButton *findnext;
    QCheckBox *casecheckbox;
    QWidget *bottom;
    QPushButton *closeButton;
    QComboBox *comboLanguage;
    QLabel *viewLanguage;
};

#endif

