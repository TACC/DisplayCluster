#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "GLWindow.h"
#include <QtGui>
#include <QGLWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        GLWindow * getGLWindow();

    public slots:

        void openContent();

    private:

        GLWindow * glWindow_;

        // widget listing contents in the left dock
        QListWidget * contentsListWidget_;
};

#endif
