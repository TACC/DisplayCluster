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

#include "DisplayGroupInterface.h"
#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "Content.h"
#include "globals.h"

DisplayGroupInterface::DisplayGroupInterface(DisplayGroupManagerPtr displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;

    // copy all members from displayGroupManager
    if(displayGroupManager != NULL)
    {
        contentWindowManagers_ = displayGroupManager->contentWindowManagers_;
    }

    // connect signals from this to slots on the DisplayGroupManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowManagerAdded(ContentWindowManagerPtr, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(addContentWindowManager(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerRemoved(ContentWindowManagerPtr, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(removeContentWindowManager(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerMovedToFront(ContentWindowManagerPtr, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(moveContentWindowManagerToFront(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroupManager to slots on this
    // use queued connections for thread-safety
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerAdded(ContentWindowManagerPtr, DisplayGroupInterface *)), this, SLOT(addContentWindowManager(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerRemoved(ContentWindowManagerPtr, DisplayGroupInterface *)), this, SLOT(removeContentWindowManager(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerMovedToFront(ContentWindowManagerPtr, DisplayGroupInterface *)), this, SLOT(moveContentWindowManagerToFront(ContentWindowManagerPtr, DisplayGroupInterface *)), Qt::QueuedConnection);

    // destruction
    connect(displayGroupManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

DisplayGroupManagerPtr DisplayGroupInterface::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
}

ContentWindowManagerPtrs DisplayGroupInterface::getContentWindowManagers()
{
    return contentWindowManagers_;
}

ContentWindowManagerPtr DisplayGroupInterface::getContentWindowManager(const QString& uri, CONTENT_TYPE contentType)
{
    for(size_t i=0; i<contentWindowManagers_.size(); i++)
    {
        if( contentWindowManagers_[i]->getContent()->getURI() == uri &&
           (contentType == CONTENT_TYPE_ANY || contentWindowManagers_[i]->getContent()->getType() == contentType))
        {
            return contentWindowManagers_[i];
        }
    }

    return ContentWindowManagerPtr();
}

void DisplayGroupInterface::setContentWindowManagers(ContentWindowManagerPtrs contentWindowManagers)
{
    // remove existing content window managers
    while(contentWindowManagers_.size() > 0)
    {
        removeContentWindowManager(contentWindowManagers_[0]);
    }

    // add new content window managers
    for(unsigned int i=0; i<contentWindowManagers.size(); i++)
    {
        addContentWindowManager(contentWindowManagers[i]);
    }
}

void DisplayGroupInterface::addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWindowManagers_.push_back(contentWindowManager);

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerAdded(contentWindowManager, source));
    }
}

void DisplayGroupInterface::removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    Event event;
    event.type = Event::EVT_CLOSE;
    contentWindowManager->setEvent( event );

    // find vector entry for content window manager
    ContentWindowManagerPtrs::iterator it = find(contentWindowManagers_.begin(),
                                                 contentWindowManagers_.end(), contentWindowManager);

    if(it != contentWindowManagers_.end())
    {
        // we found the entry
        // now, remove it
        contentWindowManagers_.erase(it);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerRemoved(contentWindowManager, source));
    }
}

void DisplayGroupInterface::moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window manager
    ContentWindowManagerPtrs::iterator it;

    it = find(contentWindowManagers_.begin(), contentWindowManagers_.end(), contentWindowManager);

    if(it != contentWindowManagers_.end())
    {
        // we found the entry
        // now, move it to end of the list (last item rendered is on top)
        contentWindowManagers_.erase(it);
        contentWindowManagers_.push_back(contentWindowManager);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerMovedToFront(contentWindowManager, source));
    }
}
