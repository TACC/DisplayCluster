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

#include "MainWindow.h"
#include "main.h"
#include "../../../src/log.h"
#include "../../../src/MessageHeader.h"
#include "DesktopSelectionRectangle.h"
#include <turbojpeg.h>

#ifdef _WIN32
    typedef __int32 int32_t;
    #include <windows.h>
#else
    #include <stdint.h>
#endif

ParallelPixelStreamSegment computeSegmentJpeg(const ParallelPixelStreamSegment & segment)
{
    ParallelPixelStreamSegment newSegment = segment;

    QImage image = g_mainWindow->getImage();

    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = tjInitCompress();
    int pixelFormat = TJPF_BGRX;
    unsigned char ** jpegBuf;
    unsigned char * jpegBufPtr = NULL;
    jpegBuf = &jpegBufPtr;
    unsigned long jpegSize = 0;
    int jpegSubsamp = TJSAMP_444;
    int jpegQual = JPEG_QUALITY;
    int flags = 0;

    int success = tjCompress2(handle, image.scanLine(newSegment.parameters.y) + newSegment.parameters.x * image.depth()/8, newSegment.parameters.width, image.bytesPerLine(), newSegment.parameters.height, pixelFormat, jpegBuf, &jpegSize, jpegSubsamp, jpegQual, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");

        return newSegment;
    }

    // move the JPEG buffer to a byte array and free the libjpeg-turbo allocated memory
    QByteArray byteArray((char *)jpegBufPtr, jpegSize);
    free(jpegBufPtr);

    // copy byte array to new segment
    newSegment.imageData = byteArray;

    return newSegment;
}

MainWindow::MainWindow()
{
    // defaults
    updatedDimensions_ = true;
    parallelStreaming_ = false;
    deviceScale_ = 1.f;
    cursor_ = QImage( ":/cursor.png" ).scaled( 20 * deviceScale_,
                                               20 * deviceScale_,
                                               Qt::KeepAspectRatio );
    QWidget * widget = new QWidget();
    QFormLayout * layout = new QFormLayout();
    widget->setLayout(layout);

    setCentralWidget(widget);

    // connect widget signals / slots
    connect(&xSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&ySpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&widthSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&heightSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&retinaBox_, SIGNAL(released()), this, SLOT(updateCoordinates()));

    hostnameLineEdit_.setText( "bbplxviz03.epfl.ch" );

    char hostname[256] = {0};
    gethostname( hostname, 256 );
    uriLineEdit_.setText( hostname );

    const int screen = -1;
    QRect desktopRect = QApplication::desktop()->screenGeometry( screen );

    xSpinBox_.setRange(0, desktopRect.width());
    ySpinBox_.setRange(0, desktopRect.height());
    widthSpinBox_.setRange(1, desktopRect.width());
    heightSpinBox_.setRange(1, desktopRect.height());

    // default to full screen
    xSpinBox_.setValue(0);
    ySpinBox_.setValue(0);
    widthSpinBox_.setValue( desktopRect.width( ));
    heightSpinBox_.setValue( desktopRect.height( ));

    // call updateCoordinates() to commit coordinates from the UI
    updateCoordinates();

    // frame rate limiting
    frameRateSpinBox_.setRange(1, 60);
    frameRateSpinBox_.setValue(24);

    // add widgets to UI
    layout->addRow("Hostname", &hostnameLineEdit_);
    layout->addRow("Stream name", &uriLineEdit_);
    layout->addRow("X", &xSpinBox_);
    layout->addRow("Y", &ySpinBox_);
    layout->addRow("Width", &widthSpinBox_);
    layout->addRow("Height", &heightSpinBox_);
    layout->addRow("Retina Display", &retinaBox_);
    layout->addRow("Max frame rate", &frameRateSpinBox_);
    layout->addRow("Actual frame rate", &frameRateLabel_);

    // share desktop action
    shareDesktopAction_ = new QAction("Share Desktop", this);
    shareDesktopAction_->setStatusTip("Share desktop");
    shareDesktopAction_->setCheckable(true);
    shareDesktopAction_->setChecked(false);
    connect(shareDesktopAction_, SIGNAL(toggled(bool)), this, SLOT(shareDesktop(bool)));

    // show desktop selection window action
    showDesktopSelectionWindowAction_ = new QAction("Show Rectangle", this);
    showDesktopSelectionWindowAction_->setStatusTip("Show desktop selection rectangle");
    showDesktopSelectionWindowAction_->setCheckable(true);
    showDesktopSelectionWindowAction_->setChecked(false);
    connect(showDesktopSelectionWindowAction_, SIGNAL(toggled(bool)), this, SLOT(showDesktopSelectionWindow(bool)));

    // set parallel streaming action
    QAction * setParallelStreamingAction = new QAction("Enable Parallel Streaming", this);
    setParallelStreamingAction->setStatusTip("Enable parallel streaming");
    setParallelStreamingAction->setCheckable(true);
    setParallelStreamingAction->setChecked(parallelStreaming_);
    connect(setParallelStreamingAction, SIGNAL(toggled(bool)), this, SLOT(setParallelStreaming(bool)));

    // create toolbar
    QToolBar * toolbar = addToolBar("toolbar");

    // add buttons to toolbar
    toolbar->addAction(shareDesktopAction_);
    toolbar->addAction(showDesktopSelectionWindowAction_);

    // create options menu
    QMenu * optionsMenu = menuBar()->addMenu("&Options");

    // add actions to options menu
    optionsMenu->addAction(setParallelStreamingAction);

    // timer will trigger updating of the desktop image
    connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));

    show();
}

void MainWindow::closeEvent( QCloseEvent* event )
{
    shareDesktopUpdateTimer_.stop();
    sendQuit();
    QMainWindow::closeEvent( event );
}

void MainWindow::getCoordinates(int &x, int &y, int &width, int &height)
{
    x = x_;
    y = y_;
    width = width_;
    height = height_;
}

void MainWindow::setCoordinates(int x, int y, int width, int height)
{
    if(width != width_ || height != height_)
    {
        updatedDimensions_ = true;
    }

    xSpinBox_.setValue(x);
    ySpinBox_.setValue(y);
    widthSpinBox_.setValue(width);
    heightSpinBox_.setValue(height);

    // the spinboxes only update the UI; we must update the actual values too
    updateCoordinates();
}

QImage MainWindow::getImage()
{
    return image_;
}

void MainWindow::shareDesktop(bool set)
{
    if( !set )
    {
        shareDesktopUpdateTimer_.stop();
        sendQuit();
        tcpSocket_.disconnectFromHost();
        frameRateLabel_.setText("");
        return;
    }

    // save values from UI: hostname and uri can only be updated here--not during streaming
    hostname_ = hostnameLineEdit_.text().toStdString();
    uri_ = uriLineEdit_.text().toStdString();

    // open connection (disconnecting from an existing connection if necessary)
    tcpSocket_.disconnectFromHost();
    tcpSocket_.connectToHost(hostname_.c_str(), 1701);

    if(tcpSocket_.waitForConnected() != true)
    {
        put_flog(LOG_ERROR, "could not connect");
        QMessageBox::warning(this, "Error", "Could not connect.", QMessageBox::Ok, QMessageBox::Ok);

        shareDesktopAction_->setChecked(false);
        return;
    }

    // handshake
    while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < (int)sizeof(int32_t))
    {
#ifndef _WIN32
        usleep(10);
#endif
    }

    int32_t protocolVersion = -1;
    tcpSocket_.read((char *)&protocolVersion, sizeof(int32_t));

    if(protocolVersion != SUPPORTED_NETWORK_PROTOCOL_VERSION)
    {
        tcpSocket_.disconnectFromHost();
        shareDesktopAction_->setChecked(false);

        put_flog(LOG_ERROR, "unsupported protocol version %i > %i", protocolVersion, SUPPORTED_NETWORK_PROTOCOL_VERSION);
        QMessageBox::warning(this, "Error", "This version is incompatible with the DisplayCluster instance you connected to. (" + QString::number(protocolVersion) + " != " + QString::number(SUPPORTED_NETWORK_PROTOCOL_VERSION) + ")", QMessageBox::Ok, QMessageBox::Ok);

        return;
    }

    // make sure dimensions get updated
    updatedDimensions_ = true;

    shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
}

void MainWindow::showDesktopSelectionWindow(bool set)
{
    set ?
        g_desktopSelectionWindow->showFullScreen() :
        g_desktopSelectionWindow->hide();

    // this slot may be called externally, so make sure the action checked value matches the set argument
    if(showDesktopSelectionWindowAction_->isChecked() != set)
    {
        showDesktopSelectionWindowAction_->setChecked(set);
    }
}

void MainWindow::setParallelStreaming(bool set)
{
    parallelStreaming_ = set;
}

void MainWindow::shareDesktopUpdate()
{
    // time the frame
    QTime frameTime;
    frameTime.start();

    if(tcpSocket_.state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");
        QMessageBox::warning(this, "Error", "Socket is not connected.", QMessageBox::Ok, QMessageBox::Ok);

        shareDesktopAction_->setChecked(false);
        return;
    }

    // take screenshot
    const int w = width_ * deviceScale_;
    const int h = height_ * deviceScale_;

    QPixmap desktopPixmap =
        QPixmap::grabWindow( QApplication::desktop()->winId(), x_, y_, w, h );
    //std::cout << desktopPixmap.devicePixelRatio() << std::endl;

    if( desktopPixmap.isNull( ))
    {
        put_flog(LOG_ERROR, "got NULL desktop pixmap");
        QMessageBox::warning(this, "Error", "Got NULL desktop pixmap.", QMessageBox::Ok, QMessageBox::Ok);

        sendQuit();
        tcpSocket_.disconnectFromHost();
        shareDesktopAction_->setChecked(false);
        return;
    }

    // convert to QImage
    image_ = desktopPixmap.toImage();

    // render mouse cursor
    QPoint mousePos = ( QCursor::pos() - QPoint( x_, y_ )) * deviceScale_ -
        QPoint( cursor_.width()/2, cursor_.height()/2);
    QPainter painter( &image_ );
    painter.drawImage( mousePos, cursor_ );

    bool success;

    if( parallelStreaming_ )
    {
        success = parallelStream();
        // no need to watch for dimension changes; server handles it automatically
    }
    else
    {
        // stream as one big image
        success = serialStream();
        sendDimensions();
    }

    // check for failure
    if( !success )
    {
        put_flog(LOG_ERROR, "streaming failure");
        QMessageBox::warning(this, "Error", "Streaming failure.", QMessageBox::Ok, QMessageBox::Ok);

        sendQuit();
        tcpSocket_.disconnectFromHost();
        shareDesktopAction_->setChecked(false);
        return;
    }

    // elapsed time (milliseconds)
    const int elapsedFrameTime = frameTime.elapsed();

    // frame rate limiting
    const int maxFrameRate = frameRateSpinBox_.value();
    const int desiredFrameTime = (int)(1000. * 1. / (float)maxFrameRate);
    const int sleepTime = desiredFrameTime - elapsedFrameTime;

    if(sleepTime > 0)
    {
#ifdef _WIN32
        Sleep(sleepTime);
#else
        usleep(1000 * sleepTime);
#endif
    }

    // frame rate is calculated for every FRAME_RATE_AVERAGE_NUM_FRAMES sequential frames
    frameSentTimes_.push_back(QTime::currentTime());

    if(frameSentTimes_.size() > FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        frameSentTimes_.clear();
    }
    else if(frameSentTimes_.size() == FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        const float fps = (float)frameSentTimes_.size() / (float)frameSentTimes_.front().msecsTo(frameSentTimes_.back()) * 1000.;

        frameRateLabel_.setText(QString::number(fps) + QString(" fps"));
    }
}

void MainWindow::sendDimensions()
{
    if( !updatedDimensions_ )
        return;

    // updated dimensions
    const int dimensions[2] = {  int(width_*deviceScale_),
                                 int(height_*deviceScale_) };
    const int dimensionsSize = 2 * sizeof(int);

    // header
    MessageHeader mh;
    mh.size = dimensionsSize;
    mh.type = MESSAGE_TYPE_PIXELSTREAM_DIMENSIONS_CHANGED;

    // add the truncated URI to the header
    const size_t len = uri_.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
    mh.uri[len] = '\0';

    // send the header
    int sent = tcpSocket_.write((const char *)&mh, sizeof(MessageHeader));
    while(sent < (int)sizeof(MessageHeader))
    {
        sent += tcpSocket_.write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    // send the message
    sent = tcpSocket_.write((const char *)dimensions, dimensionsSize);
    while(sent < dimensionsSize)
    {
        sent += tcpSocket_.write((const char *)dimensions + sent, dimensionsSize - sent);
    }

    // wait for acknowledgment
    while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
    {
#ifndef _WIN32
        usleep(10);
#endif
    }

    tcpSocket_.read(3);
    updatedDimensions_ = false;
}

void MainWindow::sendQuit()
{
    if( tcpSocket_.state() != QAbstractSocket::ConnectedState )
        return;

    // header
    MessageHeader mh;
    mh.size = 0;
    mh.type = MESSAGE_TYPE_QUIT;

    // add the truncated URI to the header
    const size_t len = uri_.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
    mh.uri[len] = '\0';

    // send the header
    int sent = tcpSocket_.write((const char *)&mh, sizeof(MessageHeader));
    while(sent < (int)sizeof(MessageHeader))
    {
        sent += tcpSocket_.write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    // wait for acknowledgment
    while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
    {
#ifndef _WIN32
        usleep(10);
#endif
    }

    tcpSocket_.read(3);
}

void MainWindow::updateCoordinates()
{
    const float newScale = retinaBox_.checkState() ? 2.f : 1.f;
    if( widthSpinBox_.value() != width_ || heightSpinBox_.value() != height_ ||
        newScale != deviceScale_ )
    {
        updatedDimensions_ = true;
    }

    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();
    deviceScale_ = newScale;
    cursor_ = QImage( ":/cursor.png" ).scaled( 20 * deviceScale_,
                                               20 * deviceScale_,
                                               Qt::KeepAspectRatio );
    // update DesktopSelectionRectangle
    if( g_desktopSelectionWindow )
    {
        g_desktopSelectionWindow->getDesktopSelectionView()->getDesktopSelectionRectangle()->setCoordinates( x_, y_, width_, height_ );
    }

    // update ParallelPixelStreamSegment parameters, whether or not we are
    // currently streaming in parallel. Users can toggle parallel streaming at
    // any time
    segments_.clear();

    // segment dimensions will be approximately this
    const int w = width_ * deviceScale_;
    const int h = height_ * deviceScale_;
    const int nominalSegmentSize = 512;

    // number of subdivisions in each dimensions
    const int numSubdivisionsX = (int)floor((float)w / (float)nominalSegmentSize + 0.5f);
    const int numSubdivisionsY = (int)floor((float)h / (float)nominalSegmentSize + 0.5f);

    // now, create segments with appropriate parameters
    for(int i=0; i<numSubdivisionsX; i++)
    {
        for(int j=0; j<numSubdivisionsY; j++)
        {
            ParallelPixelStreamSegment segment;

            segment.parameters.sourceIndex = i*numSubdivisionsY + j;
            segment.parameters.x = i * (int)((float)w / (float)numSubdivisionsX);
            segment.parameters.y = j * (int)((float)h / (float)numSubdivisionsY);
            segment.parameters.width = (int)((float)w / (float)numSubdivisionsX);
            segment.parameters.height = (int)((float)h / (float)numSubdivisionsY);
            segment.parameters.totalWidth = w;
            segment.parameters.totalHeight = h;

            segments_.push_back(segment);
        }
    }
}

bool MainWindow::serialStream()
{
    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = tjInitCompress();
    const int pixelFormat = TJPF_BGRX;
    unsigned char* jpegBuf = NULL;
    unsigned long jpegSize = 0;
    int jpegSubsamp = TJSAMP_444;
    int jpegQual = JPEG_QUALITY;
    int flags = 0;

    int success = tjCompress2(handle, image_.scanLine(0), image_.width(), image_.bytesPerLine(), image_.height(), pixelFormat, &jpegBuf, &jpegSize, jpegSubsamp, jpegQual, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
        QMessageBox::warning(this, "Error", "Image conversion failure.", QMessageBox::Ok, QMessageBox::Ok);

        return false;
    }

    // move the JPEG buffer to a byte array and free the libjpeg-turbo allocated memory
    QByteArray byteArray((char *)jpegBuf, jpegSize);
    free( jpegBuf );

    if(byteArray != previousImageData_)
    {
        MessageHeader mh;
        mh.size = byteArray.size();
        mh.type = MESSAGE_TYPE_PIXELSTREAM;

        // add the truncated URI to the header
        size_t len = uri_.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
        mh.uri[len] = '\0';

        // send the header
        int sent = tcpSocket_.write((const char *)&mh, sizeof(MessageHeader));

        while(sent < (int)sizeof(MessageHeader))
        {
            sent += tcpSocket_.write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
        }

        // send the message
        sent = tcpSocket_.write((const char *)byteArray.data(), byteArray.size());

        while(sent < byteArray.size())
        {
            sent += tcpSocket_.write((const char *)byteArray.data() + sent, byteArray.size() - sent);
        }

        previousImageData_ = byteArray;

        // wait for acknowledgment
        while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
        {
#ifndef _WIN32
            usleep(10);
#endif
        }

        tcpSocket_.read(3);
    }

    return true;
}

bool MainWindow::parallelStream()
{
    // frame index
    static int frameIndex = 0;

    // create JPEGs for each segment, in parallel
    std::vector<ParallelPixelStreamSegment> segments = QtConcurrent::blockingMapped<std::vector<ParallelPixelStreamSegment> >(segments_, &computeSegmentJpeg);

    // stream segments
    // we have to stream all of them in case stream synchronization is on (we can't ignore unchanged segments... todo: fix this)
    for(unsigned int i=0; i<segments.size(); i++)
    {
        // update frame index
        segments[i].parameters.frameIndex = frameIndex;

        // send the parameters and image data
        MessageHeader mh;
        mh.size = sizeof(ParallelPixelStreamSegmentParameters) + segments[i].imageData.size();
        mh.type = MESSAGE_TYPE_PARALLEL_PIXELSTREAM;

        // add the truncated URI to the header
        size_t len = uri_.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
        mh.uri[len] = '\0';

        // send the header
        int sent = tcpSocket_.write((const char *)&mh, sizeof(MessageHeader));

        while(sent < (int)sizeof(MessageHeader))
        {
            sent += tcpSocket_.write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
        }

        // send the message

        // part 1: parameters
        sent = tcpSocket_.write((const char *)&(segments[i].parameters), sizeof(ParallelPixelStreamSegmentParameters));

        while(sent < (int)sizeof(ParallelPixelStreamSegmentParameters))
        {
            sent += tcpSocket_.write((const char *)&(segments[i].parameters) + sent, sizeof(ParallelPixelStreamSegmentParameters) - sent);
        }

        // part 2: image data
        sent = tcpSocket_.write((const char *)segments[i].imageData.data(), segments[i].imageData.size());

        while(sent < segments[i].imageData.size())
        {
            sent += tcpSocket_.write((const char *)segments[i].imageData.data() + sent, segments[i].imageData.size() - sent);
        }

        // wait for acknowledgment
        while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
        {
#ifndef _WIN32
            usleep(10);
#endif
        }

        tcpSocket_.read(3);
    }

    // update segments vector
    segments_ = segments;

    // increment frame index
    ++frameIndex;

    return true;
}
