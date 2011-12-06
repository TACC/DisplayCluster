#include "GLWindow.h"
#include "main.h"
#include "Marker.h"
#include "ContentWindowManager.h"
#include <QtOpenGL>
#include "log.h"

GLWindow::GLWindow(int tileIndex)
{
    // defaults
    viewInitialized_ = false;

    tileIndex_ = tileIndex;

    // disable automatic buffer swapping
    setAutoBufferSwap(false);
}

GLWindow::GLWindow(int tileIndex, QRect windowRect, QGLWidget * shareWidget) : QGLWidget(0, shareWidget)
{
    // defaults
    viewInitialized_ = false;

    tileIndex_ = tileIndex;
    setGeometry(windowRect);

    // make sure sharing succeeded
    if(shareWidget != 0 && isSharing() != true)
    {
        put_flog(LOG_FATAL, "failed to share OpenGL context");
        exit(-1);
    }

    // disable automatic buffer swapping
    setAutoBufferSwap(false);
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

Factory<Movie> & GLWindow::getMovieFactory()
{
    return movieFactory_;
}

Factory<PixelStream> & GLWindow::getPixelStreamFactory()
{
    return pixelStreamFactory_;
}

void GLWindow::initializeGL()
{
    // enable depth testing; disable lighting
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

void GLWindow::paintGL()
{
    setView(width(), height());

    // render the markers
    std::vector<boost::shared_ptr<Marker> > markers = g_displayGroupManager->getMarkers();

    for(unsigned int i=0; i<markers.size(); i++)
    {
        markers[i]->render();
    }

    // render content windows
    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

    for(unsigned int i=0; i<contentWindowManagers.size(); i++)
    {
        // manage depth order
        // the visible depths seem to be in the range (-1,1); make the content window depths be in the range (-1,0)
        glPushMatrix();
        glTranslatef(0.,0.,-((float)contentWindowManagers.size() - (float)i) / ((float)contentWindowManagers.size() + 1.));

        contentWindowManagers[i]->render();

        glPopMatrix();
    }
}

void GLWindow::resizeGL(int width, int height)
{
    glViewport (0, 0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    update();
}

void GLWindow::setView(int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // invert y-axis to put origin at lower-left corner
    glScalef(1.,-1.,1.);

    // compute view bounds if view has not been initialized
    if(viewInitialized_ == false)
    {
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

        viewInitialized_ = true;
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
