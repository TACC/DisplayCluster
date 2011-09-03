#ifndef TEXTURE_FACTORY_H
#define TEXTURE_FACTORY_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

class DynamicTexture;

class TextureFactory {

    public:

        boost::shared_ptr<DynamicTexture> getTexture(std::string uri);

    private:

        // all existing textures
        std::map<std::string, boost::shared_ptr<DynamicTexture> > map_;
};

#endif
