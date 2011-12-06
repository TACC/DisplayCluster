#include "Marker.h"
#include <QGLWidget>

Marker::Marker()
{
    x_ = y_ = 0.;
}

void Marker::setPosition(float x, float y)
{
    x_ = x;
    y_ = y;

    emit(positionChanged());
}

void Marker::getPosition(float &x, float &y)
{
    x = x_;
    y = y_;
}

void Marker::render()
{
    glPushAttrib(GL_CURRENT_BIT | GL_POINT_BIT);
    glPointSize(100.);

    glColor4f(1,0,0,1.);

    glBegin(GL_POINTS);
    glVertex3f(x_, y_, 0.);
    glEnd();

    glPopAttrib();
}
