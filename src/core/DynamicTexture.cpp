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

#include "DynamicTexture.h"
#include "globals.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "vector.h"
#include "log.h"

#include <algorithm>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <QImageReader>

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

DynamicTexture::DynamicTexture(QString uri, boost::shared_ptr<DynamicTexture> parent, float parentX, float parentY, float parentW, float parentH, int childIndex)
{
    // defaults
    depth_ = 0;
    useImagePyramid_ = false;
    threadCount_ = 0;
    loadImageThreadStarted_ = false;
    imageWidth_ = 0;
    imageHeight_ = 0;
    textureBound_ = false;

    // assign values
    uri_ = uri;
    parent_ = parent;
    parentX_ = parentX;
    parentY_ = parentY;
    parentW_ = parentW;
    parentH_ = parentH;

    // if we're a child...
    if(parent != NULL)
    {
        depth_ = parent->depth_ + 1;

        // append childIndex to parent's path to form this object's path
        treePath_ = parent->treePath_;
        treePath_.push_back(childIndex);
    }

    // if we're the top-level object
    if(depth_ == 0)
    {
        // this is the top-level object, so its path is 0
        treePath_.push_back(0);

        // see if this is an image pyramid metadata filename
        if(uri.endsWith(".pyr"))
        {
            std::ifstream ifs(uri.toAscii());

            // read the whole line
            std::string lineString;
            getline(ifs, lineString);

            // parse the arguments, allowing escaped characters, quotes, etc., and assign them to a vector
            std::string separator1("\\"); // allow escaped characters
            std::string separator2(" "); // split on spaces
            std::string separator3("\"\'"); // allow quoted arguments

            boost::escaped_list_separator<char> els(separator1, separator2, separator3);
            boost::tokenizer<boost::escaped_list_separator<char> > tok(lineString, els);

            std::vector<std::string> tokVector;
            tokVector.assign(tok.begin(), tok.end());

            if(tokVector.size() < 3)
            {
                put_flog(LOG_ERROR, "require 3 arguments, got %i", tokVector.size());
                return;
            }

            imagePyramidPath_ = tokVector[0];
            imageWidth_ = atoi(tokVector[1].c_str());
            imageHeight_ = atoi(tokVector[2].c_str());

            useImagePyramid_ = true;

            put_flog(LOG_DEBUG, "got image pyramid path %s, imageWidth = %i, imageHeight = %i", imagePyramidPath_.c_str(), imageWidth_, imageHeight_);
        }

        // always load image for top-level object
        incrementThreadCount();
        loadImageThread_ = QtConcurrent::run(loadImageThread, this);
        loadImageThreadStarted_ = true;
    }
}

DynamicTexture::~DynamicTexture()
{
    // delete bound texture
    if(textureBound_ == true)
    {
        // let the OpenGL window delete the texture, so the destructor can occur in any thread...
        g_mainWindow->getGLWindow()->insertPurgeTextureId(textureId_);

        textureBound_ = false;
    }
}

void DynamicTexture::loadImage(bool convertToGLFormat)
{
    // get the root node for later use
    // note that getRoot() is not safe to be called in the root object during construction, since it uses shared_from_this()
    DynamicTexture * root = NULL;

    if(depth_ == 0)
    {
        root = this;
    }
    else
    {
        root = getRoot().get();
    }

    if(root->useImagePyramid_ == true)
    {
        // form filename
        std::string filename = root->imagePyramidPath_ + '/';

        for(unsigned int i=0; i<treePath_.size(); i++)
        {
            filename += QString::number(treePath_[i]).toStdString();

            if(i != treePath_.size() - 1)
            {
                filename += "-";
            }
        }

        filename += ".jpg";

        scaledImage_.load(QString(filename.c_str()), "jpg");
    }
    else
    {
        // root node
        if(depth_ == 0)
        {
            image_.load(uri_);

            if(image_.isNull())
            {
                put_flog(LOG_ERROR, "error loading %s", uri_.toLocal8Bit().constData());
            }
        }
        else
        {
            // get image from parent
            boost::shared_ptr<DynamicTexture> parent = parent_.lock();
            image_ = parent->getImageFromParent(parentX_, parentY_, parentW_, parentH_, this);
        }

        // if we managed to get a valid image, go ahead and scale it
        // otherwise, we'll need to read it in differently...
        if(image_.isNull() != true)
        {
            // save image dimensions for later use; recall image may be deleted
            imageWidth_ = image_.width();
            imageHeight_ = image_.height();

            // compute the scaled image
            scaledImage_ = image_.scaled(TEXTURE_SIZE, TEXTURE_SIZE);

            // only the root needs to keep the non-scaled image in this case
            // we only want to keep the top-most valid image_ in the tree for memory efficiency
            if(depth_ != 0)
            {
                image_ = QImage();
            }
        }
        else
        {
            // we could not get a valid image_ from a parent
            // try alternative methods of reading it using QImageReader
            QImageReader imageReader(root->uri_);

            if(imageReader.canRead() == true)
            {
                put_flog(LOG_DEBUG, "image can be read. trying alternate methods.");

                // get image rectangle for this object in the root's coordinates
                QRect rootRect;

                if(depth_ == 0)
                {
                    rootRect = QRect(0,0, imageReader.size().width(), imageReader.size().height());
                }
                else
                {
                    rootRect = getRootImageCoordinates(0., 0., 1., 1.);
                }

                // save the image dimensions (in terms of the root image) that this object represents
                imageWidth_ = rootRect.width();
                imageHeight_ = rootRect.height();

                put_flog(LOG_DEBUG, "reading clipped region of image");

                imageReader.setClipRect(rootRect);
                image_ = imageReader.read();

                if(image_.isNull() != true)
                {
                    // successfully loaded clipped image
                    // compute the scaled image
                    scaledImage_ = image_.scaled(TEXTURE_SIZE, TEXTURE_SIZE);
                }
                else
                {
                    // failed to load the clipped image
                    put_flog(LOG_DEBUG, "failed to read clipped region of image; attempting to read clipped and scaled region of image");

                    QImageReader imageScaledReader(root->uri_);
                    imageScaledReader.setClipRect(rootRect);
                    imageScaledReader.setScaledSize(QSize(TEXTURE_SIZE, TEXTURE_SIZE));
                    scaledImage_ = imageScaledReader.read();
                }

                // this means we couldn't get a scaled image by any means
                if(scaledImage_.isNull() == true)
                {
                    put_flog(LOG_ERROR, "failed to read the image. aborting.");
                    exit(-1);
                    return;
                }
            }
            else
            {
                put_flog(LOG_ERROR, "image cannot be read. aborting.");
                exit(-1);
                return;
            }
        }
    }

    // optionally convert the image to OpenGL format
    // note that the resulting image can only be used for width(), height(), and bits() calls for OpenGL
    // save(), etc. won't work.
    if(convertToGLFormat == true)
    {
        scaledImage_ = QGLWidget::convertToGLFormat(scaledImage_);
    }
}

void DynamicTexture::getDimensions(int &width, int &height)
{
    // if we don't have a width and height, and the load image thread is running, wait for it to finish
    if(imageWidth_ == 0 && imageHeight_ == 0 && loadImageThreadStarted_ == true)
    {
        loadImageThread_.waitForFinished();
    }

    width = imageWidth_;
    height = imageHeight_;
}

void DynamicTexture::render(float tX, float tY, float tW, float tH, bool computeOnDemand, bool considerChildren)
{
    if(depth_ == 0)
    {
        updateRenderedFrameIndex();
    }

    if(considerChildren == true && getProjectedPixelArea(true) > 0. && getProjectedPixelArea(false) > TEXTURE_SIZE*TEXTURE_SIZE && (getRoot()->imageWidth_ / pow(2,depth_) > TEXTURE_SIZE || getRoot()->imageHeight_ / pow(2,depth_) > TEXTURE_SIZE))
    {
        // mark this object as having rendered children in this frame
        renderChildrenFrameCount_ = g_frameCount;

        renderChildren(tX,tY,tW,tH);
    }
    else
    {
        // want to render this object

        // see if we need to start loading the image
        if(computeOnDemand == true && loadImageThreadStarted_ == false)
        {
            // only start the thread if this DynamicTexture tree has one available
            // each DynamicTexture tree is limited to (maxThreads - 2) threads, where the max is determined by the global QThreadPool instance
            // we increase responsiveness / interactivity by not queuing up image loading
            // todo: this doesn't perform well with too many threads; restricting to 1 thread for now
            int maxThreads = 1; // std::max(QThreadPool::globalInstance()->maxThreadCount() - 2, 1);

            if(getThreadCount() < maxThreads)
            {
                incrementThreadCount();

                // give the thread shared_ptr's to all of this object's parents to prevent their destruction during thread execution
                std::vector<boost::shared_ptr<DynamicTexture> > objects;
                getObjectsAscending(objects);

                loadImageThread_ = QtConcurrent::run(loadImageThread, shared_from_this(), objects);
                loadImageThreadStarted_ = true;
            }
        }

        // see if we need to load the texture
        if(loadImageThreadStarted_ == true && loadImageThread_.isFinished() == true && textureBound_ == false)
        {
            uploadTexture();
        }

        // if we don't yet have a texture, try to render from parent's texture
        // however, we won't force an image/texture computation on the parent
        if(textureBound_ == false)
        {
            // render from parent if we can
            boost::shared_ptr<DynamicTexture> parent = parent_.lock();

            if(parent != NULL)
            {
                float pX = parentX_ + tX * parentW_;
                float pY = parentY_ + tY * parentH_;
                float pW = tW * parentW_;
                float pH = tH * parentH_;

                parent->render(pX, pY, pW, pH, false, false);
            }
        }
        else
        {
#ifdef DYNAMIC_TEXTURE_SHOW_BORDER
            // draw the border
            glPushAttrib(GL_CURRENT_BIT);

            glColor4f(0.,1.,0.,1.);

            glBegin(GL_LINE_LOOP);
            glVertex2f(0.,0.);
            glVertex2f(1.,0.);
            glVertex2f(1.,1.);
            glVertex2f(0.,1.);
            glEnd();

            glPopAttrib();
#endif

            // draw the texture
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureId_);

            glBegin(GL_QUADS);

            // note we need to flip the y coordinate since the textures are loaded upside down
            glTexCoord2f(tX,1.-tY);
            glVertex2f(0.,0.);

            glTexCoord2f(tX+tW,1.-tY);
            glVertex2f(1.,0.);

            glTexCoord2f(tX+tW,1.-(tY+tH));
            glVertex2f(1.,1.);

            glTexCoord2f(tX,1.-(tY+tH));
            glVertex2f(0.,1.);

            glEnd();

            glPopAttrib();
        }
    }
}

void DynamicTexture::clearOldChildren(uint64_t minFrameCount)
{
    // clear children if renderChildrenFrameCount_ < minFrameCount
    if(children_.size() > 0 && renderChildrenFrameCount_ < minFrameCount && getThreadsDoneDescending())
    {
        children_.clear();
    }

    // run on my children (if i still have any)
    for(unsigned int i=0; i<children_.size(); i++)
    {
        children_[i]->clearOldChildren(minFrameCount);
    }
}

void DynamicTexture::computeImagePyramid(std::string imagePyramidPath)
{
    if(depth_ == 0)
    {
        // make directory if necessary
        if(QDir(imagePyramidPath.c_str()).exists() != true)
        {
            bool success = QDir().mkdir(imagePyramidPath.c_str());

            if(success != true)
            {
                put_flog(LOG_ERROR, "error creating directory %s", imagePyramidPath.c_str());
                return;
            }
        }

        // wait for initial image load to finish thread
        loadImageThread_.waitForFinished();

        // write metadata file
        std::string metadataFilename = imagePyramidPath + "/pyramid.pyr";

        std::ofstream ofs(metadataFilename.c_str());
        ofs << "\"" << imagePyramidPath << "\" " << imageWidth_ << " " << imageHeight_;

        // write a more conveniently named metadata file in the same directory as the original image, if possible
        // path ends with ".pyramid"; the new metadata file will end with ".pyr"
        QString secondMetadataFilename = QString(imagePyramidPath.c_str());
        int amidLastIndex = secondMetadataFilename.lastIndexOf("amid");

        secondMetadataFilename.truncate(amidLastIndex);

        std::ofstream secondOfs(secondMetadataFilename.toStdString().c_str());

        if(secondOfs.good() == true)
        {
            secondOfs << "\"" << imagePyramidPath << "\" " << imageWidth_ << " " << imageHeight_;
        }
        else
        {
            put_flog(LOG_WARN, "could not write second metadata file %s", secondMetadataFilename.toStdString().c_str());
        }
    }

    // generate this object's image and write to disk

    // this will give us scaledImage_
    // don't convert scaledImage_ to the GL format; we need to be able to save it
    // note that for depth_ == 0 we already have scaledImage_, but it is in the OpenGL format
    // so, we need to load it again in the non-OpenGL format so we're able to save it
    // todo: in the future it might be nice not to require the re-loading of the image for depth_ == 0
    loadImage(false);

    // form filename
    std::string filename = imagePyramidPath + '/';

    for(unsigned int i=0; i<treePath_.size(); i++)
    {
        filename += QString::number(treePath_[i]).toStdString();

        if(i != treePath_.size() - 1)
        {
            filename += "-";
        }
    }

    filename += ".jpg";

    put_flog(LOG_DEBUG, "saving %s", filename.c_str());

    scaledImage_.save(QString(filename.c_str()), "jpg");

    // no longer need scaled image
    scaledImage_ = QImage();

    // if we need to descend further...
    if(getRoot()->imageWidth_ / pow(2,depth_) > TEXTURE_SIZE || getRoot()->imageHeight_ / pow(2,depth_) > TEXTURE_SIZE)
    {
        // image rectangle a child quadrant contains
        QRectF imageBounds[4];
        imageBounds[0] = QRectF(0.,0.,0.5,0.5);
        imageBounds[1] = QRectF(0.5,0.,0.5,0.5);
        imageBounds[2] = QRectF(0.5,0.5,0.5,0.5);
        imageBounds[3] = QRectF(0.,0.5,0.5,0.5);

        // generate and compute children
#pragma omp parallel for
        for(unsigned int i=0; i<4; i++)
        {
            boost::shared_ptr<DynamicTexture> c(new DynamicTexture("", shared_from_this(), imageBounds[i].x(), imageBounds[i].y(), imageBounds[i].width(), imageBounds[i].height(), i));

            c->computeImagePyramid(imagePyramidPath);
        }
    }
}

void DynamicTexture::decrementThreadCount()
{
    if(depth_ == 0)
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ - 1;
    }
    else
    {
        return getRoot()->decrementThreadCount();
    }
}

boost::shared_ptr<DynamicTexture> DynamicTexture::getRoot()
{
    if(depth_ == 0)
    {
        return shared_from_this();
    }
    else
    {
        boost::shared_ptr<DynamicTexture> parent = parent_.lock();
        return parent->getRoot();
    }
}

void DynamicTexture::getObjectsAscending(std::vector<boost::shared_ptr<DynamicTexture> > &objects)
{
    // order from parent -> child; e.g. depths 0,1,2,3...

    objects.insert(objects.begin(), shared_from_this());

    // get the shared_ptr from weak_ptr
    boost::shared_ptr<DynamicTexture> parent = parent_.lock();

    if(parent != NULL)
    {
        parent->getObjectsAscending(objects);
    }
}

QRect DynamicTexture::getRootImageCoordinates(float x, float y, float w, float h)
{
    if(depth_ == 0)
    {
        // if necessary, block and wait for image loading to complete
        if(loadImageThreadStarted_ == true && loadImageThread_.isFinished() == false)
        {
            loadImageThread_.waitForFinished();
        }

        QRect rect = QRect(x*imageWidth_, y*imageHeight_, w*imageWidth_, h*imageHeight_);
        return rect;
    }
    else
    {
        boost::shared_ptr<DynamicTexture> parent = parent_.lock();

        float pX = parentX_ + x * parentW_;
        float pY = parentY_ + y * parentH_;
        float pW = w * parentW_;
        float pH = h * parentH_;

        return parent->getRootImageCoordinates(pX, pY, pW, pH);
    }
}

QImage DynamicTexture::getImageFromParent(float x, float y, float w, float h, DynamicTexture * start)
{
    // if we're in the starting node, we must ascend
    if(start == this)
    {
        if(depth_ == 0)
        {
            put_flog(LOG_ERROR, "starting from root object and cannot ascend");
            return QImage();
        }

        boost::shared_ptr<DynamicTexture> parent = parent_.lock();

        float pX = parentX_ + x * parentW_;
        float pY = parentY_ + y * parentH_;
        float pW = w * parentW_;
        float pH = h * parentH_;

        return parent->getImageFromParent(pX, pY, pW, pH, start);
    }

    // wait for the load image thread to complete if it's in progress
    if(loadImageThreadStarted_ == true && loadImageThread_.isFinished() == false)
    {
        loadImageThread_.waitForFinished();
    }

    if(image_.isNull() != true)
    {
        // we have a valid image, return the clipped image
        QImage copy = image_.copy(x*image_.width(), y*image_.height(), w*image_.width(), h*image_.height());
        return copy;
    }
    else
    {
        // we don't have a valid image
        // if we're the root object, return a NULL image
        // otherwise, continue up the tree looking for an image
        if(depth_ == 0)
        {
            return QImage();
        }
        else
        {
            boost::shared_ptr<DynamicTexture> parent = parent_.lock();

            float pX = parentX_ + x * parentW_;
            float pY = parentY_ + y * parentH_;
            float pW = w * parentW_;
            float pH = h * parentH_;

            return parent->getImageFromParent(pX, pY, pW, pH, start);
        }
    }
}

void DynamicTexture::uploadTexture()
{
    // generate new texture
    // no need to compute mipmaps
    // note that scaledImage_ is already in the GL format so we can use glTexImage2D directly
    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaledImage_.width(), scaledImage_.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledImage_.bits());

    // linear min / max filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    textureBound_ = true;

    // no longer need the scaled image
    scaledImage_ = QImage();
}

void DynamicTexture::renderChildren(float tX, float tY, float tW, float tH)
{
    // texture rectangle we're showing with this parent object
    QRectF textureRect(tX,tY,tW,tH);

    // children rectangles
    float inf = 1000000.;

    // texture rectangle a child quadrant may contain
    QRectF textureBounds[4];
    textureBounds[0].setCoords(-inf,-inf, 0.5,0.5);
    textureBounds[1].setCoords(0.5,-inf, inf,0.5);
    textureBounds[2].setCoords(0.5,0.5, inf,inf);
    textureBounds[3].setCoords(-inf,0.5, 0.5,inf);

    // image rectange a child quadrant contains
    QRectF imageBounds[4];
    imageBounds[0] = QRectF(0.,0.,0.5,0.5);
    imageBounds[1] = QRectF(0.5,0.,0.5,0.5);
    imageBounds[2] = QRectF(0.5,0.5,0.5,0.5);
    imageBounds[3] = QRectF(0.,0.5,0.5,0.5);

    // see if we need to generate children
    if(children_.size() == 0)
    {
        for(unsigned int i=0; i<4; i++)
        {
            boost::shared_ptr<DynamicTexture> c(new DynamicTexture("", shared_from_this(), imageBounds[i].x(), imageBounds[i].y(), imageBounds[i].width(), imageBounds[i].height(), i));
            children_.push_back(c);
        }
    }

    // render children
    for(unsigned int i=0; i<children_.size(); i++)
    {
        // portion of texture for this child
        QRectF childTextureRect = textureRect.intersected(textureBounds[i]);

        // translate and scale to child texture coordinates
        QRectF childTextureRectTranslated = childTextureRect.translated(-imageBounds[i].x(), -imageBounds[i].y());

        QRectF childTextureRectTranslatedAndScaled(childTextureRectTranslated.x() / imageBounds[i].width(), childTextureRectTranslated.y() / imageBounds[i].height(), childTextureRectTranslated.width() / imageBounds[i].width(), childTextureRectTranslated.height() / imageBounds[i].height());

        // find rendering position based on portion of textureRect we occupy
        // recall the parent object (this one) is rendered as a (0,0,1,1) rectangle
        QRectF renderRect((childTextureRect.x()-textureRect.x()) / textureRect.width(), (childTextureRect.y()-textureRect.y()) / textureRect.height(), childTextureRect.width() / textureRect.width(), childTextureRect.height() / textureRect.height());

        glPushMatrix();
        glTranslatef(renderRect.x(), renderRect.y(), 0.);
        glScalef(renderRect.width(), renderRect.height(), 1.);

        children_[i]->render(childTextureRectTranslatedAndScaled.x(), childTextureRectTranslatedAndScaled.y(), childTextureRectTranslatedAndScaled.width(), childTextureRectTranslatedAndScaled.height());

        glPopMatrix();
    }
}

double DynamicTexture::getProjectedPixelArea(bool onScreenOnly)
{
    // get four corners in object space (recall we're in normalized 0->1 dimensions)
    double x[4][3];

    x[0][0] = 0.;
    x[0][1] = 0.;
    x[0][2] = 0.;

    x[1][0] = 1.;
    x[1][1] = 0.;
    x[1][2] = 0.;

    x[2][0] = 1.;
    x[2][1] = 1.;
    x[2][2] = 0.;

    x[3][0] = 0.;
    x[3][1] = 1.;
    x[3][2] = 0.;

    // get four corners in screen space
    GLdouble modelview[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    GLdouble projection[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble xWin[4][3];

    for(int i=0; i<4; i++)
    {
        gluProject(x[i][0], x[i][1], x[i][2], modelview, projection, viewport, &xWin[i][0], &xWin[i][1], &xWin[i][2]);

        if(onScreenOnly == true)
        {
            // clamp to on-screen portion
            if(xWin[i][0] < 0.)
                xWin[i][0] = 0.;

            if(xWin[i][0] > (double)g_mainWindow->getGLWindow()->width())
                xWin[i][0] = (double)g_mainWindow->getGLWindow()->width();

            if(xWin[i][1] < 0.)
                xWin[i][1] = 0.;

            if(xWin[i][1] > (double)g_mainWindow->getGLWindow()->height())
                xWin[i][1] = (double)g_mainWindow->getGLWindow()->height();
        }
    }

    // get area from two triangles
    // use this method to accomodate warped / transformed views in screen space
    double vec1[3];
    vec1[0] = xWin[1][0] - xWin[0][0];
    vec1[1] = xWin[1][1] - xWin[0][1];
    vec1[2] = xWin[1][2] - xWin[0][2];

    double vec2[3];
    vec2[0] = xWin[2][0] - xWin[0][0];
    vec2[1] = xWin[2][1] - xWin[0][1];
    vec2[2] = xWin[2][2] - xWin[0][2];

    double vec3[3];
    vec3[0] = xWin[3][0] - xWin[0][0];
    vec3[1] = xWin[3][1] - xWin[0][1];
    vec3[2] = xWin[3][2] - xWin[0][2];

    double cp[3];

    vectorCrossProduct(vec1, vec2, cp);
    double A1 = 0.5 * vectorMagnitude(cp);

    vectorCrossProduct(vec1, vec3, cp);
    double A2 = 0.5 * vectorMagnitude(cp);

    double A = A1 + A2;

    return A;
}

bool DynamicTexture::getThreadsDoneDescending()
{
    if(loadImageThread_.isFinished() == false)
    {
        return false;
    }

    for(unsigned int i=0; i<children_.size(); i++)
    {
        if(children_[i]->getThreadsDoneDescending() == false)
        {
            return false;
        }
    }

    return true;
}

int DynamicTexture::getThreadCount()
{
    if(depth_ == 0)
    {
        QMutexLocker locker(&threadCountMutex_);
        return threadCount_;
    }
    else
    {
        return getRoot()->getThreadCount();
    }
}

void DynamicTexture::incrementThreadCount()
{
    if(depth_ == 0)
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ + 1;
    }
    else
    {
        return getRoot()->incrementThreadCount();
    }
}

void loadImageThread(boost::shared_ptr<DynamicTexture> dynamicTexture, std::vector<boost::shared_ptr<DynamicTexture> > objects)
{
    loadImageThread(dynamicTexture.get());
    return;
}

void loadImageThread(DynamicTexture * dynamicTexture)
{
    dynamicTexture->loadImage();
    dynamicTexture->decrementThreadCount();
    return;
}
