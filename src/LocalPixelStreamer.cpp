#include "LocalPixelStreamer.h"
#include "main.h"

LocalPixelStreamer::LocalPixelStreamer(DisplayGroupManager *displayGroupManager, QString uri)
    : uri_(uri)
{
    connect(this, SIGNAL(segmentUpdated(QString,PixelStreamSegment)), displayGroupManager, SLOT(processPixelStreamSegment(QString,PixelStreamSegment)));
    connect(this, SIGNAL(streamClosed(QString)), displayGroupManager, SLOT(deletePixelStream(QString)));
}

LocalPixelStreamer::~LocalPixelStreamer()
{
    emit(streamClosed(uri_));
}

