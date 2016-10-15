#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <limits>

#include <QString>
#include <QLocale>
#include <QVariant>
#include <QDebug>

#include "Settings.h"
#include "Error.h"
#include "DataElement.h"
#include "Constants.h"

// convert and compare functions
// to change DataElement into float, int, or QString and to do string and
// numeric comparison (using epislon)
// forward error or warning to the error object

class Convert
{
	public:

		Convert(Error *, QLocale *);
		~Convert();

		void setdecimaldigits(int);

		bool isNumeric(DataElement*);

		int getInt(DataElement*);
		long getLong(DataElement*);
		double getFloat(DataElement*);
		QString getString(DataElement*);
		QString getString(DataElement*, int);
		int getBool(DataElement*);

		int compare(DataElement*, DataElement*);
		int compareFloats(double, double);

	private:
		Error *error;
		QLocale *locale;
		QRegExp *isnumeric;
		int ec(int, int);
		int decimaldigits;	// display n decinal digits 12 default - 8 to 15 valid
		bool floattail;		// display floats with a tail of ".0" if whole numbers
        bool replaceDecimalPoint; // user can chose if INPUT and PRINT should use localized decimal point
        QChar decimalPoint;

};
