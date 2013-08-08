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
#include "main.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;

    // copy all members from displayGroupManager
    if(displayGroupManager != NULL)
    {
        contentWindowManagers_ = displayGroupManager->contentWindowManagers_;
    }

    // needed for state saving / loading signals and slots
    qRegisterMetaType<std::string>("std::string");

    // connect signals from this to slots on the DisplayGroupManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(stateSaved(std::string, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(saveState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(stateLoaded(std::string, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(loadState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroupManager to slots on this
    // use queued connections for thread-safety
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(stateSaved(std::string, DisplayGroupInterface *)), this, SLOT(saveState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(stateLoaded(std::string, DisplayGroupInterface *)), this, SLOT(loadState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);

    // destruction
    connect(displayGroupManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<DisplayGroupManager> DisplayGroupInterface::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
}

std::vector<boost::shared_ptr<ContentWindowManager> > DisplayGroupInterface::getContentWindowManagers()
{
    return contentWindowManagers_;
}

boost::shared_ptr<ContentWindowManager> DisplayGroupInterface::getContentWindowManager(std::string uri, CONTENT_TYPE contentType)
{
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        if(contentWindowManagers_[i]->getContent()->getURI() == uri && (contentType == CONTENT_TYPE_ANY || contentWindowManagers_[i]->getContent()->getType() == contentType))
        {
            return contentWindowManagers_[i];
        }
    }

    return boost::shared_ptr<ContentWindowManager>();
}

void DisplayGroupInterface::setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers)
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

void DisplayGroupInterface::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
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

void DisplayGroupInterface::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    InteractionState interactionState;
    interactionState.type = InteractionState::EVT_CLOSE;
    contentWindowManager->setInteractionState( interactionState );

    // find vector entry for content window manager
    std::vector<boost::shared_ptr<ContentWindowManager> >::iterator it;

    it = find(contentWindowManagers_.begin(), contentWindowManagers_.end(), contentWindowManager);

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

void DisplayGroupInterface::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window manager
    std::vector<boost::shared_ptr<ContentWindowManager> >::iterator it;

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

void DisplayGroupInterface::saveState(std::string filename, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // only do the actual state saving if we're the DisplayGroupManager
    // we'll let the other DisplayGroupInterfaces know about the signal anyway though
    DisplayGroupManager * dgm = dynamic_cast<DisplayGroupManager *>(this);

    if(dgm != NULL)
    {
        dgm->saveStateXMLFile(filename);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(stateSaved(filename, source));
    }
}

void DisplayGroupInterface::loadState(std::string filename, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // only do the actual state loading if we're the DisplayGroupManager
    // we'll let the other DisplayGroupInterfaces know about the signal anyway though
    DisplayGroupManager * dgm = dynamic_cast<DisplayGroupManager *>(this);

    if(dgm != NULL)
    {
        dgm->loadStateXMLFile(filename);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(stateLoaded(filename, source));
    }
}
