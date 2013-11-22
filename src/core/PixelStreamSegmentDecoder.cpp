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

#include "PixelStreamSegmentDecoder.h"

#include "PixelStreamSegment.h"
#include "log.h"

#include <QtConcurrentRun>

#include "globals.h"


PixelStreamSegmentDecoder::PixelStreamSegmentDecoder()
    : handle_(0)
{
    // initialize libjpeg-turbo handle
    handle_ = tjInitDecompress();
}

PixelStreamSegmentDecoder::~PixelStreamSegmentDecoder()
{
    // destroy libjpeg-turbo handle
    tjDestroy(handle_);
}

void decodeSegment(boost::shared_ptr<PixelStreamSegmentDecoder> segmentDecoder, PixelStreamSegment* segment)
{
    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = segmentDecoder->getTjHandle();

    // get information from header
    int width, height, jpegSubsamp;
    int success = tjDecompressHeader2(handle, (unsigned char *)segment->imageData.data(), (unsigned long)segment->imageData.size(), &width, &height, &jpegSubsamp);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo header decompression failure");
        return;
    }

    // decompress image data
    int pixelFormat = TJPF_BGRX;
    int pitch = width * tjPixelSize[pixelFormat];
    int flags = TJ_FASTUPSAMPLE;

    QByteArray decodedData;
    decodedData.resize(height*pitch);

    success = tjDecompress2(handle, (unsigned char *)segment->imageData.data(), (unsigned long)segment->imageData.size(), (unsigned char *)decodedData.data(), width, pitch, height, pixelFormat, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image decompression failure");
        return;
    }

    // Modify the inupt segment
    segment->imageData = decodedData;
    segment->parameters.compressed = false;
}

void PixelStreamSegmentDecoder::startDecoding(dc::PixelStreamSegment& segment)
{
    // drop frames if we're currently processing
    if(isRunning())
    {
        put_flog(LOG_WARN, "Decoding in process, Frame dropped. See if we need to change this...");
        return;
    }

    decodingThread_ = QtConcurrent::run(decodeSegment, shared_from_this(), &segment);
}

bool PixelStreamSegmentDecoder::isRunning() const
{
    return decodingThread_.isRunning();
}

tjhandle PixelStreamSegmentDecoder::getTjHandle() const
{
    return handle_;
}
