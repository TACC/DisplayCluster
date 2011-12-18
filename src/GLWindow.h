#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include "Factory.hpp"
#include "Texture.h"
#include "DynamicTexture.h"
#include "Movie.h"
#include "PixelStream.h"
#include <QGLWidget>

class GLWindow : public QGLWidget
{

    public:

        GLWindow(int tileIndex);
        GLWindow(int tileIndex, QRect windowRect, QGLWidget * shareWidget = 0);
        ~GLWindow();

        Factory<Texture> & getTextureFactory();
        Factory<DynamicTexture> & getDynamicTextureFactory();
        Factory<Movie> & getMovieFactory();
        Factory<PixelStream> & getPixelStreamFactory();

        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void setOrthographicView();
        void setPerspectiveView();

        bool isScreenRectangleVisible(double x, double y, double w, double h);

        static bool isRectangleVisible(double x, double y, double w, double h);
        static void drawRectangle(double x, double y, double w, double h);

    private:

        int tileIndex_;

        bool viewInitialized_;

        double left_;
        double right_;
        double bottom_;
        double top_;

        Factory<Texture> textureFactory_;
        Factory<DynamicTexture> dynamicTextureFactory_;
        Factory<Movie> movieFactory_;
        Factory<PixelStream> pixelStreamFactory_;
};

#endif
