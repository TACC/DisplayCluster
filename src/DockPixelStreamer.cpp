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

#include "DockPixelStreamer.h"

#include "Pictureflow.h"
#include "Configuration.h"
#include "Content.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "globals.h"
#include "types.h"
#include "AsyncImageLoader.h"
#include "thumbnail/ThumbnailGeneratorFactory.h"
#include "thumbnail/FolderThumbnailGenerator.h"

#define COVERFLOW_SLIDE_RELATIVE_WIDTH   0.05
#define SLIDE_SIZE_MIN  128
#define SLIDE_SIZE_MAX  512

#define COVERFLOW_WIDTH_FACTOR   3.5
#define COVERFLOW_HEIGHT_FACTOR  1.5

#define COVERFLOW_SPEED_FACTOR 0.1

QString DockPixelStreamer::getUniqueURI()
{
    return "Dock";
}

DockPixelStreamer::DockPixelStreamer()
    : LocalPixelStreamer(getUniqueURI())
{
    const int coverflowSlideSize = std::max(std::min((int)(g_configuration->getTotalWidth()*COVERFLOW_SLIDE_RELATIVE_WIDTH), SLIDE_SIZE_MAX), SLIDE_SIZE_MIN);
    flow_ = new PictureFlow;
    flow_->resize(COVERFLOW_WIDTH_FACTOR*coverflowSlideSize, COVERFLOW_HEIGHT_FACTOR*coverflowSlideSize);
    flow_->setSlideSize( QSize( coverflowSlideSize, coverflowSlideSize ));
    flow_->setBackgroundColor( Qt::darkGray );

    connect( flow_, SIGNAL( imageUpdated( const QImage& )), this, SLOT( update( const QImage& )));
    connect( flow_, SIGNAL( targetIndexChanged(int)), this, SLOT(loadThumbnails(int)) );

    loader_ = new AsyncImageLoader(flow_->slideSize());
    loader_->moveToThread( &loadThread_ );
    connect( loader_, SIGNAL(imageLoaded(int, QImage)),
             flow_, SLOT(setSlide( int, QImage )));
    connect( this, SIGNAL(renderPreview( const QString&, const int )),
             loader_, SLOT(loadImage( const QString&, const int )));
    connect( loader_, SIGNAL(imageLoadingFinished()),
             this, SLOT(loadNextThumbnailInList()));
    loadThread_.start();

    changeDirectory( g_configuration->getDockStartDir( ));
}

DockPixelStreamer::~DockPixelStreamer()
{
    loadThread_.quit();
    loadThread_.wait();
    delete flow_;
    delete loader_;
}


void DockPixelStreamer::updateInteractionState(InteractionState interactionState)
{
    if (interactionState.type == InteractionState::EVT_CLICK)
    {
        // xPos is click position in (pixel) units inside the dock
        const int xPos = interactionState.mouseX * flow_->size().width();

        // mid is half the width of the dock in (pixel) units
        const int dockHalfWidth = flow_->size().width() / 2;

        // SlideMid is half the slide width in pixels
        const int slideHalfWidth = flow_->slideSize().width() / 2;

        if( xPos > dockHalfWidth-slideHalfWidth && xPos < dockHalfWidth+slideHalfWidth )
        {
            onItem();
        }
        else
        {
            if( xPos > dockHalfWidth )
                flow_->showNext();
            else
                flow_->showPrevious();
        }
    }

    else if (interactionState.type == InteractionState::EVT_MOVE || interactionState.type == InteractionState::EVT_WHEEL)
    {
        const int offs = interactionState.dx * flow_->size().width() * COVERFLOW_SPEED_FACTOR;
        flow_->showSlide( flow_->centerIndex() - offs );
    }
}

QSize DockPixelStreamer::size() const
{
    return flow_->size();
}


void DockPixelStreamer::open()
{
    flow_->triggerRender();
}

void DockPixelStreamer::onItem()
{
    const QImage& image = flow_->slide( flow_->centerIndex( ));
    const QString& source = image.text( "source" );

    if( image.text( "dir" ).isEmpty( ))
    {
        if (openFile( source ))
        {
            emit(streamClosed(getUniqueURI())); // Hide the dock
        }
    }
    else
    {
        changeDirectory( source );
    }
}

void DockPixelStreamer::update(const QImage& image)
{
    PixelStreamSegment segment;
    segment.parameters = makeSegmentHeader();

    segment.imageData = QByteArray::fromRawData((const char*)image.bits(), image.byteCount());
    segment.imageData.detach();

    emit segmentUpdated(uri_, segment);
}

void DockPixelStreamer::loadThumbnails(int newCenterIndex)
{
    const int nbThumbnails = 2;

    slideImagesToLoad_.clear();

    int imin = std::max(newCenterIndex-nbThumbnails, 0);
    int imax = std::min(newCenterIndex+nbThumbnails, slideImagesLoaded_.size()-1);
    for (int i=imin; i<=imax; i++)
    {
        slideImagesToLoad_.append(i);
    }
    loadNextThumbnailInList();
}

void DockPixelStreamer::loadNextThumbnailInList()
{
    while(!slideImagesToLoad_.empty())
    {
        int i = slideImagesToLoad_.front();
        slideImagesToLoad_.pop_front();
        if (!slideImagesLoaded_[i].first)
        {
            slideImagesLoaded_[i].first = true;
            emit renderPreview( slideImagesLoaded_[i].second, i );
            return;
        }
    }
}

PixelStreamSegmentParameters DockPixelStreamer::makeSegmentHeader()
{
    PixelStreamSegmentParameters parameters;
    parameters.totalHeight = flow_->size().height();
    parameters.totalWidth = flow_->size().width();
    parameters.height = parameters.totalHeight;
    parameters.width = parameters.totalWidth;
    parameters.compressed = false;
    return parameters;
}

bool DockPixelStreamer::openFile(const QString& filename)
{
    const QString& extension = QFileInfo(filename).suffix().toLower();

    if( extension == "dcx" )
    {
        return g_displayGroupManager->loadStateXMLFile( filename );
    }

    ContentPtr content = ContentFactory::getContent( filename );
    if( !content )
    {
        return false;
    }

    ContentWindowManagerPtr cwm(new ContentWindowManager(content));
    g_displayGroupManager->addContentWindowManager( cwm );
    cwm->adjustSize( SIZE_1TO1 ); // TODO Remove this when content dimensions request is no longer needed

    // Center the content where the dock is
    ContentWindowManagerPtr dockCwm = g_displayGroupManager->getContentWindowManager(getUniqueURI(), CONTENT_TYPE_PIXEL_STREAM);
    double dockCenterX, dockCenterY;
    dockCwm->getWindowCenterPosition(dockCenterX, dockCenterY);
    cwm->centerPositionAround(dockCenterX, dockCenterY, true);

    return true;
}

void DockPixelStreamer::changeDirectory( const QString& dir )
{
    slideIndex_[currentDir_.path()] = flow_->centerIndex();

    flow_->clear();
    slideImagesToLoad_.clear();
    slideImagesLoaded_.clear();

    currentDir_ = dir;
    addRootDirToFlow();
    addFilesToFlow();
    addFoldersToFlow();

    flow_->setCenterIndex( slideIndex_[currentDir_.path()] );
    loadThumbnails( slideIndex_[currentDir_.path()] );
}

void DockPixelStreamer::addRootDirToFlow()
{
    QDir rootDir = currentDir_;
    const bool hasRootDir = rootDir.cdUp();

    if( hasRootDir)
    {
        FolderThumbnailGeneratorPtr folderGenerator = ThumbnailGeneratorFactory::getFolderGenerator(flow_->slideSize());

        QImage img = folderGenerator->generateUpFolderImage(rootDir);
        flow_->addSlide( img, "UP: " + rootDir.path() );
        slideImagesLoaded_.append(qMakePair(true, QString()));
    }
}

void DockPixelStreamer::addFilesToFlow()
{
    currentDir_.setFilter( QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot );
    QStringList filters = ContentFactory::getSupportedFilesFilter();
    filters.append( "*.dcx" );
    currentDir_.setNameFilters( filters );
    const QFileInfoList& fileList = currentDir_.entryInfoList();

    ThumbnailGeneratorPtr defaultGenerator = ThumbnailGeneratorFactory::getDefaultGenerator(flow_->slideSize());

    for( int i = 0; i < fileList.size(); ++i )
    {
        const QFileInfo& fileInfo = fileList.at( i );
        const QString& fileName = currentDir_.absoluteFilePath( fileInfo.fileName( ));
        QImage img = defaultGenerator->generate(fileName);
        flow_->addSlide( img, fileInfo.fileName( ));
        slideImagesLoaded_.append(qMakePair(false, fileName));
    }
}


void DockPixelStreamer::addFoldersToFlow()
{
    currentDir_.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
    currentDir_.setNameFilters( QStringList( ));
    const QFileInfoList& dirList = currentDir_.entryInfoList();

    FolderThumbnailGeneratorPtr folderGenerator = ThumbnailGeneratorFactory::getFolderGenerator(flow_->slideSize());

    for( int i = 0; i < dirList.size(); ++i )
    {
        const QFileInfo& fileInfo = dirList.at( i );
        const QString& fileName = currentDir_.absoluteFilePath( fileInfo.fileName( ));
        if( !fileName.endsWith( ".pyramid" ))
        {
            QImage img = folderGenerator->generatePlaceholderImage(QDir(fileName));
            flow_->addSlide( img, fileInfo.fileName( ));
            slideImagesLoaded_.append(qMakePair(false, fileName));
        }
    }
}
