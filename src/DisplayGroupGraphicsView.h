#ifndef DISPLAY_GROUP_GRAPHICS_VIEW_H
#define DISPLAY_GROUP_GRAPHICS_VIEW_H

#include <QtGui>

class DisplayGroupGraphicsView : public QGraphicsView {

    public:

        DisplayGroupGraphicsView();

    protected:

        void resizeEvent(QResizeEvent * event);

    private:

};

#endif
