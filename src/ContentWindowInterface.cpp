#include "ContentWindowInterface.h"
#include "ContentWindow.h"
#include "main.h"

ContentWindowInterface::ContentWindowInterface(boost::shared_ptr<ContentWindow> contentWindow)
{
    contentWindow_ = contentWindow;

    // copy all members from contentWindow
    if(contentWindow != NULL)
    {
        contentWidth_ = contentWindow->contentWidth_;
        contentHeight_ = contentWindow->contentHeight_;
        x_ = contentWindow->x_;
        y_ = contentWindow->y_;
        w_ = contentWindow->w_;
        h_ = contentWindow->h_;
        centerX_ = contentWindow->centerX_;
        centerY_ = contentWindow->centerY_;
        zoom_ = contentWindow->zoom_;
        selected_ = contentWindow->selected_;
    }

    // connect signals from this to slots on the ContentWindow
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), contentWindow.get(), SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), contentWindow.get(), SLOT(setCoordinates(double, double, double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), contentWindow.get(), SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), contentWindow.get(), SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), contentWindow.get(), SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), contentWindow.get(), SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(selectedChanged(bool, ContentWindowInterface *)), contentWindow.get(), SLOT(setSelected(bool, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(movedToFront(ContentWindowInterface *)), contentWindow.get(), SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(closed(ContentWindowInterface *)), contentWindow.get(), SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // connect signals on the ContentWindow object to slots on this
    // use queued connections for thread-safety
    connect(contentWindow.get(), SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), this, SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), this, SLOT(setCoordinates(double, double, double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(positionChanged(double, double, ContentWindowInterface *)), this, SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), this, SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(centerChanged(double, double, ContentWindowInterface *)), this, SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(zoomChanged(double, ContentWindowInterface *)), this, SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(selectedChanged(bool, ContentWindowInterface *)), this, SLOT(setSelected(bool, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(movedToFront(ContentWindowInterface *)), this, SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindow.get(), SIGNAL(closed(ContentWindowInterface *)), this, SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // destruction
    connect(contentWindow.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<ContentWindow> ContentWindowInterface::getContentWindow()
{
    return contentWindow_.lock();
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

bool ContentWindowInterface::getSelected()
{
    return selected_;
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

        if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
        {
            if(source == NULL)
            {
                source = this;
            }

            setSize(w_, h_);
        }
    }
}

void ContentWindowInterface::setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWidth_ = contentWidth;
    contentHeight_ = contentHeight;

    fixAspectRatio(this);

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
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
    w_ = w;
    h_ = h;

    fixAspectRatio(this);

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(coordinatesChanged(x_, y_, w_, h_, source));
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

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
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

    w_ = w;
    h_ = h;

    fixAspectRatio(this);

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(sizeChanged(w_, h_, source));
    }
}

void ContentWindowInterface::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    centerX_ = centerX;
    centerY_ = centerY;

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
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

    zoom_ = zoom;

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(zoomChanged(zoom_, source));
    }
}

void ContentWindowInterface::setSelected(bool selected, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    selected_ = selected;

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(selectedChanged(selected_, source));
    }
}

void ContentWindowInterface::moveToFront(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
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

    if(source == NULL || dynamic_cast<ContentWindow *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(closed(source));
    }
}
