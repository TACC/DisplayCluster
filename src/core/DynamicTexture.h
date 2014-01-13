/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef DYNAMIC_TEXTURE_H
#define DYNAMIC_TEXTURE_H

// todo: get this dynamically
#define TEXTURE_SIZE 512

// define this to show borders around image tiles
#undef DYNAMIC_TEXTURE_SHOW_BORDER

#include "FactoryObject.h"
#include <QGLWidget>
#include <QtConcurrentRun>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class DynamicTexture : public boost::enable_shared_from_this<DynamicTexture>, public FactoryObject {

    public:

        DynamicTexture(QString uri = "", boost::shared_ptr<DynamicTexture> parent = boost::shared_ptr<DynamicTexture>(), float parentX=0., float parentY=0., float parentW=0., float parentH=0., int childIndex=0);
        ~DynamicTexture();

        void loadImage(bool convertToGLFormat=true); // thread needs access to this method
        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH, bool computeOnDemand=true, bool considerChildren=true);
        void clearOldChildren(uint64_t minFrameCount); // clear children of nodes with renderChildrenFrameCount_ < minFrameCount
        void computeImagePyramid(std::string imagePyramidPath);
        void decrementThreadCount(); // thread needs access to this method

    private:

        int depth_;

        // for root only: image location
        QString uri_;

        // image pyramid parameters
        std::string imagePyramidPath_;
        bool useImagePyramid_;

        // thread count
        int threadCount_;
        QMutex threadCountMutex_;

        // for children:

        // pointer to parent object, if we have one
        boost::weak_ptr<DynamicTexture> parent_;

        // image coordinates in parent image
        float parentX_, parentY_, parentW_, parentH_;

        // for all objects:

        // path through the tree
        std::vector<int> treePath_;

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

        // last children render frame count
        uint64_t renderChildrenFrameCount_;

        boost::shared_ptr<DynamicTexture> getRoot();
        void getObjectsAscending(std::vector<boost::shared_ptr<DynamicTexture> > &objects);
        QRect getRootImageCoordinates(float x, float y, float w, float h);
        QImage getImageFromParent(float x, float y, float w, float h, DynamicTexture * start);
        void uploadTexture();
        void renderChildren(float tX, float tY, float tW, float tH);
        double getProjectedPixelArea(bool onScreenOnly);
        bool getThreadsDoneDescending();
        int getThreadCount();
        void incrementThreadCount();
};

// this wrapper is used by child objects to prevent its ancestors from being destructed during thread execution.
extern void loadImageThread(boost::shared_ptr<DynamicTexture> dynamicTexture, std::vector<boost::shared_ptr<DynamicTexture> > objects);

extern void loadImageThread(DynamicTexture * dynamicTexture);

#endif
