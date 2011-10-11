#include "DynamicTextureContent.h"
#include "main.h"
#include "DynamicTexture.h"

void DynamicTextureContent::advance()
{
    // recall that advance() is called after rendering and before g_frameCount is incremented for the current frame
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->clearOldChildren(g_frameCount);
}

void DynamicTextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
