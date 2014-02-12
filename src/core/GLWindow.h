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
#include "PDF.h"
#include "SVG.h"
#include "Movie.h"
#include "PixelStream.h"
#include "FpsCounter.h"
#include <QGLWidget>

class WallConfiguration;

class GLWindow : public QGLWidget
{
public:
    GLWindow(int tileIndex);
    GLWindow(int tileIndex, QRect windowRect, QGLWidget * shareWidget = 0);
    ~GLWindow();

    /** Get the unique tile index identifier. */
    int getTileIndex() const;

    Factory<Texture> & getTextureFactory();
    Factory<DynamicTexture> & getDynamicTextureFactory();
    Factory<PDF> &getPDFFactory();
    Factory<SVG> & getSVGFactory();
    Factory<Movie> & getMovieFactory();
    Factory<PixelStream> & getPixelStreamFactory();

    void insertPurgeTextureId(GLuint textureId);
    void purgeTextures();

    /** Must be called before destroying this object to clear all Contents and textures. */
    void finalize();

    /**
     * Is the given region visible in this window.
     * @param rect The region in normalized global screen space, i.e. top-left
     *        of tiled display is (0,0) and bottom-right is (1,1)
     * @return true if (partially) visible, false otherwise
     */
    bool isRegionVisible(const QRectF& rect) const;

    /** Used by PDF and SVG renderers */
    QRectF getProjectedPixelRect(const bool clampToWindowArea);

    /** Draw an un-textured rectangle.
     * @param x,y postion
     * @param w,h dimensions
     */
    static void drawRectangle(double x, double y, double w, double h);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    const WallConfiguration* configuration_;

    int tileIndex_;

    // Postion and dimensions of the GLWindow in normalized Wall coordinates
    double left_;
    double right_;
    double bottom_;
    double top_;

    Factory<Texture> textureFactory_;
    Factory<DynamicTexture> dynamicTextureFactory_;
    Factory<PDF> pdfFactory_;
    Factory<SVG> svgFactory_;
    Factory<Movie> movieFactory_;
    Factory<PixelStream> pixelStreamFactory_;

    // mutex and vector of texture id's to purge
    // this allows other threads to trigger deletion of a texture during the main OpenGL thread execution
    QMutex purgeTexturesMutex_;
    std::vector<GLuint> purgeTextureIds_;

    FpsCounter fpsCounter;

    void renderBackgroundContent();
    void renderContentWindows();
    void renderMarkers();

    void setOrthographicView();
#if ENABLE_SKELETON_SUPPORT
    bool setPerspectiveView(double x=0., double y=0., double w=1., double h=1.);
#endif

    void renderTestPattern();
    void drawFps();
};

#endif
