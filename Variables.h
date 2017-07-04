// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

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
        Variable(const int, const int);
        ~Variable();

        DataElement data;
        VariableArrayPart *arr;
};


class Variables: public QObject
{
    Q_OBJECT;
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
        Variable* get(const int, int);
        Variable* get(const int);
        Variable* getRawVariable(const int);
        DataElement *getdata(int);
        DataElement *getdata(DataElement* e);
        void setdata(const int, DataElement *);
        void setdata(int, long);
        void setdata(int, double);
        void setdata(int, QString);
        void unassign(const int);
        //
        void copyarray(const int varnum1, DataElement *e);
        //
        void arraydim(const int, const int, const int, const bool);
        DataElement* arraygetdata(const int, const int, const int);
        void arraysetdata(const int, const int, const int, DataElement *);
        void arrayunassign(const int, const int, const int);
        //
        int arraysize(const int);
        int arraysizerows(const int);
        int arraysizecols(const int);
        //
        void makeglobal(const int);


    private:
        Error *error;
        const int numsyms;		// size of the symbol table
        int recurselevel;
        int maxrecurselevel;
        std::vector<Variable**> varmap;
        bool *isglobal;
        void allocateRecurseLevel();
        void clearRecurseLevel();
        void clearvariable(Variable *);
};
