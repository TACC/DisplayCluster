#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

// increment this whenever when serialized state information changes
#define CONTENTS_FILE_VERSION_NUMBER 1

#include "GLWindow.h"
#include <QtGui>
#include <QGLWidget>
#include <boost/shared_ptr.hpp>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        bool getConstrainAspectRatio();

        boost::shared_ptr<GLWindow> getGLWindow(int index=0);
        std::vector<boost::shared_ptr<GLWindow> > getGLWindows();

    public slots:

        void openContent();
        void openContentsDirectory();
        void clearContents();
        void saveContents();
        void loadContents();
        void computeImagePyramid();
        void constrainAspectRatio(bool set);

        void updateGLWindows();

    signals:

        void updateGLWindowsFinished();

    private:

        std::vector<boost::shared_ptr<GLWindow> > glWindows_;

        bool constrainAspectRatio_;
};

#endif
