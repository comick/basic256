/** Copyright (C) 2013, J.M.Reneau
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

#include "PrefPrinterWin.h"
#include "Settings.h"
#include "MainWindow.h"
#include "md5.h"

PrefPrinterWin::PrefPrinterWin (QWidget * parent)
		:QDialog(parent)
{

    SETTINGS;
	//resize(settings.value(SETTINGSPREFSIZE, QSize(500, 500)).toSize());
	move(settings.value(SETTINGSPREFPOS, QPoint(200, 200)).toPoint());
	setWindowTitle(tr("BASIC-256 Printer Preferences and Settings"));

	QGridLayout * layout = new QGridLayout();

	int r=0;
	resolutiongroup = new QGroupBox(tr("Printer Resolution:"));
	resolutionhigh = new QRadioButton(tr("High"));
	resolutionscreen = new QRadioButton(tr("Screen"));
	QVBoxLayout *resolutionbox = new QVBoxLayout;
	resolutionbox->addWidget(resolutionhigh);
	resolutionbox->addWidget(resolutionscreen);
	resolutiongroup->setLayout(resolutionbox);	
	layout->addWidget(resolutiongroup,r,2,1,2);
	resolutionhigh->setChecked(settings.value(SETTINGSPRINTERRESOLUTION, SETTINGSPRINTERRESOLUTIONDEFAULT)==QPrinter::HighResolution);
	resolutionscreen->setChecked(settings.value(SETTINGSPRINTERRESOLUTION, SETTINGSPRINTERRESOLUTIONDEFAULT)==QPrinter::ScreenResolution);
	//
	r++;
	orientgroup = new QGroupBox(tr("Orientation:"));
	orientportrait = new QRadioButton(tr("Portrait"));
	orientlandscape = new QRadioButton(tr("Landscape"));
	QVBoxLayout *orientbox = new QVBoxLayout;
	orientbox->addWidget(orientportrait);
	orientbox->addWidget(orientlandscape);
	orientgroup->setLayout(orientbox);	
	layout->addWidget(orientgroup,r,2,1,2);
	orientportrait->setChecked(settings.value(SETTINGSPRINTERORIENT, SETTINGSPRINTERORIENTDEFAULT)==QPrinter::Portrait);
	orientlandscape->setChecked(settings.value(SETTINGSPRINTERORIENT, SETTINGSPRINTERORIENTDEFAULT)==QPrinter::Landscape);
	//
	
	//
	r++;
	cancelbutton = new QPushButton(tr("Cancel"), this);
	connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
	savebutton = new QPushButton(tr("Save"), this);
	connect(savebutton, SIGNAL(clicked()), this, SLOT (clickSaveButton()));
	layout->addWidget(cancelbutton,r,2,1,1);
	layout->addWidget(savebutton,r,3,1,1);
	
	this->setLayout(layout);
	this->show();

}

void PrefPrinterWin::clickCancelButton() {
	close();
}

void PrefPrinterWin::clickSaveButton() {
    SETTINGS;
	//
	if (resolutionhigh->isChecked()) settings.setValue(SETTINGSPRINTERRESOLUTION, QPrinter::HighResolution);
	if (resolutionscreen->isChecked()) settings.setValue(SETTINGSPRINTERRESOLUTION, QPrinter::ScreenResolution);
	//
	if (orientportrait->isChecked()) settings.setValue(SETTINGSPRINTERORIENT, QPrinter::Portrait);
	if (orientlandscape->isChecked()) settings.setValue(SETTINGSPRINTERORIENT, QPrinter::Landscape);
	//
	QMessageBox msgBox;
	msgBox.setText(tr("Printer preferences have been saved."));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
	//
	close();
}

void PrefPrinterWin::closeEvent(QCloseEvent *e) {
	// save current screen posision
    SETTINGS;
	//settings.setValue(SETTINGSPREFSIZE, size());
	settings.setValue(SETTINGSPREFPOS, pos());

}

