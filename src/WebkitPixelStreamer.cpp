#include "WebkitPixelStreamer.h"

#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebView>

#include <QTimer>

#include "log.h"
#include "PixelStreamSegmentJpegCompressor.h"

#define WEPPAGE_DEFAULT_WIDTH   1280
#define WEBPAGE_DEFAULT_HEIGHT  1024


WebkitPixelStreamer::WebkitPixelStreamer(DisplayGroupManager *displayGroupManager, QString uri)
    : LocalPixelStreamer(displayGroupManager, uri)
    , webView_(0)
    , timer_(0)
    , frameIndex_(0)
{
    webView_ = new QWebView();
    webView_->page()->setViewportSize( QSize( WEPPAGE_DEFAULT_WIDTH, WEBPAGE_DEFAULT_HEIGHT ));

    image_ = QImage( webView_->page()->viewportSize(), QImage::Format_ARGB32 );

//    connect(webView_->page(), SIGNAL(repaintRequested(QRect)), this, SLOT(updateRegion(QRect)));
//    connect(webView_->page(), SIGNAL(geometryChangeRequested(QRect)), this, SLOT(updateRegion(QRect)));
//    connect(webView_->page(), SIGNAL(contentsChanged()), this, SLOT(update()));
//    connect(webView_->page()->mainFrame(), SIGNAL(pageChanged()), this, SLOT(update()));

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
    webView_->load( QUrl(url) );
}

#include <string>
#include <QtWebKit/QWebElement>


void WebkitPixelStreamer::updateInteractionState(InteractionState interactionState)
{
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
        QWebHitTestResult hitResult = pFrame->hitTestContent(pointerPos);
        if (!hitResult.linkUrl().isEmpty())
        {
            webView_->load(hitResult.linkUrl());
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
}

void WebkitPixelStreamer::update()
{
    QWebPage* page = webView_->page();
    if( !page->viewportSize().isEmpty())
    {
        QPainter painter( &image_ );
        page->mainFrame()->render( &painter );
        painter.end();

        PixelStreamSegment segment;
        segment.parameters = makeSegmentHeader();
        // TODO remove this crappy compression when merging with Daniel's no-compression codebase
        computeSegmentJpeg(image_, segment);

        ++frameIndex_;

        emit segmentUpdated(uri_, segment);
    }
}

//void WebkitPixelStreamer::updateRegion(const QRect & dirtyRect)
//{
//    QWebPage* page = webView_->page();
//    if( !page->viewportSize().isEmpty())
//    {
//        QPainter painter( &image_ );
//        page->mainFrame()->render( &painter, dirtyRect );
//        painter.end();

//        PixelStreamSegment segment;
//        segment.parameters = makeSegmentHeader();
//        // TODO remove this crappy compression when merging with Daniel's no-compression codebase
//        computeSegmentJpeg(image_, segment);

//        ++frameIndex_;

//        emit segmentUpdated(uri_, segment);
//    }
//}


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

    return parameters;
}
