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

#include "PixelStreamSource.h"

PixelStreamSource::PixelStreamSource(std::string uri)
{
    // defaults
    imageDataCount_ = 0;
    getImageDataCount_ = 0;

    dimensionsCount_ = 0;
    getDimensionsCount_ = 0;

    // assign values
    uri_ = uri;
}

QByteArray PixelStreamSource::getImageData(bool & updated)
{
    QMutexLocker locker(&imageDataMutex_);

    // whether or not this is an updated image since the last call to getImageData()
    if(imageDataCount_ > getImageDataCount_)
    {
        updated = true;
    }
    else
    {
        updated = false;
    }

    getImageDataCount_ = imageDataCount_;

    return imageData_;
}

void PixelStreamSource::setImageData(QByteArray imageData)
{
    QMutexLocker locker(&imageDataMutex_);

    // only take the update if the image data has changed
    if(imageData_ != imageData)
    {
        imageData_ = imageData;
        imageDataCount_++;
    }
}

void PixelStreamSource::getDimensions(int &width, int &height, bool & updated)
{
    QMutexLocker locker(&dimensionsMutex_);

    // whether or not these are updated dimensions since the last call to getDimensions()
    if(dimensionsCount_ > getDimensionsCount_)
    {
        updated = true;
    }
    else
    {
        updated = false;
    }

    getDimensionsCount_ = dimensionsCount_;

    width = width_;
    height = height_;
}

void PixelStreamSource::setDimensions(int width, int height)
{
    QMutexLocker locker(&dimensionsMutex_);

    // only take the update if the dimensions have changed
    if(width != width_ || height != height_)
    {
        width_ = width;
        height_ = height;

        dimensionsCount_++;
    }
}

Factory<PixelStreamSource> g_pixelStreamSourceFactory;
