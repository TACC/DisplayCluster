#include "Content.h"
#include "ContentGraphicsItem.h"
#include "main.h"
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "MovieContent.h"

Content::Content(std::string uri)
{
    uri_ = uri;

    // default position / size
    x_ = y_ = 0.;
    w_ = h_ = 0.25;

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

void Content::setCoordinates(double x, double y, double w, double h, bool setGraphicsItem)
{
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;

    if(setGraphicsItem == true)
    {
        if(graphicsItem_ != NULL)
        {
            graphicsItem_->setRect(x_, y_, w_, h_);
        }
    }

    // force synchronization
    g_displayGroup->sendDisplayGroup();
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
    g_displayGroup->sendDisplayGroup();
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
    g_displayGroup->sendDisplayGroup();
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

    // transform to a normalize coordinate system so the content can be rendered at (x,y,w,h) = (0,0,1,1)
    glPushMatrix();

    glTranslatef(x_, y_, 0.);
    glScalef(w_, h_, 1.);

    // render the factory object
    renderFactoryObject(tX, tY, tW, tH);

    glPopMatrix();

    // render the border
    double border = 0.0025;

    glPushAttrib(GL_CURRENT_BIT);

    glColor4f(1,1,1,1);
    GLWindow::drawRectangle(x_-border,y_-border,w_+2.*border,h_+2.*border);

    glPopAttrib();
}

boost::shared_ptr<Content> Content::getContent(std::string uri)
{
    // see if this is an image
    QImageReader imageReader(uri.c_str());

    if(imageReader.canRead() == true)
    {
        // get its size
        QSize size = imageReader.size();

        // small images will use Texture; larger images will use DynamicTexture
        boost::shared_ptr<Content> c;

        if(size.width() <= 4096 && size.height() <= 4096)
        {
            boost::shared_ptr<Content> temp(new TextureContent(uri));
            c = temp;
        }
        else
        {
            boost::shared_ptr<Content> temp(new DynamicTextureContent(uri));
            c = temp;
        }

        return c;
    }
    // see if this is a movie
    // todo: need a better way to determine file type
    else if(QString::fromStdString(uri).endsWith(".mov") || QString::fromStdString(uri).endsWith(".avi") || QString::fromStdString(uri).endsWith(".mp4") || QString::fromStdString(uri).endsWith(".mkv"))
    {
        boost::shared_ptr<Content> c(new MovieContent(uri));

        return c;
    }
    // see if this is an image pyramid
    else if(QString::fromStdString(uri).endsWith(".pyr"))
    {
        boost::shared_ptr<Content> c(new DynamicTextureContent(uri));

        return c;
    }

    // otherwise, return NULL
    return boost::shared_ptr<Content>();
}
