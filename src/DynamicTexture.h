#ifndef DYNAMIC_TEXTURE_H
#define DYNAMIC_TEXTURE_H

// todo: get this dynamically
#define TEXTURE_SIZE 512

#include <QGLWidget>
#include <QtConcurrentRun>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class DynamicTexture : public boost::enable_shared_from_this<DynamicTexture> {

    public:

        DynamicTexture(std::string uri = "", boost::shared_ptr<DynamicTexture> parent = boost::shared_ptr<DynamicTexture>(), float parentX=0., float parentY=0., float parentW=0., float parentH=0.);
        ~DynamicTexture();

        void loadImage(); // thread needs access to this method
        void render(float tX, float tY, float tW, float tH, bool computeOnDemand=true, bool considerChildren=true);

    private:

        int depth_;

        // for root only: image location
        std::string uri_;

        // for children:

        // pointer to parent object, if we have one
        boost::weak_ptr<DynamicTexture> parent_;

        // image coordinates in parent image
        float parentX_, parentY_, parentW_, parentH_;

        // for all objects:

        // thread for loading images
        QFuture<void> loadImageThread_;
        bool loadImageThreadStarted_;

        // full scale image and dimensions; image may be deleted, but dimensions are necessary for later use
        QImage image_;
        int imageWidth_;
        int imageHeight_;

        // scaled image used for texture construction
        QImage scaledImage_;

        // texture information
        bool textureBound_;
        GLuint textureId_;

        // children
        std::vector<boost::shared_ptr<DynamicTexture> > children_;

        boost::shared_ptr<DynamicTexture> getRoot();
        QImage getImageFromRoot(float x, float y, float w, float h);
        void uploadTexture();
        void renderChildren(float tX, float tY, float tW, float tH);
        double getProjectedPixelArea();
        bool getThreadsDoneDescending();
};

extern void loadImageThread(DynamicTexture * dynamicTexture);

#endif
