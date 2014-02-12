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
#include "globals.h"
#include "log.h"
#include "MainWindow.h"
#include "GLWindow.h"

#include <cmath>

#define MULTI_SAMPLE_ANTI_ALIASING_SAMPLES 8

SVG::SVG(QString uri)
    : uri_(uri)
    , width_(0)
    , height_(0)
{
    // open file corresponding to URI
    QFile file(uri);

    if(!file.open(QIODevice::ReadOnly))
    {
        put_flog(LOG_WARN, "could not open file %s", uri.toLocal8Bit().constData());
        return;
    }

    if(!setImageData(file.readAll()))
    {
        put_flog(LOG_WARN, "could not setImageData %s", uri.toLocal8Bit().constData());
        return;
    }
}

SVG::~SVG()
{
    // no need to delete textures, that's handled in FBO destructor
}

void SVG::getDimensions(int &width, int &height)
{
    width = width_;
    height = height_;
}

bool SVG::setImageData(QByteArray imageData)
{
    if( !svgRenderer_.load(imageData) || !svgRenderer_.isValid() )
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.toLocal8Bit().constData());
        return false;
    }

    // save logical coordinates
    svgExtents_ = svgRenderer_.viewBoxF();

    // save image dimensions
    width_ = svgRenderer_.defaultSize().width();
    height_ = svgRenderer_.defaultSize().height();

    // reset rendered texture information to force regenerating the texture
    for (std::map<int, SVGTextureData>::iterator it = textureData_.begin(); it != textureData_.end(); ++it)
    {
        it->second.region = QRectF();
    }

    return true;
}

void SVG::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameIndex();

    // get on-screen and full rectangle corresponding to the window in pixel units
    const QRectF screenRect = g_mainWindow->getGLWindow()->getProjectedPixelRect(true);
    const QRectF fullRect = g_mainWindow->getGLWindow()->getProjectedPixelRect(false); // maps to [tX, tY, tW, tH]

    // If we're not visible or we don't have a valid SVG, we're done.
    if(screenRect.isEmpty() || !svgRenderer_.isValid())
    {
        textureData_.erase(g_mainWindow->getActiveGLWindow()->getTileIndex());
        return;
    }

    // Get the texture for the current GLWindow
    SVGTextureData& textureData = textureData_[g_mainWindow->getActiveGLWindow()->getTileIndex()];

    const QRectF textureRect = computeTextureRect(screenRect, fullRect, tX, tY, tW, tH);
    const QSize textureSize(round(screenRect.width()), round(screenRect.height()));

    const bool recreateTextureFbo = !textureData.fbo || textureData.fbo->size() != textureSize;

    if( recreateTextureFbo || textureRect != textureData.region )
    {
        if ( recreateTextureFbo )
        {
            textureData.fbo.reset( new QGLFramebufferObject( textureSize ));
        }

        renderToTexture(textureRect, textureData.fbo);

        // keep rendered texture information so we know when to rerender
        // this works great when the SVG is only rendered once per GLWindow
        // however, it will rerender every time otherwise, for example if the zoom context is shown
        textureData.region = textureRect;
    }
    assert(textureData.fbo);

    // figure out what visible region is for screenRect, a subregion of [0, 0, 1, 1]
    const float xp = (screenRect.x() - fullRect.x()) / fullRect.width();
    const float yp = (screenRect.y() - fullRect.y()) / fullRect.height();
    const float wp = screenRect.width() / fullRect.width();
    const float hp = screenRect.height() / fullRect.height();

    drawTexturedQuad(xp, yp, wp, hp, textureData.fbo->texture());
}

QRectF SVG::computeTextureRect(const QRectF& screenRect, const QRectF& fullRect,
                               const float tX, const float tY, const float tW, const float tH) const
{
    // figure out what visible [tX, tY, tW, tH] is for screenRect
    const float tXp = tX + (screenRect.x() - fullRect.x()) / fullRect.width() * tW;
    const float tYp = tY + (screenRect.y() - fullRect.y()) / fullRect.height() * tH;
    const float tWp = screenRect.width() / fullRect.width() * tW;
    const float tHp = screenRect.height() / fullRect.height() * tH;

    return QRectF(tXp, tYp, tWp, tHp);
}

void SVG::renderToTexture(const QRectF& textureRect, QGLFramebufferObjectPtr targetFbo)
{
    saveGLState();

    // generate and set view box in logical coordinates
    QRectF viewbox(svgExtents_.x() + textureRect.x() * svgExtents_.width(),
                   svgExtents_.y() + textureRect.y() * svgExtents_.height(),
                   textureRect.width() * svgExtents_.width(),
                   textureRect.height() * svgExtents_.height());
    svgRenderer_.setViewBox(viewbox);

    // Multisampled FBO for anti-aliased rendering
    QGLFramebufferObjectFormat format;
    format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(MULTI_SAMPLE_ANTI_ALIASING_SAMPLES);

    QGLFramebufferObjectPtr renderFbo( new QGLFramebufferObject( targetFbo->size(), format ));

    // Render to multisampled FBO
    QPainter painter(renderFbo.get());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    svgRenderer_.render(&painter);
    painter.end();

    // Copy to texture FBO
    QGLFramebufferObject::blitFramebuffer(
            targetFbo.get(), QRect(0, 0, renderFbo->width(), renderFbo->height()),
            renderFbo.get(), QRect(0, 0, renderFbo->width(), renderFbo->height()));

    glBindTexture(GL_TEXTURE_2D, targetFbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    restoreGLState();
}

void SVG::saveGLState()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
}

void SVG::restoreGLState()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

void SVG::drawTexturedQuad(const float posX, const float posY,
                           const float width, const float height, const GLuint textureID)
{
    // Texture coordinates
    const float tX = 0.f;
    const float tY = 0.f;
    const float tW = 1.f;
    const float tH = 1.f;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBegin(GL_QUADS);

    // note we need to flip the y coordinate since the textures are loaded upside down
    glTexCoord2f(tX, 1.-tY);
    glVertex2f(posX, posY);

    glTexCoord2f(tX+tW, 1.-tY);
    glVertex2f(posX+width, posY);

    glTexCoord2f(tX+tW, 1.-(tY+tH));
    glVertex2f(posX+width, posY+height);

    glTexCoord2f(tX, 1.-(tY+tH));
    glVertex2f(posX, posY+height);

    glEnd();

    glPopAttrib();
}
