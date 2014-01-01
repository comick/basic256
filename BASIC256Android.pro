###
### qmake for android
###
###

QT       += core gui widgets printsupport

###message($$[QT_VERSION])

DEFINES                                         +=	ANDROID

DEFINES                                         +=      DISABLESQLITE
DEFINES                                         +=      DISABLESYNTAXHIGHLIGHT

TEMPLATE					=	app
TARGET						=	BASIC256
DEPENDPATH					+=	.
INCLUDEPATH					+=	.
QMAKE_CXXFLAGS                                  +=	-g
CONFIG						+=	qt debug_and_release
OBJECTS_DIR					=	tmp/obj
MOC_DIR						=	tmp/moc

RESOURCES					+=	resources/resource.qrc
RC_FILE						=	resources/basic256.rc
TRANSLATIONS                                    =	Translations/basic256_en.ts \
                                                        Translations/basic256_de.ts \
                                                        Translations/basic256_ru.ts \
                                                        Translations/basic256_es.ts \
                                                        Translations/basic256_fr.ts \
                                                        Translations/basic256_pt.ts \
                                                        Translations/basic256_nl.ts

DEFINES                         		+=	USEQSOUND

	########
	# TTS control - How Say statement works
	########

	########
	# Sound class - How Sound statement works
	########

	# Sound - Option 1 - QT Mobility Multimedia AudioOut
	DEFINES					+=	SOUND_QMOBILITY
        QT					+=	multimedia
	INCLUDEPATH				+=	QtMultimediaKit
	INCLUDEPATH				+=	QtMobility
	CONFIG					+=	mobility
	MOBILITY				+=	multimedia

exists( ./LEX/Makefile ) {
	message( Running make for ./LEX/Makefile )
	system( make -C ./LEX )
} else { 
        error( Could not make LEX project - aborting... )
}


# Input
HEADERS 					+=	LEX/basicParse.tab.h 
HEADERS 					+=	*.h
HEADERS						+=	android/AndroidTTS.h

SOURCES 					+=	LEX/lex.yy.c 
SOURCES 					+=	LEX/basicParse.tab.c 
SOURCES 					+=	*.cpp
SOURCES 					+=	android/AndroidTTS.cpp

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/version.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java \
    android/AndroidManifest.xml \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/version.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java

