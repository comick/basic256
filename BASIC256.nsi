; BASIC256.nsi


; Modification History
; date...... programmer... description...
; 2008-09-01 j.m.reneau    original coding
; 2012-12-02 j.m.reneau    changed to require the unsis version for unicode support
;                          More information at http://www.scratchpaper.com/
; 2013-11-12 j.m.reneau    major rewrite for 1.0.0.0 and change to QT5.1
; 2014-06-03 j.m.reneau    updated to qt5.3 and new media plugins
; 2015-12-30 j.m.reneau    updated to QT 5.7
# 2016-10-31 j.m.reneau    updated to qt 5.7
; 2020-04-26 j.m.reneau    updated to qt 5.14.2
; 2020-09-05 j.m.reneau    Updated to Qt 5.15.0
;

; MEMO TO MYSELF - DO NOT EDIT WITH geany - IT MESSES WITH THE UNICODE
; On Windows use notepad.

!include nsDialogs.nsh

!define VERSION "2.0.99.0"
!define VERSIONDATE "2020-09-06"
!define SDK_BIN "C:\Qt\5.15.0\mingw81_32\bin"
!define SDK_LIB "C:\Qt\5.15.0\mingw81_32\lib"
!define SDK_PLUGINS "C:\Qt\5.15.0\mingw81_32\plugins"

var customDialog
var customLabel0
var customLabel1
var customImage
var customImageHandle

Function .onInit
FunctionEnd

Function customPage

	nsDialogs::Create /NOUNLOAD 1018
	Pop $customDialog

	${If} $customDialog == error
		Abort
	${EndIf}

	${NSD_CreateBitmap} 0 0 100% 100% ""
	Pop $customImage
	${NSD_SetImage} $customImage resources\images\basic256.bmp $customImageHandle

	${NSD_CreateLabel} 50 0 80% 10% "BASIC256 ${VERSION} (${VERSIONDATE})"
	Pop $customLabel0
	${NSD_CreateLabel} 0 50 100% 80% "This installer will load BASIC256.  Previous versions will be overwritten and any saved files in the program folder may or may not be preserved."
	Pop $customLabel1

	nsDialogs::Show
FunctionEnd


; The name of the installer
Name "BASIC256 ${VERSION} (${VERSIONDATE})"

; The file to write
OutFile BASIC256-${VERSION}_Win32_Install.exe

; The default installation directory
InstallDir $PROGRAMFILES\BASIC256

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\BASIC256" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

InstType "Full"
InstType "Minimal"
;--------------------------------

; Pages

Page custom customPage "" ": BASIC256 Welcome"
Page license
LicenseData "license.txt"
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "BASIC256"

  SectionIn 1 RO
  
  SetOutPath $INSTDIR\Translations
  SetFileAttributes $INSTDIR\Translations HIDDEN
  File .\Translations\*.qm

  SetOutPath $INSTDIR\Modules
  File .\Modules\*

  SetOutPath $INSTDIR\audio
  SetFileAttributes $INSTDIR\audio HIDDEN
  File "${SDK_PLUGINS}\audio\qtaudio_windows.dll"

  SetOutPath $INSTDIR\imageformats
  SetFileAttributes $INSTDIR\imageformats HIDDEN
  File "${SDK_PLUGINS}\imageformats\qgif.dll"
  File "${SDK_PLUGINS}\imageformats\qico.dll"
  File "${SDK_PLUGINS}\imageformats\qjpeg.dll"
  File "${SDK_PLUGINS}\imageformats\qsvg.dll"
  File "${SDK_PLUGINS}\imageformats\qtga.dll"
  File "${SDK_PLUGINS}\imageformats\qtiff.dll"
  File "${SDK_PLUGINS}\imageformats\qwbmp.dll"

  SetOutPath $INSTDIR\platforms
  SetFileAttributes $INSTDIR\platforms HIDDEN
  File "${SDK_PLUGINS}\platforms\qwindows.dll"

  SetOutPath $INSTDIR\printsupport
  SetFileAttributes $INSTDIR\printsupport HIDDEN
  File "${SDK_PLUGINS}\printsupport\windowsprintersupport.dll"

  SetOutPath $INSTDIR\sqldrivers
  SetFileAttributes $INSTDIR\sqldrivers HIDDEN
  File "${SDK_PLUGINS}\sqldrivers\qsqlite.dll"

  SetOutPath $INSTDIR\mediaservice
  SetFileAttributes $INSTDIR\mediaservice HIDDEN
  File "${SDK_PLUGINS}\mediaservice\dsengine.dll"
  File "${SDK_PLUGINS}\mediaservice\qtmedia_audioengine.dll"

  SetOutPath $INSTDIR\playlistformats
  SetFileAttributes $INSTDIR\playlistformats HIDDEN
  File "${SDK_PLUGINS}\playlistformats\qtmultimedia_m3u.dll"


  SetOutPath $INSTDIR\texttospeech
  SetFileAttributes $INSTDIR\texttospeech HIDDEN
  File "${SDK_PLUGINS}\texttospeech\qtexttospeech_sapi.dll"

  SetOutPath $INSTDIR
  
  File .\release\BASIC256.exe
  File ChangeLog
  File CONTRIBUTORS
  File license.txt

  File "${SDK_BIN}\libgcc_s_dw2-1.dll"
  File "${SDK_BIN}\libstdc++-6.dll"
  File "${SDK_BIN}\libwinpthread-1.dll"

  File "${SDK_BIN}\Qt5Core.dll"
  File "${SDK_BIN}\Qt5Gui.dll"
  File "${SDK_BIN}\Qt5Multimedia.dll"
  File "${SDK_BIN}\Qt5MultimediaWidgets.dll"
  File "${SDK_BIN}\Qt5Network.dll"
  File "${SDK_BIN}\Qt5OpenGL.dll"
  File "${SDK_BIN}\Qt5PrintSupport.dll"
  File "${SDK_BIN}\Qt5SerialPort.dll"
  File "${SDK_BIN}\Qt5Sql.dll"
  File "${SDK_BIN}\Qt5TextToSpeech.dll"
  File "${SDK_BIN}\Qt5Widgets.dll"

 ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\BASIC256 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "DisplayName" "BASIC-256"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Start Menu Shrtcuts (can be disabled by the user)
Section "Start Menu Shortcuts"
  SectionIn 1
  SetOutPath $INSTDIR 
  CreateDirectory "$SMPROGRAMS\BASIC256"
  CreateShortCut "$SMPROGRAMS\BASIC256\BASIC256.lnk" "$INSTDIR\BASIC256.exe" "" "$INSTDIR\BASIC256.exe" 0
  CreateShortCut "$SMPROGRAMS\BASIC256\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

; Examples (can be disabled by the user)
Section "Example Programs"
  SectionIn 1
  SetOutPath $INSTDIR
  File /r /x ".svn" Examples
SectionEnd

; Test Suite (can be disabled by the user)
Section "Test Suite"
  SectionIn 1
  SetOutPath $INSTDIR
  File /r /x ".svn" TestSuite
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BASIC256"
  DeleteRegKey HKLM SOFTWARE\BASIC256

  ; Remove files and uninstaller
  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\ChangeLog
  Delete $INSTDIR\CONTRIBUTORS
  Delete $INSTDIR\license.txt
  RMDir /r $INSTDIR\espeak-data
  RMDir /r $INSTDIR\Examples
  RMDir /r $INSTDIR\TestSuite
  RMDir /r $INSTDIR\help
  RMDir /r $INSTDIR\Translations
  RMDir /r $INSTDIR\Modules
  RMDir /r $INSTDIR\accessible
  RMDir /r $INSTDIR\audio
  RMDir /r $INSTDIR\imageformats
  RMDir /r $INSTDIR\platforms
  RMDir /r $INSTDIR\printsupport
  RMDir /r $INSTDIR\sqldrivers
  RMDir /r $INSTDIR\mediaservice
  RMDir /r $INSTDIR\playlistformats
  RMDir /r $INSTDIR\texttospeech

  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\BASIC256\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\BASIC256"
  RMDir "$INSTDIR"

SectionEnd
