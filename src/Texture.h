#ifndef TEXTURE_H
#define TEXTURE_H

#include <QGLWidget>

class Texture {

    public:

        Texture(std::string uri);
        ~Texture();

        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH);

    private:

        // image location
        std::string uri_;

        // original image dimensions
        int imageWidth_;
        int imageHeight_;

        // texture information
        bool textureBound_;
        GLuint textureId_;
};

#endif
