#ifndef WEBKITPIXELSTREAMER_H
#define WEBKITPIXELSTREAMER_H

#include "LocalPixelStreamer.h"
#include <QString>
#include <QImage>

class QWebView;
class QTimer;
class QRect;

class WebkitPixelStreamer : public LocalPixelStreamer
{
    Q_OBJECT

public:
    WebkitPixelStreamer(DisplayGroupManager* displayGroupManager, QString uri);
    ~WebkitPixelStreamer();

    void setUrl(QString url);

public slots:
    virtual void updateInteractionState(InteractionState interactionState);

    void update();
//    void updateRegion(const QRect & dirtyRect);

private:

    QWebView* webView_;
    QTimer* timer_;
    int frameIndex_;

    QImage image_;

    PixelStreamSegmentParameters makeSegmentHeader();
};

#endif // WEBKITPIXELSTREAMER_H
