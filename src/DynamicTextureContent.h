#ifndef DYNAMIC_TEXTURE_CONTENT_H
#define DYNAMIC_TEXTURE_CONTENT_H

#include "Content.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class DynamicTextureContent : public Content {

    public:
        DynamicTextureContent(std::string uri = "") : Content(uri) { }

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            // serialize base class information
            ar & boost::serialization::base_object<Content>(*this);
        }

        void renderFactoryObject(float tX, float tY, float tW, float tH);
};

BOOST_CLASS_EXPORT_GUID(DynamicTextureContent, "DynamicTextureContent")

#endif
