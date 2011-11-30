#ifndef CONTENT_H
#define CONTENT_H

#include <string>
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/assume_abstract.hpp>

class ContentWindowManager;

class Content : public QObject {
    Q_OBJECT

    public:

        Content(std::string uri = "");

        std::string getURI();

        void getDimensions(int &width, int &height);
        void setDimensions(int width, int height);
        virtual void getFactoryObjectDimensions(int &width, int &height) = 0;

        void render(boost::shared_ptr<ContentWindowManager>);

        // virtual method for implementing actions on advancing to a new frame
        // useful when a process has multiple GLWindows
        virtual void advance(boost::shared_ptr<ContentWindowManager> ) { }

        // get a Content object of the appropriate derived type based on the URI given
        static boost::shared_ptr<Content> getContent(std::string uri);

    signals:

        void dimensionsChanged(int width, int height);

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & uri_;
            ar & width_;
            ar & height_;
        }

        std::string uri_;
        int width_;
        int height_;

        virtual void renderFactoryObject(float tX, float tY, float tW, float tH) = 0;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Content)

typedef boost::shared_ptr<Content> pContent;

class pyContent {
public:
  pyContent(const char *str) { ptr = boost::shared_ptr<Content>(Content::getContent((std::string)str)); }
  pyContent(boost::shared_ptr<Content> c) { ptr = c; }

  const char *getURI() {return (const char *)get()->getURI().c_str(); }
  
  boost::shared_ptr<Content> get() {return ptr;}
  void dump() {
    std::cerr << "pyContent: " << ptr->getURI() << "\n";
  }

protected:
  boost::shared_ptr<Content> ptr;
};


#endif
