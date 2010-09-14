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

	// main window
	#define SETTINGSSIZE "Main/Size"
	#define SETTINGSPOS "Main/Pos"
	// documentation window
	#define SETTINGSDOCSIZE "Doc/Size"
	#define SETTINGSDOCPOS "Doc/Pos"
	// preferences window
	#define SETTINGSPREFPOS "Pref/Pos"
	#define SETTINGSPREFPASSWORD "Pref/Password"

	// permissions
	#define SETTINGSALLOWSYSTEM "Allow/System"
	#define SETTINGSALLOWSYSTEMDEFAULT true
	#define SETTINGSALLOWSETTING "Allow/Setting"
	#define SETTINGSALLOWSETTINGDEFAULT true


	// store history of files as SaveHistory/0 ... SaveHistory/8 
	#define SETTINGSGROUPHIST "SaveHistory"
	#define SETTINGSGROUPHISTN 9

	// store user settings (setsetting/getsetting) in seperate group
	#define SETTINGSGROUPUSER "UserSettings"


#endif
