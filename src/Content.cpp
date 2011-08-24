#include "Content.h"
#include "ContentGraphicsItem.h"
#include "main.h"
#include "TextureFactory.h"

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

void Content::render()
{
    double border = 0.0025;

    QRectF rect(x_,y_,w_,h_);

    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_TEXTURE_2D);
    g_mainWindow->getGLWindow()->drawTexture(rect, g_mainWindow->getGLWindow()->getTextureFactory().getTexture(getURI()));

    glDisable(GL_TEXTURE_2D);
    glColor4f(1,1,1,1);
    g_mainWindow->getGLWindow()->drawRectangle(x_-border,y_-border,w_+2.*border,h_+2.*border);

    glPopAttrib();
}
