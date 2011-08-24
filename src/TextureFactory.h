#ifndef TEXTURE_FACTORY_H
#define TEXTURE_FACTORY_H

#include <QGLWidget>
#include <map>
#include <string>

class TextureFactory {

    public:

        GLuint getTexture(std::string uri);

    private:

        // all existing textures
        std::map<std::string, GLuint> map_;
};

#endif
