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

#ifndef PIXEL_STREAM_SEGMENT_RENDERER_H
#define PIXEL_STREAM_SEGMENT_RENDERER_H

#include <boost/enable_shared_from_this.hpp>
#include <QGLWidget>
#include <QtConcurrentRun>
#include <turbojpeg.h>

class PixelStreamSegmentRenderer : public boost::enable_shared_from_this<PixelStreamSegmentRenderer> {

    public:

        PixelStreamSegmentRenderer();
        ~PixelStreamSegmentRenderer();

        void getDimensions(int &width, int &height);
        bool render(float tX, float tY, float tW, float tH); // return true on successful render; false if no texture available
        bool setImageData(QByteArray imageData, bool compressed=true, int w=0, int h=0); // returns true if load image thread was spawned; false if frame was dropped
        bool getLoadImageDataThreadRunning();
        void setAutoUpdateTexture(bool set);
        void updateTextureIfAvailable();

        // for use by loadImageDataThread()
        tjhandle getHandle();
        void imageReady(QImage image);

    private:

        // pixel stream identifier
        std::string uri_;

        // texture
        GLuint textureId_;
        int textureWidth_;
        int textureHeight_;
        bool textureBound_;

        // thread for generating images from image data
        QFuture<void> loadImageDataThread_;

        // libjpeg-turbo handle for decompression
        tjhandle handle_;

        // image, mutex, and ready status
        QMutex imageReadyMutex_;
        bool imageReady_;
        QImage image_;

        // whether updateTexture() should be called automatically every render() or not
        // this can be set to false to allow for synchronization across multiple streams, for example.
        bool autoUpdateTexture_;

        void updateTexture(QImage & image);
};

extern void loadImageDataThread(boost::shared_ptr<PixelStreamSegmentRenderer> pixelStreamRenderer, const QByteArray imageData, bool compressed, int w, int h);

#endif
