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

#include "ImageJpegCompressor.h"

#include "ImageWrapper.h"

#include "log.h"
#include <stdlib.h>

namespace dc
{

ImageJpegCompressor::ImageJpegCompressor()
    : tjHandle_(tjInitCompress())
{
}

ImageJpegCompressor::~ImageJpegCompressor()
{
    tjDestroy(tjHandle_);
}

int getTurboJpegImageFormat(const PixelFormat pixelFormat)
{
    switch(pixelFormat)
    {
        case RGB:
            return TJPF_RGB;
        case RGBA:
            return TJPF_RGBX;
        case ARGB:
            return TJPF_XRGB;
        case BGR:
            return TJPF_BGR;
        case BGRA:
            return TJPF_BGRX;
        case ABGR:
            return TJPF_XBGR;
        default:
            put_flog(LOG_ERROR, "unknown pixel format");
            return TJPF_RGB;
    }
}

QByteArray ImageJpegCompressor::computeJpeg(const ImageWrapper& sourceImage, const QRect& imageRegion)
{
    // tjCompress API is incorrect and takes a non-const input buffer, even though it does not modify it.
    // We can "safely" cast it to non-const pointer to comply to the incorrect API.
    unsigned char* tjSrcBuffer = (unsigned char*) sourceImage.data;
    tjSrcBuffer += imageRegion.y() * sourceImage.width * sourceImage.getBytesPerPixel();
    tjSrcBuffer += imageRegion.x() * sourceImage.getBytesPerPixel();

    int tjWidth = imageRegion.width();
    int tjPitch = sourceImage.width * sourceImage.getBytesPerPixel(); // assume imageBuffer isn't padded
    int tjHeight = imageRegion.height();
    int tjPixelFormat = getTurboJpegImageFormat(sourceImage.pixelFormat);
    unsigned char * tjJpegBuf = 0;
    unsigned long tjJpegSize = 0;
    int tjJpegSubsamp = TJSAMP_444;
    int tjJpegQual = sourceImage.compressionQuality;
    int tjFlags = 0; // was TJFLAG_BOTTOMUP

    int success = tjCompress2(tjHandle_, tjSrcBuffer, tjWidth, tjPitch, tjHeight, tjPixelFormat, &tjJpegBuf, &tjJpegSize, tjJpegSubsamp, tjJpegQual, tjFlags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
        return QByteArray();
    }

    // move the JPEG buffer to a byte array
    QByteArray jpegData;
    jpegData.append((char *)tjJpegBuf, tjJpegSize);

    // free the libjpeg-turbo allocated memory
    free(tjJpegBuf);

    return jpegData;
}

}

