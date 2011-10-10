#include "GLWindow.h"
#include "main.h"
#include "Marker.h"
#include "Content.h"
#include <QtOpenGL>

GLWindow::GLWindow()
{

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

    // render the marker
    g_displayGroup->getMarker().render();

    // render contents
    std::vector<boost::shared_ptr<Content> > contents = g_displayGroup->getContents();

    for(unsigned int i=0; i<contents.size(); i++)
    {
        // manage depth order
        glPushMatrix();
        glTranslatef(0.,0.,-(float)i / (float)contents.size());

        contents[i]->render();

        glPopMatrix();
    }

    // get updated data
    g_displayGroup->synchronize();

    // all render processes render simultaneously
    MPI_Barrier(g_mpiRenderComm);

    // continuously render
    update();
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

    // compute view bounds
    double left, right, bottom, top;

    if(g_mpiRank == 0)
    {
        left = 0.;
        right = 1.;
        bottom = 0.;
        top = 1.;
    }
    else
    {
        // tiled display parameters
        double tileI = (double)g_configuration->getTileI();
        double numTilesWidth = (double)g_configuration->getNumTilesWidth();
        double screenWidth = (double)g_configuration->getScreenWidth();
        double mullionWidth = (double)g_configuration->getMullionWidth();

        double tileJ = (double)g_configuration->getTileJ();
        double numTilesHeight = (double)g_configuration->getNumTilesHeight();
        double screenHeight = (double)g_configuration->getScreenHeight();
        double mullionHeight = (double)g_configuration->getMullionHeight();

        // border calculations
        left = tileI / numTilesWidth * ( numTilesWidth * screenWidth ) + tileI * mullionWidth;
        right = left + screenWidth;
        bottom = tileJ / numTilesHeight * ( numTilesHeight * screenHeight ) + tileJ * mullionHeight;
        top = bottom + screenHeight;

        // normalize to 0->1
        double totalWidth = numTilesWidth * screenWidth + (numTilesWidth - 1.) * mullionWidth;
        double totalHeight = numTilesHeight * screenHeight + (numTilesHeight - 1.) * mullionHeight;

        left /= totalWidth;
        right /= totalWidth;
        bottom /= totalHeight;
        top /= totalHeight;
    }

    gluOrtho2D(left, right, bottom, top);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();	

    glClearColor(0,0,0,0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
