#include "ContentWindowInterface.h"
#include "ContentWindowManager.h"
#include "main.h"

ContentWindowInterface::ContentWindowInterface(boost::shared_ptr<ContentWindowManager> contentWindowManager)
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
        selected_ = contentWindowManager->selected_;
    }

    // connect signals from this to slots on the ContentWindowManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCoordinates(double, double, double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(selectedChanged(bool, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setSelected(bool, ContentWindowInterface *)), Qt::QueuedConnection);
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
    connect(contentWindowManager.get(), SIGNAL(selectedChanged(bool, ContentWindowInterface *)), this, SLOT(setSelected(bool, ContentWindowInterface *)), Qt::QueuedConnection);
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

void ContentWindowInterface::setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWidth_ = contentWidth;
    contentHeight_ = contentHeight;

    fixAspectRatio(this);

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
    w_ = w;
    h_ = h;

    fixAspectRatio(this);

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
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

    w_ = w;
    h_ = h;

    fixAspectRatio(this);

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
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

    zoom_ = zoom;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
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

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
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
