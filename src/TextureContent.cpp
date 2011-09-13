#include "TextureContent.h"
#include "main.h"
#include "TextureFactory.h"
#include "Texture.h"

void TextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getTextureFactory().getTexture(getURI())->render(tX, tY, tW, tH);
}
