/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#ifndef DCIMAGEWRAPPER_H
#define DCIMAGEWRAPPER_H

#include <cstddef>

namespace dc
{
/**
 *  The PixelFormat enum describes the organisation of the bytes in the image buffer.
 *  Formats are 8 bits per channel unless specified otherwise.
 *  @version 1.0
 */
enum PixelFormat { RGB, RGBA, ARGB, BGR, BGRA, ABGR };

/** Image compression policy */
enum CompressionPolicy {
    COMPRESSION_AUTO,  /**< Implementation specific */
    COMPRESSION_ON,    /**< Force enable */
    COMPRESSION_OFF    /**< Force disable */
};

/**
 * A simple wrapper around an image data buffer.
 *
 * It is used by the Stream library to represent images and send them to a DisplayCluster instance.
 * It also contains fields to indicate if the image should be compressed for sending (disabled by default).
 * @version 1.0
 */
struct ImageWrapper
{
    /**
     * ImageWrapper constructor
     *
     * The first pixel is the top-left corner of the image, going to the
     * bottom-right corner. Data arrays which follow the GL convention (as
     * obtained by glReadPixels()) should be reordered using swapYAxis() prior
     * to constructing the image wrapper.
     *
     * @param data The source image buffer, containing getBufferSize() bytes
     * @param width The width of the image
     * @param height The height of the image
     * @param format The format of the imageBuffer
     * @param x The global position of the image in the stream
     * @param y The global position of the image in the stream
     * @version 1.0
     */
    ImageWrapper( const void *data, const unsigned int width,
                  const unsigned int height, const PixelFormat format,
                  const unsigned int x = 0, const unsigned int y = 0 );

    /** Pointer to the image data of size getBufferSize(). @version 1.0 */
    const void* const data;

    /** @name Dimensions */
    /*@{*/
    const unsigned int width;   /**< The image width in pixels. @version 1.0 */
    const unsigned int height;  /**< The image height in pixels. @version 1.0 */
    /*@}*/

    /** The pixel format describing the arrangement of the data buffer. @version 1.0 */
    const PixelFormat pixelFormat;

    /** @name Position of the image in the stream */
    /*@{*/
    const unsigned int x;  /**< The X coordinate. @version 1.0 */
    const unsigned int y;  /**< The Y coordinate. @version 1.0 */
    /*@}*/

    /** @name Compression parameters */
    /*@{*/
    CompressionPolicy compressionPolicy;  /**< Is the image to be compressed (default: auto). @version 1.0 */
    unsigned int compressionQuality;      /**< Compression quality (0 worst, 100 best, default: 75). @version 1.0 */
    /*@}*/

    /** @return The number of bytes per pixel based on the pixelFormat. @version 1.0 */
    unsigned int getBytesPerPixel() const;

    /** @return The size of the data buffer in bytes: width * height * format.bpp. @version 1.0 */
    size_t getBufferSize() const;

    /**
     * Swap an image along the Y axis.
     *
     * Used to switch between OpenGL convention (origin in bottom-left corner) and "standard" image
     * format (origin in top-left corner).
     * @param data The image buffer to be modified, containing width*height*bpp bytes
     * @param width The width of the image
     * @param height The height of the image
     * @param bpp The number of bytes per pixel (RGB=3, ARGB=4, etc.)
     * @version 1.0
     */
    static void swapYAxis(void *data, const unsigned int width, const unsigned int height,
                          const unsigned int bpp);
};

}

#endif // DCIMAGEWRAPPER_H
