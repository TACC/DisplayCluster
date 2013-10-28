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

#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupGraphicsScene.h"
#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "ContentWindowGraphicsItem.h"

DisplayGroupGraphicsViewProxy::DisplayGroupGraphicsViewProxy(boost::shared_ptr<DisplayGroupManager> displayGroupManager) : DisplayGroupInterface(displayGroupManager)
{
    // create actual graphics view
    graphicsView_ = new DisplayGroupGraphicsView();

    // connect Options updated signal
    connect(displayGroupManager->getOptions().get(), SIGNAL(updated()), this, SLOT(optionsUpdated()));
}

DisplayGroupGraphicsViewProxy::~DisplayGroupGraphicsViewProxy()
{
    delete graphicsView_;
}

DisplayGroupGraphicsView * DisplayGroupGraphicsViewProxy::getGraphicsView()
{
    return graphicsView_;
}

void DisplayGroupGraphicsViewProxy::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        ContentWindowGraphicsItem * cwgi = new ContentWindowGraphicsItem(contentWindowManager);
        graphicsView_->scene()->addItem((QGraphicsItem *)cwgi);
    }
}

void DisplayGroupGraphicsViewProxy::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindowManager
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindowManager() == contentWindowManager)
            {
                graphicsView_->scene()->removeItem(itemsList.at(i));
            }
        }
    }

    // Qt WAR: when all items with grabbed gestures are removed, the viewport
    // also looses any registered gestures, which harms our dock to open...
    // <qt-source>/qgraphicsscene.cpp::ungrabGesture called in removeItemHelper()
    if( getContentWindowManagers().empty( ))
        graphicsView_->grabGestures();
}

void DisplayGroupGraphicsViewProxy::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindowManager
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindowManager() == contentWindowManager)
            {
                // don't call cwgi->moveToFront() here or that'll lead to infinite recursion!
                cwgi->setZToFront();
            }
        }
    }
}

void DisplayGroupGraphicsViewProxy::optionsUpdated()
{
    // mullion compensation may have been enabled or disabled, so refresh the tiled display rectangles
    ((DisplayGroupGraphicsScene *)(graphicsView_->scene()))->refreshTileRects();
}
