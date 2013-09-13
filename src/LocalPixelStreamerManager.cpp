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
    DockPixelStreamer* dock = getDockInstance();
    dock->setOpeningPos( pos );
    dock->open();
}

void LocalPixelStreamerManager::clear()
{
    map_.clear();
}

void LocalPixelStreamerManager::removePixelStreamer(QString uri)
{
    if(map_.count(uri) && uri != DockPixelStreamer::getUniqueURI())
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
