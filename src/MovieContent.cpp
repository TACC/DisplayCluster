#include "MovieContent.h"
#include "main.h"
#include "Movie.h"
#include "ContentWindow.h"

void MovieContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->getDimensions(width, height);
}

void MovieContent::advance(boost::shared_ptr<ContentWindow> window)
{
    // skip a frame if the Content rectangle is not visible in ANY windows; otherwise decode normally
    bool skip = true;

    // Content window parameters
    double x, y, w, h;
    window->getCoordinates(x, y, w, h);

    std::vector<boost::shared_ptr<GLWindow> > glWindows = g_mainWindow->getGLWindows();

    for(unsigned int i=0; i<glWindows.size(); i++)
    {
        if(glWindows[i]->isScreenRectangleVisible(x, y, w, h) == true)
        {
            skip = false;
            break;
        }
    }

    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->nextFrame(skip);
}

void MovieContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
