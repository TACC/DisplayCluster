#include "PixelStreamSegmentJpegCompressor.h"

#include <turbojpeg.h>

#include "log.h"

#define JPEG_QUALITY 75

void computeSegmentJpeg(const QImage image, PixelStreamSegment & segment)
{
    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = tjInitCompress();
    int pixelFormat = TJPF_BGRX;
    unsigned char * jpegBufPtr = NULL;
    unsigned long jpegSize = 0;
    int jpegSubsamp = TJSAMP_444;
    int jpegQual = JPEG_QUALITY;
    int flags = 0;

    // Although tjCompress2 takes a non-const (uchar*) as a source image, it actually doesn't modify it, so the casting is safe
    int success = tjCompress2(handle, (uchar*)image.scanLine(segment.parameters.y) + segment.parameters.x * image.depth()/8, segment.parameters.width, image.bytesPerLine(), segment.parameters.height, pixelFormat, &jpegBufPtr, &jpegSize, jpegSubsamp, jpegQual, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");

        return;
    }

    // move the JPEG buffer to a byte array
    segment.imageData = QByteArray((char *)jpegBufPtr, jpegSize);

    // free the libjpeg-turbo allocated memory
    free(jpegBufPtr);
    tjDestroy(handle);
}

