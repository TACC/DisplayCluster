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

#include "Marker.h"
#include "log.h"
#include "configuration/Configuration.h"
#include "globals.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "GLWindow.h"

#include <QGLWidget>

#define MARKER_IMAGE_FILENAME ":/img/marker.png"

GLuint Marker::textureId_ = 0;

Marker::Marker()
{
    x_ = y_ = 0.;

    if(g_mpiRank != 0 && textureId_ == 0 && g_mainWindow->getGLWindow( ))
    {
        // load marker texture
        QImage image(MARKER_IMAGE_FILENAME);

        if(image.isNull())
        {
            put_flog(LOG_ERROR, "error loading marker texture '%s'", MARKER_IMAGE_FILENAME);
            return;
        }

        textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::DefaultBindOption);
    }
}

Marker::~Marker()
{
}

void Marker::releaseTexture()
{
    g_mainWindow->getGLWindow()->deleteTexture(textureId_);
    textureId_ = 0;
}

void Marker::setPosition(float x, float y)
{
    x_ = x;
    y_ = y;
    updatedTimestamp_ = g_displayGroupManager->getTimestamp();

    emit(positionChanged());
}

void Marker::getPosition(float &x, float &y)
{
    x = x_;
    y = y_;
}

bool Marker::getActive()
{
    if((g_displayGroupManager->getTimestamp() - updatedTimestamp_).total_seconds() > MARKER_TIMEOUT_SECONDS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void Marker::render()
{
    // only render recently active markers
    if(getActive() == false)
    {
        return;
    }

    float markerWidth = MARKER_WIDTH;

    // marker height needs to be scaled by the tiled display aspect ratio
    float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();
    float markerHeight = markerWidth * tiledDisplayAspect;

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    // disable depth testing and enable blending
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // enable texturing
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glPushMatrix();
    glTranslated(x_, y_, 0.);

    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(-markerWidth,-markerHeight);

    glTexCoord2f(1,0);
    glVertex2f(markerWidth,-markerHeight);

    glTexCoord2f(1,1);
    glVertex2f(markerWidth,markerHeight);

    glTexCoord2f(0,1);
    glVertex2f(-markerWidth,markerHeight);

    glEnd();

    glPopMatrix();
    glPopAttrib();
}
