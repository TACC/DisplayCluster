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

#include "Content.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "globals.h"
#include "GLWindow.h"
#include "log.h"
#include <QGLWidget>

Content::Content(QString uri)
{
    uri_ = uri;
    width_ = 0;
    height_ = 0;
    blockAdvance_ = false;
}

const QString& Content::getURI() const
{
    return uri_;
}

void Content::getDimensions(int &width, int &height)
{
    width = width_;
    height = height_;
}

void Content::setDimensions(int width, int height)
{
    width_ = width;
    height_ = height;

    emit(dimensionsChanged(width_, height_));
}

void Content::render(boost::shared_ptr<ContentWindowManager> window)
{
    double x, y, w, h;
    window->getCoordinates(x, y, w, h);

    double centerX, centerY;
    window->getCenter(centerX, centerY);

    double zoom = window->getZoom();

    // calculate texture coordinates
    float tX = centerX - 0.5 / zoom;
    float tY = centerY - 0.5 / zoom;
    float tW = 1./zoom;
    float tH = 1./zoom;

    // transform to a normalize coordinate system so the content can be rendered at (x,y,w,h) = (0,0,1,1)
    glPushMatrix();

    glTranslatef(x, y, 0.);
    glScalef(w, h, 1.);

    // render the factory object
    renderFactoryObject(tX, tY, tW, tH);

    // render the context view
    if(g_displayGroupManager->getOptions()->getShowZoomContext() == true && zoom > 1.)
    {
        float sizeFactor = 0.25;
        float padding = 0.02;
        float deltaZ = 0.001;
        float alpha = 0.5;
        float borderPixels = 5.;

        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
        glPushMatrix();

        // position at lower left
        glTranslatef(padding, 1. - sizeFactor - padding, deltaZ);
        glScalef(sizeFactor, sizeFactor, 1.);

        // render border rectangle
        glColor4f(1,1,1,1);

        glLineWidth(borderPixels);

        glBegin(GL_LINE_LOOP);

        glVertex2d(0., 0.);
        glVertex2d(1., 0.);
        glVertex2d(1., 1.);
        glVertex2d(0., 1.);

        glEnd();

        // render the factory object (full view)
        glTranslatef(0., 0., deltaZ);
        renderFactoryObject(0., 0., 1., 1.);

        // draw context rectangle border
        glTranslatef(0., 0., deltaZ);

        glLineWidth(borderPixels);

        glBegin(GL_LINE_LOOP);

        glVertex2d(tX, tY);
        glVertex2d(tX + tW, tY);
        glVertex2d(tX + tW, tY + tH);
        glVertex2d(tX, tY + tH);

        glEnd();

        // draw context rectangle blended
        glTranslatef(0., 0., deltaZ);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(1.,1.,1., alpha);
        GLWindow::drawRectangle(tX, tY, tW, tH);

        glPopMatrix();
        glPopAttrib();
    }

    glPopMatrix();
}
