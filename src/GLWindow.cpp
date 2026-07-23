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

#include "GLWindow.h"
#include "main.h"
#include "Marker.h"
#include "ContentWindowManager.h"
#include "log.h"
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QSurfaceFormat>
#include <QPainter>
#include <boost/shared_ptr.hpp>

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#elif defined(_WIN32)
    // GL/gl.h and GL/glu.h use Windows calling-convention macros
    // (APIENTRY, WINGDIAPI) that only exist once windows.h has been
    // included first - unlike Linux/Mac, these headers aren't self-
    // contained on Windows
    #include <windows.h>
    #include <GL/glu.h>
#else
    #include <GL/glu.h>
#endif

GLWindow::GLWindow(int tileIndex)
    : tileIndex_(tileIndex), context_(NULL), initializedGL_(false)
{
    init(NULL);
}

GLWindow::GLWindow(int tileIndex, QRect windowRect, GLWindow * shareWindow)
    : tileIndex_(tileIndex), context_(NULL), initializedGL_(false)
{
    setGeometry(windowRect);
    init(shareWindow);
}

void GLWindow::init(GLWindow * shareWindow)
{
    setSurfaceType(QWindow::OpenGLSurface);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);

    // the rendering code here (and in Content subclasses) is fixed-function
    // GL (glBegin/glMatrixMode/etc.), so we need a compatibility profile
    format.setProfile(QSurfaceFormat::CompatibilityProfile);

    setFormat(format);

    context_ = new QOpenGLContext(this);
    context_->setFormat(format);

    if(shareWindow != 0)
    {
        context_->setShareContext(shareWindow->context_);
    }

    context_->create();

    create();

    // make sure sharing succeeded
    if(shareWindow != 0 && context_->shareContext() != shareWindow->context_)
    {
        put_flog(LOG_FATAL, "failed to share OpenGL context");
        exit(-1);
    }
}

GLWindow::~GLWindow()
{

}

Factory<Texture> & GLWindow::getTextureFactory()
{
    return textureFactory_;
}

Factory<DynamicTexture> & GLWindow::getDynamicTextureFactory()
{
    return dynamicTextureFactory_;
}

Factory<SVG> & GLWindow::getSVGFactory()
{
    return svgFactory_;
}

Factory<Movie> & GLWindow::getMovieFactory()
{
    return movieFactory_;
}

Factory<PixelStream> & GLWindow::getPixelStreamFactory()
{
    return pixelStreamFactory_;
}

Factory<ParallelPixelStream> & GLWindow::getParallelPixelStreamFactory()
{
    return parallelPixelStreamFactory_;
}

void GLWindow::insertPurgeTextureId(GLuint textureId)
{
    QMutexLocker locker(&purgeTexturesMutex_);

    purgeTextureIds_.push_back(textureId);
}

void GLWindow::purgeTextures()
{
    QMutexLocker locker(&purgeTexturesMutex_);

    for(unsigned int i=0; i<purgeTextureIds_.size(); i++)
    {
        glDeleteTextures(1, &purgeTextureIds_[i]);
    }

    purgeTextureIds_.clear();
}

void GLWindow::updateGL()
{
    context_->makeCurrent(this);

    if(initializedGL_ == false)
    {
        initializeGL();
        initializedGL_ = true;
    }

    resizeGL(width(), height());
    paintGL();
}

void GLWindow::swapBuffers()
{
    context_->swapBuffers(this);
}

GLuint GLWindow::bindTextureFromImage(const QImage & image, bool generateMipmaps)
{
    context_->makeCurrent(this);

    // match QGLWidget::bindTexture()'s behavior: convert to a GL-ready byte
    // order (Format_RGBA8888 matches GL_RGBA/GL_UNSIGNED_BYTE regardless of
    // platform endianness) and flip vertically, since QImage rows are
    // top-down but GL texture data is expected bottom-up
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.constBits());

    if(generateMipmaps)
    {
        // glGenerateMipmap is OpenGL 3.0+/ARB_framebuffer_object and isn't
        // declared by the system's classic GL headers; resolve it through
        // Qt, which also makes this portable to platforms that need
        // explicit extension-function loading (e.g. Windows/WGL)
        context_->extraFunctions()->glGenerateMipmap(GL_TEXTURE_2D);
    }

    return textureId;
}

void GLWindow::queueText(int x, int y, const QString & text, const QFont & font, const QColor & color)
{
    QueuedText qt;
    qt.x = x;
    qt.y = y;
    qt.text = text;
    qt.font = font;
    qt.color = color;

    queuedText_.push_back(qt);
}

void GLWindow::queueText(double x, double y, double z, const QString & text, const QFont & font, const QColor & color)
{
    // project the object-space point through the current matrix stack (as
    // set up at the call site) to get a window pixel position, matching
    // what QGLWidget::renderText(x,y,z,...) used to do internally
    GLdouble modelview[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    GLdouble projection[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble winX, winY, winZ;
    gluProject(x, y, z, modelview, projection, viewport, &winX, &winY, &winZ);

    // gluProject's window Y is measured from the bottom; QPainter measures from the top
    queueText((int)winX, viewport[3] - (int)winY, text, font, color);
}

void GLWindow::drawQueuedText()
{
    if(queuedText_.empty() == true)
    {
        return;
    }

    QOpenGLPaintDevice paintDevice(size());
    QPainter painter(&paintDevice);

    for(unsigned int i=0; i<queuedText_.size(); i++)
    {
        painter.setFont(queuedText_[i].font);
        painter.setPen(queuedText_[i].color);
        painter.drawText(queuedText_[i].x, queuedText_[i].y, queuedText_[i].text);
    }

    queuedText_.clear();
}

void GLWindow::initializeGL()
{
    // enable depth testing; disable lighting
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

void GLWindow::paintGL()
{
    setOrthographicView();

    // if the show test pattern option is enabled, render the test pattern and return
    if(g_displayGroupManager->getOptions()->getShowTestPattern() == true)
    {
        renderTestPattern();
        drawQueuedText();
        return;
    }

    // render content windows
    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

    for(unsigned int i=0; i<contentWindowManagers.size(); i++)
    {
        // skip hidden windows entirely
        if(contentWindowManagers[i]->getHidden())
            continue;

        // manage depth order
        // the visible depths seem to be in the range (-1,1); make the content window depths be in the range (-1,0)
        glPushMatrix();
        glTranslatef(0.,0.,-((float)contentWindowManagers.size() - (float)i) / ((float)contentWindowManagers.size() + 1.));

        contentWindowManagers[i]->render();

        // render identifying label overlay on each content window
        if(g_displayGroupManager->getOptions()->getShowContentLabels() == true)
        {
            double x, y, w, h;
            contentWindowManagers[i]->getCoordinates(x, y, w, h);

            // label bar height: 5% of window height in normalized coords
            double labelHeight = h * 0.05;

            // draw semi-transparent dark background bar at the top of the window
            glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
            glEnable(GL_BLEND);
            glDepthMask(GL_FALSE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0., 0., 0., 0.65);
            GLWindow::drawRectangle(x, y, w, labelHeight);
            glPopAttrib();

            // Only render text if the label bar overlaps this tile's viewport.
            // left_, right_, bottom_, top_ are the normalized global-display bounds
            // of this tile, set by setOrthographicView().
            bool labelInTile = (x < right_) && (x + w > left_) &&
                               (y < top_)   && (y + labelHeight > bottom_);

            if(labelInTile)
            {
                // extract filename from URI
                std::string uri = contentWindowManagers[i]->getContent()->getURI();
                std::string filename = uri.substr(uri.find_last_of("/\\") + 1);
                QString label = QString::fromStdString(filename);

                // Convert the label's top-left corner from normalized global display
                // space into this tile's local pixel space.
                //
                // The tile covers [left_, right_] x [bottom_, top_] of the display.
                // Map x -> fraction across this tile -> pixel column.
                // The y-axis is inverted (origin at top in Qt pixel space).
                double tileW = right_ - left_;
                double tileH = top_   - bottom_;

                int pixelX = (int)((x - left_) / tileW * (double)width()) + 4;

                // fontSize scaled so it looks the same physical size regardless of
                // how many tiles tall the display is
                int fontSize = std::max(10, (int)(labelHeight / tileH * (double)height() * 0.75));

                // y in global coords, mapped to pixel row (Qt y=0 is top of window)
                int pixelY = (int)((y - bottom_) / tileH * (double)height()) + fontSize;

                QFont font;
                font.setPixelSize(fontSize);
                font.setBold(true);

                queueText(pixelX, pixelY, label, font, Qt::white);
            }
        }

        glPopMatrix();
    }

    // render the markers
    // these should be rendered last since they're blended
    std::vector<boost::shared_ptr<Marker> > markers = g_displayGroupManager->getMarkers();

    for(unsigned int i=0; i<markers.size(); i++)
    {
        markers[i]->render();
    }

    drawQueuedText();
}

void GLWindow::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLWindow::setOrthographicView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // invert y-axis to put origin at lower-left corner
    glScalef(1.,-1.,1.);

    // compute view bounds
    if(g_mpiRank == 0)
    {
        left_ = 0.;
        right_ = 1.;
        bottom_ = 0.;
        top_ = 1.;
    }
    else
    {
        // tiled display parameters
        double tileI = (double)g_configuration->getTileI(tileIndex_);
        double numTilesWidth = (double)g_configuration->getNumTilesWidth();
        double screenWidth = (double)g_configuration->getScreenWidth();
        double mullionWidth = (double)g_configuration->getMullionWidth();

        double tileJ = (double)g_configuration->getTileJ(tileIndex_);
        double numTilesHeight = (double)g_configuration->getNumTilesHeight();
        double screenHeight = (double)g_configuration->getScreenHeight();
        double mullionHeight = (double)g_configuration->getMullionHeight();

        // border calculations
        left_ = tileI / numTilesWidth * ( numTilesWidth * screenWidth ) + tileI * mullionWidth;
        right_ = left_ + screenWidth;
        bottom_ = tileJ / numTilesHeight * ( numTilesHeight * screenHeight ) + tileJ * mullionHeight;
        top_ = bottom_ + screenHeight;

        // normalize to 0->1
        double totalWidth = (double)g_configuration->getTotalWidth();
        double totalHeight = (double)g_configuration->getTotalHeight();

        left_ /= totalWidth;
        right_ /= totalWidth;
        bottom_ /= totalHeight;
        top_ /= totalHeight;
    }

    gluOrtho2D(left_, right_, bottom_, top_);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0,0,0,0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool GLWindow::isScreenRectangleVisible(double x, double y, double w, double h)
{
    // works in "screen space" where the rectangle for the entire tiled display is (0,0,1,1)

    // screen rectangle
    QRectF screenRect(left_, bottom_, right_-left_, top_-bottom_);

    // the given rectangle
    QRectF rect(x, y, w, h);

    if(screenRect.intersects(rect) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GLWindow::isRectangleVisible(double x, double y, double w, double h)
{
    // get four corners in object space
    double xObj[4][3];

    xObj[0][0] = x;
    xObj[0][1] = y;
    xObj[0][2] = 0.;

    xObj[1][0] = x+w;
    xObj[1][1] = y;
    xObj[1][2] = 0.;

    xObj[2][0] = x+w;
    xObj[2][1] = y+h;
    xObj[2][2] = 0.;

    xObj[3][0] = x;
    xObj[3][1] = y+h;
    xObj[3][2] = 0.;

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
        gluProject(xObj[i][0], xObj[i][1], xObj[i][2], modelview, projection, viewport, &xWin[i][0], &xWin[i][1], &xWin[i][2]);
    }

    // screen rectangle
    QRectF screenRect(0.,0., (double)g_mainWindow->getGLWindow()->width(), (double)g_mainWindow->getGLWindow()->height());

    // the given rectangle
    QRectF rect(xWin[0][0], xWin[0][1], xWin[2][0]-xWin[0][0], xWin[2][1]-xWin[0][1]);

    if(screenRect.intersects(rect) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void GLWindow::drawRectangle(double x, double y, double w, double h)
{
    glBegin(GL_QUADS);

    glVertex2d(x,y);
    glVertex2d(x+w,y);
    glVertex2d(x+w,y+h);
    glVertex2d(x,y+h);

    glEnd();
}

void GLWindow::finalize()
{
    textureFactory_.clear();
    dynamicTextureFactory_.clear();
    svgFactory_.clear();
    movieFactory_.clear();
    pixelStreamFactory_.clear();
    parallelPixelStreamFactory_.clear();

    purgeTextures();
}

void GLWindow::renderTestPattern()
{
    glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);
    glPushMatrix();

    // cross pattern
    glLineWidth(10);

    glBegin(GL_LINES);

    for(double y=-1.; y<=2.; y+=0.1)
    {
        QColor color = QColor::fromHsvF((y + 1.)/3., 1., 1.);
        glColor3f(color.redF(), color.greenF(), color.blueF());

        glVertex2d(0., y);
        glVertex2d(1., y+1.);

        glVertex2d(0., y);
        glVertex2d(1., y-1.);
    }

    glEnd();

    // screen information in front of cross pattern
    glTranslatef(0., 0., 0.1);

    QString label1 = "Rank: " + QString::number(g_mpiRank);
    QString label2 = "Host: " + QString(g_configuration->getMyHost().c_str());
    QString label3 = "Display: " + QString(g_configuration->getMyDisplay().c_str());
    QString label4 = "Tile coordinates: (" + QString::number(g_configuration->getTileI(tileIndex_)) + ", " + QString::number(g_configuration->getTileJ(tileIndex_)) + ")";
    QString label5 = "Resolution: " + QString::number(g_configuration->getScreenWidth()) + " x " + QString::number(g_configuration->getScreenHeight());
    QString label6 = "Fullscreen mode: ";

    if(g_configuration->getFullscreen() == true)
    {
        label6 += "True";
    }
    else
    {
        label6 += "False";
    }

    int fontSize = 64;

    QFont font;
    font.setPixelSize(fontSize);

    queueText(50, 1*fontSize, label1, font, Qt::white);
    queueText(50, 2*fontSize, label2, font, Qt::white);
    queueText(50, 3*fontSize, label3, font, Qt::white);
    queueText(50, 4*fontSize, label4, font, Qt::white);
    queueText(50, 5*fontSize, label5, font, Qt::white);
    queueText(50, 6*fontSize, label6, font, Qt::white);

    glPopMatrix();
    glPopAttrib();
}
