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
    if (errornumber==ERROR_LONGRANGE) {
        if (typeconverror==SETTINGSERRORNONE) return;
        if (typeconverror==SETTINGSERRORWARN) errornumber = WARNING_LONGRANGE;
    }
    if (errornumber==ERROR_INTEGERRANGE) {
        if (typeconverror==SETTINGSERRORNONE) return;
        if (typeconverror==SETTINGSERRORWARN) errornumber = WARNING_INTEGERRANGE;
    }
    if (errornumber==ERROR_STRING2NOTE) {
        if (typeconverror==SETTINGSERRORNONE) return;
        if (typeconverror==SETTINGSERRORWARN) errornumber = WARNING_STRING2NOTE;
    }

    if (errornumber==ERROR_VARNOTASSIGNED) {
        if (varnotassignederror==SETTINGSERRORNONE) return;
        if (varnotassignederror==SETTINGSERRORWARN) errornumber = WARNING_VARNOTASSIGNED;
    }
    if (errornumber==ERROR_ARRAYELEMENT) {
        if (varnotassignederror==SETTINGSERRORNONE) return;
        if (varnotassignederror==SETTINGSERRORWARN) errornumber = WARNING_ARRAYELEMENT;
    }
    if (errornumber==ERROR_REFNOTASSIGNED) {
        if (varnotassignederror==SETTINGSERRORNONE) return;
        if (varnotassignederror==SETTINGSERRORWARN) errornumber = WARNING_REFNOTASSIGNED;
    }
    if(newe == ERROR_NONE){
        //store only first error from last operation
        //example: print a/b
        //will print error: "ERROR on line 1: Division by zero."
        //but the first error is the most important to debug the problem:
        //"ERROR on line 1: Variable b has not been assigned a value."
        newe = errornumber;
        newvar = variablenumber;
        newextra = extratext;
    }
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
        case ERROR_NOSUCHSUBROUTINE:
            errormessage = tr("No such SUBROUTINE %VARNAME%");
            break;
        case ERROR_NOSUCHFUNCTION:
            errormessage = tr("No such FUNCTION %VARNAME%");
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
		case ERROR_ARRAYEVEN:
			errormessage = tr("Array data must be one dimensional with an even number of elements or two dimemsional with two elements in each row");
			break;
		case ERROR_NOTARRAY:
			errormessage = tr("Variable %VARNAME% is not an array");
			break;
		case ERROR_ARRAYINDEX:
			errormessage = tr("Array %VARNAME% index out of bounds");
			break;
		case ERROR_ARRAYNITEMS:
			errormessage = tr("Array %VARNAME% rows must have the same number of items");
			break;
		case ERROR_STRSTART:
			errormessage = tr("Starting position less than zero");
			break;
		case ERROR_RGB:
			errormessage = tr("RGB Color values must be in the range of 0 to 255");
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
        case ERROR_EXPECTEDARRAY:
            errormessage = tr("Expected array");
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
		case ERROR_ARRAYINDEXMISSING:
			errormessage = tr("Array variable %VARNAME% has no value without an index");
			break;
		case ERROR_IMAGESCALE:
			errormessage = tr("Image scale must be greater than or equal to zero");
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
		case ERROR_FILEOPERATION:
			errormessage = tr("Can not perform that operation on a Serial Port");
			break;
		case ERROR_SERIALPARAMETER:
			errormessage = tr("Invalid serial port parameter");
			break;
		case ERROR_VARNOTASSIGNED:
			errormessage = tr("Variable %VARNAME% has not been assigned a value");
			break;
		case ERROR_LONGRANGE:
			errormessage = tr("Number exceeds long integer range (") + QString::number(LONG_MIN) + tr(" to ") + QString::number(LONG_MAX) + tr(")");
			break;
		case ERROR_INTEGERRANGE:
			errormessage = tr("Number exceeds integer range (") + QString::number(INT_MIN) + tr(" to ") + QString::number(INT_MAX) + tr(")");
			break;
        case ERROR_UNSERIALIZEFORMAT:
            errormessage = tr("Unable to UnSerialize string");
			break;
		case ERROR_SLICESIZE:
			errormessage = tr("Invalid Slice dimensions");
			break;
		case ERROR_ARRAYLENGTH2D:
			errormessage = tr("Can not get a single length if a two dimensional array");
			break;
        case ERROR_VARNULL:
            errormessage = tr("Variable %VARNAME% refers to a non-existent variable");
            break;
        case ERROR_VARCIRCULAR:
            errormessage = tr("Circular reference in variable %VARNAME%");
            break;
        case ERROR_IMAGERESOURCE:
            errormessage = tr("Specified image resource not found");
            break;
        case ERROR_SOUNDRESOURCE:
            errormessage = tr("Specified sound resource not found");
            break;
        case ERROR_INVALIDRESOURCE:
            errormessage = tr("Specified resource not found");
            break;
        case ERROR_SOUNDFILE:
            errormessage = tr("Unable to load sound file");
            break;
        case ERROR_DOWNLOAD:
            errormessage = tr("Error downloading file");
            break;
        case ERROR_EXPECTEDSOUND:
            errormessage = tr("Expected sound");
            break;
        case ERROR_TOOMANYSOUNDS:
            errormessage = tr("Too many sound instances");
            break;
        case ERROR_ENVELOPEODD:
            errormessage = tr("Envelope data must contain at least 4 elements and an odd number of elements");
            break;
        case ERROR_ENVELOPEMAX:
            errormessage = tr("It was exceeded the maximum length of an envelope, which is 20 seconds");
            break;
        case ERROR_HARMONICNUMBER:
            errormessage = tr("Harmonic number must be an integer greater than zero");
            break;
        case ERROR_HARMONICLIST:
            errormessage = tr("Harmonics list must be one dimensional array with an even number of elements or two dimemsional array with two elements in each row");
            break;
        case ERROR_ONEDIMENSIONAL:
            errormessage = tr("Expects one dimensional array or one dimensional list of elements");
            break;
        case ERROR_WAVEFORMLOGICAL:
            errormessage = tr("Creating custom waveform using logical coordinates it request at least 3 elements");
            break;
        case ERROR_STRING2NOTE:
            errormessage = tr("Unable to convert string to musical note");
            break;
        case ERROR_ARRAYELEMENT:
            errormessage = tr("Element of array %VARNAME% has not been assigned a value");
            break;
        case ERROR_SETTINGSGETACCESS:
            errormessage = tr("The program does not have permission to read settings from other program. Check access level from 'Preferences' panel");
            break;
        case ERROR_SETTINGSSETACCESS:
            errormessage = tr("The program does not have permission to write settings for other program. Check access level from 'Preferences' panel");
            break;
        case ERROR_INVALIDPROGNAME:
            errormessage = tr("Invalid program name");
            break;
        case ERROR_INVALIDKEYNAME:
            errormessage = tr("Invalid key name");
            break;
        case ERROR_SETTINGMAXLEN:
            errormessage = tr("Setting string exceeds maximum length of 16,383 characters");
            break;
        case ERROR_SETTINGMAXKEYS:
            errormessage = tr("The maximum number of keys for this program has been exceeded");
            break;
        case ERROR_REFNOTASSIGNED:
            errormessage = tr("Variable %VARNAME% refers to an unassigned variable");
            break;
        case ERROR_UNEXPECTEDRETURN:
            errormessage = tr("Unexpected RETURN");
            break;










		// put ERROR new messages here
		case ERROR_NOTIMPLEMENTED:
			errormessage = tr("Feature not implemented in this environment");
			break;
		// warnings
		case WARNING_TYPECONV:
			errormessage = tr("Unable to convert string to number, zero used");
			break;
		case WARNING_VARNOTASSIGNED:
			errormessage = tr("Variable %VARNAME% has not been assigned a value");
			break;
		case WARNING_LONGRANGE:
			errormessage = tr("Number exceeds long integer range (") + QString::number(LONG_MIN) + tr(" to ") + QString::number(LONG_MAX) + tr("), zero used");
			break;
		case WARNING_INTEGERRANGE:
			errormessage = tr("Number exceeds integer range (") + QString::number(INT_MIN) + tr(" to ") + QString::number(INT_MAX) + tr("), zero used");
			break;
        case WARNING_SOUNDNOTSEEKABLE:
            errormessage = tr("Media file is not seekable");
            break;
        case WARNING_SOUNDLENGTH:
            errormessage = tr("Duration is not available for media file");
            break;
        case WARNING_WAVOBSOLETE:
            errormessage = tr("WAVPLAY suite is obsolete. Use SOUND/SOUNDPLAY/SOUNDPLAYER instead");
            break;
        case WARNING_SOUNDFILEFORMAT:
            errormessage = tr("Unable to play the selected file");
            break;
        case WARNING_SOUNDERROR:
            errormessage = tr("Unable to play the sound");
            break;
        case WARNING_ARRAYELEMENT:
            errormessage = tr("Element of array %VARNAME% has not been assigned a value");
            break;
        case WARNING_REFNOTASSIGNED:
            errormessage = tr("Variable %VARNAME% refers to an unassigned variable");
            break;



		// put WARNING new messages here
		default:
			errormessage = tr("User thrown error number");

	}
	if (variablenumber>=0) {
		if (errormessage.contains(QString("%VARNAME%"),Qt::CaseInsensitive)) {
			if (symtable[variablenumber]) {
                errormessage.replace(QString("%VARNAME%"),QString("'") + QString(symtable[variablenumber]) + QString("'"),Qt::CaseInsensitive);
			} else {
				errormessage.replace(QString("%VARNAME%"),QString("unknown"),Qt::CaseInsensitive);
			}
		}
	}
	return errormessage;
}

