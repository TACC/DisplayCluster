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

#include "ContentWindowManager.h"
#include "Content.h"
#include "DisplayGroupManager.h"
#include "globals.h"
#include "ContentInteractionDelegate.h"
#include "Configuration.h"
#include "GLWindow.h"
#include "config.h"

// Specialized delegate implementations
#include "PixelStreamInteractionDelegate.h"
#include "ZoomInteractionDelegate.h"
#if ENABLE_PDF_SUPPORT
#  include "PDFInteractionDelegate.h"
#endif


ContentWindowManager::ContentWindowManager(boost::shared_ptr<Content> content)
{
    // ContentWindowManagers must always belong to the main thread!
    moveToThread(QApplication::instance()->thread());

    // content dimensions
    content->getDimensions(contentWidth_, contentHeight_);

    adjustSize( SIZE_1TO1 );

    // default to centered
    centerX_ = 0.5;
    centerY_ = 0.5;

    // default to no zoom
    zoom_ = 1.;

    controlState_ = STATE_LOOP;

    // set content object
    content_ = content;

    // receive updates to content dimensions
    connect(content.get(), SIGNAL(dimensionsChanged(int, int)), this, SLOT(setContentDimensions(int, int)));

    if (g_mpiRank == 0)
    {
        if (getContent()->getType() == CONTENT_TYPE_PIXEL_STREAM)
        {
            interactionDelegate_ = new PixelStreamInteractionDelegate(this);
        }
#if ENABLE_PDF_SUPPORT
        else if (getContent()->getType() == CONTENT_TYPE_PDF)
        {
            interactionDelegate_ = new PDFInteractionDelegate(this);
        }
#endif
        else
        {
            interactionDelegate_ = new ZoomInteractionDelegate(this);
        }
    }
}

boost::shared_ptr<Content> ContentWindowManager::getContent()
{
    return content_;
}

boost::shared_ptr<DisplayGroupManager> ContentWindowManager::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
}

void ContentWindowManager::setDisplayGroupManager(boost::shared_ptr<DisplayGroupManager> displayGroupManager)
{
    // disconnect any existing signals to previous DisplayGroupManager
    boost::shared_ptr<DisplayGroupManager> oldDisplayGroupManager = getDisplayGroupManager();

    if(oldDisplayGroupManager != NULL)
    {
        disconnect(this, 0, oldDisplayGroupManager.get(), 0);
    }

    displayGroupManager_ = displayGroupManager;

    // make connections to new DisplayGroupManager
    // don't use queued connections; we want these to execute immediately and we're in the same thread
    if(displayGroupManager != NULL)
    {
        connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));
        // TODO check: do we really need to serialize and MPI send the InteractionState? Isn't it used on Rank0 only?
        connect(this, SIGNAL(interactionStateChanged(InteractionState, ContentWindowInterface *)), displayGroupManager.get(), SLOT(sendDisplayGroup()));

        // we don't call sendDisplayGroup() on movedToFront() or destroyed() since it happens already
    }
}

ContentInteractionDelegate* ContentWindowManager::getInteractionDelegate()
{
    return interactionDelegate_;
}

void ContentWindowManager::moveToFront(ContentWindowInterface * source)
{
    ContentWindowInterface::moveToFront(source);

    if(source != this)
    {
        getDisplayGroupManager()->moveContentWindowManagerToFront(shared_from_this());
    }
}

void ContentWindowManager::close(ContentWindowInterface * source)
{
    ContentWindowInterface::close(source);

    if(source != this)
    {
        getDisplayGroupManager()->removeContentWindowManager(shared_from_this());
    }
}

void ContentWindowManager::getWindowCenterPosition(double &x, double &y)
{
    x = x_ + 0.5 * w_;
    y = y_ + 0.5 * h_;
}

void ContentWindowManager::centerPositionAround(double x, double y, bool constrainToWindowBorders)
{
    double newX = x - 0.5 * w_;
    double newY = y - 0.5 * h_;

    if (constrainToWindowBorders)
    {
        if (newX + w_ > 1.0)
            newX = 1.0-w_;
        if (newY + h_ > 1.0)
            newY = 1.0-h_;

        newX = std::max(0.0, newX);
        newY = std::max(0.0, newY);
    }

    setPosition(newX, newY);
}

void ContentWindowManager::render()
{
    content_->render(shared_from_this());

    // optionally render the border
    bool showWindowBorders = true;

    boost::shared_ptr<DisplayGroupManager> dgm = getDisplayGroupManager();

    if(dgm != NULL)
    {
        showWindowBorders = dgm->getOptions()->getShowWindowBorders();
    }

    if(showWindowBorders || selected() )
    {
        double horizontalBorder = 5. / (double)g_configuration->getTotalHeight(); // 5 pixels

        // enlarge the border if we're highlighted
        if(getHighlighted() == true)
        {
            horizontalBorder *= 4.;
        }

        double verticalBorder = (double)g_configuration->getTotalHeight() / (double)g_configuration->getTotalWidth() * horizontalBorder;

        glPushAttrib(GL_CURRENT_BIT);

        // color the border based on window state
        if(selected())
        {
            glColor4f(1,0,0,1);
        }
        else
        {
            glColor4f(1,1,1,1);
        }

        GLWindow::drawRectangle(x_-verticalBorder,y_-horizontalBorder,w_+2.*verticalBorder,h_+2.*horizontalBorder);

        glPopAttrib();
    }

#if 0	// not needed for multitouch
    glPushAttrib(GL_CURRENT_BIT);

    // render buttons if any of the markers are over the window
    bool markerOverWindow = false;

    std::vector<boost::shared_ptr<Marker> > markers = getDisplayGroupManager()->getMarkers();

    for(unsigned int i=0; i<markers.size(); i++)
    {
        // don't consider inactive markers
        if(markers[i]->getActive() == false)
        {
            continue;
        }

        float markerX, markerY;
        markers[i]->getPosition(markerX, markerY);

        if(QRectF(x_, y_, w_, h_).contains(markerX, markerY) == true)
        {
            markerOverWindow = true;
            break;
        }
    }

    if(markerOverWindow == true)
    {
        // we need this to be slightly in front of the rest of the window
        glPushMatrix();
        glTranslatef(0,0,0.001);

        // button dimensions
        float buttonWidth, buttonHeight;
        getButtonDimensions(buttonWidth, buttonHeight);

        // draw close button
        QRectF closeRect(x_ + w_ - buttonWidth, y_, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(1,0,0,0.125);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(1,0,0,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x() + closeRect.width(), closeRect.y() + closeRect.height());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x(), closeRect.y() + closeRect.height());
        glEnd();

        // resize indicator
        QRectF resizeRect(x_ + w_ - buttonWidth, y_ + h_ - buttonHeight, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(0.5,0.5,0.5,0.25);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(0.5,0.5,0.5,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x(), resizeRect.y() + resizeRect.height());
        glEnd();

        glPopMatrix();
    }

    glPopAttrib();
#endif
}
