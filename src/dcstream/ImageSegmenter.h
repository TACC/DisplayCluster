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

#ifndef DCIMAGESEGMENTER_H
#define DCIMAGESEGMENTER_H

#include <vector>

namespace dc
{

struct PixelStreamSegment;
struct PixelStreamSegmentParameters;
typedef std::vector<PixelStreamSegment> PixelStreamSegments;
typedef std::vector<PixelStreamSegmentParameters> SegmentParameters;

struct ImageWrapper;

/**
 * Transform images into PixelStreamSegments.
 *
 * Used by the Stream library to transmit images over a Socket.
 */
class ImageSegmenter
{
public:
    /**
     * Construct an ImageSegmenter.
     */
    ImageSegmenter();

    /**
     * Segment an image.
     * @param image The image to be segmented
     * @return A collection of segments containing a copy of the source image data.
     * @see setNominalSegmentDimensions()
     */
    PixelStreamSegments generateSegments(const ImageWrapper& image) const;

    /**
     * Set the nominal segment dimensions.
     *
     * If both dimensions are non-zero, the images will be devided as many segments
     * of the desired dimensions as possible. If the image size is not an exact multiple of
     * the segement size, the remaining segments will be of size: image.size % nominalSize.
     * @param nominalSegmentWidth The nominal width of the segments to generate (default: 0).
     * @param nominalSegmentHeight The nominal height of the segments to generate (default: 0).
     */
    void setNominalSegmentDimensions(const unsigned int nominalSegmentWidth, const unsigned int nominalSegmentHeight);

private:
    SegmentParameters generateSegmentParameters(const ImageWrapper &image) const;

    PixelStreamSegments generateJpegSegments(const ImageWrapper& image) const;
    PixelStreamSegments generateRawSegments(const ImageWrapper& image) const;

    unsigned int nominalSegmentWidth_;
    unsigned int nominalSegmentHeight_;
};

}
#endif // DCIMAGESEGMENTER_H
