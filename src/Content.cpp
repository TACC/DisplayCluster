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
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "SVGContent.h"
#include "MovieContent.h"
#include "main.h"
#include "GLWindow.h"
#include "log.h"
#include <QGLWidget>

Content::Content(std::string uri)
{
    uri_ = uri;
    width_ = 0;
    height_ = 0;
}

std::string Content::getURI()
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
    // get parameters from window
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

boost::shared_ptr<Content> Content::getContent(std::string uri)
{
    // make sure file exists; otherwise use error image
    if(QFile::exists(uri.c_str()) != true)
    {
        put_flog(LOG_ERROR, "could not find file %s", uri.c_str());

        std::string errorImageFilename = std::string(g_displayClusterDir) + std::string("/data/") + std::string(ERROR_IMAGE_FILENAME);
        boost::shared_ptr<Content> c(new TextureContent(errorImageFilename));

        return c;
    }

    // convert to lower case for case-insensitivity in determining file type
    QString fileTypeString = QString::fromStdString(uri).toLower();

    // see if this is an image
    QImageReader imageReader(uri.c_str());

    // see if this is an SVG image (must do this first, since SVG can also be read as an image directly)
    if(fileTypeString.endsWith(".svg"))
    {
        boost::shared_ptr<Content> c(new SVGContent(uri));

        return c;
    }
    else if(imageReader.canRead() == true)
    {
        // get its size
        QSize size = imageReader.size();

        // small images will use Texture; larger images will use DynamicTexture
        boost::shared_ptr<Content> c;

        if(size.width() <= 4096 && size.height() <= 4096)
        {
            boost::shared_ptr<Content> temp(new TextureContent(uri));
            c = temp;
        }
        else
        {
            boost::shared_ptr<Content> temp(new DynamicTextureContent(uri));
            c = temp;
        }

        // set the size if valid
        if(size.isValid() == true)
        {
            c->setDimensions(size.width(), size.height());
        }

        return c;
    }
    // see if this is a movie
    // todo: need a better way to determine file type
    else if(fileTypeString.endsWith(".mov") || fileTypeString.endsWith(".avi") || fileTypeString.endsWith(".mp4") || fileTypeString.endsWith(".mkv") || fileTypeString.endsWith(".mpg") || fileTypeString.endsWith(".flv") || fileTypeString.endsWith(".wmv"))
    {
        boost::shared_ptr<Content> c(new MovieContent(uri));

        return c;
    }
    // see if this is an image pyramid
    else if(fileTypeString.endsWith(".pyr"))
    {
        boost::shared_ptr<Content> c(new DynamicTextureContent(uri));

        return c;
    }

    // otherwise, return NULL
    return boost::shared_ptr<Content>();
}
