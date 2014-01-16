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

#ifndef PIXELSTREAMSEGMENTDECODER_H
#define PIXELSTREAMSEGMENTDECODER_H

#include <QFuture>

namespace dc
{
struct PixelStreamSegment;
}
using dc::PixelStreamSegment;

class ImageJpegDecompressor;

/**
 * Decode a PixelStreamSegment image data asynchronously.
 */
class PixelStreamSegmentDecoder
{
public:
    /** Construct a Decoder */
    PixelStreamSegmentDecoder();

    /** Destruct a Decoder */
    ~PixelStreamSegmentDecoder();

    /**
     * Start decoding a segment.
     *
     * This function will silently ignore the request if a decoding is already in progress.
     * @param segment The segement to decode. The segment is NOT copied internally and is modified by this
     * function. It must remain valid and should not be accessed until the decoding procedure has completed.
     * @see isRunning()
     */
    void startDecoding(PixelStreamSegment& segment);

    /** Check if the decoding thread is running. */
    bool isRunning() const;

private:
    /** The decompressor instance */
    ImageJpegDecompressor* decompressor_;

    /** Async image decoding future */
    QFuture<void> decodingFuture_;
};

#endif // PIXELSTREAMSEGMENTDECODER_H
