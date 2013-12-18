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

#include "PixelStreamSegmentRenderer.h"

#include "log.h"
#include "FpsCounter.h"

#include "globals.h"
#include "MainWindow.h"
#include "GLWindow.h"

PixelStreamSegmentRenderer::PixelStreamSegmentRenderer(const QString &uri)
    : uri_(uri)
    , textureId_ (0)
    , textureWidth_(0)
    , textureHeight_(0)
    , x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
    , segmentStatistics(new FpsCounter())
    , textureNeedsUpdate_(true)
{
}

PixelStreamSegmentRenderer::~PixelStreamSegmentRenderer()
{
    // delete bound texture
    if(textureId_)
    {
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }

    delete segmentStatistics;
}

QRect PixelStreamSegmentRenderer::getRect() const
{
    return QRect(x_, y_, width_, height_);
}

void PixelStreamSegmentRenderer::updateTexture(const QImage& image)
{
    segmentStatistics->tick();

    // if the size has changed, create a new texture
    if(textureId_ && (image.width() != textureWidth_ || image.height() != textureHeight_))
    {
        // delete bound texture
        glDeleteTextures(1, &textureId_);
        textureId_ = 0;
    }

    if(!textureId_)
    {
        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
        textureWidth_ = image.width();
        textureHeight_ = image.height();
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.width(), image.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    }

    textureNeedsUpdate_ = false;
}

bool PixelStreamSegmentRenderer::textureNeedsUpdate() const
{
    return textureNeedsUpdate_;
}

void PixelStreamSegmentRenderer::setTextureNeedsUpdate()
{
    textureNeedsUpdate_ = true;
}

void PixelStreamSegmentRenderer::setParameters(const unsigned int x, const unsigned int y,
                                               const unsigned int width, const unsigned int height)
{
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

bool PixelStreamSegmentRenderer::render(bool showSegmentBorders, bool showSegmentStatistics)
{
    if(!textureId_)
    {
        return false;
    }

    // OpenGL transformation
    glPushMatrix();
    glTranslatef(x_, y_, 0.);

    // The following draw calls assume normalized coordinates, so we must pre-multiply by this segment's dimensions
    glScalef(width_, height_, 0.);

    // todo: compute actual texture bounds to render considering zoom, pan
    drawUnitTexturedQuad(0, 0, 1.f, 1.f);

    if(showSegmentBorders || showSegmentStatistics)
    {
        glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
        glLineWidth(2);

        glPushMatrix();
        glTranslatef(0.,0.,0.05);

        // render segment borders
        if(showSegmentBorders)
        {
            drawSegmentBorders();
        }

        // render segment statistics
        if(showSegmentStatistics)
        {
            drawSegmentStatistics();
        }

        glPopMatrix();
        glPopAttrib();
    }

    glPopMatrix();

    return true;
}

void PixelStreamSegmentRenderer::drawUnitTexturedQuad(float tX, float tY, float tW, float tH)
{
    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glBegin(GL_QUADS);

    glTexCoord2f(tX,tY);
    glVertex2f(0.,0.);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(1.,0.);

    glTexCoord2f(tX+tW,tY+tH);
    glVertex2f(1.,1.);

    glTexCoord2f(tX,tY+tH);
    glVertex2f(0.,1.);

    glEnd();

    glPopAttrib();
}

void PixelStreamSegmentRenderer::drawSegmentBorders()
{
    glColor4f(1.,1.,1.,1.);

    glBegin(GL_LINE_LOOP);

    glVertex2f(0.,0.);
    glVertex2f(1.,0.);
    glVertex2f(1.,1.);
    glVertex2f(0.,1.);

    glEnd();
}

void PixelStreamSegmentRenderer::drawSegmentStatistics()
{
    QFont font;
    font.setPixelSize(48);

    glDisable(GL_DEPTH_TEST);
    glColor4f(1.,0.,0.,1.);
    g_mainWindow->getActiveGLWindow()->renderText(0.1, 0.95, 0., segmentStatistics->toString(), font);
}
