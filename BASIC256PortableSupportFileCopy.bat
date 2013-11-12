REM # BATCH FILE TO COPY THE DLL AND SUPPORT FILES INTO
REM # THE BASIC256Portable folder SO THAT THEY MAY BE
REM # INCLUDED IN THE PORTABLE DISTRIBUTION

REM # DATE...... PROGRAMMER... DESCRIPTION...
REM # 2013-11-11 j.m.reneau    original coding

set SDK_BIN=C:\Qt\Qt5.1.1\5.1.1\mingw48_32\bin
set SDK_LIB=C:\Qt\Qt5.1.1\5.1.1\mingw48_32\lib
set SDK_PLUGINS=C:\Qt\Qt5.1.1\5.1.1\mingw48_32\plugins

# folder where app will live and support files need to be
set INSTDIR=BASIC256Portable\App\BASIC256
  
mkdir %INSTDIR%\Translations
xcopy Translations\*.qm %INSTDIR%\Translations

mkdir %INSTDIR%\espeak-data
xcopy /e release\espeak-data %INSTDIR%\espeak-data

mkdir %INSTDIR%\accessible
copy %SDK_PLUGINS%\accessible\qtaccessiblewidgets.dll %INSTDIR%\accessable

mkdir %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qgif.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qico.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qjpeg.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qmng.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qsvg.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qtga.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qtiff.dll %INSTDIR%\imageformats
copy %SDK_PLUGINS%\imageformats\qwbmp.dll %INSTDIR%\imageformats

mkdir %INSTDIR%\platforms
copy %SDK_PLUGINS%\platforms\qwindows.dll %INSTDIR%\platforms

mkdir %INSTDIR%\printsupport
copy %SDK_PLUGINS%\printsupport\windowsprintersupport.dll %INSTDIR%\printsupport

copy ChangeLog %INSTDIR%
copy CONTRIBUTORS %INSTDIR%
copy license.txt %INSTDIR%

copy %SDK_BIN%\icudt51.dll %INSTDIR%
copy %SDK_BIN%\icuin51.dll %INSTDIR%
copy %SDK_BIN%\icuuc51.dll %INSTDIR%
copy %SDK_BIN%\libgcc_s_dw2-1.dll %INSTDIR%
copy %SDK_BIN%\\libstdc++-6.dll %INSTDIR%
copy %SDK_BIN%\\libwinpthread-1.dll %INSTDIR%
copy %SDK_BIN%\\Qt5Multimedia.dll %INSTDIR%
copy %SDK_BIN%\\Qt5Core.dll %INSTDIR%
copy %SDK_BIN%\\Qt5Gui.dll %INSTDIR%
copy %SDK_BIN%\\Qt5WebKit.dll %INSTDIR%
copy %SDK_BIN%\\Qt5PrintSupport.dll %INSTDIR%
copy %SDK_BIN%\\Qt5Widgets.dll %INSTDIR%
copy %SDK_BIN%\\\Qt5Network.dll %INSTDIR%

copy %SDK_LIB%\\libespeak.dll %INSTDIR%
copy %SDK_LIB%\\libportaudio-2.dll %INSTDIR%
copy %SDK_LIB%\\libpthread-2.dll %INSTDIR%
copy %SDK_LIB%\\sqlite3.dll %INSTDIR%
copy %SDK_LIB%\\inpout32.dll %INSTDIR%
copy %SDK_LIB%\\InstallDriver.exe %INSTDIR%

