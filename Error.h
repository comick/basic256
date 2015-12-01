#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ErrorCodes.h"

#include <QCoreApplication>
#include <QString>

class Error
{
	Q_DECLARE_TR_FUNCTIONS(Error)
	
	public:

		int e;
		int var;
		QString extra;
		int newe;
		int newvar;
		QString newextra;
		  
		Error();
		
		bool pending();

		void process();
		
		void q(int);
		void q(int, int);
		void q(int, int, QString);

		bool isFatal();
		bool isFatal(int);

		QString getErrorMessage(char **);
		QString getErrorMessage(int, int, char**);
 		
};
