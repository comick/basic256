######################################################################
# BASIC256Portable.pro - create make files to compile BASCI256 for
# portableapps
######################################################################

QT_VERSION=$$[QT_VERSION]

TEMPLATE					=	app
TARGET						=	BASIC256
DEPENDPATH					+=	.
INCLUDEPATH					+=	.
QMAKE_CXXFLAGS				+=	-g 
CONFIG						+=	 qt release
OBJECTS_DIR					=	portabletmp/obj
MOC_DIR						=	portabletmp/moc

QT							+=	webkit
contains( QT_VERSION, "^5.*" ) {
	QT						+=	widgets
	QT						+=	printsupport
}

RESOURCES					+=	resources/resource.qrc
RC_FILE						=	resources/basic256.rc
TRANSLATIONS				=	Translations/basic256_en.ts \
								Translations/basic256_de.ts \
								Translations/basic256_ru.ts \
								Translations/basic256_es.ts \
								Translations/basic256_fr.ts \
								Translations/basic256_pt.ts \
								Translations/basic256_nl.ts
LIBS						+= -lsqlite3

DESTDIR = BASIC256Portable/App/BASIC256

win32 {
	# use SAPI for speech
	DEFINES 				+=	WIN32
	DEFINES 				+=	USEQSOUND
	LIBS					+=	-lole32 \
								-lsapi \
								-lws2_32 \
								-lwinmm
				
	QMAKE_CXXFLAGS			+=	-mstackrealign
	QMAKE_CXXFLAGS_RELEASE	+=	-mstackrealign

	########
	# Sound class - How Sound statement works
	########
	# uncomment one of the options

	# Sound - Option 0 - Microsoft built in 32 bit API
	#DEFINES				+=	SOUND_WIN32

	# Sound - Option 1 - QT Mobility Multimedia AudioOut
	DEFINES					+=	SOUND_QMOBILITY
	QT						+=	multimedia
	INCLUDEPATH				+=	QtMultimediaKit
	INCLUDEPATH				+=	QtMobility
	CONFIG					+=	mobility
	MOBILITY				+=	multimedia

	# define that this is the portable version
	DEFINES					+=	WIN32PORTABLE

}

exists( ./LEX/Makefile ) {
	message( Running make for ./LEX/Makefile )
	system( make -C ./LEX )
} else { 
	error( Couldn't make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS 					+=	*.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	*.cpp

# copy files to the proper locaiton to the package
DESTDIR_WIN = $${DESTDIR}
DESTDIR_WIN ~= s,/,\\,g
QMAKE_POST_LINK += copy /y PortableAppsFiles\\BASIC256Portable.exe $$DESTDIR_WIN $$escape_expand(\n\t)
QMAKE_POST_LINK += copy /y PortableAppsFiles\\appicon.ico $$DESTDIR_WIN\\AppInfo $$escape_expand(\n\t)
QMAKE_POST_LINK += copy /y PortableAppsFiles\\appicon_16.png $$DESTDIR_WIN\\AppInfo $$escape_expand(\n\t)
QMAKE_POST_LINK += copy /y PortableAppsFiles\\appicon_32.png $$DESTDIR_WIN\\AppInfo $$escape_expand(\n\t)
QMAKE_POST_LINK += copy /y PortableAppsFiles\\appinfo.ini $$DESTDIR_WIN\\AppInfo $$escape_expand(\n\t)
QMAKE_POST_LINK += copy /y PortableAppsFiles\\BASIC256Portable.ini $$DESTDIR_WIN\\AppInfo\\Launcher $$escape_expand(\n\t)





