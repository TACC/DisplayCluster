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

#if !defined(SIP_SIP_SIP)
BOOST_CLASS_EXPORT_GUID(TextureContent, "TextureContent")
#endif

typedef boost::shared_ptr<TextureContent> pTextureContent;

class pyTextureContent : public pyContent
{
public:
  pyTextureContent(const char *uri) {ptr = pTextureContent(new TextureContent(std::string(uri)));}
  ~pyTextureContent() {}
  pTextureContent get() {return pTextureContent(static_cast<TextureContent *>(ptr.get()));}
};

#endif
