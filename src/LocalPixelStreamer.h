#ifndef LOCALPIXELSTREAMER_H
#define LOCALPIXELSTREAMER_H

#include <QString>
#include <QObject>

#include "PixelStreamSegment.h"
#include "InteractionState.h"

class DisplayGroupManager;

class LocalPixelStreamer : public QObject {
    Q_OBJECT

public:
    LocalPixelStreamer(DisplayGroupManager* displayGroupManager, QString uri);
    virtual ~LocalPixelStreamer();

public slots:
    virtual void updateInteractionState(InteractionState interactionState) = 0;

protected:
    QString uri_;

signals:
    void segmentUpdated(QString uri, PixelStreamSegment segment);
    void streamClosed(QString uri);
};

#endif // LOCALPIXELSTREAMER_H
