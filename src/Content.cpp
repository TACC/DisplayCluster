#include "Content.h"
#include "ContentWindow.h"
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "MovieContent.h"
#include <QGLWidget>

Content::Content(std::string uri)
{
    uri_ = uri;
    width_ = 0;
    height_ = 0;
}

std::string Content::getURI()
{
    return uri_;
}

void Content::getDimensions(int &width, int &height)
{
    width = width_;
    height = height_;
}

void Content::setDimensions(int width, int height)
{
    width_ = width;
    height_ = height;

    emit(dimensionsChanged(width_, height_));
}

void Content::render(boost::shared_ptr<ContentWindow> window)
{
    // get parameters from window
    double x, y, w, h;
    window->getCoordinates(x, y, w, h);

    double centerX, centerY;
    window->getCenter(centerX, centerY);

    double zoom = window->getZoom();

    // calculate texture coordinates
    float tX = centerX - 0.5 / zoom;
    float tY = centerY - 0.5 / zoom;
    float tW = 1./zoom;
    float tH = 1./zoom;

    // transform to a normalize coordinate system so the content can be rendered at (x,y,w,h) = (0,0,1,1)
    glPushMatrix();

    glTranslatef(x, y, 0.);
    glScalef(w, h, 1.);

    // render the factory object
    renderFactoryObject(tX, tY, tW, tH);

    glPopMatrix();
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
    else if(QString::fromStdString(uri).endsWith(".mov") || QString::fromStdString(uri).endsWith(".avi") || QString::fromStdString(uri).endsWith(".mp4") || QString::fromStdString(uri).endsWith(".mkv") || QString::fromStdString(uri).endsWith(".mpg") || QString::fromStdString(uri).endsWith(".flv"))
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
