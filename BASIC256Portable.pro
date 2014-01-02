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
OBJECTS_DIR					=	tmp/obj
MOC_DIR						=	tmp/moc

QT							+=	webkit
contains( QT_VERSION, "^5.*" ) {
	QT						+=	widgets
	QT						+=	printsupport
	QT						+=	sql
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
	DEFINES 				+=	WIN32
	DEFINES 				+=	USEQSOUND
	LIBS					+=	-lole32 \
								-lws2_32 \
								-lwinmm
				
	QMAKE_CXXFLAGS			+=	-mstackrealign
	QMAKE_CXXFLAGS_RELEASE	+=	-mstackrealign

	########
	# TTS control - How Say statement works
	########
	# uncomment one of the options

	## TTS Option 1 - ececute 'espak' command to speak 
	#DEFINES				+=	ESPEAK_EXECUTE

	## TTS Option 2 - use the espeak library
	DEFINES					+=	ESPEAK
	LIBS					+=	-lespeak

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
	error( Couldnt make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS 					+=	*.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	*.cpp




