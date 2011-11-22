#include "ContentWindow.h"
#include "Content.h"
#include "DisplayGroup.h"
#include "main.h"

ContentWindow::ContentWindow(boost::shared_ptr<Content> content)
{
    // content dimensions
    content->getDimensions(contentWidth_, contentHeight_);

    // default position / size
    x_ = y_ = 0.;
    w_ = h_ = 0.25;

    // default to centered
    centerX_ = 0.5;
    centerY_ = 0.5;

    // default to no zoom
    zoom_ = 1.;

    // default window state
    selected_ = false;

    // set content object
    content_ = content;

    // receive updates to content dimensions
    connect(content.get(), SIGNAL(dimensionsChanged(int, int)), this, SLOT(setContentDimensions(int, int)));
}

boost::shared_ptr<Content> ContentWindow::getContent()
{
    return content_;
}

boost::shared_ptr<DisplayGroup> ContentWindow::getDisplayGroup()
{
    return displayGroup_.lock();
}

void ContentWindow::setDisplayGroup(boost::shared_ptr<DisplayGroup> displayGroup)
{
    // disconnect any existing signals to previous DisplayGroup
    boost::shared_ptr<DisplayGroup> oldDisplayGroup = getDisplayGroup();

    if(oldDisplayGroup != NULL)
    {
        disconnect(this, 0, oldDisplayGroup.get(), 0);
    }

    displayGroup_ = displayGroup;

    // make connections to new DisplayGroup
    // don't use queued connections; we want these to execute immediately and we're in the same thread
    if(displayGroup != NULL)
    {
        connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(coordinatesChanged(double, double, double, double, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));
        connect(this, SIGNAL(selectedChanged(bool, ContentWindowInterface *)), displayGroup.get(), SLOT(sendDisplayGroup()));

        // we don't call sendDisplayGroup() on movedToFront() or destroyed() since it happens already
    }
}

void ContentWindow::moveToFront(ContentWindowInterface * source)
{
    ContentWindowInterface::moveToFront(source);

    if(source != this)
    {
        getDisplayGroup()->moveContentWindowToFront(shared_from_this());
    }
}

void ContentWindow::close(ContentWindowInterface * source)
{
    ContentWindowInterface::close(source);

    if(source != this)
    {
        getDisplayGroup()->removeContentWindow(shared_from_this());
    }
}

void ContentWindow::render()
{
    content_->render(shared_from_this());

    // render the border
    double horizontalBorder = 5. / (double)g_configuration->getTotalHeight(); // 5 pixels
    double verticalBorder = (double)g_configuration->getTotalHeight() / (double)g_configuration->getTotalWidth() * horizontalBorder;

    glPushAttrib(GL_CURRENT_BIT);

    // color the border based on window state
    if(selected_ == true)
    {
        glColor4f(1,0,0,1);
    }
    else
    {
        glColor4f(1,1,1,1);
    }

    GLWindow::drawRectangle(x_-verticalBorder,y_-horizontalBorder,w_+2.*verticalBorder,h_+2.*horizontalBorder);

    // render buttons if the marker is over the window
    float markerX, markerY;
    getDisplayGroup()->getMarker().getPosition(markerX, markerY);

    if(QRectF(x_, y_, w_, h_).contains(markerX, markerY) == true)
    {
        // we need this to be slightly in front of the rest of the window
        glPushMatrix();
        glTranslatef(0,0,0.001);

        // button dimensions
        float buttonWidth, buttonHeight;
        getButtonDimensions(buttonWidth, buttonHeight);

        // draw close button
        QRectF closeRect(x_ + w_ - buttonWidth, y_, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(1,0,0,0.125);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(1,0,0,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x() + closeRect.width(), closeRect.y() + closeRect.height());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x(), closeRect.y() + closeRect.height());
        glEnd();

        // resize indicator
        QRectF resizeRect(x_ + w_ - buttonWidth, y_ + h_ - buttonHeight, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(0.5,0.5,0.5,0.25);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(0.5,0.5,0.5,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x(), resizeRect.y() + resizeRect.height());
        glEnd();

        glPopMatrix();
    }

    glPopAttrib();
}

void ContentWindow::getButtonDimensions(float &width, float &height)
{
    float sceneHeightFraction = 0.125;
    double screenAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

    width = sceneHeightFraction / screenAspect;
    height = sceneHeightFraction;

    // clamp to half rect dimensions
    if(width > 0.5 * w_)
    {
        width = 0.49 * w_;
    }

    if(height > 0.5 * h_)
    {
        height = 0.49 * h_;
    }
}
