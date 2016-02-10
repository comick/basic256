#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ErrorCodes.h"
#include "Settings.h"

#include <QCoreApplication>
#include <QString>

class Error
{
	Q_DECLARE_TR_FUNCTIONS(Error)
	
	public:

		int e;
		int var;
		QString extra;
		int line;
		int newe;
		int newvar;
		QString newextra;
		
		void loadSettings();
		  
		Error();
		
		bool pending();

		void process(int);
		
		void q(int);
		void q(int, int);
		void q(int, int, QString);
		
		void deq();

		bool isFatal();
		bool isFatal(int);

		QString getErrorMessage(char **);
		QString getErrorMessage(int, int, char**);
 		
 	private:
 		int typeconverror;
 		int varnotassignederror;
};
