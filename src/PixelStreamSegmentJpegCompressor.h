#ifndef PIXELSTREAMSEGMENTJPEGCOMPRESSOR_H
#define PIXELSTREAMSEGMENTJPEGCOMPRESSOR_H

#include <QImage>
#include "PixelStreamSegment.h"

void computeSegmentJpeg(const QImage image, PixelStreamSegment & segment);

#endif // PIXELSTREAMSEGMENTJPEGCOMPRESSOR_H
