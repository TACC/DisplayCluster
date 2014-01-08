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

#include "PixelStreamBuffer.h"


PixelStreamBuffer::PixelStreamBuffer()
    : lastFrameComplete_(0)
{
}

void PixelStreamBuffer::addSource(const size_t sourceIndex)
{
    assert(!sourceBuffers_.count(sourceIndex));

    sourceBuffers_[sourceIndex] = SourceBuffer();
    sourceBuffers_[sourceIndex].segments.push(PixelStreamSegments());
}

void PixelStreamBuffer::removeSource(const size_t sourceIndex)
{
    sourceBuffers_.erase(sourceIndex);
}

size_t PixelStreamBuffer::getSourceCount() const
{
    return sourceBuffers_.size();
}

void PixelStreamBuffer::insertSegment(const dc::PixelStreamSegment& segment, const size_t sourceIndex)
{
    assert(sourceBuffers_.count(sourceIndex));

    sourceBuffers_[sourceIndex].segments.back().push_back(segment);
}

void PixelStreamBuffer::finishFrameForSource(const size_t sourceIndex)
{
    assert(sourceBuffers_.count(sourceIndex));

    sourceBuffers_[sourceIndex].frameIndex++;
    sourceBuffers_[sourceIndex].segments.push(PixelStreamSegments());
}

bool PixelStreamBuffer::hasFrameComplete() const
{
    assert(!sourceBuffers_.empty());

    // Check if all sources for Stream have reached the same index
    for(SourceBufferMap::const_iterator it = sourceBuffers_.begin(); it != sourceBuffers_.end(); it++)
    {
        if (it->second.frameIndex <= lastFrameComplete_)
            return false;
    }
    return true;
}

bool PixelStreamBuffer::isFirstFrame() const
{
    return lastFrameComplete_ == 0;
}

PixelStreamSegments PixelStreamBuffer::getFrame()
{
    PixelStreamSegments frame;
    for(SourceBufferMap::iterator it = sourceBuffers_.begin(); it != sourceBuffers_.end(); it++)
    {
        SourceBuffer& buffer = it->second;
        frame.insert(frame.end(), buffer.segments.front().begin(), buffer.segments.front().end());
        buffer.segments.pop();
    }
    ++lastFrameComplete_;
    return frame;
}

QSize PixelStreamBuffer::getFrameSize() const
{
    QSize size(0,0);

    for(SourceBufferMap::const_iterator it = sourceBuffers_.begin(); it != sourceBuffers_.end(); it++)
    {
        const SourceBuffer& buffer = it->second;
        if (!buffer.segments.empty())
        {
            const PixelStreamSegments& segments = buffer.segments.front();

            for(size_t i=0; i<segments.size(); i++)
            {
                const PixelStreamSegmentParameters& params = segments[i].parameters;
                size.setWidth(std::max(size.width(), (int)(params.width+params.x)));
                size.setHeight(std::max(size.height(), (int)(params.height+params.y)));
            }
        }
    }

    return size;
}

QSize PixelStreamBuffer::computeFrameDimensions(const PixelStreamSegments &segments)
{
    QSize size(0,0);

    for(size_t i=0; i<segments.size(); i++)
    {
        const PixelStreamSegmentParameters& params = segments[i].parameters;
        size.setWidth(std::max(size.width(), (int)(params.width+params.x)));
        size.setHeight(std::max(size.height(), (int)(params.height+params.y)));
    }

    return size;
}


