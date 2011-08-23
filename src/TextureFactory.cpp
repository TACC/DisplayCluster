#include "TextureFactory.h"
#include "main.h"
#include "log.h"

TextureFactory::TextureFactory()
{

}

GLuint TextureFactory::getTexture(std::string uri)
{
    // see if we need to create the texture
    if(map_.count(uri) == 0)
    {
        QImage image(uri.c_str());

        if(image.isNull() == true)
        {
            put_flog(LOG_ERROR, "error loading %s", uri.c_str());
            return 0;
        }

        map_[uri] = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::DefaultBindOption);
    }

    return map_[uri];
}
