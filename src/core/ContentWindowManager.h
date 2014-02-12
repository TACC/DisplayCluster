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

#ifndef CONTENT_WINDOW_MANAGER_H
#define CONTENT_WINDOW_MANAGER_H

#include "ContentWindowInterface.h"
#include "Content.h" // need pyContent for pyContentWindowManager
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

class DisplayGroupManager;
class ContentInteractionDelegate;

class ContentWindowManager : public ContentWindowInterface, public boost::enable_shared_from_this<ContentWindowManager>
{
    public:

        ContentWindowManager(); // no-argument constructor required for serialization
        ContentWindowManager(ContentPtr content);
        virtual ~ContentWindowManager();

        ContentPtr getContent();

        DisplayGroupManagerPtr getDisplayGroupManager();
        void setDisplayGroupManager(DisplayGroupManagerPtr displayGroupManager);

        ContentInteractionDelegate& getInteractionDelegate();

        // re-implemented ContentWindowInterface slots
        void moveToFront(ContentWindowInterface * source=NULL);
        void close(ContentWindowInterface * source=NULL);

        QPointF getWindowCenterPosition() const;
        void centerPositionAround(const QPointF& position, const bool constrainToWindowBorders);

        // GLWindow rendering
        void render();

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & content_;
            ar & displayGroupManager_;
            ar & contentWidth_;
            ar & contentHeight_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
            ar & centerX_;
            ar & centerY_;
            ar & zoom_;
            ar & controlState_;
            ar & windowState_;
            ar & highlightedTimestamp_;
        }

    private:
        ContentPtr content_;

        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;

        // Rank0: Delegate to handle user inputs
        ContentInteractionDelegate* interactionDelegate_;
};

// typedef needed for SIP
typedef ContentWindowManagerPtr pContentWindowManager;

class pyContentWindowManager
{
    public:

        pyContentWindowManager(pyContent content)
        {
            ContentWindowManagerPtr contentWindow(new ContentWindowManager(content.get()));
            ptr_ = contentWindow;
        }

        pyContentWindowManager(ContentWindowManagerPtr contentWindow)
        {
            ptr_ = contentWindow;
        }

        ContentWindowManagerPtr get()
        {
            return ptr_;
        }

        pyContent getPyContent()
        {
            return pyContent(get()->getContent());
        }

    private:

        ContentWindowManagerPtr ptr_;
};

#endif
