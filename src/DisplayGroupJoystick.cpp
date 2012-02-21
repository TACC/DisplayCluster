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

#include "DisplayGroupJoystick.h"
#include "DisplayGroupManager.h"
#include "ContentWindowInterface.h"
#include "ContentWindowManager.h"
#include "Marker.h"

DisplayGroupJoystick::DisplayGroupJoystick(boost::shared_ptr<DisplayGroupManager> displayGroupManager) : DisplayGroupInterface(displayGroupManager)
{
    marker_ = displayGroupManager->getNewMarker();

    // add ContentWindowInterfaces for existing ContentWindowManagers
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        boost::shared_ptr<ContentWindowInterface> cwi(new ContentWindowInterface(contentWindowManagers_[i]));
        contentWindowInterfaces_.push_back(cwi);
    }
}

void DisplayGroupJoystick::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        boost::shared_ptr<ContentWindowInterface> cwi(new ContentWindowInterface(contentWindowManager));
        contentWindowInterfaces_.push_back(cwi);
    }

}

void DisplayGroupJoystick::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        for(unsigned int i=0; i<contentWindowInterfaces_.size(); i++)
        {
            if(contentWindowInterfaces_[i]->getContentWindowManager() == contentWindowManager)
            {
                contentWindowInterfaces_.erase(contentWindowInterfaces_.begin() + i);
                return;
            }
        }
    }
}

void DisplayGroupJoystick::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        for(unsigned int i=0; i<contentWindowInterfaces_.size(); i++)
        {
            if(contentWindowInterfaces_[i]->getContentWindowManager() == contentWindowManager)
            {
                boost::shared_ptr<ContentWindowInterface> cwi = contentWindowInterfaces_[i];
                contentWindowInterfaces_.erase(contentWindowInterfaces_.begin() + i);
                contentWindowInterfaces_.push_back(cwi);
                return;
            }
        }
    }
}

boost::shared_ptr<Marker> DisplayGroupJoystick::getMarker()
{
    return marker_;
}

std::vector<boost::shared_ptr<ContentWindowInterface> > DisplayGroupJoystick::getContentWindowInterfaces()
{
    return contentWindowInterfaces_;
}

boost::shared_ptr<ContentWindowInterface> DisplayGroupJoystick::getContentWindowInterfaceUnderMarker()
{
    // find the last item in our vector underneath the marker
    float markerX, markerY;
    marker_->getPosition(markerX, markerY);

    // need signed int here
    for(int i=(int)contentWindowInterfaces_.size()-1; i>=0; i--)
    {
        // get rectangle
        double x,y,w,h;
        contentWindowInterfaces_[i]->getCoordinates(x,y,w,h);

        if(QRectF(x,y,w,h).contains(markerX, markerY) == true)
        {
            return contentWindowInterfaces_[i];
        }
    }

    return boost::shared_ptr<ContentWindowInterface>();
}
