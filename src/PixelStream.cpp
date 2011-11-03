#include "PixelStream.h"
#include "main.h"
#include "log.h"

PixelStream::PixelStream(std::string uri)
{
    // defaults
    textureId_ = 0;
    textureWidth_ = 0;
    textureHeight_ = 0;
    textureBound_ = false;
    imageReady_ = false;

    // assign values
    uri_ = uri;
}

PixelStream::~PixelStream()
{
    if(textureBound_ == true)
    {
        // delete bound texture
        glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);
    }
}

void PixelStream::getDimensions(int &width, int &height)
{
    width = textureWidth_;
    height = textureHeight_;
}

void PixelStream::render(float tX, float tY, float tW, float tH)
{
    imageReadyMutex_.lock();

    if(imageReady_ == true)
    {
        setImage(image_);
        imageReady_ = false;
    }

    imageReadyMutex_.unlock();

    if(textureBound_ != true)
    {
        return;
    }

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to border (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBegin(GL_QUADS);

    glTexCoord2f(tX,tY);
    glVertex2f(0.,0.);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(1.,0.);

    glTexCoord2f(tX+tW,tY+tH);
    glVertex2f(1.,1.);

    glTexCoord2f(tX,tY+tH);
    glVertex2f(0.,1.);

    glEnd();

    glPopAttrib();
}

void PixelStream::setImageData(QByteArray imageData)
{
    // drop frames if we're currently processing
    if(loadImageDataThread_.isRunning() == true)
    {
        return;
    }

    loadImageDataThread_ = QtConcurrent::run(loadImageDataThread, this, imageData);
}

void PixelStream::imageReady(QImage image)
{
    QMutexLocker locker(&imageReadyMutex_);
    imageReady_ = true;
    image_ = image;
}

void PixelStream::setImage(QImage & image)
{
    // todo: consider if the image has changed dimensions

    if(textureBound_ == false)
    {
        // want mipmaps disabled
        textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
        textureWidth_ = image.width();
        textureHeight_ = image.height();
        textureBound_ = true;
    }
    else
    {
        // update the texture. note that generally we would need to convert the image to an OpenGL supported format
        // however, we're lucky and can use GL_BGRA on the original image...
        // example conversion to GL format: QImage glImage = QGLWidget::convertToGLFormat(image);

        // if the size has changed, create a new texture
        if(image.width() != textureWidth_ || image.height() != textureHeight_)
        {
            // delete bound texture
            glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
            g_mainWindow->getGLWindow()->deleteTexture(textureId_);

            textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
            textureWidth_ = image.width();
            textureHeight_ = image.height();
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, textureId_);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, image.width(), image.height(), GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
        }
    }
}

void loadImageDataThread(PixelStream * pixelStream, QByteArray imageData)
{
    QImage image;
    image.loadFromData((const uchar *)imageData.data(), imageData.size(), "JPEG");

    pixelStream->imageReady(image);
}
