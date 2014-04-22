REM # BATCH FILE TO COPY THE DLL AND SUPPORT FILES INTO
REM # THE BASIC256PortableDebug folder SO THAT THEY MAY BE
REM # INCLUDED IN THE DEBUGGING PORTABLE DISTRIBUTION

REM # DATE...... PROGRAMMER... VERSION....	DESCRIPTION...
REM # 2014-04-20 j.m.reneau    1.1.1.3		original coding

set SDK_BIN=C:\Qt\5.2.1\mingw48_32\bin
set SDK_LIB=C:\Qt\5.2.1\mingw48_32\lib
set SDK_PLUGINS=C:\Qt\5.2.1\mingw48_32\plugins

# folder where app will live and support files need to be
set INSTDIR=BASIC256PortableDebug\App\BASIC256
 
rmdir /s /q %INSTDIR%
mkdir %INSTDIR%

mkdir %INSTDIR%\Translations
xcopy Translations\*.qm %INSTDIR%\Translations

mkdir %INSTDIR%\espeak-data
xcopy /e release\espeak-data %INSTDIR%\espeak-data

mkdir %INSTDIR%\accessible
xcopy %SDK_PLUGINS%\accessible\qtaccessiblewidgetsd.dll %INSTDIR%\accessible

mkdir %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qgifd.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qicod.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qjpegd.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qmngd.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qsvgd.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qtgad.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qtiffd.dll %INSTDIR%\imageformats
xcopy %SDK_PLUGINS%\imageformats\qwbmpd.dll %INSTDIR%\imageformats

mkdir %INSTDIR%\platforms
xcopy %SDK_PLUGINS%\platforms\qwindowsd.dll %INSTDIR%\platforms

mkdir %INSTDIR%\printsupport
xcopy %SDK_PLUGINS%\printsupport\windowsprintersupportd.dll %INSTDIR%\printsupport

mkdir %INSTDIR%\sqldrivers
xcopy %SDK_PLUGINS%\sqldrivers\qsqlited.dll %INSTDIR%\sqldrivers

mkdir %INSTDIR%\mediaservice
xcopy %SDK_PLUGINS%\mediaservice\dsengined.dll %INSTDIR%\mediaservice
xcopy %SDK_PLUGINS%\mediaservice\qtmedia_audioengined.dll %INSTDIR%\mediaservice

mkdir %INSTDIR%\playlistformats
xcopy %SDK_PLUGINS%\playlistformats\qtmultimedia_m3ud.dll %INSTDIR%\playlistformats

xcopy ChangeLog %INSTDIR%
xcopy CONTRIBUTORS %INSTDIR%
xcopy license.txt %INSTDIR%

xcopy %SDK_BIN%\icudt51.dll %INSTDIR%
xcopy %SDK_BIN%\icuin51.dll %INSTDIR%
xcopy %SDK_BIN%\icuuc51.dll %INSTDIR%
xcopy %SDK_BIN%\libgcc_s_dw2-1.dll %INSTDIR%
xcopy %SDK_BIN%\libstdc++-6.dll %INSTDIR%
xcopy %SDK_BIN%\libwinpthread-1.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Cored.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Guid.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Multimediad.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5MultimediaWidgetsd.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Networkd.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5OpenGLd.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5PrintSupportd.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Sqld.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5WebKitd.dll %INSTDIR%
xcopy %SDK_BIN%\Qt5Widgetsd.dll %INSTDIR%

xcopy %SDK_LIB%\libespeak.dll %INSTDIR%
xcopy %SDK_LIB%\libportaudio-2.dll %INSTDIR%
xcopy %SDK_LIB%\libpthread-2.dll %INSTDIR%

