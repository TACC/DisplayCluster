#ifndef TEXTURE_FACTORY_H
#define TEXTURE_FACTORY_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

class Texture;

class TextureFactory {

    public:

        boost::shared_ptr<Texture> getTexture(std::string uri);

    private:

        // all existing textures
        std::map<std::string, boost::shared_ptr<Texture> > map_;
};

#endif
