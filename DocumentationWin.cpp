/** Copyright (C) 2010, J.M.Reneau, S.W.Irupin, Florin Oprea.
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with this program; if not, write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include <iostream>
using namespace std;

#include "DocumentationWin.h"
#include "MainWindow.h"

DocumentationWin::DocumentationWin (QWidget * parent){
    setWindowFlags(Qt::Window);
    localecode = ((MainWindow *) parent)->localecode.left(2);;
    indexfile = "start.html";
    defaultlocale = "en";
    occurrences = 0;

    //Position where it was last on screen
    SETTINGS;
    resize(settings.value(SETTINGSDOCSIZE, QSize(700, 500)).toSize());
    move(settings.value(SETTINGSDOCPOS, QPoint(150, 150)).toPoint());

    //Prepare docs
    docs = new QTextBrowser( this );
    //Set sleceted text to have the same color if QTextBrowser has not focus
    QPalette p = docs->palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
    docs->setPalette(p);
    docs->setOpenLinks(false);
    docs->setOpenExternalLinks(true);
    docs->setSearchPaths(QStringList()
                         <<	QApplication::applicationDirPath() + "/help/"
                         <<	"/usr/share/basic256/help/"
                         <<	"/usr/local/share/basic256/help/"
                         <<	"./"
                         <<	QApplication::applicationDirPath() + "/../wikihelp/help/"
                         <<	QApplication::applicationDirPath() + "/wikihelp/help/"
                        );
    connect(docs,SIGNAL(anchorClicked(QUrl)),this,SLOT(hijackAnchorClicked(QUrl)));
    connect(docs, SIGNAL(sourceChanged(const QUrl)), this, SLOT(sourceChangedSlot(const QUrl)));

    //Prepare toolbar
    toolbar = new QToolBar( this );
    QAction *home = new QAction(QIcon(":images/home.png"), tr("&Home"), this);
    connect(home, SIGNAL(triggered()), docs, SLOT(home()));
    toolbar->addAction(home);
    QAction *backward = new QAction(QIcon(":images/backward.png"), tr("&Back"), this);
    connect(backward, SIGNAL(triggered()), docs, SLOT(backward()));
    connect(docs, SIGNAL(backwardAvailable(bool)), backward, SLOT(setEnabled(bool)));
    toolbar->addAction(backward);
    QAction *forward = new QAction(QIcon(":images/forward.png"), tr("&Forward"), this);
    connect(forward, SIGNAL(triggered()), docs, SLOT(forward()));
    connect(docs, SIGNAL(forwardAvailable(bool)), forward, SLOT(setEnabled(bool)));
    toolbar->addAction(forward);
    toolbar->addSeparator();
    QAction* find = new QAction(QIcon(":images/find.png"), tr("&Find"), this);
    find->setShortcuts(QKeySequence::keyBindings(QKeySequence::Find));
    connect(find, SIGNAL(triggered()), this, SLOT (searchSetFocus()));
    toolbar->addAction(find);
    QAction *zoomin = new QAction(QIcon(":images/zoom-in.png"), tr("Zoom in"), this);
    zoomin->setShortcuts(QKeySequence::keyBindings(QKeySequence::ZoomIn));
    connect(zoomin, SIGNAL(triggered()), docs, SLOT(zoomIn()));
    toolbar->addAction(zoomin);
    QAction *zoomout = new QAction(QIcon(":images/zoom-out.png"), tr("Zoom out"), this);
    zoomout->setShortcuts(QKeySequence::keyBindings(QKeySequence::ZoomOut));
    connect(zoomout, SIGNAL(triggered()), docs, SLOT(zoomOut()));
    toolbar->addAction(zoomout);
    toolbar->addSeparator();
    QAction *print_act = new QAction(QIcon(":images/print.png"), tr("&Print..."), this);
    print_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));
    connect(print_act, SIGNAL(triggered()), this, SLOT(slotPrintHelp()));
    toolbar->addAction(print_act);
    toolbar->addSeparator();
    QAction *onlinehact = new QAction(QIcon(":images/firefox.png"), tr("&Online help..."), this);
    connect(onlinehact, SIGNAL(triggered()), this, SLOT(showOnlineHelpPage()));
    toolbar->addAction(onlinehact);
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    toolbar->addWidget(empty);
    viewLanguage = new QLabel(tr("View this page in") + ": ",this);
    toolbar->addWidget(viewLanguage);
    comboLanguage = new QComboBox;
    comboLanguage->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(comboLanguage, SIGNAL(activated (const QString & )), this, SLOT(userSelectLanguage(const QString & )));
    toolbar->addWidget(comboLanguage);

    //Prepare search bar elements
    searchlabel = new QLabel(tr("Find in page") + ": ",this);
    searchinput = new QLineEdit;
    searchinput->setMaxLength(50);
    searchinput->setMaximumWidth(400);
    searchinput->setClearButtonEnabled(true);
    connect(searchinput, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextChanged()));
    connect(searchinput, SIGNAL(textChanged(const QString &)), this, SLOT(searchTextChanged()));
    findprev = new QToolButton(this);
    findprev->setIcon(QIcon(":images/previous.png"));
    connect(findprev, SIGNAL(clicked()), this, SLOT (clickFindPrev()));
    findnext = new QToolButton(this);
    findnext->setIcon(QIcon(":images/next.png"));
    connect(findnext, SIGNAL(clicked()), this, SLOT (clickFindNext()));
    casecheckbox = new QCheckBox(tr("Case sensitive"),this);
    connect(casecheckbox, SIGNAL(stateChanged(int)), this, SLOT(searchTextChanged()));
    resultslabel = new QLabel("",this);
    QPalette* grayPalette = new QPalette;
    grayPalette->setColor(QPalette::WindowText,Qt::darkGray);
    resultslabel->setPalette(*grayPalette);
    closeButton = new QPushButton(this);
    QStyle *style = qApp->style();
    QIcon closeIcon = style->standardIcon(QStyle::SP_TitleBarCloseButton);
    closeButton->setIcon(closeIcon);
    closeButton->setFlat(true);
    connect(closeButton, SIGNAL(clicked()), this, SLOT (clickCloseFind()));

    //Prepare search bar
    bottom = new QWidget;
    bottom->hide();
    footer = new QHBoxLayout;
    footer->setContentsMargins(5,5,5,5);
    footer->addWidget(searchlabel);
    footer->addWidget(searchinput);
    footer->addWidget(findprev);
    footer->addWidget(findnext);
    footer->addSpacing(10);
    footer->addWidget(casecheckbox);
    footer->addSpacing(10);
    footer->addWidget(resultslabel);
    footer->addStretch(1);
    footer->addWidget(closeButton);
    bottom->setLayout(footer);

    //Prepare main layout
    layout = new QVBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(toolbar);
    layout->addWidget(docs);
    layout->addWidget(bottom);
    this->setLayout(layout);
    //this->show();

    //Shortcuts
    QAction* findnext = new QAction (this);
    findnext->setShortcuts(QKeySequence::keyBindings(QKeySequence::FindNext));
    connect(findnext, SIGNAL(triggered()), this, SLOT (clickFindNext()));
    addAction (findnext);
    QAction* findprev = new QAction (this);
    findprev->setShortcuts(QKeySequence::keyBindings(QKeySequence::FindPrevious));
    connect(findprev, SIGNAL(triggered()), this, SLOT (clickFindPrev()));
    addAction (findprev);
}


void DocumentationWin::go(QString word) {
    static bool firstTime = true;
    // pass word for context level help or pass "" for general help
    QString u;
    if (word == "") {
        u = localecode + "_" + indexfile;
        if(setBestSourceForHelp(u.toLower())==false)
            docs->setHtml(tr("<h2>Local help files are not available.<h2><p>Try the online documentation at <a href='http://doc.basic256.org'>http://doc.basic256.org</a>.</p>"));
    } else {
        if(firstTime)
            go("");
        u = localecode + "_" + word + ".html";
        if(setBestSourceForHelp(u.toLower())==false)
            go("");
    }
    firstTime = false;
}


void DocumentationWin::resizeEvent(QResizeEvent *e) {
    (void) e;
    this->resize(size());
}


void DocumentationWin::closeEvent(QCloseEvent *e) {
    (void) e;
    SETTINGS;
    settings.setValue(SETTINGSDOCSIZE, size());
    settings.setValue(SETTINGSDOCPOS, pos());
}


void DocumentationWin::sourceChangedSlot(const QUrl url){
    //Grab title for new source
    QString title;
    QString html = docs->toHtml();
    QRegExp rxlen("<title>(.*)</title>", Qt::CaseInsensitive);
    if (rxlen.indexIn(html)) {
        title = rxlen.cap(1).trimmed();
        if(!title.isEmpty())
            this->setWindowTitle(tr("BASIC-256") + " - " + title);
    }
    if(title.isEmpty())
        this->setWindowTitle(tr("BASIC-256 Reference"));

    setLanguageAlternatives(url.toString());
    highlight();
}


void DocumentationWin::clickFindNext() {
    findWordInHelp(true);
}


void DocumentationWin::clickFindPrev() {
    findWordInHelp(true, true);
}


void DocumentationWin::findWordInHelp(bool verbose, bool reverse) {
    //Find word in text (verbose is used to display or not MessageBox

    if(searchinput->text().length() == 0) return;
    QTextDocument::FindFlags flag;
    if (reverse) flag |= QTextDocument::FindBackward;
    if (casecheckbox->isChecked()) flag |= QTextDocument::FindCaseSensitively;
    QTextCursor cursor = docs->textCursor();
    // here we save the cursor position and the verticalScrollBar value
    QTextCursor cursorSaved = cursor;
    int scroll = docs->verticalScrollBar()->value();

    if (!docs->find(searchinput->text(), flag))
    {
        //nothing is found | jump to start/end
        setUpdatesEnabled(false);
        cursor.movePosition(reverse?QTextCursor::End:QTextCursor::Start);
        docs->setTextCursor(cursor);

        if (!docs->find(searchinput->text(), flag))
        {
            // word not found : we set the cursor back to its initial position and restore verticalScrollBar value
            docs->setTextCursor(cursorSaved);
            docs->verticalScrollBar()->setValue(scroll);
            if(verbose)
            QMessageBox::information(this, tr("Find"),
                tr("String not found."),
                QMessageBox::Ok, QMessageBox::Ok);
        }
        setUpdatesEnabled(true);
    }}


void DocumentationWin::searchSetFocus(){
    QTextCursor cursor = docs->textCursor();
    if(cursor.hasSelection()){
        searchinput->setText(cursor.selectedText());
    }
    searchinput->selectAll();
    bottom->show();
    searchinput->setFocus();
}


void DocumentationWin::highlight(){
    occurrences = 0;
    QList<QTextEdit::ExtraSelection> selections;
    if(searchinput->text().length() == 0){
        docs->setExtraSelections(selections);
        resultslabel->setText(" ");
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::white);
        searchinput->setPalette(palette);
        return;
    }
    QTextDocument::FindFlags flag;
    if (casecheckbox->isChecked()) flag |= QTextDocument::FindCaseSensitively;
    QTextCharFormat fmt;
    fmt.setBackground(QColor(Qt::yellow).lighter(125));
    int scroll = docs->verticalScrollBar()->value();
    QTextCursor cursor = docs->textCursor();
    QTextCursor cursorSaved = cursor;
    cursor.movePosition(QTextCursor::Start);
    docs->setTextCursor(cursor);
    while(docs->find(searchinput->text(), flag)) {
        occurrences++;
        cursor = docs->textCursor();
        QTextEdit::ExtraSelection sel = { cursor, fmt };
        selections.append(sel);
    }
    docs->setExtraSelections(selections);
    docs->setTextCursor(cursorSaved);
    docs->verticalScrollBar()->setValue(scroll);
    setUpdatesEnabled(true);
    if(occurrences>0){
        resultslabel->setText(tr("Occurrence(s): ") + QString::number(occurrences));
        QPalette palette;
        palette.setColor(QPalette::Base,Qt::white);
        searchinput->setPalette(palette);
    }else{
        resultslabel->setText(" ");
        QPalette palette;
        palette.setColor(QPalette::Base,QColor(Qt::red).lighter(175));
        searchinput->setPalette(palette);
    }
}


void DocumentationWin::searchTextChanged(){
    if(searchinput->text().length() == 0){
        highlight();
    }else{
        QTextCursor cursor = docs->textCursor();
        int s = cursor.selectionStart();
        int e = cursor.selectionEnd();
        cursor.setPosition(s<=e?s:e);
        docs->setTextCursor(cursor);
        highlight();
        findWordInHelp(false);
    }
}


void DocumentationWin::keyPressEvent(QKeyEvent *e) {
    if(e->key()==Qt::Key_Enter || e->key()==Qt::Key_Return){
        e->accept();
        findWordInHelp(true);
    }else if(e->text()!="" && (e->modifiers()==Qt::NoModifier || e->modifiers()==Qt::ShiftModifier)){
        if(e->text()[0].isPrint()){
            e->accept();
            searchinput->setText( e->text() );
            bottom->show();
            searchinput->setFocus();
        }
    }else{
        e->ignore();
    }
}


void DocumentationWin::slotPrintHelp() {
#ifdef ANDROID
    QMessageBox::warning(this, QObject::tr("Print"),
        QObject::tr("Printing is not supported in this platform at this time."));
#else
    QTextDocument *document = docs->document();
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(QObject::tr("Print help page"));

    if (dialog->exec() == QDialog::Accepted) {
        if ((printer.printerState() != QPrinter::Error) && (printer.printerState() != QPrinter::Aborted)) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            document->print(&printer);
            QApplication::restoreOverrideCursor();
        } else {
            QMessageBox::warning(this, QObject::tr("Print"),
                QObject::tr("Unable to carry out printing.\nPlease check your printer settings."));
        }
    }
    delete dialog;
#endif
}


void DocumentationWin::hijackAnchorClicked(QUrl u){
    if(!u.isValid())
        return;
    if(u.isRelative()){
        //if file does not exist at all
        if(setBestSourceForHelp(u.toString())==false){
            QMessageBox::critical(this, tr("Error"),
                tr("<p>This appears to be a broken link. Maybe local help files are not available.<br>You can try the online documentation at <a href='http://doc.basic256.org'>http://doc.basic256.org</a>.</p>"),
                QMessageBox::Ok, QMessageBox::Ok);
        }
     }else{
        //open external links with browser
        QDesktopServices::openUrl(QUrl(u));
    }
}


bool DocumentationWin::setBestSourceForHelp(QString s){
    QString initial = s;
    QString startWith = "";
    bool badFormat = true;

    if(s.size()>2)
        if(s[2]=='_')
            badFormat = false;

    if(!badFormat){
        startWith = s.left(2);
        if(localecode!=defaultlocale){
            //try the native language page
            s[0]=localecode[0];
            s[1]=localecode[1];
            if(helpFileExists(s)==true){
                docs->setSource(s);
                return true;
            }
        }
        //Change to defaultlocale
        s[0]=defaultlocale[0];
        s[1]=defaultlocale[1];
        if(helpFileExists(s)==true){
            docs->setSource(s);
            if(localecode!=defaultlocale){
                //Message when fallback to default page
                QTextCursor cursor = docs->textCursor();
                cursor.movePosition(QTextCursor::Start);
                docs->setTextCursor(cursor);
                docs->insertHtml("<p style=\"background-color:#ff8;\">The documentation page for your language does not exist. This is the corresponding page in English.</p><br>");
            }
            return true;
        }
    }
    //if target is not in format 'xx_' or not found in locale or in default language, try to set initial source
    if(startWith!=localecode && startWith!=defaultlocale){
        if(helpFileExists(initial)==true){
            docs->setSource(initial);
            return true;
        }
    }
    return false;
}


bool DocumentationWin::helpFileExists(QString check) {
    return docs->loadResource(QTextDocument::HtmlResource, check).isValid();
}


void DocumentationWin::clickCloseFind(){
    bottom->hide();
}


void DocumentationWin::setLanguageAlternatives(QString s) {
    QStringList filesList;
    QStringList filters;
    QStringList langs;
    if(s.size()>2){
        if(s[2]=='_'){
            QString mask = s;
            QString lang;
            mask[0]='?';
            mask[1]='?';
            filters << mask;
            QDir d;
            d.setFilter(QDir::Files|QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable);
            QStringList paths = docs->searchPaths();
            for (int i = 0; i < paths.size(); ++i){
                d.setPath(paths.at(i));
                if(d.exists()){
                    filesList.clear();
                    filesList = d.entryList(filters);
                    for (int j = 0; j < filesList.size(); ++j){
                        lang = filesList.at(j).left(2);
                        if(!langs.contains(lang))
                            langs.append(lang);
                    }
                }
            }
        }
    }
    comboLanguage->clear();
    langs.sort();
    comboLanguage->addItems(langs);
    int index = comboLanguage->findText(s.left(2));
    if ( index != -1 ) { // -1 for not found
       comboLanguage->setCurrentIndex(index);
    }
    viewLanguage->show();
    comboLanguage->show();
}


void DocumentationWin::userSelectLanguage(const QString s){
    QString before = docs->source().toString();
    QString after = before;
    after[0]=s[0];
    after[1]=s[1];
    if(before!=after){
        docs->setSource(after);
    }
}


void DocumentationWin::showOnlineHelpPage() {
    bool thisPage = false;
    QString fileName = docs->source().toString();
    if(fileName.size()>2){
        if(fileName[2]=='_'){
            fileName[2]=':';
            fileName.remove(".html");
            thisPage=true;
        }
    }
    if(thisPage){
        QDesktopServices::openUrl(QUrl("http://doc.basic256.org/doku.php?id="+fileName));
    }else{
       QDesktopServices::openUrl(QUrl("http://doc.basic256.org/"));
    }
}

