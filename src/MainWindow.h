#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#define SHARE_DESKTOP_UPDATE_DELAY 25

#include "GLWindow.h"
#include <QtGui>
#include <QGLWidget>
#include <boost/shared_ptr.hpp>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        boost::shared_ptr<GLWindow> getGLWindow();

    public slots:

        void openContent();
        void refreshContentsList();
        void shareDesktop(bool set);
        void shareDesktopUpdate();

    private:

        boost::shared_ptr<GLWindow> glWindow_;

        // widget listing contents in the left dock
        QListWidget * contentsListWidget_;

        QTimer shareDesktopUpdateTimer_;
        int shareDesktopWidth_;
        int shareDesktopHeight_;
};

#endif
