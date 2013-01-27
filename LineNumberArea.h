#include "BasicEdit.h"

 class LineNumberArea : public QWidget
 {
 public:
     LineNumberArea(BasicEdit *editor);

     QSize sizeHint();
     
 protected:
     void paintEvent(QPaintEvent *event);
     
 private:
     BasicEdit *be;
 };
