/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include "Factory.hpp"
#include "Texture.h"
#include "DynamicTexture.h"
#include "SVG.h"
#include "Movie.h"
#include "PixelStream.h"
#include "ParallelPixelStream.h"
#include <QWindow>
#include <QMutex>
#include <QFont>
#include <QColor>
#include <QImage>

class QOpenGLContext;

// GLWindow is a QWindow (not a QWidget) with its own explicit QOpenGLContext,
// rather than QOpenGLWidget. The tiled display wall depends on manually
// separating "render into the back buffer" (updateGL()) from "present"
// (swapBuffers()), with an MPI_Barrier across the whole cluster in between
// (see MainWindow::updateGLWindows()) so that every node flips to its next
// frame in lockstep. QOpenGLWidget composites through an internal FBO on
// Qt's own schedule and has no equivalent manual swap step.
class GLWindow : public QWindow
{

    public:

        GLWindow(int tileIndex);
        GLWindow(int tileIndex, QRect windowRect, GLWindow * shareWindow = 0);
        ~GLWindow();

        Factory<Texture> & getTextureFactory();
        Factory<DynamicTexture> & getDynamicTextureFactory();
        Factory<SVG> & getSVGFactory();
        Factory<Movie> & getMovieFactory();
        Factory<PixelStream> & getPixelStreamFactory();
        Factory<ParallelPixelStream> & getParallelPixelStreamFactory();

        void insertPurgeTextureId(GLuint textureId);
        void purgeTextures();

        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void setOrthographicView();

        bool isScreenRectangleVisible(double x, double y, double w, double h);

        static bool isRectangleVisible(double x, double y, double w, double h);
        static void drawRectangle(double x, double y, double w, double h);

        void finalize();

        // make the context current, run resizeGL()+paintGL(), but don't present
        void updateGL();

        // present the frame rendered by the most recent updateGL()
        void swapBuffers();

        // replaces QGLWidget::bindTexture(); generateMipmaps mirrors the
        // DefaultBindOption (true) vs LinearFilteringBindOption (false)
        // distinction the old call sites relied on
        GLuint bindTextureFromImage(const QImage & image, bool generateMipmaps);

        // replace QGLWidget::renderText(): queue text to be drawn with
        // QPainter after all raw GL drawing for the frame is done
        void queueText(int x, int y, const QString & text, const QFont & font, const QColor & color = Qt::white);
        void queueText(double x, double y, double z, const QString & text, const QFont & font, const QColor & color = Qt::white);

    private:

        void init(GLWindow * shareWindow);
        void drawQueuedText();

        int tileIndex_;

        QOpenGLContext * context_;
        bool initializedGL_;

        double left_;
        double right_;
        double bottom_;
        double top_;

        Factory<Texture> textureFactory_;
        Factory<DynamicTexture> dynamicTextureFactory_;
        Factory<SVG> svgFactory_;
        Factory<Movie> movieFactory_;
        Factory<PixelStream> pixelStreamFactory_;
        Factory<ParallelPixelStream> parallelPixelStreamFactory_;

        // mutex and vector of texture id's to purge
        // this allows other threads to trigger deletion of a texture during the main OpenGL thread execution
        QMutex purgeTexturesMutex_;
        std::vector<GLuint> purgeTextureIds_;

        struct QueuedText
        {
            int x;
            int y;
            QString text;
            QFont font;
            QColor color;
        };

        std::vector<QueuedText> queuedText_;

        void renderTestPattern();
};

#endif
