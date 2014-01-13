/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef SVG_H
#define SVG_H

#include "FactoryObject.h"
#include "types.h"

#include <QtSvg>
#include <QGLWidget>
#include <QGLFramebufferObject>
#include <boost/shared_ptr.hpp>
#include <map>

class GLWindow;

typedef boost::shared_ptr<QGLFramebufferObject> QGLFramebufferObjectPtr;

/**
 * Hold texture FBO and region rendered for one GLWindow.
 */
struct SVGTextureData
{
    /** Frame buffer Object */
    QGLFramebufferObjectPtr fbo;

    /** Texture region */
    QRectF region;
};

class SVG : public FactoryObject
{
public:
    SVG(QString uri);
    ~SVG();

    void getDimensions(int &width, int &height);
    bool setImageData(QByteArray imageData);
    void render(float tX, float tY, float tW, float tH);

private:
    // image location
    QString uri_;

    // SVG renderer
    QRectF svgExtents_;
    QSvgRenderer svgRenderer_;

    // Per-GLWindow texture data
    std::map<int, SVGTextureData> textureData_;

    // SVG default dimensions
    int width_;
    int height_;

    QRectF computeTextureRect(const QRectF& screenRect, const QRectF& fullRect,
                              const float tX, const float tY, const float tW, const float tH) const;

    void renderToTexture(const QRectF& textureRect, QGLFramebufferObjectPtr targetFbo);

    void saveGLState();
    void restoreGLState();

    void drawTexturedQuad(const float posX, const float posY,
                          const float width, const float height, const GLuint textureID);
};

#endif
