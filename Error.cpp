#include "Error.h"

#include <string>


Error::Error() {
	e = ERROR_NONE;
	var = -1;
	extra = "";
	line = 0;
	newe = ERROR_NONE;
	newvar = -1;
	newextra = "";
	loadSettings();
}

void Error::loadSettings() {
	// get the error reporting flags from settings
	// typeconverror	0- report no errors 1 - report problems as warning 	2 - report problems as an error
	SETTINGS;
	typeconverror = settings.value(SETTINGSTYPECONV, SETTINGSTYPECONVDEFAULT).toInt();
	varnotassignederror = settings.value(SETTINGSVARNOTASSIGNED, SETTINGSVARNOTASSIGNEDDEFAULT).toInt();
}

bool Error::pending() {
	// there is a pending error that needs to be handled
	return newe != ERROR_NONE;
}

void Error::process(int currentlinenumber) {
	// move the new error into the current error for
	// reporting
	e = newe;
	var = newvar;
	extra = newextra;
	line = currentlinenumber;
	deq();
}

void Error::deq() {
	// clear the pending (q'd) error
	newe = ERROR_NONE;
	newvar = -1;
	newextra = "";
}

void Error::q(int errornumber) {
	// queue up an error without a variable or extra message
	q(errornumber, -1, "");
}

void Error::q(int errornumber, int variablenumber) {
	// queue up an error with a variable number
	q(errornumber, variablenumber, "");
}

void Error::q(int errornumber, int variablenumber, QString extratext) {
	// queue up an error with all three
	if (errornumber==ERROR_TYPECONV) {
		if (typeconverror==SETTINGSERRORNONE) return;
		if (typeconverror==SETTINGSERRORWARN) errornumber = WARNING_TYPECONV;
	}
	if (errornumber==ERROR_VARNOTASSIGNED) {
		if (varnotassignederror==SETTINGSERRORNONE) return;
		if (varnotassignederror==SETTINGSERRORWARN) errornumber = WARNING_VARNOTASSIGNED;
	}
	newe = errornumber;
	newvar = variablenumber;
	newextra = extratext;
}


bool Error::isFatal() {
	return isFatal(e);
}

bool Error::isFatal(int errornumber) {
	return errornumber<WARNING_START;
}

QString Error::getErrorMessage(char **symtable) {
	return getErrorMessage(e, var, symtable);
}

QString Error::getErrorMessage(int errornumber, int variablenumber, char **symtable) {
    QString errormessage("");
    switch(errornumber) {
        case ERROR_NOSUCHLABEL:
            errormessage = tr("No such label %VARNAME%");
            break;
        case ERROR_FOR1:
            errormessage = tr("Illegal FOR -- start number > end number");
            break;
        case ERROR_FOR2 :
            errormessage = tr("Illegal FOR -- start number < end number");
            break;
        case ERROR_NEXTNOFOR:
            errormessage = tr("Next without FOR");
            break;
        case ERROR_FILENUMBER:
            errormessage = tr("Invalid File Number");
            break;
        case ERROR_FILEOPEN:
            errormessage = tr("Unable to open file");
            break;
        case ERROR_FILENOTOPEN:
            errormessage = tr("File not open");
            break;
        case ERROR_FILEWRITE:
            errormessage = tr("Unable to write to file");
            break;
        case ERROR_FILERESET:
            errormessage = tr("Unable to reset file");
            break;
        case ERROR_ARRAYSIZELARGE:
            errormessage = tr("Array %VARNAME% dimension too large");
            break;
        case ERROR_ARRAYSIZESMALL:
            errormessage = tr("Array %VARNAME% dimension too small");
            break;
        case ERROR_NOSUCHVARIABLE:
            errormessage = tr("Unknown variable %VARNAME%");
            break;
        case ERROR_NOTARRAY:
            errormessage = tr("Variable %VARNAME% is not an array");
            break;
            break;
        case ERROR_ARRAYINDEX:
            errormessage = tr("Array %VARNAME% index out of bounds");
            break;
        case ERROR_STRNEGLEN:
            errormessage = tr("Substring length less that zero");
            break;
        case ERROR_STRSTART:
            errormessage = tr("Starting position less than zero");
            break;
        case ERROR_NONNUMERIC:
            errormessage = tr("Non-numeric value in numeric expression");
            break;
        case ERROR_RGB:
            errormessage = tr("RGB Color values must be in the range of 0 to 255");
            break;
        case ERROR_PUTBITFORMAT:
            errormessage = tr("String input to putbit incorrect");
            break;
        case ERROR_POLYARRAY:
            errormessage = tr("Argument not an array for poly()/stamp()");
            break;
        case ERROR_POLYPOINTS:
            errormessage = tr("Not enough points in array for poly()/stamp()");
            break;
        case ERROR_IMAGEFILE:
            errormessage = tr("Unable to load image file");
            break;
        case ERROR_SPRITENUMBER:
            errormessage = tr("Sprite number out of range");
            break;
        case ERROR_SPRITENA:
            errormessage = tr("Sprite has not been assigned");
            break;
        case ERROR_SPRITESLICE:
            errormessage = tr("Unable to slice image");
            break;
        case ERROR_FOLDER:
            errormessage = tr("Invalid directory name");
            break;
        case ERROR_INFINITY:
            errormessage = tr("Operation returned infinity");
            break;
        case ERROR_DBOPEN:
            errormessage = tr("Unable to open SQLITE database");
            break;
        case ERROR_DBQUERY:
            errormessage = tr("Database query error (message follows)");
            break;
        case ERROR_DBNOTOPEN:
            errormessage = tr("Database must be opened first");
            break;
        case ERROR_DBCOLNO:
            errormessage = tr("Column number out of range or column name not in data set");
            break;
        case ERROR_DBNOTSET:
            errormessage = tr("Record set must be opened first");
            break;
        case ERROR_NETSOCK:
            errormessage = tr("Error opening network socket");
            break;
        case ERROR_NETHOST:
            errormessage = tr("Error finding network host");
            break;
        case ERROR_NETCONN:
            errormessage = tr("Unable to connect to network host");
            break;
        case ERROR_NETREAD:
            errormessage = tr("Unable to read from network connection");
            break;
        case ERROR_NETNONE:
            errormessage = tr("Network connection has not been opened");
            break;
        case ERROR_NETWRITE:
            errormessage = tr("Unable to write to network connection");
            break;
        case ERROR_NETSOCKOPT:
            errormessage = tr("Unable to set network socket options");
            break;
        case ERROR_NETBIND:
            errormessage = tr("Unable to bind network socket");
            break;
        case ERROR_NETACCEPT:
            errormessage = tr("Unable to accept network connection");
            break;
        case ERROR_NETSOCKNUMBER:
            errormessage = tr("Invalid Socket Number");
            break;
        case ERROR_PERMISSION:
            errormessage = tr("You do not have permission to use this statement/function");
            break;
        case ERROR_IMAGESAVETYPE:
            errormessage = tr("Invalid image save type");
            break;
        case ERROR_ARGUMENTCOUNT:
            errormessage = tr("Number of arguments passed does not match FUNCTION/SUBROUTINE definition");
            break;
        case ERROR_MAXRECURSE:
            errormessage = tr("Maximum levels of recursion exceeded");
            break;
        case ERROR_DIVZERO:
            errormessage = tr("Division by zero");
            break;
        case ERROR_BYREF:
            errormessage = tr("Function/Subroutine expecting variable reference in call");
            break;
        case ERROR_BYREFTYPE:
            errormessage = tr("Function/Subroutine variable incorrect reference type in call");
            break;
        case ERROR_FREEFILE:
            errormessage = tr("There are no free file numbers to allocate");
            break;
        case ERROR_FREENET:
            errormessage = tr("There are no free network connections to allocate");
            break;
        case ERROR_FREEDB:
            errormessage = tr("There are no free database connections to allocate");
            break;
        case ERROR_DBCONNNUMBER:
            errormessage = tr("Invalid Database Connection Number");
            break;
        case ERROR_FREEDBSET:
            errormessage = tr("There are no free data sets to allocate for that database connection");
            break;
        case ERROR_DBSETNUMBER:
            errormessage = tr("Invalid data set number");
            break;
        case ERROR_DBNOTSETROW:
            errormessage = tr("You must advance the data set using DBROW before you can read data from it");
            break;
        case ERROR_PENWIDTH:
            errormessage = tr("Drawing pen width must be a non-negative number");
            break;
        case ERROR_COLORNUMBER:
            errormessage = tr("Color values must be in the range of -1 to 16,777,215");
            break;
        case ERROR_ARRAYINDEXMISSING:
            errormessage = tr("Array variable %VARNAME% has no value without an index");
            break;
        case ERROR_IMAGESCALE:
            errormessage = tr("Image scale must be greater than or equal to zero");
            break;
        case ERROR_FONTSIZE:
            errormessage = tr("Font size, in points, must be greater than or equal to zero");
            break;
        case ERROR_FONTWEIGHT:
            errormessage = tr("Font weight must be greater than or equal to zero");
            break;
        case ERROR_RADIXSTRING:
            errormessage = tr("Unable to convert radix string back to a decimal number");
            break;
        case ERROR_RADIX:
            errormessage = tr("Radix conversion base muse be between 2 and 36");
            break;
        case ERROR_LOGRANGE:
            errormessage = tr("Unable to calculate the logarithm or root of a negative number");
            break;
        case ERROR_STRINGMAXLEN:
            errormessage = tr("String exceeds maximum length of 16,777,216 characters");
            break;
        case ERROR_STACKUNDERFLOW:
            errormessage = tr("Stack Underflow Error");
            break;
        case ERROR_NOTANUMBER:
            errormessage = tr("Mathematical operation returned an undefined value");
            break;
        case ERROR_PRINTERNOTON:
            errormessage = tr("Printer is not on");
            break;
        case ERROR_PRINTERNOTOFF:
            errormessage = tr("Printing is already on");
            break;
        case ERROR_PRINTEROPEN:
            errormessage = tr("Unable to open printer");
            break;
        case ERROR_TYPECONV:
            errormessage = tr("Unable to convert string to number");
            break;
        case ERROR_WAVFILEFORMAT:
            errormessage = tr("Media file does not exist or in an unsupported format");
            break;
        case ERROR_FILEOPERATION:
            errormessage = tr("Can not perform that operation on a Serial Port");
            break;
        case ERROR_SERIALPARAMETER:
            errormessage = tr("Invalid serial port parameter");
            break;
        case ERROR_VARNOTASSIGNED:
            errormessage = tr("Variable has not been assigned a value");
            break;


        // put ERROR new messages here
        case ERROR_NOTIMPLEMENTED:
            errormessage = tr("Feature not implemented in this environment");
            break;
        // warnings
        case WARNING_TYPECONV:
            errormessage = tr("Unable to convert string to number, zero used");
            break;
        case WARNING_WAVNOTSEEKABLE:
            errormessage = tr("Media file is not seekable");
            break;
        case WARNING_WAVNODURATION:
            errormessage = tr("Duration is not available for media file");
            break;
        case WARNING_VARNOTASSIGNED:
            errormessage = tr("Variable has not been assigned a value");
            break;


        // put WARNING new messages here
        default:
            errormessage = tr("User thrown error number");

    }
	if (variablenumber>=0) {
		if (errormessage.contains(QString("%VARNAME%"),Qt::CaseInsensitive)) {
			if (symtable[variablenumber]) {
				errormessage.replace(QString("%VARNAME%"),QString(symtable[variablenumber]),Qt::CaseInsensitive);
			} else {
				errormessage.replace(QString("%VARNAME%"),QString("unknown"),Qt::CaseInsensitive);
			}
		}
	}
    return errormessage;
}

