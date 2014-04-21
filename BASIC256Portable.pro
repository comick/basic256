######################################################################
# BASIC256Portable.pro - create make files to compile BASCI256 for
# portableapps
######################################################################

lessThan(QT_MAJOR_VERSION, 5) {
  message( FATAL_ERROR "BASIC-256 requires QT 5 or better." )
}


TEMPLATE					=	app
TARGET						=	BASIC256
DEPENDPATH					+=	.
INCLUDEPATH					+=	.
QMAKE_CXXFLAGS				+=	-g 
CONFIG						+=	 qt debug_and_release
OBJECTS_DIR					=	tmp/obj
MOC_DIR						=	tmp/moc

QT							+=	webkit
QT							+=	widgets
QT							+=	printsupport
QT							+=	sql

RESOURCES					+=	resources/resource.qrc
RC_FILE						=	resources/basic256.rc
TRANSLATIONS				=	Translations/basic256_en.ts \
								Translations/basic256_de.ts \
								Translations/basic256_ru.ts \
								Translations/basic256_es.ts \
								Translations/basic256_fr.ts \
								Translations/basic256_pt.ts \
								Translations/basic256_nl.ts

CONFIG(debug, debug|release) {
	DESTDIR = BASIC256PortableDebug/App/BASIC256
} else {
	DESTDIR = BASIC256Portable/App/BASIC256
}

win32 {
	DEFINES 				+=	WIN32
	LIBS					+=	-lole32 \
								-lws2_32 \
								-lwinmm
				
	QMAKE_CXXFLAGS			+=	-mstackrealign
	QMAKE_CXXFLAGS_RELEASE	+=	-mstackrealign

	########
	# TTS control - How Say statement works
	########

	DEFINES					+=	ESPEAK
	LIBS					+=	-lespeak

	########
	# Depencencies and setup for Sound and BasicMediaPlayer classes
	########
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
	error( Couldnt make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS 					+=	*.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	*.cpp




