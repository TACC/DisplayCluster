#include "Marker.h"
#include "log.h"
#include "main.h"
#include <QGLWidget>

GLuint Marker::textureId_ = 0;

Marker::Marker()
{
    x_ = y_ = 0.;

    if(g_mpiRank != 0 && textureId_ == 0 && g_mainWindow->getGLWindow() != NULL)
    {
        // load marker texture
        std::string markerImageFilename = std::string(g_displayClusterDir) + std::string("/data/") + std::string(MARKER_IMAGE_FILENAME);

        QImage image(markerImageFilename.c_str());

        if(image.isNull() == true)
        {
            put_flog(LOG_ERROR, "error loading marker texture %s", markerImageFilename.c_str());
            return;
        }

        textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::DefaultBindOption);
    }
}

void Marker::setPosition(float x, float y)
{
    x_ = x;
    y_ = y;
    updatedTimestamp_ = *(g_displayGroupManager->getTimestamp());

    emit(positionChanged());
}

void Marker::getPosition(float &x, float &y)
{
    x = x_;
    y = y_;
}

void Marker::render()
{
    // only render recently active markers
    if((*(g_displayGroupManager->getTimestamp()) - updatedTimestamp_).total_seconds() > MARKER_TIMEOUT_SECONDS)
    {
        return;
    }

    float markerWidth = MARKER_WIDTH;

    // marker height needs to be scaled by the tiled display aspect ratio
    float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();
    float markerHeight = markerWidth * tiledDisplayAspect;

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    // disable depth testing and enable blending
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // enable texturing
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glPushMatrix();
    glTranslated(x_, y_, 0.);

    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(-markerWidth,-markerHeight);

    glTexCoord2f(1,0);
    glVertex2f(markerWidth,-markerHeight);

    glTexCoord2f(1,1);
    glVertex2f(markerWidth,markerHeight);

    glTexCoord2f(0,1);
    glVertex2f(-markerWidth,markerHeight);

    glEnd();

    glPopMatrix();
    glPopAttrib();
}
