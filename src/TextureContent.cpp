#include "TextureContent.h"
#include "main.h"
#include "Texture.h"

void TextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
