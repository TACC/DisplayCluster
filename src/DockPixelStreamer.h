#ifndef DOCKPIXELSTREAMER_H
#define DOCKPIXELSTREAMER_H

#include "LocalPixelStreamer.h"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtGui/QImage>


class PictureFlow;

// TODO make this class extend QRunnable!!
class AsyncImageLoader : public QObject
{
    Q_OBJECT

public:
    AsyncImageLoader(QSize defaultSize);

public slots:
    void loadImage( const QString& fileName, const int index );

signals:
    void imageLoaded(int index, QImage image);

private:
    QSize defaultSize_;
};



class DockPixelStreamer : public LocalPixelStreamer
{
    Q_OBJECT

public:

    DockPixelStreamer(DisplayGroupManager* displayGroupManager);
    ~DockPixelStreamer();

    static QString getUniqueURI();

    virtual void updateInteractionState(InteractionState interactionState);

    void open();

    void onItem();

    void setOpeningPos( const QPointF& pos ) { posOpening_ = pos; }
    QPointF getOpeningPos() const { return posOpening_; }

public slots:
    void update(const QImage &image);

signals:
    void renderPreview( const QString& fileName, const int index );
    void close(QString selfUri);

private:

    QThread loadThread_;

    PictureFlow* flow_;
    AsyncImageLoader* loader_;

    QDir currentDir_;
    QPointF posOpening_;
    QHash< QString, int > slideIndex_;

    int frameIndex_;

    void changeDirectory( const QString& dir );

    PixelStreamSegmentParameters makeSegmentHeader();
};

#endif // DOCKPIXELSTREAMER_H
