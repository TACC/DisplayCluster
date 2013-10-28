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

#ifndef DOCKPIXELSTREAMER_H
#define DOCKPIXELSTREAMER_H

#include "LocalPixelStreamer.h"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtCore/QLinkedList>
#include <QtGui/QImage>

class PictureFlow;
class AsyncImageLoader;

class DockPixelStreamer : public LocalPixelStreamer
{
    Q_OBJECT

public:

    DockPixelStreamer();
    ~DockPixelStreamer();

    virtual QSize size() const;

    static QString getUniqueURI();

    void open();

    void onItem();

public slots:
    void update(const QImage &image);
    void loadThumbnails(int newCenterIndex);
    void loadNextThumbnailInList();

    virtual void updateInteractionState(InteractionState interactionState);

signals:
    void renderPreview( const QString& fileName, const int index );

private:

    QThread loadThread_;

    PictureFlow* flow_;
    AsyncImageLoader* loader_;

    QDir currentDir_;
    QHash< QString, int > slideIndex_;

    typedef QPair<bool, QString> SlideImageLoadingStatus;
    QVector<SlideImageLoadingStatus> slideImagesLoaded_;
    QLinkedList<int> slideImagesToLoad_;

    PixelStreamSegmentParameters makeSegmentHeader();
    bool openFile(const QString &filename);
    void changeDirectory( const QString& dir );
    void addRootDirToFlow();
    void addFilesToFlow();
    void addFoldersToFlow();
};

#endif // DOCKPIXELSTREAMER_H
