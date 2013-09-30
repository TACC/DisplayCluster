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

#include "ContentWindowInterface.h"
#include "ContentWindowManager.h"
#include "main.h"

ContentWindowInterface::ContentWindowInterface(boost::shared_ptr<ContentWindowManager> contentWindowManager)\
    : windowState_(UNSELECTED)
    , boundInteractions_( 0 )
{
    contentWindowManager_ = contentWindowManager;

    // copy all members from contentWindowManager
    if(contentWindowManager != NULL)
    {
        contentWidth_ = contentWindowManager->contentWidth_;
        contentHeight_ = contentWindowManager->contentHeight_;
        x_ = contentWindowManager->x_;
        y_ = contentWindowManager->y_;
        w_ = contentWindowManager->w_;
        h_ = contentWindowManager->h_;
        centerX_ = contentWindowManager->centerX_;
        centerY_ = contentWindowManager->centerY_;
        zoom_ = contentWindowManager->zoom_;
        sizeState_ = contentWindowManager->sizeState_;
        controlState_ = contentWindowManager->controlState_;
        windowState_ = contentWindowManager->windowState_;
        interactionState_ = contentWindowManager->interactionState_;
    }

    // register WindowState in Qt
    qRegisterMetaType<ContentWindowInterface::WindowState>("ContentWindowInterface::WindowState");

    // connect signals from this to slots on the ContentWindowManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCoordinates(double, double, double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setWindowState(ContentWindowInterface::WindowState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(interactionStateChanged(InteractionState, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setInteractionState(InteractionState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(highlighted(ContentWindowInterface *)), contentWindowManager.get(), SLOT(highlight(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(movedToFront(ContentWindowInterface *)), contentWindowManager.get(), SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(closed(ContentWindowInterface *)), contentWindowManager.get(), SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // connect signals on the ContentWindowManager object to slots on this
    // use queued connections for thread-safety
    connect(contentWindowManager.get(), SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), this, SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), this, SLOT(setCoordinates(double, double, double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(positionChanged(double, double, ContentWindowInterface *)), this, SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), this, SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(centerChanged(double, double, ContentWindowInterface *)), this, SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(zoomChanged(double, ContentWindowInterface *)), this, SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)), this, SLOT(setWindowState(ContentWindowInterface::WindowState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(interactionStateChanged(InteractionState, ContentWindowInterface *)), this, SLOT(setInteractionState(InteractionState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(highlighted(ContentWindowInterface *)), this, SLOT(highlight(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(movedToFront(ContentWindowInterface *)), this, SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(closed(ContentWindowInterface *)), this, SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // destruction
    connect(contentWindowManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<ContentWindowManager> ContentWindowInterface::getContentWindowManager()
{
    return contentWindowManager_.lock();
}

void ContentWindowInterface::getContentDimensions(int &contentWidth, int &contentHeight)
{
    contentWidth = contentWidth_;
    contentHeight = contentHeight_;
}

void ContentWindowInterface::getCoordinates(double &x, double &y, double &w, double &h)
{
    x = x_;
    y = y_;
    w = w_;
    h = h_;
}

void ContentWindowInterface::getPosition(double &x, double &y)
{
    x = x_;
    y = y_;
}

void ContentWindowInterface::getSize(double &w, double &h)
{
    w = w_;
    h = h_;
}

void ContentWindowInterface::getCenter(double &centerX, double &centerY)
{
    centerX = centerX_;
    centerY = centerY_;
}

double ContentWindowInterface::getZoom()
{
    return zoom_;
}

void ContentWindowInterface::toggleWindowState()
{
    setWindowState( windowState_ == UNSELECTED ? SELECTED : UNSELECTED );
}

ContentWindowInterface::WindowState ContentWindowInterface::getWindowState()
{
    return windowState_;
}

InteractionState ContentWindowInterface::getInteractionState()
{
    return interactionState_;
}

bool ContentWindowInterface::getHighlighted()
{
    long dtMilliseconds = (g_displayGroupManager->getTimestamp() - highlightedTimestamp_).total_milliseconds();

    if(dtMilliseconds > HIGHLIGHT_TIMEOUT_MILLISECONDS || dtMilliseconds % (HIGHLIGHT_BLINK_INTERVAL*2) < HIGHLIGHT_BLINK_INTERVAL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

SizeState ContentWindowInterface::getSizeState() const
{
    return sizeState_;
}

void ContentWindowInterface::getButtonDimensions(float &width, float &height)
{
    float sceneHeightFraction = 0.125;
    double screenAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

    width = sceneHeightFraction / screenAspect;
    height = sceneHeightFraction;

    // clamp to half rect dimensions
    if(width > 0.5 * w_)
    {
        width = 0.49 * w_;
    }

    if(height > 0.5 * h_)
    {
        height = 0.49 * h_;
    }
}

void ContentWindowInterface::fixAspectRatio(ContentWindowInterface * source)
{
    if(g_mainWindow->getConstrainAspectRatio() != true || (contentWidth_ == 0 && contentHeight_ == 0))
    {
        return;
    }

    double aspect = (double)contentWidth_ / (double)contentHeight_;
    double screenAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

    aspect /= screenAspect;

    double w = w_;
    double h = h_;

    if(aspect > w_ / h_)
    {
        h = w_ / aspect;
    }
    else if(aspect <= w_ / h_)
    {
        w = h_ * aspect;
    }

    // we don't want to call setSize unless necessary, since it will emit a signal
    if(w != w_ || h != h_)
    {
        w_ = w;
        h_ = h;

        if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
        {
            if(source == NULL)
            {
                source = this;
            }

            setSize(w_, h_);
        }
    }
}

void ContentWindowInterface::adjustSize( const SizeState state,
                                         ContentWindowInterface * source )
{
    sizeState_ = state;

    const double contentAR = contentHeight_ == 0 ? 16./9 :
                                 double(contentWidth_) / double(contentHeight_);
    const double configAR = double(g_configuration->getTotalHeight()) /
                            double(g_configuration->getTotalWidth());

    double h = contentHeight_ == 0 ? 1. : double(contentHeight_) /
                                    double(g_configuration->getTotalHeight());
    double w = contentWidth_ == 0 ? configAR * contentAR * h :
                                    double(contentWidth_) /
                                       double(g_configuration->getTotalWidth());

    switch( state )
    {
    case SIZE_FULLSCREEN:
        {
            const double resize = std::min( 1. / h, 1. / w );
            h *= resize;
            w *= resize;
        } break;

    case SIZE_1TO1:
        h = std::min( h, 1. );
        w = configAR * contentAR * h;
        if( w > 1. )
        {
            h /= w;
            w /= w;
        }
        break;

    case SIZE_CUSTOM:
    default:
        return;
    }

    // center on the wall
    const double y = (1. - h) * .5;
    const double x = (1. - w) * .5;

    setCoordinates( x, y, w, h, source );
}

void ContentWindowInterface::setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWidth_ = contentWidth;
    contentHeight_ = contentHeight;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentDimensionsChanged(contentWidth_, contentHeight_, source));
    }
}

void ContentWindowInterface::setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    x_ = x;
    y_ = y;

    // don't allow negative width or height
    if(w > 0. && h > 0.)
    {
        w_ = w;
        h_ = h;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        fixAspectRatio(source);

        emit(coordinatesChanged(x_, y_, w_, h_, source));

        setInteractionStateToNewDimensions();
    }
}

void ContentWindowInterface::setPosition(double x, double y, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    x_ = x;
    y_ = y;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(positionChanged(x_, y_, source));
    }
}

void ContentWindowInterface::setSize(double w, double h, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // don't allow negative width or height
    if(w > 0. && h > 0.)
    {
        w_ = w;
        h_ = h;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        fixAspectRatio(source);

        emit(sizeChanged(w_, h_, source));

        setInteractionStateToNewDimensions();
    }
}

void ContentWindowInterface::scaleSize(double factor, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // don't allow negative factor
    if(factor < 0.)
    {
        return;
    }

    // calculate new coordinates
    double x = x_ - (factor - 1.) * w_ / 2.;
    double y = y_ - (factor - 1.) * h_ / 2.;
    double w = w_ * factor;
    double h = h_ * factor;

    setCoordinates(x, y, w, h);

    // we don't need to emit any signals since setCoordinates() takes care of this
}

void ContentWindowInterface::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // clamp center point such that view rectangle dimensions are constrained [0,1]
    float tX = centerX - 0.5 / zoom_;
    float tY = centerY - 0.5 / zoom_;
    float tW = 1./zoom_;
    float tH = 1./zoom_;

    // handle centerX, clamping it if necessary
    if(tX >= 0. && tX+tW <= 1.)
    {
        centerX_ = centerX;
    }
    else if(tX < 0.)
    {
        centerX_ = 0.5 / zoom_;
    }
    else if(tX+tW > 1.)
    {
        centerX_ = 1. - tW + 0.5 / zoom_;
    }

    // handle centerY, clamping it if necessary
    if(tY >= 0. && tY+tH <= 1.)
    {
        centerY_ = centerY;
    }
    else if(tY < 0.)
    {
        centerY_ = 0.5 / zoom_;
    }
    else if(tY+tH > 1.)
    {
        centerY_ = 1. - tH + 0.5 / zoom_;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(centerChanged(centerX_, centerY_, source));
    }
}

void ContentWindowInterface::setZoom(double zoom, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // clamp zoom to be >= 1
    if(zoom < 1.)
    {
        zoom = 1.;
    }

    zoom_ = zoom;

    float tX = centerX_ - 0.5 / zoom;
    float tY = centerY_ - 0.5 / zoom;
    float tW = 1./zoom;
    float tH = 1./zoom;

    // see if we need to adjust the center point since the rectangle view bounds are outside [0,1]
    if(QRectF(0.,0.,1.,1.).contains(QRectF(tX,tY,tW,tH)) != true)
    {
        // handle centerX, clamping it if necessary
        if(tX < 0.)
        {
            centerX_ = 0.5 / zoom_;
        }
        else if(tX+tW > 1.)
        {
            centerX_ = 1. - tW + 0.5 / zoom_;
        }

        // handle centerY, clamping it if necessary
        if(tY < 0.)
        {
            centerY_ = 0.5 / zoom_;
        }
        else if(tY+tH > 1.)
        {
            centerY_ = 1. - tH + 0.5 / zoom_;
        }

        setCenter(centerX_, centerY_);
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(zoomChanged(zoom_, source));
    }
}

void ContentWindowInterface::setWindowState(ContentWindowInterface::WindowState windowState, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    windowState_ = windowState;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(windowStateChanged(windowState_, source));
    }
}

void ContentWindowInterface::setInteractionState(InteractionState interactionState, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    interactionState_ = interactionState;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(interactionStateChanged(interactionState_, source));
    }
}

void ContentWindowInterface::highlight(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // set highlighted timestamp
    highlightedTimestamp_ = g_displayGroupManager->getTimestamp();

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(highlighted(source));
    }
}

void ContentWindowInterface::moveToFront(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(movedToFront(source));
    }
}

void ContentWindowInterface::close(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(closed(source));
    }
}

void ContentWindowInterface::setInteractionStateToNewDimensions()
{
    InteractionState state;
    state.type = InteractionState::EVT_VIEW_SIZE_CHANGED;
    state.dx = w_ * g_configuration->getTotalWidth();
    state.dy = h_ * g_configuration->getTotalHeight();
    setInteractionState(state);
}

void ContentWindowInterface::bindInteraction( const QObject* receiver,
                                              const char* slot )
{
    connect( this, SIGNAL(interactionStateChanged( InteractionState,
                                                   ContentWindowInterface* )),
             receiver, slot, Qt::QueuedConnection );
    ++boundInteractions_;
}
