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
	{
		int dflt;
		int setprinter;
		printerslabel = new QLabel(tr("Printer:"));
		layout->addWidget(printerslabel,r,1,1,1);
		printerscombo = new QComboBox();
		// build list of printers
		QList<QPrinterInfo> printerList=QPrinterInfo::availablePrinters();
		for (int i=0; i< printerList.count(); i++) {
			printerscombo->addItem(printerList[i].printerName(),i);
			if (printerList[i].isDefault()) dflt = i;
		}
		printerscombo->addItem(tr("PDF File Output"),-1);
		// set setting and select
		setprinter = settings.value(SETTINGSPRINTERPRINTER, dflt).toInt();
		if (setprinter<-1||setprinter>=printerList.count()) setprinter = dflt;
		int index = printerscombo->findData(setprinter);
		if ( index != -1 ) { // -1 for not found
			printerscombo->setCurrentIndex(index);
		}
		// add to layout
		layout->addWidget(printerscombo,r,2,1,2);
	}
	//
	r++;
	{
		int setpaper;
		paperlabel = new QLabel(tr("Paper:"));
		layout->addWidget(paperlabel,r,1,1,1);
		papercombo = new QComboBox();
		papercombo->addItem("A0 (841 x 1189 mm)",	QPrinter::A0);
		papercombo->addItem("A1 (594 x 841 mm)", QPrinter::A1);	
		papercombo->addItem("A2 (420 x 594 mm", QPrinter::A2);	
		papercombo->addItem("A3 (297 x 420 mm", QPrinter::A3);	
		papercombo->addItem("A4 (210 x 297 mm, 8.26 x 11.69 inches", QPrinter::A4);	
		papercombo->addItem("A5 (148 x 210 mm", QPrinter::A5);	
		papercombo->addItem("A6 (105 x 148 mm", QPrinter::A6);
		papercombo->addItem("A7 (74 x 105 mm", QPrinter::A7);
		papercombo->addItem("A9 (52 x 74 mm", QPrinter::A8);	
		papercombo->addItem("A9 (37 x 52 mm", QPrinter::A9);
		papercombo->addItem("B0 (1000 x 1414 mm", QPrinter::B0);	
		papercombo->addItem("B1 (707 x 1000 mm", QPrinter::B1);	
		papercombo->addItem("B2 (500 x 707 mm", QPrinter::B2);	
		papercombo->addItem("B3 (353 x 500 mm", QPrinter::B3);
		papercombo->addItem("B4 (250 x 353 mm", QPrinter::B4);	
		papercombo->addItem("B5 (176 x 250 mm, 6.93 x 9.84 inches", QPrinter::B5);	
		papercombo->addItem("B6 (125 x 176 mm", QPrinter::B6);
		papercombo->addItem("B7 (88 x 125 mm", QPrinter::B7);	
		papercombo->addItem("B8 (62 x 88 mm", QPrinter::B8);	
		papercombo->addItem("B9 (33 x 62 mm", QPrinter::B9);	
		papercombo->addItem("B10 (31 x 44 mm", QPrinter::B10);
		papercombo->addItem("#5 Envelope (163 x 229 mm", QPrinter::C5E);	
		papercombo->addItem("#10 Envelope (105 x 241 mm", QPrinter::Comm10E);
		papercombo->addItem("DLE (110 x 220 mm", QPrinter::DLE);	
		papercombo->addItem("Executive (7.5 x 10 inches, 190.5 x 254 mm", QPrinter::Executive);	
		papercombo->addItem("Folio (210 x 330 mm", QPrinter::Folio);
		papercombo->addItem("Ledger (431.8 x 279.4 mm", QPrinter::Ledger);	
		papercombo->addItem("Legal (8.5 x 14 inches, 215.9 x 355.6 mm", QPrinter::Legal);
		papercombo->addItem("Letter (8.5 x 11 inches, 215.9 x 279.4 mm", QPrinter::Letter);
		papercombo->addItem("Tabloid (279.4 x 431.8 mm", QPrinter::Tabloid);
		// set setting and select
		setpaper = settings.value(SETTINGSPRINTERPAPER, SETTINGSPRINTERPAPERDEFAULT).toInt();
		int index = papercombo->findData(setpaper);
		if ( index != -1 ) { // -1 for not found
			papercombo->setCurrentIndex(index);
		}
		// add to layout
		layout->addWidget(papercombo,r,2,1,2);
	}
	//
	r++;
	{
		pdffilelabel = new QLabel(tr("PDF File Name:"),this);
		pdffileinput = new QLineEdit(QString::null,this);
		pdffileinput->setText(settings.value(SETTINGSPRINTERPDFFILE, "").toString());
		pdffileinput->setMaxLength(96);
		layout->addWidget(pdffilelabel,r,1,1,1);
		layout->addWidget(pdffileinput,r,2,1,2);
	}
	//
	r++;
	{
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
	}
	//
	r++;
	{
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
	}
	//
	r++;
	{
		cancelbutton = new QPushButton(tr("Cancel"), this);
		connect(cancelbutton, SIGNAL(clicked()), this, SLOT (clickCancelButton()));
		savebutton = new QPushButton(tr("Save"), this);
		connect(savebutton, SIGNAL(clicked()), this, SLOT (clickSaveButton()));
		layout->addWidget(cancelbutton,r,2,1,1);
		layout->addWidget(savebutton,r,3,1,1);
	}
	//
	this->setLayout(layout);
	this->show();

}

void PrefPrinterWin::clickCancelButton() {
	close();
}

void PrefPrinterWin::clickSaveButton() {
    //
    // validate file name if PDF
    if (printerscombo->itemData(printerscombo->currentIndex())==-1 && pdffileinput->text()=="") {
		//
		QMessageBox msgBox;
		msgBox.setText(tr("File name required for PDF output."));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
	} else {
	    //
		SETTINGS;
		//
		if (printerscombo->currentIndex()!=-1) {
			settings.setValue(SETTINGSPRINTERPRINTER, printerscombo->itemData(printerscombo->currentIndex()));
		}
		//
		if (papercombo->currentIndex()!=-1) {
			settings.setValue(SETTINGSPRINTERPAPER, papercombo->itemData(papercombo->currentIndex()));
		}
		//
		settings.setValue(SETTINGSPRINTERPDFFILE, pdffileinput->text());
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
}

void PrefPrinterWin::closeEvent(QCloseEvent *e) {
	// save current screen posision
    SETTINGS;
	//settings.setValue(SETTINGSPREFSIZE, size());
	settings.setValue(SETTINGSPREFPOS, pos());

}

