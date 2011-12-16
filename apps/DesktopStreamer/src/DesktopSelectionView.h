#ifndef DESKTOP_SELECTION_VIEW_H
#define DESKTOP_SELECTION_VIEW_H

#include <QtGui>

class DesktopSelectionRectangle;

class DesktopSelectionView : public QGraphicsView {

    public:

        DesktopSelectionView();

        DesktopSelectionRectangle * getDesktopSelectionRectangle();

    protected:

        void resizeEvent(QResizeEvent * event);

    private:

        DesktopSelectionRectangle * desktopSelectionRectangle_;
};

#endif
