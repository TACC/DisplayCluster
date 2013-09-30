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
#include "Content.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"
#include "MovieContent.h"
#include "main.h"

#define DOCK_UNIQUE_URI "menu"

namespace
{
QImage createSlideImage_( const QSize& size, const QString& fileName,
                          const bool isDir, const QColor& bgcolor1,
                          const QColor& bgcolor2 )
{
    QImage img( size, QImage::Format_RGB32 );
    img.setText( "source", fileName );
    if( isDir)
        img.setText( "dir", "true" );
    QPainter painter( &img );
    QPoint p1( img.width()*4/10, 0 );
    QPoint p2( img.width()*6/10, img.height( ));
    QLinearGradient linearGrad( p1, p2 );
    linearGrad.setColorAt( 0, bgcolor1 );
    linearGrad.setColorAt( 1, bgcolor2 );
    painter.setBrush(linearGrad);
    painter.fillRect( 0, 0, img.width(), img.height(), QBrush(linearGrad));
    painter.end();
    return img;
}
}


QString DockPixelStreamer::getUniqueURI()
{
    return QString(DOCK_UNIQUE_URI);
}

DockPixelStreamer::DockPixelStreamer()
    : LocalPixelStreamer(QString(DOCK_UNIQUE_URI))
{
    av_register_all();

    flow_ = new PictureFlow;
    flow_->resize( g_configuration->getTotalWidth()/8,
                   g_configuration->getTotalHeight()/8 );
    const int height = flow_->height() * .8;
    flow_->setSlideSize( QSize( 0.6 * height, height ));
    flow_->setBackgroundColor( Qt::darkGray );

    connect( flow_, SIGNAL( imageUpdated( const QImage& )), this, SLOT( update( const QImage& )));


    loader_ = new AsyncImageLoader(flow_->slideSize());
    loader_->moveToThread( &loadThread_ );
    connect( loader_, SIGNAL(imageLoaded(int, QImage)),
             flow_, SLOT(setSlide( int, QImage )));
    connect( this, SIGNAL(renderPreview( const QString&, const int )),
             loader_, SLOT(loadImage( const QString&, const int )));
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
        const int offs = interactionState.dx * flow_->size().width() / 4;
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
        const QString extension = QFileInfo(source).suffix().toLower();
        if( extension == "dcx" )
        {
            g_displayGroupManager->loadStateXMLFile( source.toStdString( ));

            emit(streamClosed(getUniqueURI()));
        }
        else {
            boost::shared_ptr< Content > c = ContentFactory::getContent( source );
            if( c )
            {
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));
                g_displayGroupManager->addContentWindowManager( cwm );
                cwm->adjustSize( SIZE_1TO1 ); // TODO Remove this when content dimensions request is no longer needed

                // Center the content where the dock is
                boost::shared_ptr<ContentWindowManager> dockCwm = g_displayGroupManager->getContentWindowManager(getUniqueURI(), CONTENT_TYPE_PIXEL_STREAM);
                double dockX, dockY;
                dockCwm->getPosition(dockX, dockY);
                cwm->centerPositionAround(dockX, dockY, true);

                emit(streamClosed(getUniqueURI()));
            }
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

void DockPixelStreamer::changeDirectory( const QString& dir )
{
    slideIndex_[currentDir_.path()] = flow_->centerIndex();

    flow_->clear();

    currentDir_ = dir;
    currentDir_.setFilter( QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot );
    QStringList filters = ContentFactory::getSupportedFilesFilter();
    filters.append( "*.dcx" );
    currentDir_.setNameFilters( filters );
    const QFileInfoList& fileList = currentDir_.entryInfoList();

    currentDir_.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
    currentDir_.setNameFilters( QStringList( ));
    const QFileInfoList& dirList = currentDir_.entryInfoList();

    QDir rootDir = dir;
    const bool hasRootDir = rootDir.cdUp();

    if( hasRootDir)
    {
        QString upFolder = rootDir.path();
        QImage img = createSlideImage_( flow_->slideSize(), upFolder, true,
                                        Qt::darkGray, Qt::lightGray );
        flow_->addSlide( img, "UP: " + upFolder );
    }

    for( int i = 0; i < fileList.size(); ++i )
    {
        QFileInfo fileInfo = fileList.at( i );
        const QString& fileName = currentDir_.absoluteFilePath( fileInfo.fileName( ));
        QImage img = createSlideImage_( flow_->slideSize(), fileName, false,
                                        Qt::black, Qt::white );
        flow_->addSlide( img, fileInfo.fileName( ));
        emit renderPreview( fileName, flow_->slideCount()-1 );
    }

    for( int i = 0; i < dirList.size(); ++i )
    {
        QFileInfo fileInfo = dirList.at( i );
        const QString& fileName = currentDir_.absoluteFilePath( fileInfo.fileName( ));
        if( !fileName.endsWith( ".pyramid" ))
        {
            QImage img = createSlideImage_( flow_->slideSize(), fileName, true,
                                            Qt::black, Qt::white );
            flow_->addSlide( img, fileInfo.fileName( ));
        }
    }

    flow_->setCenterIndex( slideIndex_[currentDir_.path()] );
}



AsyncImageLoader::AsyncImageLoader(QSize defaultSize)
    : defaultSize_(defaultSize)
{
}

void AsyncImageLoader::loadImage( const QString& fileName, const int index )
{
    const QString extension = QFileInfo(fileName).suffix().toLower();

    if( extension == "dcx" )
    {
        QImage image = createSlideImage_( defaultSize_, fileName, false,
                                        Qt::darkCyan, Qt::cyan );
        emit imageLoaded(index, image);
        return;
    }

    if( extension == "pyr" )
    {
        QImageReader reader( fileName + "amid/0.jpg" );
        QImage image = reader.read();
        image.setText( "source", fileName );
        emit imageLoaded(index, image);
        return;
    }

    QImageReader reader( fileName );
    if( reader.canRead( ))
    {
        QImage image = reader.read();
        image.setText( "source", fileName );
        emit imageLoaded(index, image);
        return;
    }
    else if( reader.error() != QImageReader::UnsupportedFormatError )
        return;

    if( !MovieContent::getSupportedExtensions().contains( extension ))
        return;

    AVFormatContext* avFormatContext;
    if( avformat_open_input( &avFormatContext, fileName.toAscii(), 0, 0 ) != 0 )
        return;

    if( avformat_find_stream_info( avFormatContext, 0 ) < 0 )
        return;

    // find the first video stream
    int streamIdx = -1;
    for( unsigned int i = 0; i < avFormatContext->nb_streams; ++i )
    {
        if( avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            streamIdx = i;
            break;
        }
    }

    if( streamIdx == -1 )
        return;

    AVStream* videostream = avFormatContext->streams[streamIdx];
    AVCodecContext* avCodecContext = videostream->codec;
    AVCodec* codec = avcodec_find_decoder( avCodecContext->codec_id );
    if( !codec )
        return;

    if( avcodec_open2( avCodecContext, codec, 0 ) < 0 )
        return;

    AVFrame* avFrame = avcodec_alloc_frame();
    AVFrame* avFrameRGB = avcodec_alloc_frame();

    QImage image( avCodecContext->width, avCodecContext->height,
                  QImage::Format_RGB32 );
    int numBytes = avpicture_get_size( PIX_FMT_RGB24, image.width(),
                                       image.height( ));
    uint8_t* buffer = (uint8_t*)av_malloc( numBytes * sizeof(uint8_t));
    avpicture_fill( (AVPicture*)avFrameRGB, buffer, PIX_FMT_RGB24,
                    image.width(), image.height( ));

    SwsContext* swsContext = sws_getContext( avCodecContext->width,
                                             avCodecContext->height,
                                             avCodecContext->pix_fmt,
                                             image.width(), image.height(),
                                             PIX_FMT_RGB24, SWS_FAST_BILINEAR,
                                             0, 0, 0 );

    // seek to 10% of movie time
    const int64_t den2 = videostream->time_base.den * videostream->r_frame_rate.den;
    const int64_t num2 = videostream->time_base.num * videostream->r_frame_rate.num;
    const int64_t num_frames = av_rescale( videostream->duration, num2, den2 );
    const int64_t desiredTimestamp = videostream->start_time +
                                       av_rescale( num_frames / 10, den2, num2 );
    if( avformat_seek_file( avFormatContext, streamIdx, 0, desiredTimestamp,
                            desiredTimestamp, 0 ) != 0 )
    {
        return;
    }

    AVPacket packet;
    while( av_read_frame( avFormatContext, &packet ) >= 0 )
    {
        if( packet.stream_index != streamIdx )
            continue;

        int frameFinished;
        avcodec_decode_video2( avCodecContext, avFrame, &frameFinished,
                               &packet );

        if( !frameFinished )
            continue;

        sws_scale( swsContext, avFrame->data, avFrame->linesize, 0,
                   avCodecContext->height, avFrameRGB->data,
                   avFrameRGB->linesize );

        unsigned char* src = (unsigned char*)avFrameRGB->data[0];
        for( int y = 0; y < image.height(); ++y )
        {
            QRgb* scanLine = (QRgb*)image.scanLine(y);
            for( int x = 0; x < image.width(); ++x )
                scanLine[x] = qRgb(src[3*x], src[3*x+1], src[3*x+2]);
            src += avFrameRGB->linesize[0];
        }

        av_free_packet( &packet );

        image.setText( "source", fileName );
        emit imageLoaded(index, image);
        break;
    }

    avformat_close_input( &avFormatContext );
    sws_freeContext( swsContext );
    av_free( avFrame );
    av_free( avFrameRGB );
}
