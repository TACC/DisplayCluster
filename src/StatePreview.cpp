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

#include "StatePreview.h"

#include <QRectF>

#include "ContentWindowManager.h"
#include "log.h"

#include "thumbnail/ThumbnailGeneratorFactory.h"
#include "thumbnail/ThumbnailGenerator.h"

#define PREVIEW_IMAGE_SIZE       512
#define CONTENT_THUMBNAIL_SIZE   128

StatePreview::StatePreview(const QString &dcxFileName)
    : dcxFileName_(dcxFileName)
{
}

QString StatePreview::getFileExtension()
{
    return QString(".dcxpreview");
}

QImage StatePreview::getImage() const
{
    return previewImage_;
}

QString StatePreview::previewFilename() const
{
    QFileInfo fileinfo(dcxFileName_);

    if (fileinfo.suffix().toLower() != "dcx")
    {
        put_flog(LOG_WARN, "wrong state file extension (expected .dcx)");
        return QString();
    }
    return fileinfo.path() + "/" + fileinfo.completeBaseName() + getFileExtension();
}

void StatePreview::generateImage(const QSize& wallDimensions, const ContentWindowManagerPtrs &contentWindowManagers)
{
    QSize previewDimension(wallDimensions);
    previewDimension.scale(QSize(PREVIEW_IMAGE_SIZE, PREVIEW_IMAGE_SIZE), Qt::KeepAspectRatio);

    // Transparent image
    QImage preview(wallDimensions, QImage::Format_ARGB32);
    preview.fill(qRgba(0,0,0,0));

    // Paint all Contents at their correct location
    QPainter painter(&preview);
    const QSize contentThumbnailSize(CONTENT_THUMBNAIL_SIZE, CONTENT_THUMBNAIL_SIZE);

    for(size_t i=0; i<contentWindowManagers.size(); i++)
    {
        ContentWindowManager* cwm = contentWindowManagers[i].get();
        if (cwm->getContent()->getType() != CONTENT_TYPE_PIXEL_STREAM)
        {
            // Use ThumbnailFactory to generate thumbnails for the Contents
            const QString& filename = contentWindowManagers[i]->getContent()->getURI();
            QImage image = ThumbnailGeneratorFactory::getGenerator(filename, contentThumbnailSize)->generate(filename);

            double x, y, w ,h;
            cwm->getCoordinates(x, y, w ,h);
            QRectF area(x*preview.size().width(), y*preview.size().height(), w*preview.size().width(), h*preview.size().height());

            painter.drawImage(area, image);
        }
    }

    painter.end();

    previewImage_ = preview;
}

bool StatePreview::saveToFile() const
{
    bool success = previewImage_.save(previewFilename(), "PNG");

    if (!success)
        put_flog(LOG_ERROR, "Saving StatePreview image failed: %s", previewFilename().toLocal8Bit().constData());

    return success;
}

bool StatePreview::loadFromFile()
{
    QImageReader reader(previewFilename());
    if (reader.canRead())
    {
        return reader.read(&previewImage_);
    }
    return false;
}
