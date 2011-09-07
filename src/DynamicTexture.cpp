#include "DynamicTexture.h"
#include "main.h"
#include "vector.h"
#include "log.h"
#include <algorithm>

DynamicTexture::DynamicTexture(std::string uri, boost::shared_ptr<DynamicTexture> parent, float parentX, float parentY, float parentW, float parentH)
{
    // defaults
    depth_ = 0;
    loadImageThreadStarted_ = false;
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
    }

    // always load image for top-level object
    if(depth_ == 0)
    {
        loadImageThread_ = QtConcurrent::run(loadImageThread, this);
        loadImageThreadStarted_ = true;
    }
}

DynamicTexture::~DynamicTexture()
{
    // delete bound texture
    if(textureBound_ == true)
    {
        glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);
        textureBound_ = false;
    }
}

void DynamicTexture::loadImage()
{
    // root node
    if(depth_ == 0)
    {
        image_.load(uri_.c_str());

        if(image_.isNull() == true)
        {
            put_flog(LOG_ERROR, "error loading %s", uri_.c_str());
            return;
        }
    }
    else
    {
        // get image from parent
        boost::shared_ptr<DynamicTexture> parent = parent_.lock();
        image_ = parent->getImageFromRoot(parentX_, parentY_, parentW_, parentH_);
    }

    // save image dimensions for later use; recall image may be deleted
    imageWidth_ = image_.width();
    imageHeight_ = image_.height();

    // compute the scaled image
    scaledImage_ = image_.scaled(TEXTURE_SIZE, TEXTURE_SIZE);

    // only the root needs to keep the non-scaled image
    if(depth_ != 0)
    {
        image_ = QImage();
    }
}

void DynamicTexture::render(float tX, float tY, float tW, float tH, bool computeOnDemand, bool considerChildren)
{
    if(considerChildren == true && getProjectedPixelArea() > TEXTURE_SIZE*TEXTURE_SIZE && (getRoot()->imageWidth_ / pow(2,depth_) > TEXTURE_SIZE || getRoot()->imageHeight_ / pow(2,depth_) > TEXTURE_SIZE)) // todo: need to scale projected pixel area by shown texture width and height (clamped to 0->1)!
    {
        renderChildren(tX,tY,tW,tH);
    }
    else
    {
        // want to render this object

        // see if we need to start loading the image
        if(computeOnDemand == true && loadImageThreadStarted_ == false)
        {
            loadImageThread_ = QtConcurrent::run(loadImageThread, this);
            loadImageThreadStarted_ = true;
        }

        // see if we need to load the texture
        if(loadImageThreadStarted_ == true && loadImageThread_.isFinished() == true && textureBound_ == false)
        {
            uploadTexture();
        }

        // see if we have children we can delete (i.e., we're not trying to render them)
        // also, make sure no threads are computing on them
        if(considerChildren == true && getThreadsDoneDescending() == true)
        {
            children_.clear();
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

            // draw the texture
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureId_);

            // on zoom-out, clamp to border (instead of showing the texture tiled / repeated)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

QImage DynamicTexture::getImageFromRoot(float x, float y, float w, float h)
{
    if(depth_ == 0)
    {
        // if necessary, block and wait for image loading to complete
        if(loadImageThreadStarted_ == true && loadImageThread_.isFinished() == false)
        {
            loadImageThread_.waitForFinished();
        }

        QImage copy = image_.copy(x*image_.width(), y*image_.height(), w*image_.width(), h*image_.height());
        return copy;
    }
    else
    {
        boost::shared_ptr<DynamicTexture> parent = parent_.lock();

        float pX = parentX_ + x * parentW_;
        float pY = parentY_ + y * parentH_;
        float pW = w * parentW_;
        float pH = h * parentH_;

        return parent->getImageFromRoot(pX, pY, pW, pH);
    }
}

void DynamicTexture::uploadTexture()
{
    // generate new texture
    // no need to compute mipmaps
    textureId_ = g_mainWindow->getGLWindow()->bindTexture(scaledImage_, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption | QGLContext::InvertedYBindOption);
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
            boost::shared_ptr<DynamicTexture> c(new DynamicTexture("", shared_from_this(), imageBounds[i].x(), imageBounds[i].y(), imageBounds[i].width(), imageBounds[i].height()));
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

double DynamicTexture::getProjectedPixelArea()
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

void loadImageThread(DynamicTexture * dynamicTexture)
{
    dynamicTexture->loadImage();
    return;
}
