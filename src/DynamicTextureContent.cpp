#include "DynamicTextureContent.h"
#include "main.h"
#include "DynamicTexture.h"

void DynamicTextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
