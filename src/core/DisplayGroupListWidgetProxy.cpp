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

#include "DisplayGroupListWidgetProxy.h"
#include "ContentWindowManager.h"
#include "Content.h"
#include "ContentWindowListWidgetItem.h"

DisplayGroupListWidgetProxy::DisplayGroupListWidgetProxy(DisplayGroupManagerPtr displayGroupManager)
    : DisplayGroupInterface(displayGroupManager)
{
    // create actual list widget
    listWidget_ = new QListWidget();

    connect(listWidget_, SIGNAL(itemClicked(QListWidgetItem * )), this, SLOT(moveListWidgetItemToFront(QListWidgetItem *)));
}

DisplayGroupListWidgetProxy::~DisplayGroupListWidgetProxy()
{
    delete listWidget_;
}

QListWidget * DisplayGroupListWidgetProxy::getListWidget()
{
    return listWidget_;
}

void DisplayGroupListWidgetProxy::addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // for now, just clear and refresh the entire list, since this is just a read-only interface
        // later this could be modeled after DisplayGroupGraphicsViewProxy if we want to expand the interface
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::moveListWidgetItemToFront(QListWidgetItem * item)
{
    ContentWindowListWidgetItem * cwlwi = dynamic_cast<ContentWindowListWidgetItem *>(listWidget_->takeItem(listWidget_->currentRow()));

    if(cwlwi != NULL)
    {
        cwlwi->moveToFront();

        // just move the item to the top of the list, rather than refresh the entire list...
        listWidget_->insertItem(0, cwlwi);
    }
}

void DisplayGroupListWidgetProxy::refreshListWidget()
{
    // clear list
    listWidget_->clear();

    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        // add to list view
        ContentWindowListWidgetItem * newItem = new ContentWindowListWidgetItem(contentWindowManagers_[i]);
        newItem->setText(contentWindowManagers_[i]->getContent()->getURI());

        listWidget_->insertItem(0, newItem);
    }
}
