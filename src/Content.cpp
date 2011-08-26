#include "Content.h"
#include "ContentGraphicsItem.h"
#include "main.h"
#include "TextureFactory.h"

Content::Content(std::string uri)
{
    uri_ = uri;

    // default to centered
    centerX_ = 0.5;
    centerY_ = 0.5;

    // default to no zoom
    zoom_ = 1.;
}

boost::shared_ptr<DisplayGroup> Content::getDisplayGroup()
{
    return displayGroup_.lock();
}

void Content::setDisplayGroup(boost::shared_ptr<DisplayGroup> displayGroup)
{
    displayGroup_ = displayGroup;
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
    g_displayGroup->synchronizeContents();
}

void Content::getCoordinates(double &x, double &y, double &w, double &h)
{
    x = x_;
    y = y_;
    w = w_;
    h = h_;
}

void Content::setCenterCoordinates(double centerX, double centerY)
{
    centerX_ = centerX;
    centerY_ = centerY;

    // force synchronization
    g_displayGroup->synchronizeContents();
}

void Content::getCenterCoordinates(double &centerX, double &centerY)
{
    centerX = centerX_;
    centerY = centerY_;
}

void Content::setZoom(double zoom)
{
    zoom_ = zoom;

    // force synchronization
    g_displayGroup->synchronizeContents();
}

double Content::getZoom()
{
    return zoom_;
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
    // calculate texture coordinates
    float tX = centerX_ - 0.5 / zoom_;
    float tY = centerY_ - 0.5 / zoom_;
    float tW = 1./zoom_;
    float tH = 1./zoom_;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_mainWindow->getGLWindow()->getTextureFactory().getTexture(getURI()));

    // on zoom-out, clamp to border (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBegin(GL_QUADS);

    // note we need to flip the y coordinate since the textures are loaded upside down
    glTexCoord2f(tX,1.-tY);
    glVertex2f(x_,y_);

    glTexCoord2f(tX+tW,1.-tY);
    glVertex2f(x_+w_,y_);

    glTexCoord2f(tX+tW,1.-(tY+tH));
    glVertex2f(x_+w_,y_+h_);

    glTexCoord2f(tX,1.-(tY+tH));
    glVertex2f(x_,y_+h_);

    glEnd();

    glDisable(GL_TEXTURE_2D);

    // border
    double border = 0.0025;

    glColor4f(1,1,1,1);
    g_mainWindow->getGLWindow()->drawRectangle(x_-border,y_-border,w_+2.*border,h_+2.*border);

    glPopAttrib();
}
