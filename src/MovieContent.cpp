#include "MovieContent.h"
#include "main.h"
#include "Movie.h"
#include "ContentWindowManager.h"

BOOST_CLASS_EXPORT_GUID(MovieContent, "MovieContent")

void MovieContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->getDimensions(width, height);
}

void MovieContent::advance(boost::shared_ptr<ContentWindowManager> window)
{
    // skip a frame if the Content rectangle is not visible in ANY windows; otherwise decode normally
    bool skip = true;

    // window parameters
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
