#pragma once

#include <list>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>
#include <QLocale>

#include "Error.h"
#include "Convert.h"
#include "DataElement.h"
#include "Settings.h"



class Stack
{
	public:
		Stack(Error *, Convert *, QLocale *);
		~Stack();

		Error *error;
		Convert *convert;
		void pushdataelement(DataElement*);
		void pushbool(bool);
		void pushstring(QString);
		void pushvariant(QString, int);
		void pushint(int);
		void pushlong(long);
        void pushvarref(int, int);
		void pushfloat(double);
		void swap();
		void swap2();
		void topto2();
		void dup();
		void dup2();
		int peekType();
        int peekType(int);
		DataElement *popelement();
		int popint();
		int popbool();
		long poplong();
		double popfloat();
		QString popstring();
		QString debug();
		int height();


	private:
		std::vector<DataElement*> stackdata;
        int stackpointer; //faster than unsigned int and is quite enough as size
        int stacksize;
        void stackGrow();
		QLocale *locale;
};
