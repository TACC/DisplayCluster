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

#ifndef PIXELSTREAMBUFFER_H
#define PIXELSTREAMBUFFER_H

#include "PixelStreamSegment.h"

#include <QSize>

#include <vector>
#include <queue>
#include <map>

using dc::PixelStreamSegment;
using dc::PixelStreamSegmentParameters;

typedef unsigned int FrameIndex;

typedef std::vector<PixelStreamSegment> PixelStreamSegments;

/**
 * Buffer for a single source of segements.
 */
struct SourceBuffer
{
    SourceBuffer() : frameIndex(0) {}

    /** The current index of the frame for this source */
    FrameIndex frameIndex;

    /** The collection of segments */
    std::queue<PixelStreamSegments> segments;
};

typedef std::map<size_t, SourceBuffer> SourceBufferMap;

/**
 * Buffer PixelStreamSegments from (multiple) sources
 *
 * The buffer aggregates segments coming from different sources and delivers complete frames.
 */
class PixelStreamBuffer
{
public:
    /** Construct a Buffer */
    PixelStreamBuffer();

    /**
     * Add a source of segments.
     * @param sourceIndex Unique source identifier
     */
    void addSource(const size_t sourceIndex);

    /**
     * Remove a source of segments.
     * @param sourceIndex Unique source identifier
     */
    void removeSource(const size_t sourceIndex);

    /** Get the number of sources for this Stream */
    size_t getSourceCount() const;

    /**
     * Insert a segement for the current frame and source.
     * @param segment The segment to insert
     * @param sourceIndex Unique source identifier
     */
    void insertSegment(const PixelStreamSegment& segment, const size_t sourceIndex);

    /**
     * Notify that the given source has finished sending segment for the current frame.
     * @param sourceIndex Unique source identifier
     */
    void finishFrameForSource(const size_t sourceIndex);

    /** Does the Buffer have a complete frame (from all sources) */
    bool hasFrameComplete() const;

    /** Is this the first frame */
    bool isFirstFrame() const;

    /** Get the size of the frame. Only meaningful if hasFrameComplete() is true */
    QSize getFrameSize() const;

    /**
     * Get the finished frame.
     * @return A collection of segments that form a frame
     */
    PixelStreamSegments getFrame();

    /**
     * Compute the overall dimensions of a frame
     * @param segments A collection of segments that form a frame
     * @return The dimensions of the frame
     */
    static QSize computeFrameDimensions(const PixelStreamSegments& segments);

private:
    FrameIndex lastFrameComplete_;
    SourceBufferMap sourceBuffers_;
};

#endif // PIXELSTREAMBUFFER_H
