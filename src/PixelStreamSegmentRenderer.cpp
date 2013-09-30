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
#include "main.h"
#include "log.h"

PixelStreamSegmentRenderer::PixelStreamSegmentRenderer()
{
    // defaults
    textureId_ = 0;
    textureWidth_ = 0;
    textureHeight_ = 0;
    textureBound_ = false;
    imageReady_ = false;
    autoUpdateTexture_ = true;

    // initialize libjpeg-turbo handle
    handle_ = tjInitDecompress();
}

PixelStreamSegmentRenderer::~PixelStreamSegmentRenderer()
{
    // delete bound texture
    if(textureBound_ == true)
    {
        // let the OpenGL window delete the texture, so the destructor can occur in any thread...
        g_mainWindow->getGLWindow()->insertPurgeTextureId(textureId_);

        textureBound_ = false;
    }

    // destroy libjpeg-turbo handle
    tjDestroy(handle_);
}

void PixelStreamSegmentRenderer::getDimensions(int &width, int &height)
{
    width = textureWidth_;
    height = textureHeight_;
}

bool PixelStreamSegmentRenderer::render(float tX, float tY, float tW, float tH)
{
    // automatically upload a new texture if a new image is available
    if(autoUpdateTexture_ == true)
    {
        updateTextureIfAvailable();
    }

    if(textureBound_ != true)
    {
        return false;
    }

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to edge (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

    return true;
}

bool PixelStreamSegmentRenderer::setImageData(QByteArray imageData, bool compressed, int w, int h)
{
    // drop frames if we're currently processing
    if(loadImageDataThread_.isRunning() == true)
    {
        return false;
    }

    loadImageDataThread_ = QtConcurrent::run(loadImageDataThread, shared_from_this(), imageData, compressed, w, h);

    return true;
}

bool PixelStreamSegmentRenderer::getLoadImageDataThreadRunning()
{
    return loadImageDataThread_.isRunning();
}

void PixelStreamSegmentRenderer::setAutoUpdateTexture(bool set)
{
    autoUpdateTexture_ = set;
}

void PixelStreamSegmentRenderer::updateTextureIfAvailable()
{
    // upload a new texture if a new image is available
    QMutexLocker locker(&imageReadyMutex_);

    if(imageReady_ == true)
    {
        updateTexture(image_);
        imageReady_ = false;
    }
}

tjhandle PixelStreamSegmentRenderer::getHandle()
{
    return handle_;
}

void PixelStreamSegmentRenderer::imageReady(QImage image)
{
    QMutexLocker locker(&imageReadyMutex_);
    imageReady_ = true;
    image_ = image;
}

void PixelStreamSegmentRenderer::updateTexture(QImage & image)
{
    // todo: consider if the image has changed dimensions

    if(textureBound_ == false)
    {
        // want mipmaps disabled
        textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
        textureWidth_ = image.width();
        textureHeight_ = image.height();
        textureBound_ = true;
    }
    else
    {
        // update the texture. note that generally we would need to convert the image to an OpenGL supported format
        // however, we're lucky and can use GL_BGRA on the original image...
        // example conversion to GL format: QImage glImage = QGLWidget::convertToGLFormat(image);

        // if the size has changed, create a new texture
        if(image.width() != textureWidth_ || image.height() != textureHeight_)
        {
            // delete bound texture
            glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
            g_mainWindow->getGLWindow()->deleteTexture(textureId_);

            textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
            textureWidth_ = image.width();
            textureHeight_ = image.height();
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, textureId_);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, image.width(), image.height(), GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
        }
    }
}

void loadImageDataThread(boost::shared_ptr<PixelStreamSegmentRenderer> pixelStreamRenderer, const QByteArray imageData, bool compressed, int w, int h)
{
    if( !compressed )
    {
        QImage image((const uchar*)imageData.data(), w, h, QImage::Format_RGB32);
        image.detach();
        pixelStreamRenderer->imageReady(image);
        return;
    }

    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = pixelStreamRenderer->getHandle();

    // get information from header
    int width, height, jpegSubsamp;
    int success =  tjDecompressHeader2(handle, (unsigned char *)imageData.data(), (unsigned long)imageData.size(), &width, &height, &jpegSubsamp);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo header decompression failure");
        return;
    }

    // decompress image data
    int pixelFormat = TJPF_BGRX;
    int pitch = width * tjPixelSize[pixelFormat];
    int flags = TJ_FASTUPSAMPLE;

    QImage image = QImage(width, height, QImage::Format_RGB32);

    success = tjDecompress2(handle, (unsigned char *)imageData.data(), (unsigned long)imageData.size(), (unsigned char *)image.scanLine(0), width, pitch, height, pixelFormat, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image decompression failure");
        return;
    }

    pixelStreamRenderer->imageReady(image);
}
