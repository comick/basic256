/** Copyright (C) 2010, J.M.Reneau.
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



#include <QSettings>

#ifndef SETTINGSH
	#define SETTINGSH
	#define SETTINGSORG "BASIC-256 Consortium"
	#define SETTINGSAPP "BASIC-256 IDE"
	#define SETTINGSPORTABLEINI	"Data/settings/BASIC256_IDE.ini"

	// main window
	#define SETTINGSSIZE "Main/Size"
	#define SETTINGSPOS "Main/Pos"
    #define SETTINGSFONT "Main/Font"
    #define SETTINGSFONTDEFAULT "Courier,10,-1,5,50,0,0,0,0,0"

	// documentation window
	#define SETTINGSDOCSIZE "Doc/Size"
	#define SETTINGSDOCPOS "Doc/Pos"
	
	// preferences window
	#define SETTINGSPREFPOS "Pref/Pos"
	#define SETTINGSPREFPASSWORD "Pref/Password"

	// Replace window
	#define SETTINGSREPLACEPOS "Replace/Pos"
	#define SETTINGSREPLACEFROM "Replace/From"
	#define SETTINGSREPLACETO "Replace/To"
	#define SETTINGSREPLACECASE "Replace/Case"
	#define SETTINGSREPLACECASEDEFAULT false
	#define SETTINGSREPLACEBACK "Replace/Back"
	#define SETTINGSREPLACEBACKDEFAULT false

	// permissions
	#define SETTINGSALLOWSYSTEM "Allow/System"
	#define SETTINGSALLOWSYSTEMDEFAULT true
	#define SETTINGSALLOWSETTING "Allow/Setting"
	#define SETTINGSALLOWSETTINGDEFAULT true
	#define SETTINGSALLOWPORT "Allow/Port"
	#define SETTINGSALLOWPORTDEFAULT true
	#define SETTINGSALLOWWARNINGS "Allow/Warnings"
	#define SETTINGSALLOWWARNINGSDEFAULT true

	// store history of files as SaveHistory/0 ... SaveHistory/8 
	#define SETTINGSGROUPHIST "SaveHistory"
	#define SETTINGSGROUPHISTN 9

	// store user settings (setsetting/getsetting) in seperate group
	#define SETTINGSGROUPUSER "UserSettings"


    // You need an SETTINGS; statement when you are using settings in a function
    // this defines a QSettings variable named "setings" for your use
    #ifdef WIN32PORTABLE
        #define SETTINGS QSettings settings(SETTINGSPORTABLEINI, QSettings::IniFormat)
    #else
        #define SETTINGS QSettings settings(SETTINGSORG, SETTINGSAPP);
    #endif


#endif
