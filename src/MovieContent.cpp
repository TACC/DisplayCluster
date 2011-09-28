#include "MovieContent.h"
#include "main.h"
#include "Movie.h"

void MovieContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
