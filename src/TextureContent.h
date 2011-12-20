#ifndef TEXTURE_CONTENT_H
#define TEXTURE_CONTENT_H

#include "Content.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class TextureContent : public Content {

    public:
        TextureContent(std::string uri = "") : Content(uri) { }

        void getFactoryObjectDimensions(int &width, int &height);

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            // serialize base class information
            ar & boost::serialization::base_object<Content>(*this);
        }

        void renderFactoryObject(float tX, float tY, float tW, float tH);
};

BOOST_CLASS_EXPORT_GUID(TextureContent, "TextureContent")

#endif
