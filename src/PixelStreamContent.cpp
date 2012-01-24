#include "PixelStreamContent.h"
#include "main.h"
#include "PixelStream.h"

BOOST_CLASS_EXPORT_GUID(PixelStreamContent, "PixelStreamContent")

void PixelStreamContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(getURI())->getDimensions(width, height);
}

void PixelStreamContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
