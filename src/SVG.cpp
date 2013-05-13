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

#include "SVG.h"
#include "main.h"
#include "log.h"

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

SVG::SVG(std::string uri)
{
    // defaults
    imageWidth_ = 0;
    imageHeight_ = 0;

    // assign values
    uri_ = uri;

    // open file corresponding to URI
    QFile file(uri.c_str());

    if(file.open(QIODevice::ReadOnly) != true)
    {
        put_flog(LOG_WARN, "could not open file %s", uri.c_str());
        return;
    }

    if(setImageData(file.readAll()) != true)
    {
        return;
    }
}

SVG::~SVG()
{
    // no need to delete textures, that's handled in FBO destructor
}

void SVG::getDimensions(int &width, int &height)
{
    width = imageWidth_;
    height = imageHeight_;
}

void SVG::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameCount();

    // get on-screen and full rectangle corresponding to the window
    QRectF screenRect = getProjectedPixelRect(true);
    QRectF fullRect = getProjectedPixelRect(false); // corresponds to original [tX, tY, tW, tH]

    // if we're not visible or we don't have a valid SVG, we're done...
    if(screenRect.isEmpty() == true || svgRenderer_.isValid() != true)
    {
        // clear existing FBO for this OpenGL window
        fbos_.erase(g_mainWindow->getActiveGLWindow());

        return;
    }

    // generate texture corresponding to the visible part of these texture coordinates
    generateTexture(screenRect, fullRect, tX, tY, tW, tH);

    // we'll now render the entire generated texture
    tX = tY = 0.;
    tW = tH = 1.;

    // figure out what visible region is for screenRect, a subregion of [0, 0, 1, 1]
    double xp = (screenRect.x() - fullRect.x()) / fullRect.width();
    double yp = (screenRect.y() - fullRect.y()) / fullRect.height();
    double wp = screenRect.width() / fullRect.width();
    double hp = screenRect.height() / fullRect.height();

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to edge (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBegin(GL_QUADS);

    // note we need to flip the y coordinate since the textures are loaded upside down
    glTexCoord2f(tX,1.-tY);
    glVertex2f(xp, yp);

    glTexCoord2f(tX+tW,1.-tY);
    glVertex2f(xp+wp,yp);

    glTexCoord2f(tX+tW,1.-(tY+tH));
    glVertex2f(xp+wp,yp+hp);

    glTexCoord2f(tX,1.-(tY+tH));
    glVertex2f(xp,yp+hp);

    glEnd();

    glPopAttrib();
}

bool SVG::setImageData(QByteArray imageData)
{
    if(svgRenderer_.load(imageData) != true || svgRenderer_.isValid() == false)
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.c_str());
        return false;
    }

    // save logical coordinates
    svgExtents_ = svgRenderer_.viewBoxF();

    // save image dimensions
    imageWidth_ = svgRenderer_.defaultSize().width();
    imageHeight_ = svgRenderer_.defaultSize().height();

    // reset rendered texture information
    textureRect_ = QRectF();
    textureSize_ = QSizeF(0,0);

    return true;
}

void SVG::generateTexture(QRectF screenRect, QRectF fullRect, float tX, float tY, float tW, float tH)
{
    // figure out what visible [tX, tY, tW, tH] is for screenRect
    double tXp = tX + (screenRect.x() - fullRect.x()) / fullRect.width() * tW;
    double tYp = tY + (screenRect.y() - fullRect.y()) / fullRect.height() * tH;
    double tWp = screenRect.width() / fullRect.width() * tW;
    double tHp = screenRect.height() / fullRect.height() * tH;

    QRectF textureRect(tXp, tYp, tWp, tHp);

    if(textureRect == textureRect_ && screenRect.size() == textureSize_)
    {
        // no need to regenerate texture
        return;
    }

    // keep rendered texture information so we know when to rerender
    // todo: this works great when the SVG is only rendered once per GLWindow
    // however, it will rerender every time otherwise, for example if the zoom context is shown
    textureRect_ = textureRect;
    textureSize_ = screenRect.size();

    // generate and set view box in logical coordinates
    QRectF viewbox(svgExtents_.x() + tXp * svgExtents_.width(), svgExtents_.y() + tYp * svgExtents_.height(), tWp * svgExtents_.width(), tHp * svgExtents_.height());

    svgRenderer_.setViewBox(viewbox);

    // save OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // generate new texture
    boost::shared_ptr<QGLFramebufferObject> fbo(new QGLFramebufferObject(screenRect.width(), screenRect.height(), QGLFramebufferObject::CombinedDepthStencil));

    // keep fbos in a map so they stick around -- they're needed for the texture to be rendered
    fbos_[g_mainWindow->getActiveGLWindow()] = fbo;

    QPainter painter(fbo.get());
    svgRenderer_.render(&painter);
    painter.end();

    // restore OpenGL state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    textureId_ = fbo->texture();
}

QRectF SVG::getProjectedPixelRect(bool onScreenOnly)
{
    // get four corners in object space (recall we're in normalized 0->1 dimensions)
    double x[4][3];

    x[0][0] = 0.;
    x[0][1] = 0.;
    x[0][2] = 0.;

    x[1][0] = 1.;
    x[1][1] = 0.;
    x[1][2] = 0.;

    x[2][0] = 1.;
    x[2][1] = 1.;
    x[2][2] = 0.;

    x[3][0] = 0.;
    x[3][1] = 1.;
    x[3][2] = 0.;

    // get four corners in screen space
    GLdouble modelview[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    GLdouble projection[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble xWin[4][3];

    for(int i=0; i<4; i++)
    {
        gluProject(x[i][0], x[i][1], x[i][2], modelview, projection, viewport, &xWin[i][0], &xWin[i][1], &xWin[i][2]);

        if(onScreenOnly == true)
        {
            // clamp to on-screen portion
            if(xWin[i][0] < 0.)
                xWin[i][0] = 0.;

            if(xWin[i][0] > (double)g_mainWindow->getGLWindow()->width())
                xWin[i][0] = (double)g_mainWindow->getGLWindow()->width();

            if(xWin[i][1] < 0.)
                xWin[i][1] = 0.;

            if(xWin[i][1] > (double)g_mainWindow->getGLWindow()->height())
                xWin[i][1] = (double)g_mainWindow->getGLWindow()->height();
        }
    }

    return QRectF(QPointF(xWin[0][0], (double)g_mainWindow->getGLWindow()->height() - xWin[0][1]), QPointF(xWin[2][0], (double)g_mainWindow->getGLWindow()->height() - xWin[2][1]));
}
