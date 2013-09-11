#include "LocalPixelStreamerManager.h"

#include "LocalPixelStreamer.h"
#include "WebkitPixelStreamer.h"
#include "DockPixelStreamer.h"

#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"

LocalPixelStreamerManager::LocalPixelStreamerManager(DisplayGroupManager* displayGroupManager)
    : displayGroupManager_(displayGroupManager)
{
    connect(displayGroupManager, SIGNAL(pixelStreamViewClosed(QString)), this, SLOT(removePixelStreamer(QString)));
    connect(displayGroupManager, SIGNAL(pixelStreamViewAdded(QString, boost::shared_ptr<ContentWindowManager>)), this, SLOT(bindPixelStreamerInteraction(QString, boost::shared_ptr<ContentWindowManager>)));
}

bool LocalPixelStreamerManager::createWebBrowser(QString uri, QString url)
{
    // see if we need to create the object
    if(map_.count(uri) == 0)
    {
        boost::shared_ptr<WebkitPixelStreamer> lpc(new WebkitPixelStreamer(displayGroupManager_, uri));

        lpc->setUrl(url);

        map_[uri] = lpc;

        return true;
    }
    else
    {
        return false;
    }
}

DockPixelStreamer* LocalPixelStreamerManager::getDockInstance()
{
    const QString uri = DockPixelStreamer::getUniqueURI();

    // see if we need to create the object
    if(map_.count(uri) == 0)
    {
        boost::shared_ptr<DockPixelStreamer> dock(new DockPixelStreamer(displayGroupManager_));

        map_[uri] = dock;

        return dock.get();
    }
    else
    {
        DockPixelStreamer* dock = dynamic_cast<DockPixelStreamer*>(map_[uri].get());
        return dock;
    }
}

bool LocalPixelStreamerManager::isDockOpen()
{
    const QString uri = DockPixelStreamer::getUniqueURI();

    return map_.count(uri);
}

void LocalPixelStreamerManager::openDockAt(QPointF pos)
{
    if (isDockOpen())
    {
        boost::shared_ptr<ContentWindowManager> cwm = displayGroupManager_->getContentWindowManager(DockPixelStreamer::getUniqueURI());
        if (cwm)
        {
            setWindowManagerPosition(cwm, pos);
        }
    }
    else
    {
        DockPixelStreamer* dock = getDockInstance();
        dock->setOpeningPos( pos );
        dock->open();
    }
}

void LocalPixelStreamerManager::clear()
{
    map_.clear();
}

void LocalPixelStreamerManager::removePixelStreamer(QString uri)
{
    if(map_.count(uri))
    {
        map_.erase(uri);
    }
}

void LocalPixelStreamerManager::bindPixelStreamerInteraction(QString uri, boost::shared_ptr<ContentWindowManager> cwm)
{
    if(map_.count(uri))
    {
        cwm->bindInteraction(map_[uri].get(), SLOT(updateInteractionState(InteractionState)));

        // Special case: the dock needs to be positioned and selected after the pixelStream is created
        if (uri == DockPixelStreamer::getUniqueURI())
        {
            DockPixelStreamer* dock = dynamic_cast<DockPixelStreamer*>(map_[uri].get());

            setWindowManagerPosition(cwm, dock->getOpeningPos());
        }
    }
}

void LocalPixelStreamerManager::setWindowManagerPosition(boost::shared_ptr<ContentWindowManager> cwm, QPointF pos)
{
    double w,h;
    cwm->getSize(w,h);
    cwm->setPosition(pos.x()-0.5*w, pos.y()-0.5*h);
    cwm->setWindowState(ContentWindowInterface::SELECTED);
}
