#include "TextureFactory.h"
#include "DynamicTexture.h"

boost::shared_ptr<DynamicTexture> TextureFactory::getTexture(std::string uri)
{
    // see if we need to create the texture
    if(map_.count(uri) == 0)
    {
        boost::shared_ptr<DynamicTexture> dt(new DynamicTexture(uri));

        map_[uri] = dt;
    }

    return map_[uri];
}
