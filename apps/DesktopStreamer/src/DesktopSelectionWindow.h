#ifndef DESKTOP_SELECTION_WINDOW_H
#define DESKTOP_SELECTION_WINDOW_H

#include "DesktopSelectionView.h"
#include <QtGui>

class DesktopSelectionWindow : public QMainWindow {

    public:

        DesktopSelectionWindow();

        DesktopSelectionView * getDesktopSelectionView();

    protected:

        void hideEvent(QHideEvent * event);

    private:

        DesktopSelectionView desktopSelectionView_;
};

#endif
