#include "Cursor.h"
#include <QGLWidget>

Cursor::Cursor()
{
    x_ = y_ = 0.;
}

void Cursor::setPosition(float x, float y)
{
    x_ = x;
    y_ = y;
}

void Cursor::getPosition(float &x, float &y)
{
    x = x_;
    y = y_;
}

void Cursor::render()
{
    glPushAttrib(GL_POINT_BIT);
    glPointSize(10.);

    glBegin(GL_POINTS);
    glVertex3f(x_, y_, 0.);
    glEnd();

    glPopAttrib();
}
