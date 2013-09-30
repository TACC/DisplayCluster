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

#include "WebkitPixelStreamer.h"

#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebView>
#include <QtWebKit/QWebElement>
#include <QKeyEvent>

#include <QTimer>

#include "log.h"

#define WEPPAGE_DEFAULT_WIDTH   1280
#define WEBPAGE_DEFAULT_HEIGHT  1024
#define WEBPAGE_DEFAULT_ZOOM    2.0
#define WEPPAGE_MIN_WIDTH   (WEPPAGE_DEFAULT_WIDTH/2)
#define WEBPAGE_MIN_HEIGHT  (WEBPAGE_DEFAULT_HEIGHT/2)



WebkitPixelStreamer::WebkitPixelStreamer(QString uri)
    : LocalPixelStreamer(uri)
    , webView_(0)
    , timer_(0)
    , frameIndex_(0)
{
    webView_ = new QWebView();
    webView_->page()->setViewportSize( QSize( WEPPAGE_DEFAULT_WIDTH*WEBPAGE_DEFAULT_ZOOM, WEBPAGE_DEFAULT_HEIGHT*WEBPAGE_DEFAULT_ZOOM ));
    webView_->setZoomFactor(WEBPAGE_DEFAULT_ZOOM);

    webView_->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    webView_->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);

    timer_ = new QTimer();
    connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
    timer_->start(30);
}

WebkitPixelStreamer::~WebkitPixelStreamer()
{
    timer_->stop();
    delete timer_;
    delete webView_;
}


void WebkitPixelStreamer::setUrl(QString url)
{
    QMutexLocker locker(&mutex_);

    webView_->load( QUrl(url) );
}


void WebkitPixelStreamer::updateInteractionState(InteractionState interactionState)
{
    QMutexLocker locker(&mutex_);

    QWebPage* page = webView_->page();

    int x = interactionState.mouseX*page->viewportSize().width();
    int y = interactionState.mouseY*page->viewportSize().height();

    x = std::max(0, std::min(x, page->viewportSize().width()-1));
    y = std::max(0, std::min(y, page->viewportSize().height()-1));

    int dx = interactionState.dx*page->viewportSize().width();
    int dy = interactionState.dy*page->viewportSize().height();

    QPoint pointerPos(x, y);
    QWebFrame *pFrame = page->frameAt(pointerPos);

    if (interactionState.type == InteractionState::EVT_CLICK)
    {
        // History navigation (until swipe gestures are fixed)
        if (interactionState.mouseX < 0.02)
        {
            webView_->back();
            return;
        }
        else if (interactionState.mouseX > 0.98)
        {
            webView_->forward();
            return;
        }

        QWebHitTestResult hitResult = pFrame->hitTestContent(pointerPos);

        if ( !hitResult.isNull() )
        {
            if (!hitResult.linkUrl().isEmpty())
            {
                webView_->load(hitResult.linkUrl());
            }
            else
            {
                hitResult.element().evaluateJavaScript("this.click()");

                if (hitResult.isContentEditable())
                {
                    hitResult.element().setFocus();
                    // Ugly hack to work around a focus problem: some input fields receive
                    // focus yet key events are still directed to a different INPUT field..
                    if (hitResult.element().tagName()== "INPUT")
                        hitResult.element().evaluateJavaScript("this.value = this.value");
                }
            }
        }
    }

    else if (interactionState.type == InteractionState::EVT_MOVE || interactionState.type == InteractionState::EVT_WHEEL)
    {
        pFrame->scroll(-dx,-dy);
    }

    else if (interactionState.type == InteractionState::EVT_SWIPE_LEFT)
    {
        webView_->back();
    }
    else if (interactionState.type == InteractionState::EVT_SWIPE_RIGHT)
    {
        webView_->forward();
    }

    else if (interactionState.type == InteractionState::EVT_KEY_PRESS)
    {
        QKeyEvent myEvent(QEvent::KeyPress, interactionState.key,
                          (Qt::KeyboardModifiers)interactionState.modifiers,
                          QString::fromStdString(interactionState.text)
                          );
        page->event(&myEvent);
    }
    else if (interactionState.type == InteractionState::EVT_KEY_RELEASE)
    {
        QKeyEvent myEvent(QEvent::KeyRelease, interactionState.key,
                          (Qt::KeyboardModifiers)interactionState.modifiers,
                          QString::fromStdString(interactionState.text)
                          );
        page->event(&myEvent);
    }

    else if (interactionState.type == InteractionState::EVT_VIEW_SIZE_CHANGED)
    {
        QSize newSize( std::max((int)interactionState.dx, WEPPAGE_MIN_WIDTH), std::max((int)interactionState.dy, WEBPAGE_MIN_HEIGHT) );

        double zoomFactor = (double)newSize.width() / (double)WEPPAGE_DEFAULT_WIDTH;

        webView_->page()->setViewportSize( newSize );
        webView_->setZoomFactor(zoomFactor);
    }
}

QSize WebkitPixelStreamer::size() const
{
    return webView_->page()->viewportSize();
}

void WebkitPixelStreamer::update()
{
    QMutexLocker locker(&mutex_);

    QWebPage* page = webView_->page();
    if( !page->viewportSize().isEmpty())
    {
        if (image_.size() != page->viewportSize())
	        image_ = QImage( page->viewportSize(), QImage::Format_ARGB32 );

        QPainter painter( &image_ );
        page->mainFrame()->render( &painter );
        painter.end();

        PixelStreamSegment segment;
        segment.parameters = makeSegmentHeader();

        segment.imageData = QByteArray::fromRawData((const char*)image_.bits(), image_.byteCount());
        segment.imageData.detach();

        ++frameIndex_;

        emit segmentUpdated(uri_, segment);
    }
}

PixelStreamSegmentParameters WebkitPixelStreamer::makeSegmentHeader()
{
    PixelStreamSegmentParameters parameters;

    parameters.sourceIndex = 0;
    parameters.frameIndex = frameIndex_;

    parameters.totalHeight = webView_->page()->viewportSize().height();
    parameters.totalWidth = webView_->page()->viewportSize().width();

    parameters.height = parameters.totalHeight;
    parameters.width = parameters.totalWidth;

    parameters.x = 0;
    parameters.y = 0;

    parameters.compressed = false;

    return parameters;
}
