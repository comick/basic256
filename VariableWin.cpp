/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include "VariableWin.h"
#include "DataElement.h"
#include "MainWindow.h"

extern MainWindow * mainwin;

extern "C" {
extern char **symtable;
extern int numsyms;
}

VariableWin::VariableWin () {
    setColumnCount(3);
    setHeaderLabels(QStringList() << tr("Level - Name") << tr("Ty") << tr("Value"));
    resizeColumnToContents(COLUMNTYPE);
    sortByColumn(COLUMNNAME, Qt::AscendingOrder);
    setSortingEnabled(true);
    convert = new Convert(NULL, mainwin->locale);
    //hold translations to speed up things
    tr_stringType = tr("Type: String");
    tr_integerType = tr("Type: Integer");
    tr_floatType = tr("Type: Float");
    tr_arrayType = tr("Type: Array");
    tr_referenceType = tr("Type: Reference");
    tr_unknownType = tr("?");
}

VariableWin::~VariableWin() {
    delete(convert);
}

void VariableWin::setTypeAndValue(QTreeWidgetItem *r, DataElement *d) {
    // set thr type and value columns (2 and 3)
    if (r) {
        if (d!=NULL) {
            switch (d->type) {
            case T_STRING:{
                r->setText(COLUMNTYPE, QStringLiteral("S"));
                r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_stringType);
                r->setData(COLUMNVALUE, Qt::UserRole + 1, d->stringval);
                r->setText(COLUMNVALUE, d->stringval);
                break;
            }
            case T_INT:{
                r->setText(COLUMNTYPE, QStringLiteral("I"));
                r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_integerType);
                r->setData(COLUMNVALUE, Qt::UserRole + 1, (long long)d->intval);
                r->setText(COLUMNVALUE, convert->getString(d)); //to keep user choice about printing numbers
                break;
            }
            case T_FLOAT:{
                r->setText(COLUMNTYPE, QStringLiteral("F"));
                r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_floatType);
                r->setData(COLUMNVALUE, Qt::UserRole + 1, d->floatval);
                r->setText(COLUMNVALUE, convert->getString(d)); //using user locale format
                break;
            }
            case T_REF:{
                r->setText(COLUMNTYPE, QStringLiteral("REF"));
                r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_referenceType);
                QString var = QString::number(d->level) + QStringLiteral(" - ") + symtable[d->intval];
                r->setText(COLUMNVALUE, var);
                r->setData(COLUMNVALUE, Qt::UserRole + 1, var); //to do:00level+var
                break;
            }
            default:{
                r->setText(COLUMNTYPE, tr_unknownType);
                r->setData(COLUMNTYPE, Qt::ToolTipRole, QStringLiteral(""));
                r->setText(COLUMNVALUE, QStringLiteral(""));
                r->setData(COLUMNVALUE, Qt::UserRole + 1, QVariant());
            }
            }
        } else {
            r->setText(COLUMNTYPE, tr_unknownType);
            r->setData(COLUMNTYPE, Qt::ToolTipRole, QStringLiteral(""));
            r->setText(COLUMNVALUE, QStringLiteral(""));
            r->setData(COLUMNVALUE, Qt::UserRole + 1, QVariant());
        }
    }
}

void VariableWin::varWinAssign(Variables **variables, int varnum, int x, int y) {
    QTreeWidgetItem *rowItem = NULL;
    QString name;
    DataElement *d;

    if(*variables){
        Variable *v = (*variables)->getRawVariable(varnum);
        name = QString::number(v->data.level) + QStringLiteral(" - ") + symtable[varnum];
        d = (*variables)->arraygetdata(varnum, x, y);

        // find either name[y] or name[x,y]
        QString tname;
        if(v->arr){
            if(v->arr->xdim>1){
                tname = name + QStringLiteral("[") + QString::number(x) + QStringLiteral(",") + QString::number(y) + QStringLiteral("]");
            }else{
                tname = name + QStringLiteral("[") + QString::number(y) + QStringLiteral("]");
            }
            QList<QTreeWidgetItem *> list = findItems(tname, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);
            if (list.size()>0) rowItem = list[0]; //else add item with array in case user choose to watch just a specific array element
        }

        // set the data and data type
        setTypeAndValue(rowItem, d);
    }
}

void VariableWin::varWinAssign(Variables **variables, int varnum) {
    // refresh variable/array content
    TreeWidgetItem *rowItem = NULL;
    QString name;

    if(*variables){
        Variable *v = (*variables)->getRawVariable(varnum);
        name = QString::number(v->data.level) + QStringLiteral(" - ") + symtable[varnum];

        // see if v is on the list and change value or add
        QList<QTreeWidgetItem *> list = findItems(name, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);

        if (list.size() > 0) {
            // get existing element
            rowItem = (TreeWidgetItem *)list[0];
            if(rowItem->childCount()>0){
                // fast remove all components if array
                // it is much faster to delete array by once instead of deleting each element
                delete rowItem;
                rowItem=NULL;
            }
        }
        if(!rowItem){
            // add new element
            rowItem = new TreeWidgetItem();
            rowItem->setText(COLUMNNAME, name);
            //set correct name in format 0000000name to correctly arrange and avoid: "1 - a", "120 - a", "2 - a"
            QString id = QString("%1").arg(v->data.level, 7, 10, QChar('0')) + symtable[varnum];
            rowItem->setData(COLUMNNAME, Qt::UserRole + 1, id);
            addTopLevelItem(rowItem);
        }

        if(v->data.type!=T_ARRAY){
            // regular variable
            setTypeAndValue(rowItem, &v->data);
        }else{
            // array
            const int arraylenx = v->arr->xdim;
            const int arrayleny = v->arr->ydim;
            rowItem->setText(COLUMNTYPE, QStringLiteral("A"));
            rowItem->setData(COLUMNTYPE, Qt::ToolTipRole, tr_arrayType);
            if(arraylenx > 1){
                rowItem->setText(COLUMNVALUE, QStringLiteral("[") + QString::number(arraylenx) + QStringLiteral(",") + QString::number(arrayleny) + QStringLiteral("]"));
            }else{
                rowItem->setText(COLUMNVALUE, QStringLiteral("[") + QString::number(arrayleny) + QStringLiteral("]"));
            }
            // add place holders for the array elements as children for a new array
            if (arraylenx <= 1) {
                // 1d array
                for(int y=0; y<arrayleny; y++) {
                    TreeWidgetItem *childItem = new TreeWidgetItem();
                    childItem->setText(COLUMNNAME, name + QStringLiteral("[") + QString::number(y) + QStringLiteral("]"));
                    //allow sorting in correct order (by index)
                    childItem->setData(COLUMNNAME, Qt::UserRole + 1, y);
                    //direct access to array's data
                    setTypeAndValue(childItem, &v->arr->datavector[y]);
                    rowItem->addChild(childItem);
                }
            } else {
                // 2d array
                for(int x=0; x<arraylenx; x++) {
                    for(int y=0; y<arrayleny; y++) {
                        TreeWidgetItem *childItem = new TreeWidgetItem();
                        childItem->setText(COLUMNNAME, name + QStringLiteral("[") + QString::number(x) + QStringLiteral(",") + QString::number(y) + QStringLiteral("]"));
                        //allow sorting in correct order (by index)
                        int index=x*arrayleny+y;
                        childItem->setData(COLUMNNAME, Qt::UserRole + 1, index);
                        //direct access to array's data
                        setTypeAndValue(childItem, &v->arr->datavector[index]);
                        rowItem->addChild(childItem);
                    }
                }
            }
        }
    }
}

void VariableWin::varWinDropLevel(int level) {
    // when we return from a subroutine or function delete its variables
    QString name = QString::number(level) + QStringLiteral(" ");
    QList<QTreeWidgetItem *> items = findItems(name, Qt::MatchStartsWith, COLUMNNAME);
    for (int n=items.size()-1; n>=0; n--) {
        delete items[n];
    }
}

void VariableWin::clear() {
    // when window is cleared (Interpreter restarts) we reload convert settings
    // to make sure that convert will reflect the current settings (user may change those any time)
    delete (convert);
    convert = new Convert(NULL, mainwin->locale);
    QTreeWidget::clear();
}
