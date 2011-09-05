#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include "DynamicTextureFactory.h"
#include <QGLWidget>

class GLWindow : public QGLWidget
{

    public:

        GLWindow();
        ~GLWindow();

        DynamicTextureFactory & getDynamicTextureFactory();

        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void setView(int width, int height);

        static void drawRectangle(double x, double y, double w, double h);

    private:

        DynamicTextureFactory dynamicTextureFactory_;
};

#endif
