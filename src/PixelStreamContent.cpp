#include "PixelStreamContent.h"
#include "main.h"
#include "PixelStream.h"

void PixelStreamContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
