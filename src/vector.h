#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

template <class T> void vectorCrossProduct(const T a[3], const T b[3], T c[3])
{
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];

    return;
}

template <class T> T vectorMagnitude(const T a[3])
{
    return sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

}

template <class T> void vectorSubtraction(const T a[3], const T b[3], T c[3])
{
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
    
    return;
}

template <class T> void vectorNormalize(T a[3])
{
    T magnitude = vectorMagnitude(a);

    a[0] /= magnitude;
    a[1] /= magnitude;
    a[2] /= magnitude;

    return;
}

template <class T> T vectorDotProduct(const T a[3], const T b[3])
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

template <class T> T vectorDistance(const T a[3], const T b[3])
{
    return sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

#endif
