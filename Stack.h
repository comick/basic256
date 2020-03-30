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
		Stack(Convert *, QLocale *);
		~Stack();

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
		double popnote();
		QString popstring();
		QString debug();
		int height();
		void drop(int);


		static int getError() {
			return getError(false);
		}

		static int getError(int clear) {
			int olde = e;
			if (clear) e = ERROR_NONE;
			return olde;
		}

	private:
		std::vector<DataElement*> stackdata;
		int stackpointer; //faster than unsigned int and is quite enough as size
		int stacksize;
		void stackGrow();
		QLocale *locale;
		
		static int e;		// error number thrown - will be 0 if no error
};
