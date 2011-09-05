#ifndef DYNAMIC_TEXTURE_FACTORY_H
#define DYNAMIC_TEXTURE_FACTORY_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

class DynamicTexture;

class DynamicTextureFactory {

    public:

        boost::shared_ptr<DynamicTexture> getDynamicTexture(std::string uri);

    private:

        // all existing textures
        std::map<std::string, boost::shared_ptr<DynamicTexture> > map_;
};

#endif
