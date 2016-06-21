/** Copyright (C) 2010, J.M.Reneau
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

#include "ReplaceWin.h"
#include "Settings.h"
#include "MainWindow.h"
#include "md5.h"

extern BasicEdit * editwin;
extern MainWindow * mainwin;

ReplaceWin::ReplaceWin () {

    replaceMode = true;

    // position where it was last on screen
    SETTINGS;
    move(settings.value(SETTINGSREPLACEPOS, QPoint(200, 200)).toPoint());

    QGridLayout * layout = new QGridLayout();
    int r=0;

    fromlabel = new QLabel(tr("From:"),this);
    layout->addWidget(fromlabel,r,1,1,1);
    frominput = new QLineEdit(settings.value(SETTINGSREPLACEFROM, "").toString());
    frominput->setMaxLength(100);
    connect(frominput, SIGNAL(textChanged(QString)), this, SLOT (changeFromInput(QString)));
    layout->addWidget(frominput,r,2,1,3);
    //
    r++;
    tolabel = new QLabel(tr("To:"),this);
    layout->addWidget(tolabel,r,1,1,1);
    toinput = new QLineEdit(settings.value(SETTINGSREPLACETO, "").toString());
    toinput->setMaxLength(100);
    layout->addWidget(toinput,r,2,1,3);
    //
    r++;
    casecheckbox = new QCheckBox(tr("Case sensitive"),this);
    casecheckbox->setChecked(settings.value(SETTINGSREPLACECASE, SETTINGSREPLACECASEDEFAULT).toBool());
    layout->addWidget(casecheckbox,r,2,1,3);
    //
    r++;
    wordscheckbox = new QCheckBox(tr("Only whole words"),this);
    wordscheckbox->setChecked(settings.value(SETTINGSREPLACECASE, SETTINGSREPLACEWORDSDEFAULT).toBool());
    layout->addWidget(wordscheckbox,r,2,1,3);
    //
    r++;
    backcheckbox = new QCheckBox(tr("Search backwards"),this);
    backcheckbox->setChecked(settings.value(SETTINGSREPLACEBACK, SETTINGSREPLACEBACKDEFAULT).toBool());
    layout->addWidget(backcheckbox,r,2,1,3);
    //
    r++;
    findbutton = new QPushButton(tr("&Find"), this);
    connect(findbutton, SIGNAL(clicked()), this, SLOT (clickFindButton()));
    layout->addWidget(findbutton,r,1,1,1);
    replacebutton = new QPushButton(tr("&Replace"), this);
    connect(replacebutton, SIGNAL(clicked()), this, SLOT (clickReplaceButton()));
    layout->addWidget(replacebutton,r,2,1,1);
    replaceallbutton = new QPushButton(tr("Replace &All"), this);
    connect(replaceallbutton, SIGNAL(clicked()), this, SLOT (clickReplaceAllButton()));
    layout->addWidget(replaceallbutton,r,3,1,1);
    cancelbutton = new QPushButton(tr("Cancel"), this);
    connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
    layout->addWidget(cancelbutton,r,4,1,1);
    //
    QAction* findagain = new QAction (this);
    findagain->setShortcuts(QKeySequence::keyBindings(QKeySequence::FindNext));
    connect(findagain, SIGNAL(triggered()), this, SLOT (clickFindButton()));
    addAction (findagain);
    //
    this->setParent(mainwin);
    this->setWindowFlags(Qt::Dialog);
    this->setLayout(layout);
    this->show();
    changeFromInput(frominput->text());

    frominput->setFocus();
}

void ReplaceWin::setReplaceMode(bool m) {
    replaceMode = m;
    tolabel->setEnabled(replaceMode);
    tolabel->setVisible(replaceMode);
    toinput->setEnabled(replaceMode);
    toinput->setVisible(replaceMode);
    replaceallbutton->setEnabled(replaceMode);
    replaceallbutton->setVisible(replaceMode);
    replacebutton->setEnabled(replaceMode);
    replacebutton->setVisible(replaceMode);
    if (replaceMode) {
        setWindowTitle(tr("BASIC-256 Find/Replace"));
    } else {
        setWindowTitle(tr("BASIC-256 Find"));
    }
    frominput->setFocus();
}

void ReplaceWin::changeFromInput(QString t) {
    replacebutton->setEnabled(replaceMode && (t.length() != 0));
    replaceallbutton->setEnabled(replaceMode && t.length() != 0);
    findbutton->setEnabled(t.length() != 0);
}

void ReplaceWin::clickCancelButton() {
    close();
}

void ReplaceWin::findAgain() {
    if(frominput->text().length() != 0)
        editwin->findString(frominput->text(), backcheckbox->isChecked(), casecheckbox->isChecked(), wordscheckbox->isChecked());
}

void ReplaceWin::clickFindButton() {
    if(frominput->text().length() != 0)
        editwin->findString(frominput->text(), backcheckbox->isChecked(), casecheckbox->isChecked(), wordscheckbox->isChecked());
}

void ReplaceWin::clickReplaceButton() {
    if(frominput->text().length() != 0)
        editwin->replaceString(frominput->text(), toinput->text(), backcheckbox->isChecked(), casecheckbox->isChecked(), wordscheckbox->isChecked(), false);
}

void ReplaceWin::clickReplaceAllButton() {
    if(frominput->text().length() != 0)
        editwin->replaceString(frominput->text(), toinput->text(), backcheckbox->isChecked(), casecheckbox->isChecked(), wordscheckbox->isChecked(), true);
}

void ReplaceWin::closeEvent(QCloseEvent *e) {
    (void) e;
    saveSettings();
}

void ReplaceWin::saveSettings() {
    SETTINGS;
    settings.setValue(SETTINGSREPLACEPOS, pos());
    settings.setValue(SETTINGSREPLACEFROM, frominput->text());
    if (replaceMode) settings.setValue(SETTINGSREPLACETO, toinput->text());
    settings.setValue(SETTINGSREPLACECASE, casecheckbox->isChecked());
    settings.setValue(SETTINGSREPLACEBACK, backcheckbox->isChecked());
    settings.setValue(SETTINGSREPLACEWORDS, wordscheckbox->isChecked());
}

