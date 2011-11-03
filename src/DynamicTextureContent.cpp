#include "DynamicTextureContent.h"
#include "main.h"
#include "DynamicTexture.h"

void DynamicTextureContent::advance(boost::shared_ptr<ContentWindow> window)
{
    // recall that advance() is called after rendering and before g_frameCount is incremented for the current frame
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->clearOldChildren(g_frameCount);
}

void DynamicTextureContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->getDimensions(width, height);
}

void DynamicTextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
