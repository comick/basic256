QT_VERSION=$$[QT_VERSION]

TEMPLATE					=	app
TARGET						=	BASIC256
DEPENDPATH					+=	.
INCLUDEPATH					+=	.
QMAKE_CXXFLAGS				+=	-g 
CONFIG						+=	 qt debug_and_release
OBJECTS_DIR					=	tmp/obj
MOC_DIR						=	tmp/moc

QT						+=	webkit
QT						+=	gui
contains( QT_VERSION, "^5.*" ) {
	QT						+=	widgets
	QT						+=	printsupport
}

RESOURCES					+=	resources/resource.qrc
RC_FILE						=	resources/basic256.rc
TRANSLATIONS				=	Translations/basic256_en_US.ts \
								Translations/basic256_de.ts \
								Translations/basic256_ru_RU.ts \
								Translations/basic256_sp.ts \
								Translations/basic256_nl.ts
LIBS						+= -lsqlite3

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


}

unix:!macx {
	## this is the LINUX (unix-non-mac)
	DEFINES					+=	LINUX
	
	########
	# TTS control - How Say statement works
	########
	# uncomment one of the options

	## TTS Option 1 - ececute 'espak' command to speak 
	#DEFINES				+=	LINUX_ESPEAK_EXECUTE

	## TTS Option 2 - use the espeak library
	DEFINES					+=	LINUX_ESPEAK
	INCLUDEPATH				+=	/usr/include/espeak
	LIBS					+=	-lespeak

	## TTS Option 3 - use the flite library
	#DEFINES				+= 	LINUX_FLITE
	#INCLUDEPATH			+=	/usr/include/flite
	#LIBS					+=	-lflite_cmu_us_kal16
	#LIBS					+=	-lflite
	#LIBS					+=	-lflite_cmulex
	#LIBS					+=	-lflite_usenglish
	
	LIBS					+=	-lm

	########
	# Sound class - How Sound statement works
	########
	# uncomment one of the options

	# Sound - Option 0 - /dev/dsp
	#DEFINES				+=	SOUND_DSP

	# Sound - Option 1 - SDL Mixer
	DEFINES					+=	SOUND_SDL
	LIBS					+=	-lSDL
	LIBS					+=	-lSDL_mixer

	# Sound - Option 2 - QT Mobility Multimedia AudioOut
	#DEFINES 				+=	SOUND_QMOBILITY
	#QT						+=	mobility
	#CONFIG					+=	mobility
	#MOBILITY				+=	multimedia

	########
	# rules for make install
	########
	exampleFiles.files		=	./Examples
	exampleFiles.path		=	/usr/share/basic256
	INSTALLS				+=	exampleFiles

	helpHTMLFiles.files		=	./wikihelp/help
	helpHTMLFiles.path		=	/usr/share/basic256
	INSTALLS				+=	helpHTMLFiles

	transFiles.files		=	./Translations/*.qm
	transFiles.path			=	/usr/share/basic256
	INSTALLS				+=	transFiles

	# main program executable
	target.path				=	/usr/local/bin
	INSTALLS				+=	target

}

macx {
	# macintosh
	DEFINES					+=	MACX
	DEFINES					+=	MACX_SAY
	
	ICON					=	resources/basic256.icns

	LIBS					+=	-L/opt/local/lib
	INCLUDEPATH				+=	/opt/local/include

	## include libraries for SDL audio for wav and sound output
	DEFINES 				+=	SOUND_SDL
	LIBS					+=	-lSDL
	LIBS					+=	-lSDL_mixer

	
}

exists( ./LEX/Makefile ) {
	message( Running make for ./LEX/Makefile )
	system( make -C ./LEX )
} else { 
        error( Could not make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS 					+=	*.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	*.cpp

