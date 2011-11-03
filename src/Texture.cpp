#include "Texture.h"
#include "main.h"
#include "log.h"

Texture::Texture(std::string uri)
{
    // defaults
    textureBound_ = false;

    // assign values
    uri_ = uri;

    QImage image(uri_.c_str());

    if(image.isNull() == true)
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.c_str());
        return;
    }

    // save image dimensions
    imageWidth_ = image.width();
    imageHeight_ = image.height();

    // generate new texture
    textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::DefaultBindOption);
    textureBound_ = true;
}

Texture::~Texture()
{
    // delete bound texture
    if(textureBound_ == true)
    {
        glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);
        textureBound_ = false;
    }
}

void Texture::getDimensions(int &width, int &height)
{
    width = imageWidth_;
    height = imageHeight_;
}

void Texture::render(float tX, float tY, float tW, float tH)
{
    if(textureBound_ == true)
    {
        // draw the texture
        glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureId_);

        // on zoom-out, clamp to border (instead of showing the texture tiled / repeated)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glBegin(GL_QUADS);

        // note we need to flip the y coordinate since the textures are loaded upside down
        glTexCoord2f(tX,1.-tY);
        glVertex2f(0.,0.);

        glTexCoord2f(tX+tW,1.-tY);
        glVertex2f(1.,0.);

        glTexCoord2f(tX+tW,1.-(tY+tH));
        glVertex2f(1.,1.);

        glTexCoord2f(tX,1.-(tY+tH));
        glVertex2f(0.,1.);

        glEnd();

        glPopAttrib();
    }
}
