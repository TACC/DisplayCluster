#include "TextureFactory.h"
#include "Texture.h"

boost::shared_ptr<Texture> TextureFactory::getTexture(std::string uri)
{
    // see if we need to create the texture
    if(map_.count(uri) == 0)
    {
        boost::shared_ptr<Texture> t(new Texture(uri));

        map_[uri] = t;
    }

    return map_[uri];
}
