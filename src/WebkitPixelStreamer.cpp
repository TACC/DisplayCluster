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
#include "globals.h"
#include "Configuration.h"

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
    , interactionModeActive_(false)
{
    webView_ = new QWebView();
    webView_->page()->setViewportSize( QSize( WEPPAGE_DEFAULT_WIDTH*WEBPAGE_DEFAULT_ZOOM, WEBPAGE_DEFAULT_HEIGHT*WEBPAGE_DEFAULT_ZOOM ));
    webView_->setZoomFactor(WEBPAGE_DEFAULT_ZOOM);

    QWebSettings* settings = webView_->settings();
    settings->setAttribute( QWebSettings::AcceleratedCompositingEnabled, true );
    settings->setAttribute( QWebSettings::JavascriptEnabled, true );
    settings->setAttribute( QWebSettings::PluginsEnabled, true );
#if QT_VERSION >= 0x040800
    settings->setAttribute( QWebSettings::WebGLEnabled, true );
#endif

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

QWebView* WebkitPixelStreamer::getView() const
{
    return webView_;
}

void WebkitPixelStreamer::updateInteractionState(InteractionState interactionState)
{
    QMutexLocker locker(&mutex_);

    switch(interactionState.type)
    {
    case InteractionState::EVT_CLICK:
        processClickEvent(interactionState);
        break;
    case InteractionState::EVT_PRESS:
        processPressEvent(interactionState);
        break;
    case InteractionState::EVT_MOVE:
        processMoveEvent(interactionState);
        break;
    case InteractionState::EVT_WHEEL:
        processWheelEvent(interactionState);
        break;
    case InteractionState::EVT_RELEASE:
        processReleaseEvent(interactionState);
        break;
    case InteractionState::EVT_SWIPE_LEFT:
        webView_->back();
        break;
    case InteractionState::EVT_SWIPE_RIGHT:
        webView_->forward();
        break;
    case InteractionState::EVT_KEY_PRESS:
        processKeyPress(interactionState);
        break;
    case InteractionState::EVT_KEY_RELEASE:
        processKeyRelease(interactionState);
        break;
    case InteractionState::EVT_VIEW_SIZE_CHANGED:
        processViewSizeChange(interactionState);
        break;
    default:
        break;
    }
}

void WebkitPixelStreamer::processClickEvent(const InteractionState &interactionState)
{
    // TODO History navigation (until swipe gestures are fixed)
    if (interactionState.mouseX < 0.02)
    {
        webView_->back();
        return;
    }
    if (interactionState.mouseX > 0.98)
    {
        webView_->forward();
        return;
    }

    processPressEvent(interactionState);
    processReleaseEvent(interactionState);

    const QWebHitTestResult& hitResult = performHitTest( interactionState );
    if( !hitResult.isNull() && !hitResult.linkUrl().isEmpty( ))
        webView_->load( hitResult.linkUrl( ));
}

void WebkitPixelStreamer::processPressEvent(const InteractionState &interactionState)
{
    const QWebHitTestResult& hitResult = performHitTest(interactionState);

    if(!hitResult.isNull())
    {
        interactionModeActive_ = isWebGLElement(hitResult.element());

        const QPoint& pointerPos = getPointerPosition(interactionState);

        QMouseEvent myEvent(QEvent::MouseButtonPress, pointerPos,
                            Qt::LeftButton, Qt::LeftButton,
                            (Qt::KeyboardModifiers)interactionState.modifiers);

        webView_->page()->event(&myEvent);
    }
}


void WebkitPixelStreamer::processMoveEvent(const InteractionState &interactionState)
{
    const QPoint& pointerPos = getPointerPosition(interactionState);

    if(interactionModeActive_)
    {
        QMouseEvent myEvent(QEvent::MouseMove, pointerPos,
                            Qt::LeftButton, Qt::LeftButton,
                            (Qt::KeyboardModifiers)interactionState.modifiers);

        webView_->page()->event(&myEvent);
    }
    else
    {
        QWebFrame *pFrame = webView_->page()->frameAt(pointerPos);

        int dx = interactionState.dx * webView_->page()->viewportSize().width();
        int dy = interactionState.dy * webView_->page()->viewportSize().height();

        pFrame->scroll(-dx,-dy);
    }
}

void WebkitPixelStreamer::processReleaseEvent(const InteractionState &interactionState)
{
    const QPoint& pointerPos = getPointerPosition(interactionState);

    QMouseEvent myEvent(QEvent::MouseButtonRelease, pointerPos,
                        Qt::LeftButton, Qt::LeftButton,
                        (Qt::KeyboardModifiers)interactionState.modifiers);

    webView_->page()->event(&myEvent);

    interactionModeActive_ = false;
}

void WebkitPixelStreamer::processWheelEvent(const InteractionState &interactionState)
{
    const QWebHitTestResult& hitResult = performHitTest(interactionState);

    if(!hitResult.isNull() && isWebGLElement(hitResult.element()))
    {
        int delta_y = interactionState.dy * g_configuration->getTotalHeight();

        QWheelEvent myEvent(hitResult.pos(), delta_y, Qt::NoButton,
                            (Qt::KeyboardModifiers)interactionState.modifiers,
                            Qt::Vertical);

        webView_->page()->event(&myEvent);
    }
}

void WebkitPixelStreamer::processKeyPress(const InteractionState& interactionState)
{
    QKeyEvent myEvent(QEvent::KeyPress, interactionState.key,
                      (Qt::KeyboardModifiers)interactionState.modifiers,
                      QString::fromStdString(interactionState.text)
                      );
    webView_->page()->event(&myEvent);
}

void WebkitPixelStreamer::processKeyRelease(const InteractionState &interactionState)
{
    QKeyEvent myEvent(QEvent::KeyRelease, interactionState.key,
                      (Qt::KeyboardModifiers)interactionState.modifiers,
                      QString::fromStdString(interactionState.text)
                      );
    webView_->page()->event(&myEvent);
}

void WebkitPixelStreamer::processViewSizeChange(const InteractionState &interactionState)
{
    QSize newSize( std::max((int)interactionState.dx, WEPPAGE_MIN_WIDTH), std::max((int)interactionState.dy, WEBPAGE_MIN_HEIGHT) );

    webView_->page()->setViewportSize( newSize );

    const qreal zoomFactor = qreal(newSize.width()) / WEPPAGE_DEFAULT_WIDTH;
    webView_->setZoomFactor(zoomFactor);
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
        segment.parameters = createSegmentHeader();

        segment.imageData = QByteArray::fromRawData((const char*)image_.bits(), image_.byteCount());
        segment.imageData.detach();

        ++frameIndex_;

        emit segmentUpdated(uri_, segment);
    }
}

QWebHitTestResult WebkitPixelStreamer::performHitTest(const InteractionState &interactionState) const
{
    const QPoint& pointerPos = getPointerPosition(interactionState);
    QWebFrame *pFrame = webView_->page()->frameAt(pointerPos);
    return pFrame->hitTestContent(pointerPos);
}

QPoint WebkitPixelStreamer::getPointerPosition(const InteractionState &interactionState) const
{
    QWebPage* page = webView_->page();

    int x = interactionState.mouseX * page->viewportSize().width();
    int y = interactionState.mouseY * page->viewportSize().height();

    x = std::max(0, std::min(x, page->viewportSize().width()-1));
    y = std::max(0, std::min(y, page->viewportSize().height()-1));

    return QPoint(x, y);
}

PixelStreamSegmentParameters WebkitPixelStreamer::createSegmentHeader() const
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

bool WebkitPixelStreamer::isWebGLElement(const QWebElement& element) const
{
    return element.tagName() == "CANVAS";
}
