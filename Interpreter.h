/** Copyright (C) 2006, Ian Paul Larsen.
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

#ifndef __INTERPRETER_H
#define __INTERPRETER_H

#include <QPixmap>
#include <QImage>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QTime>
#include <stdio.h>
#include <cmath>
#include <dirent.h>
#include "BasicGraph.h"
#include "Constants.h"
#include "DataElement.h"
#include "Error.h"
#include "Convert.h"
#include "Stack.h"
#include "Variables.h"
#include "Sound.h"
#include "Sleeper.h"
#include "BasicDownloader.h"

#include <QElapsedTimer>
#include <QDebug>
#include <QProcess>


#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrinterInfo>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

#ifndef ANDROID
	// includes for all ports EXCEPT android
	#include <QSerialPort>
#endif

enum run_status {R_STOPPED, R_RUNNING, R_INPUT, R_INPUTREADY, R_ERROR, R_PAUSED, R_STOPING};

#define NUMFILES 8
#define NUMSOCKETS 8
#define NUMDBCONN 8
#define NUMDBSET 8

#define STRINGMAXLEN 16777216

#define FILEWRITETIMEOUT		1			// on a file/serial write wait up to MS for the write to complete
#define FILEREADTIMEOUT			1			// on a file/serial read wait up to MS for data to be there
#define SERIALREADBUFFERSIZE	1024		// size of openserial read buffer


struct byteCodeData
{
	unsigned int size;
	void *data;
};


// used by function calls, subroutine calls, and gosubs for return location
struct frame {
	frame *next;
	int *returnAddr;
};

// used to track nested on-error and try/catch definitions
struct onerrorframe {
	onerrorframe *next;
	int onerroraddress;
	bool onerrorgosub;
};

// structure for the nested for statements
// if useInt then make loop integer safe
struct forframe {
	forframe *next;
	int variable;
	int *returnAddr;
	bool useInt;
	double floatStart;
	double floatEnd;
	double floatStep;
	long intStart;
	long intEnd;
	long intStep;
	int recurselevel;
};

typedef struct {
    bool visible;
    double x;
    double y;
    double r;	// rotate
    double s;	// scale
    double o;	// opacity
    QImage *image;
    QImage *transformed_image;
    QRect position;
    bool changed;
    bool was_printed;
    QRect last_position;
} sprite;

class Interpreter : public QThread
{
  Q_OBJECT;
	public:
        Interpreter(QLocale*);
		~Interpreter();
		int compileProgram(char *);
		void initialize();
		bool isRunning();
		bool isStopped();
        void setStopped();
		bool isAwaitingInput();
		void setInputReady();
		void cleanup();
        void run();
        int debugMode;			// 0=normal run, 1=step execution, 2=run to breakpoint
		QList<int> *debugBreakPoints;	// map of line numbers where break points ( pointer to breakpoint list in basicedit)
		QString returnString;		// return value from runcontroller emit
		int returnInt;			// return value from runcontroller emit
        int settingsAllowPort;
        int settingsAllowSystem;

    public slots:
		int execByteCode();
		void runHalted();
        void inputEntered(QString);

	signals:
		void debugNextStep();
		void fastGraphics();
        //void stopRun();
        void stopRunFinalized(bool);
		void goutputReady();
		void outputReady(QString);
		void outputError(QString);
		void getInput();
		void outputClear();
		void getKey();
		void playSounds(int, int*);
		void setVolume(int);
        //void executeSystem(QString);
		void speakWords(QString);
		void goToLine(int);
		void seekLine(int);
        void varWinAssign(Variables**, int);
        void varWinAssign(Variables**, int, int, int);
		void varWinDropLevel(int);
        void varWinDimArray(Variables**, int, int, int);
		void mainWindowsResize(int, int, int);
		void mainWindowsVisible(int, bool);
		void dialogAlert(QString);
		void dialogConfirm(QString, int);
        void dialogPrompt(QString, QString);
        void dialogAllowPortInOut(QString);
        void dialogAllowSystem(QString);
        void playSound(QString, bool);
        void playSound(std::vector<std::vector<double>>, bool);
        void loadSoundFromArray(QString, QByteArray*);
        void soundStop(int);
        void soundPlay(int);
        void soundFade(int, double, int, int);
        void soundVolume(int, double);
        //void soundExit();
        void soundPlayerOff(int);
        void soundSystem(int);

	private:
		QLocale *locale;
		Sleeper *sleeper;
        BasicDownloader *downloader;
		int optype(int op);
		QString opname(int);
		void waitForGraphics();
		void printError();
		int netSockClose(int);
		void netSockCloseAll();
		Variables *variables;
		Stack *stack;
		Error *error;
        bool isError; //flag set if program stops because of an error
		Convert *convert;
		QIODevice **filehandle;
		int *filehandletype;		// 0=QFile (normal), 1=QFile (binary), 2=QSerialPort
		int *op;
		frame *callstack;
		forframe *forstack;
		onerrorframe *onerrorstack;
		run_status status;
		run_status oldstatus;
		bool fastgraphics;
		QString inputString;
		int inputType;				// data type to convert the input into
        double double_random_max;
        int currentLine;
		void clearsprites();
        void update_sprite_screen();
        void sprite_prepare_for_new_content(int);
        void force_redraw_all_sprites_next_time();
        bool sprite_collide(int, int, bool);
		sprite *sprites;
		int nsprites;
		void closeDatabase(int);
		// watch... functions trigger the variablewatch window to display
        void watchvariable(bool, int);
        void watchvariable(bool, int, int, int);
        void watchdim(bool, int, int, int);
        void watchdecurse(bool);

		int listensockfd;				// temp socket used in netlisten
		int netsockfd[NUMSOCKETS];
		
		DIR *directorypointer;		// used by DIR function
		QTime runtimer;				// used by MSEC function
        //SoundSystem *sound;
        QString currentIncludeFile;	// set to current included file name for runtime error messages
		bool regexMinimal;			// flag to tell QRegExp to be greedy (false) or minimal (true)

		bool printing;
		QPrinter *printdocument;

        QPainter *painter;
        bool painter_pen_need_update;
        bool painter_brush_need_update;
        bool painter_last_compositionModeClear;
        unsigned long painter_brush_color; //last color value for comparison
        unsigned long painter_pen_color; //last color value for comparison
        void setGraph(QString id);
        QString drawto;
        bool setPainterTo(QPaintDevice *destination);
        QPen drawingpen;
        QBrush drawingbrush;
        int CompositionModeClear;
        int PenColorIsClear;
        bool drawingOnScreen;

        QFont font;
        QString defaultfontfamily;
        int defaultfontpointsize;
        int defaultfontweight;
        bool defaultfontitalic;

        bool painter_font_need_update;
        bool painter_custom_font_flag;



		QSqlQuery *dbSet[NUMDBCONN][NUMDBSET];		// allow NUMDBSET number of sets on a database connection

        int mediaplayer_id_legacy;

        QMap <QString, QImage*> images;
        int lastImageId;
        bool imageSmooth;

        int settingsDebugSpeed;
        bool settingsAllowSetting;
        int settingsSettingsAccess;
        int settingsSettingsMax;
        QString programName;
        int settingsPrinterResolution;
        int settingsPrinterPrinter;
        int settingsPrinterPaper;
        QString settingsPrinterPdfFile;
        int settingsPrinterOrient;
        QMap<QString, QMap<QString, QString>> fakeSettings;
        QProcess *sys;

};


#endif
