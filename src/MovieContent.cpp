#include "MovieContent.h"
#include "main.h"
#include "Movie.h"

void MovieContent::advance()
{
    // skip a frame if the Content rectangle is not visible in ANY windows; otherwise decode normally
    bool skip = true;

    std::vector<boost::shared_ptr<GLWindow> > glWindows = g_mainWindow->getGLWindows();

    for(unsigned int i=0; i<glWindows.size(); i++)
    {
        if(glWindows[i]->isScreenRectangleVisible(x_, y_, w_, h_) == true)
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
