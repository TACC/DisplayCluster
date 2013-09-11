#ifndef LOCALPIXELSTREAMERMANAGER_H
#define LOCALPIXELSTREAMERMANAGER_H

#include <map>
#include <boost/shared_ptr.hpp>
#include <QMutex>
#include <QObject>
#include <QPointF>

class LocalPixelStreamer;
class DisplayGroupManager;
class ContentWindowManager;
class DockPixelStreamer;

class LocalPixelStreamerManager : public QObject
{
Q_OBJECT

public:
    LocalPixelStreamerManager(DisplayGroupManager *displayGroupManager);

    bool createWebBrowser(QString uri, QString url);

    bool isDockOpen();
    void openDockAt(QPointF pos);
    DockPixelStreamer* getDockInstance();

    void clear();

public slots:

    void removePixelStreamer(QString uri);

    void bindPixelStreamerInteraction(QString uri, boost::shared_ptr<ContentWindowManager> cwm);

private:

    // all existing objects
    std::map<QString, boost::shared_ptr<LocalPixelStreamer> > map_;

    // To connect new LocalPixelStreamers
    DisplayGroupManager *displayGroupManager_;

    void setWindowManagerPosition(boost::shared_ptr<ContentWindowManager> cwm, QPointF pos);
};

#endif // LOCALPIXELSTREAMERMANAGER_H
