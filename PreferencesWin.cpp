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

#include "PreferencesWin.h"
#include "Settings.h"
#include "MainWindow.h"
#include "md5.h"

#ifdef ESPEAK
	#include <speak_lib.h>
#endif

PreferencesWin::PreferencesWin (QWidget * parent)
		:QDialog(parent)
{

    SETTINGS;
	//resize(settings.value(SETTINGSPREFSIZE, QSize(500, 500)).toSize());
	move(settings.value(SETTINGSPREFPOS, QPoint(200, 200)).toPoint());
	setWindowTitle(tr("BASIC-256 Preferences and Settings"));

	QGridLayout * layout = new QGridLayout();
	int r=0;

	passwordlabel = new QLabel(tr("Preferences and Settings Password:"),this);
	passwordinput = new QLineEdit(QString::null,this);
	passwordinput->setText(settings.value(SETTINGSPREFPASSWORD, "").toString());
	passwordinput->setMaxLength(32);
	passwordinput->setEchoMode(QLineEdit::Password);
	layout->addWidget(passwordlabel,r,1,1,1);
	layout->addWidget(passwordinput,r,2,1,2);
	//
	r++;
	systemcheckbox = new QCheckBox(tr("Allow SYSTEM statement"),this);
	systemcheckbox->setChecked(settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toBool());
	layout->addWidget(systemcheckbox,r,2,1,2);
	//
	r++;
	settingcheckbox = new QCheckBox(tr("Allow GETSETTING/SETSETTING statements"),this);
	settingcheckbox->setChecked(settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool());
	layout->addWidget(settingcheckbox,r,2,1,2);
	//
	#ifdef WIN32
		#ifndef WIN32PORTABLE
			r++;
			portcheckbox = new QCheckBox(tr("Allow PORTIN/PORTOUT statements"),this);
			portcheckbox->setChecked(settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toBool());
			layout->addWidget(portcheckbox,r,2,1,2);
		#endif
	#endif
	//
	r++;
	warningscheckbox = new QCheckBox(tr("Show runtime warning messages"),this);
	warningscheckbox->setChecked(settings.value(SETTINGSALLOWWARNINGS, SETTINGSALLOWWARNINGSDEFAULT).toBool());
	layout->addWidget(warningscheckbox,r,2,1,2);
	//
	#ifdef ESPEAK
		r++;
		{
			#ifdef WIN32
				// use program install folder
				int samplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK,0,(char *) QFileInfo(QCoreApplication::applicationFilePath()).absolutePath().toUtf8().data(),0);
			#else
				// use default path for espeak-data
				int samplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK,0,NULL,0);
			#endif
			
			QString setvoice;
			voicelabel = new QLabel(tr("SAY Voice:"));
			layout->addWidget(voicelabel,r,1,1,1);
			voicecombo = new QComboBox();
			
			const espeak_VOICE **voices;
			voices = espeak_ListVoices(NULL);

			voicecombo->addItem(QString("default"),	QString("default"));
			for(int i=0; voices[i] != NULL; i++) {
				QString name = voices[i]->name;
				QString lang = (voices[i]->languages + 1);
				voicecombo->addItem( lang + " (" + name + ")",	name);
			}
			
			// set setting and select
			setvoice = settings.value(SETTINGSESPEAKVOICE, SETTINGSESPEAKVOICEDEFAULT).toString();
			int index = voicecombo->findData(setvoice);
			if ( index != -1 ) { // -1 for not found
				voicecombo->setCurrentIndex(index);
			}
			// add to layout
			layout->addWidget(voicecombo,r,2,1,2);
		}
	#endif
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

void PreferencesWin::clickCancelButton() {
	close();
}

void PreferencesWin::clickSaveButton() {
    SETTINGS;
	// if password has changed - digest and save
	QString currentpw = settings.value(SETTINGSPREFPASSWORD, "").toString();
	if(QString::compare(currentpw,passwordinput->text())!=0) {
		QString pw("");
		if(passwordinput->text().length()!=0) {
			pw = MD5(passwordinput->text().toUtf8().data()).hexdigest();
		}
		settings.setValue(SETTINGSPREFPASSWORD, pw);
	}
	//
	settings.setValue(SETTINGSALLOWSYSTEM, systemcheckbox->isChecked());
	settings.setValue(SETTINGSALLOWSETTING, settingcheckbox->isChecked());
	#ifdef WIN32
		#ifndef WIN32PORTABLE
			settings.setValue(SETTINGSALLOWPORT, portcheckbox->isChecked());
		#endif
	#endif
	settings.setValue(SETTINGSALLOWWARNINGS, warningscheckbox->isChecked());
	#ifdef ESPEAK
		//
		if (voicecombo->currentIndex()!=-1) {
			settings.setValue(SETTINGSESPEAKVOICE, voicecombo->itemData(voicecombo->currentIndex()));
		}
	#endif
	//
	QMessageBox msgBox;
	msgBox.setText(tr("Preferences and settings have been saved."));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
	//
	close();
}

void PreferencesWin::closeEvent(QCloseEvent *e) {
	// save current screen posision
    SETTINGS;
	//settings.setValue(SETTINGSPREFSIZE, size());
	settings.setValue(SETTINGSPREFPOS, pos());

}

