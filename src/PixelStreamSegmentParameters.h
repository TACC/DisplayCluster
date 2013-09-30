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

#ifndef PIXELSTREAMSEGMENTPARAMETERS_H
#define PIXELSTREAMSEGMENTPARAMETERS_H

#ifdef _WIN32
    typedef __int32 int32_t;
#else
    #include <stdint.h>
#endif

#include <boost/serialization/access.hpp>

#define FRAME_INDEX_UNDEFINED -1

struct PixelStreamSegmentParameters {

    // source identifier
    int32_t sourceIndex;

    // frame index (used for synchronization)
    int32_t frameIndex;

    // coordinates of segment (pixel coordinates)
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    // coordinates of total image
    int32_t totalWidth;
    int32_t totalHeight;

    // Number of segments in the stream
    int32_t segmentCount;

    // Sender requests the view dimensions to be adjusted
    bool requestViewAdjustment;

    // Is the image raw pixel data or compressed in jpeg format
    bool compressed;

    // Default constructor
    PixelStreamSegmentParameters()
        : sourceIndex(0)
        , frameIndex(FRAME_INDEX_UNDEFINED)
        , x(0)
        , y(0)
        , width(0)
        , height(0)
        , totalWidth(0)
        , totalHeight(0)
        , segmentCount(0)
        , requestViewAdjustment(false)
        , compressed(true)
    {
    }

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & sourceIndex;
        ar & frameIndex;
        ar & x;
        ar & y;
        ar & width;
        ar & height;
        ar & totalWidth;
        ar & totalHeight;
        ar & segmentCount;
        ar & requestViewAdjustment;
        ar & compressed;
    }
};

#endif
