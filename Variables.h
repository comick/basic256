// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

#include "Error.h"
#include "DataElement.h"

#define MAX_RECURSE_LEVELS	1048576


class Variable
{
    public:
        Variable();
        ~Variable();

        DataElement *data;
};


class Variables: public QObject
{
    Q_OBJECT;
    public:
        Variables(int);
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
        Variable* getAt(const int, const int);
        Variable* getAt(const int);
        DataElement *getdata(int);
        DataElement *getdata(int, const bool);
        void setdata(const int, DataElement *);
        void setdata(int, long);
        void setdata(int, double);
        void setdata(int, QString);
        void unassign(const int);
        //
        void makeglobal(const int);


    private:
        int real_varnum;		// set by get and getAt for the actual variable number and level returned (deref/global)
        int real_level;
        const int numsyms;		// size of the symbol table
        int recurselevel;
        int maxrecurselevel;
        std::vector<Variable**> varmap;
        bool *isglobal;
        void allocateRecurseLevel();
        void clearRecurseLevel();
        void clearvariable(Variable *);
};
