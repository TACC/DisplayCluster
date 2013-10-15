/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "PDF.h"

// detect Qt version
#if QT_VERSION >= 0x050000
#define POPPLER_QT5
#include <poppler-qt5.h>
#elif QT_VERSION >= 0x040000
#define POPPLER_QT4
#include <poppler-qt4.h>
#else
#error PopplerPixelStreamer needs Qt4 or Qt5
#endif

#include "globals.h"
#include "MainWindow.h"
#include "log.h"

#define INVALID_PAGE_NUMBER -1

PDF::PDF(QString uri)
    : uri_(uri)
    , pdfDoc_(0)
    , pdfPage_(0)
    , pdfPageNumber(INVALID_PAGE_NUMBER)
    , textureId_(0)
{
    openDocument(uri_);
}

PDF::~PDF()
{
    closeDocument();
}

void PDF::closePage()
{
    if (pdfPage_) {
        deleteTexture();
        delete pdfPage_;
        pdfPage_ = 0;
        pdfPageNumber = INVALID_PAGE_NUMBER;
    }
}

void PDF::closeDocument()
{
    if (pdfDoc_) {
        closePage();
        delete pdfDoc_;
        pdfDoc_ = 0;
    }
}

void PDF::deleteTexture()
{
    if (textureId_)
    {
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);
        textureId_ = 0;
        textureRect_ = QRect();
    }
}


void PDF::openDocument(QString filename)
{
    closeDocument();

    pdfDoc_ = Poppler::Document::load(filename);
    if (!pdfDoc_ || pdfDoc_->isLocked()) {
        put_flog(LOG_DEBUG, "Could not open document %s", filename.toLocal8Bit().constData());
        closeDocument();
        return;
    }

    pdfDoc_->setRenderHint(Poppler::Document::TextAntialiasing);

    setPage(0);
}

void PDF::setPage(int pageNumber)
{
    if (pageNumber != pdfPageNumber && pageNumber < pdfDoc_->numPages())
    {
        closePage();

        pdfPage_ = pdfDoc_->page(pageNumber); // Document starts at page 0
        if (pdfPage_ == 0) {
            put_flog(LOG_DEBUG, "Could not open page %d", pageNumber);
            return;
        }

        pdfPageNumber = pageNumber;
    }
}

int PDF::getPageCount() const
{
    return pdfDoc_->numPages();
}

QImage PDF::renderToImage() const
{
    return pdfPage_->renderToImage();
}

void PDF::getDimensions(int &width, int &height) const
{
    width = pdfPage_ ? pdfPage_->pageSize().width() : 0;
    height = pdfPage_ ? pdfPage_->pageSize().height() : 0;
}

void PDF::render(float tX, float tY, float tW, float tH)
{
    // get on-screen and full rectangle corresponding to the window
    QRectF screenRect = g_mainWindow->getGLWindow()->getProjectedPixelRect(true);
    QRectF fullRect = g_mainWindow->getGLWindow()->getProjectedPixelRect(false);

    // if we're not visible or we don't have a valid SVG, we're done...
    if(screenRect.isEmpty())
    {
        // TODO clear existing FBO for this OpenGL window
        return;
    }

    // generate texture corresponding to the visible part of these texture coordinates
    generateTexture(screenRect, fullRect, tX, tY, tW, tH);

    if(textureId_ == 0)
    {
        return;
    }

    // we'll now render the entire generated texture
    tX = tY = 0.;
    tW = tH = 1.;

    // figure out what visible region is for screenRect, a subregion of [0, 0, 1, 1]
    double xp = (screenRect.x() - fullRect.x()) / fullRect.width();
    double yp = (screenRect.y() - fullRect.y()) / fullRect.height();
    double wp = screenRect.width() / fullRect.width();
    double hp = screenRect.height() / fullRect.height();

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to edge (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBegin(GL_QUADS);

    glTexCoord2f(tX,tY);
    glVertex2f(xp, yp);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(xp+wp,yp);

    glTexCoord2f(tX+tW,(tY+tH));
    glVertex2f(xp+wp,yp+hp);

    glTexCoord2f(tX,(tY+tH));
    glVertex2f(xp,yp+hp);

    glEnd();

    glPopAttrib();
}


void PDF::generateTexture(QRectF screenRect, QRectF fullRect, float tX, float tY, float tW, float tH)
{
    // figure out the coordinates of the topLeft corner of the texture in the PDF page
    double tXp = tX/tW*fullRect.width()  + (screenRect.x() - fullRect.x());
    double tYp = tY/tH*fullRect.height() + (screenRect.y() - fullRect.y());

    // Compute the actual texture dimensions
    QRect textureRect(tXp, tYp, screenRect.width(), screenRect.height());

    // TODO The instance of this class seems to be shared, causing the texture to be
    // constantly regenerated because the size changes... find a way around this!
    if(textureRect == textureRect_)
    {
        // no need to regenerate texture
        //put_flog(LOG_DEBUG, "nothing to do");
        return;
    }

    // Adjust the quality to match the actual displayed size
    // Multiply resolution by the zoom factor (1/t[W,H])
    double resFactorX = fullRect.width() / pdfPage_->pageSize().width() / tW;
    double resFactorY = fullRect.height() / pdfPage_->pageSize().height() / tH;

    // Generate a QImage of the rendered page
    QImage image = pdfPage_->renderToImage(72.0*resFactorX , 72.0*resFactorY,
                                            textureRect.x(), textureRect.y(),
                                            textureRect.width(), textureRect.height()
                                            );

    if (image.isNull())
    {
        put_flog(LOG_DEBUG, "Could not render pdf to image");
        return;
    }

    if (textureRect.size() != textureRect_.size())
    {
        // Lets recreate a texture of the appropriate size
        deleteTexture();
        textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::NoBindOption);
        //put_flog(LOG_DEBUG, "texture recreated");
    }
    else
    {
        // put the RGB image to the already-created texture
        // glTexSubImage2D uses the existing texture and is more efficient than other means
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, image.width(), image.height(), GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
        //put_flog(LOG_DEBUG, "texture updated");
    }

    // keep rendered texture information so we know when to rerender
    textureRect_ = textureRect;
}

