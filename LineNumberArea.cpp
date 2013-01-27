

// modified fron the example at http://qt-project.org/doc/qt-4.8/widgets-codeeditor-codeeditor-cpp.html

#include <QWidget>
#include <QPaintEvent>

#include "BasicEdit.h"
#include "LineNumberArea.h"


LineNumberArea::LineNumberArea(BasicEdit *editor) {
	be = editor;
}

QSize LineNumberArea::sizeHint() {
	return QSize(be->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
	be->lineNumberAreaPaintEvent(event);
}
