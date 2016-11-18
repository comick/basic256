// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <QString>

#include "Error.h"
#include "DataElement.h"

#define VARIABLE_MAXARRAYELEMENTS 1048576
#define MAX_RECURSE_LEVELS	1048576


class VariableArrayPart
{
    public:
        int xdim;
        int ydim;
        std::vector<DataElement> datavector;
};

class Variable
{
    public:
        Variable(int, int);
        ~Variable();

        DataElement data;
        VariableArrayPart *arr;
};


class VariableInfo
{
    // a variable's inal recurse level and final variable number
    // after global and varref types are processed
    // used by VariableWin to get global and referenced variable info
    public:
        int level;
        int varnum;
};

class Variables
{
    public:
        Variables(int, Error *);
        ~Variables();
        //
        QString debug();
        void increaserecurse();
        void decreaserecurse();
        int getrecurse();
        //
        int type(int);
        //
        Variable* get(int, int);
        Variable* get(int);
        VariableInfo* getInfo(int);
        DataElement *getdata(int);
        DataElement *getdata(DataElement* e);
        void setdata(int, DataElement *);
        void setdata(int, long);
        void setdata(int, double);
        void setdata(int, QString);
		void unassign(int);
        //
        void copyarray(int varnum1, DataElement *e);
        //
        void arraydim(int, int, int, bool);
        DataElement* arraygetdata(int, int, int);
        void arraysetdata(int, int, int, DataElement *);
        void arraysetdata(int, int, int, long);
        void arraysetdata(int, int, int, double);
        void arraysetdata(int, int, int, QString);
        void arrayunassign(int, int, int);
        //
        int arraysize(int);
        int arraysizerows(int);
        int arraysizecols(int);
        //
        void makeglobal(int);


    private:
        Error *error;
        int numsyms;		// size of the symbol table
        int recurselevel;
        std::unordered_map<int, std::vector<Variable*> > varmap;
        std::vector<bool> globals;
        void allocateRecurseLevel();
        void freeRecurseLevel();
        void clearvariable(Variable *);
        bool isglobal(int);

};
