#include "Content.h"
#include "ContentGraphicsItem.h"
#include "main.h"
#include <iostream>

Content::Content(std::string uri)
{
    uri_ = uri;
}

std::string Content::getURI()
{
    return uri_;
}

void Content::setCoordinates(double x, double y, double w, double h)
{
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;

    // force synchronization
    g_displayGroup.synchronizeContents();
}

void Content::getCoordinates(double &x, double &y, double &w, double &h)
{
    x = x_;
    y = y_;
    w = w_;
    h = h_;
}

boost::shared_ptr<ContentGraphicsItem> Content::getGraphicsItem()
{
    if(graphicsItem_ == NULL)
    {
        boost::shared_ptr<ContentGraphicsItem> gi(new ContentGraphicsItem(shared_from_this()));
        graphicsItem_ = gi;
    }

    return graphicsItem_;
}
