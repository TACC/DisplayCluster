// NOTE - looks like the jpeg encoder is not thread safe!  
// Still - test parallel pixel streams, just don't encode
// in parallel

#define USE_THREADING 0 

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

#include <iostream>

#include "MainWindow.h"
#include "main.h"
#include "log.h"
#include "MessageHeader.h"
#include "DesktopSelectionRectangle.h"
#include <turbojpeg.h>

#include <QtCore/QElapsedTimer>
#include <thread>
#include <chrono>

#ifdef _WIN32
    typedef __int32 int32_t;
    #include <windows.h>
#else
    #include <stdint.h>
#endif

void
computeSegmentJpeg(ParallelPixelStreamSegment *segment)
{
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

    int success = tjCompress2(handle, 
			 image.scanLine(segment->parameters.y) + segment->parameters.x * image.depth()/8,
			 segment->parameters.width,
			 image.bytesPerLine(),
			 segment->parameters.height,
			 pixelFormat,
			 jpegBuf,
			 &jpegSize,
			 jpegSubsamp,
			 jpegQual,
			 flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
    }

    // move the JPEG buffer to a byte array and free the libjpeg-turbo allocated memory
    QByteArray byteArray((char *)jpegBufPtr, jpegSize);
    free(jpegBufPtr);

    // copy byte array to new segment
    segment->imageData = byteArray;
}

MainWindow::MainWindow()
{
    // defaults
    updatedDimensions_ = true;
    parallelStreaming_ = false;

    QWidget * widget = new QWidget();
    QGridLayout * layout = new QGridLayout();
    widget->setLayout(layout);

    setCentralWidget(widget);

    // connect widget signals / slots
    connect(&xSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&ySpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&widthSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&heightSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));

    // constrain valid range and default to a quarter of the desktop, centered
    QRect desktopRect = QGuiApplication::primaryScreen()->geometry();

    xSpinBox_.setRange(0, desktopRect.width());
    ySpinBox_.setRange(0, desktopRect.height());
    widthSpinBox_.setRange(1, desktopRect.width());
    heightSpinBox_.setRange(1, desktopRect.height());

    // default to the whole screen quadrant of the screen
    xSpinBox_.setValue(0);
    ySpinBox_.setValue(0);

    widthSpinBox_.setValue(desktopRect.width());
    heightSpinBox_.setValue(desktopRect.height());

    // call updateCoordinates() to commit coordinates from the UI
    updateCoordinates();

    // frame rate limiting
    frameRateSpinBox_.setRange(1, 60);
    frameRateSpinBox_.setValue(24);

		frameRateLabel_.setText("0");

    // add widgets to UI
    int row = 0;

#define ADD_LABELLED_ROW(txt, wid) 	 \
{																		 \
  auto *l = new QLabel(txt); 						 \
	l->setBuddy(&wid);								 \
	layout->addWidget(l, row, 0);			 \
	layout->addWidget(&wid, row++, 1); \
}

    ADD_LABELLED_ROW("Hostname", hostnameLineEdit_);
    ADD_LABELLED_ROW("Stream name", uriLineEdit_);
    ADD_LABELLED_ROW("X", xSpinBox_);
    ADD_LABELLED_ROW("Y", ySpinBox_);
    ADD_LABELLED_ROW("Width", widthSpinBox_);
    ADD_LABELLED_ROW("Height", heightSpinBox_);
    ADD_LABELLED_ROW("Max frame rate", frameRateSpinBox_);
    ADD_LABELLED_ROW("Actual frame rate", frameRateLabel_);

		QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line, row++, 0, 1, 2);
		row++;

		shareDesktopButton_.setText("Share");
		connect(&shareDesktopButton_, SIGNAL(clicked()), this, SLOT(toggleShareDesktop()));
		layout->addWidget(&shareDesktopButton_, row, 0, 1, 2);
 
    // show desktop selection window action
    showDesktopSelectionWindowAction_ = new QAction("Show Rectangle", this);
    showDesktopSelectionWindowAction_->setStatusTip("Show desktop selection rectangle");
    connect(showDesktopSelectionWindowAction_, SIGNAL(triggered()), this, SLOT(showDesktopSelectionWindow()));

    // reset to full window
    resetDesktopSelectionWindowAction_ = new QAction("Reset Rectangle", this);
    resetDesktopSelectionWindowAction_->setStatusTip("Reset desktop selection rectangle");
    connect(resetDesktopSelectionWindowAction_, SIGNAL(triggered()), this, SLOT(resetDesktopSelectionWindow()));

    // set parallel streaming action
    setParallelStreamingAction_ = new QAction("Enable Parallel Streaming", this);
    setParallelStreamingAction_->setStatusTip("Enable parallel streaming");
    setParallelStreamingAction_->setCheckable(true);
    setParallelStreamingAction_->setChecked(parallelStreaming_);
    connect(setParallelStreamingAction_, SIGNAL(toggled(bool)), this, SLOT(setParallelStreaming(bool)));

    // create options menu
    QMenu * optionsMenu = menuBar()->addMenu("&Options");

    // add actions to options menu
    optionsMenu->addAction(showDesktopSelectionWindowAction_);
    optionsMenu->addAction(resetDesktopSelectionWindowAction_);
    optionsMenu->addAction(setParallelStreamingAction_);

    // timer will trigger updating of the desktop image
    connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));

		resizing_ = false;

    show();
}

void MainWindow::resetSharing()
{
		isSharing_ = true;
		toggleShareDesktop();
}

void MainWindow::toggleShareDesktop()
{
	  if (isSharing_) 
				shareDesktopButton_.setText("Share");
		else
				shareDesktopButton_.setText("End Sharing");

		isSharing_ = !isSharing_;
		shareDesktop(isSharing_);
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
    if(set == true)
    {
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
						resetSharing();
            return;
        }

        // handshake
        while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < (int)sizeof(int32_t))
        {
#ifndef _WIN32
            std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif
        }

        int32_t protocolVersion = -1;
        tcpSocket_.read((char *)&protocolVersion, sizeof(int32_t));

        if(protocolVersion != SUPPORTED_NETWORK_PROTOCOL_VERSION)
        {
            tcpSocket_.disconnectFromHost();
						resetSharing();
            put_flog(LOG_ERROR, "unsupported protocol version %i > %i", protocolVersion, SUPPORTED_NETWORK_PROTOCOL_VERSION);
            QMessageBox::warning(this, "Error", "This version is incompatible with the DisplayCluster instance you connected to. (" + QString::number(protocolVersion) + " != " + QString::number(SUPPORTED_NETWORK_PROTOCOL_VERSION) + ")", QMessageBox::Ok, QMessageBox::Ok);

            return;
        }

        // make sure dimensions get updated
        updatedDimensions_ = true;

        shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
    }
    else
    {
        tcpSocket_.disconnectFromHost();

        shareDesktopUpdateTimer_.stop();

        frameRateLabel_.setText("");
    }
}

void MainWindow::showDesktopSelectionWindow()
{
		g_desktopSelectionWindow->showFullScreen();
}

void MainWindow::resetDesktopSelectionWindow()
{
    QRect desktopRect = QGuiApplication::primaryScreen()->geometry();
		setCoordinates(0, 0, desktopRect.width(), desktopRect.height());

		g_desktopSelectionWindow->getDesktopSelectionView()->getDesktopSelectionRectangle()->reset();
		g_desktopSelectionWindow->placeButton(0, 0);
}

void MainWindow::hideDesktopSelectionWindow()
{
		g_desktopSelectionWindow->hide();
}

void MainWindow::setParallelStreaming(bool set)
{
    parallelStreaming_ = set;
}

void MainWindow::shareDesktopUpdate()
{
		if (isResizing()) return;

    // time the frame
    QElapsedTimer frameTime;
    frameTime.start();

    if(tcpSocket_.state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");
        QMessageBox::warning(this, "Error", "Socket is not connected.", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    // take screenshot

		QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap desktopPixmap = screen->grabWindow(0, x_, y_, width_, height_);

    if(desktopPixmap.isNull() == true)
    {
        put_flog(LOG_ERROR, "got NULL desktop pixmap");
        QMessageBox::warning(this, "Error", "Got NULL desktop pixmap.", QMessageBox::Ok, QMessageBox::Ok);

        tcpSocket_.disconnectFromHost();
        resetSharing();
        return;
    }

    // convert to QImage
    image_ = desktopPixmap.toImage();

#if 0
		// Color the segments for debugging
		int bpp = image_.depth() / 8;

		for (long unsigned int s = 0; s < segments_.size(); s++)
		{
			int k = s % 3;

			for (int y = 0; y < segments_[s].parameters.height; y++)
			{
				unsigned char *ptr = image_.scanLine(segments_[s].parameters.y + y) + (segments_[s].parameters.x * bpp);

				for (int x = 0; x < segments_[s].parameters.width; x++)
				{
					ptr[k] = 0xff;
					ptr += bpp;
				}
			}
		}
#endif

    bool success;
    if(parallelStreaming_ == false)
    {
        // stream as one big image
        success = serialStream();

        // check if we updated dimensions
        if(updatedDimensions_ == true)
        {
            // updated dimensions
            int dimensions[2];
            dimensions[0] = width_;
            dimensions[1] = height_;

            int dimensionsSize = 2 * sizeof(int);

            // header
            MessageHeader mh;
            mh.size = dimensionsSize;
            mh.type = MESSAGE_TYPE_PIXELSTREAM_DIMENSIONS_CHANGED;

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
            sent = tcpSocket_.write((const char *)dimensions, dimensionsSize);

            while(sent < dimensionsSize)
            {
                sent += tcpSocket_.write((const char *)dimensions + sent, dimensionsSize - sent);
            }

            updatedDimensions_ = false;

            // wait for acknowledgment
            while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
            {
    #ifndef _WIN32
                std::this_thread::sleep_for(std::chrono::microseconds(10));
    #endif
            }

            tcpSocket_.read(3);
        }
    }
    else
    {
        // stream in segments
        success = parallelStream();

        // no need to watch for dimension changes; server handles it automatically
    }

    // ceck for failure
    if(success == false)
    {
        put_flog(LOG_ERROR, "streaming failure");
        QMessageBox::warning(this, "Error", "Streaming failure.", QMessageBox::Ok, QMessageBox::Ok);

        tcpSocket_.disconnectFromHost();
        resetSharing();
        return;
    }

    // elapsed time (milliseconds)
    int elapsedFrameTime = frameTime.elapsed();

    // frame rate limiting
    int maxFrameRate = frameRateSpinBox_.value();

    int desiredFrameTime = (int)(1000. * 1. / (float)maxFrameRate);

    int sleepTime = desiredFrameTime - elapsedFrameTime;

    if(sleepTime > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }

    // frame rate is calculated for every FRAME_RATE_AVERAGE_NUM_FRAMES sequential frames
    frameSentTimes_.push_back(QTime::currentTime());

    if(frameSentTimes_.size() > FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        frameSentTimes_.clear();
    }
    else if(frameSentTimes_.size() == FRAME_RATE_AVERAGE_NUM_FRAMES)
    {
        float fps = (float)frameSentTimes_.size() / (float)frameSentTimes_.front().msecsTo(frameSentTimes_.back()) * 1000.;

        frameRateLabel_.setText(QString::number(fps) + QString(" fps"));
    }
}

void MainWindow::updateCoordinates()
{
    if(widthSpinBox_.value() != width_ || heightSpinBox_.value() != height_)
    {
        updatedDimensions_ = true;
    }

    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();

    // update DesktopSelectionRectangle
    if(g_desktopSelectionWindow != NULL)
    {
        g_desktopSelectionWindow->getDesktopSelectionView()->getDesktopSelectionRectangle()->setCoordinates(x_, y_, width_, height_);
    }

    // update ParallelPixelStreamSegment parameters, whether or not we are currently streaming in parallel
    // users can toggle parallel streaming at any time
    segments_.clear();

    // segment dimensions will be approximately this
    int nominalSegmentSize = 512;

    // number of subdivisions in each dimensions
    int numSubdivisionsX = (width_ / nominalSegmentSize) + 1;
    int numSubdivisionsY = (height_ / nominalSegmentSize) + 1;

		int dx = width_ / numSubdivisionsX;
		int dy = height_ / numSubdivisionsY;

    // now, create segments with appropriate parameters
    for(int i=0; i<numSubdivisionsX; i++)
    {
				int x = i*dx;
				int w =  (i == (numSubdivisionsX-1)) ? (width_ - x)-1 : dx;

        for(int j=0; j<numSubdivisionsY; j++)
        {
						int y = j*dy;
						int h = (j == (numSubdivisionsY-1)) ? (height_ - y)-1 : dy;

            ParallelPixelStreamSegment segment;

            segment.parameters.sourceIndex = i*numSubdivisionsY + j;
            segment.parameters.x = x;
            segment.parameters.y = y;
            segment.parameters.width = w;
            segment.parameters.height = h;
            segment.parameters.totalWidth = width_;
            segment.parameters.totalHeight = height_;

            segments_.push_back(segment);
        }
    }
}

bool MainWindow::serialStream()
{
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

    int success = tjCompress2(handle, image_.scanLine(0), image_.width(), image_.bytesPerLine(), image_.height(), pixelFormat, jpegBuf, &jpegSize, jpegSubsamp, jpegQual, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
        QMessageBox::warning(this, "Error", "Image conversion failure.", QMessageBox::Ok, QMessageBox::Ok);

        return false;
    }

    // move the JPEG buffer to a byte array and free the libjpeg-turbo allocated memory
    QByteArray byteArray((char *)jpegBufPtr, jpegSize);
    free(jpegBufPtr);

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
            std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif
        }

        tcpSocket_.read(3);
    }

    return true;
}

static void * to_jpeg(void *d);

struct args
{
	QImage* image_;
	ParallelPixelStreamSegment* segment;
	unsigned char *buf = NULL;
	unsigned long size = 0;
#if USE_THREADING
	// pthreads isn't available on Windows without an extra compatibility
	// layer - harmless to require it only when USE_THREADING is actually
	// enabled (see the comment at the top of this file: it's off by
	// default because the JPEG encoder isn't thread-safe)
	pthread_t tid;
#endif
	int frameIndex;

	args(QImage* i, ParallelPixelStreamSegment* s, int fi) : image_(i), segment(s), frameIndex(fi)
	{
#if USE_THREADING
		pthread_create(&tid, NULL, to_jpeg, (void *)this);
#else
		to_jpeg(this);
#endif
	}

	~args() { if (buf) free(buf); }
};

static void *
to_jpeg(void *d)
{
	struct args *args_ = (struct args *)d;

	tjhandle handle = tjInitCompress();
	int pixelFormat = TJPF_BGRX;
	int jpegSubsamp = TJSAMP_444;
	int jpegQual = JPEG_QUALITY;
	int flags = 0;

	int success = tjCompress2(handle, 
		args_->image_->scanLine(args_->segment->parameters.y) + args_->segment->parameters.x * args_->image_->depth()/8,
		args_->segment->parameters.width,
		args_->image_->bytesPerLine(),
		args_->segment->parameters.height,
		pixelFormat,
		&args_->buf,
		&args_->size,
		jpegSubsamp,
		jpegQual,
		flags);

	if(success != 0)
	{
			put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
	}

#if USE_THREADING
	pthread_exit(0);
#else
	return NULL;
#endif
}

bool MainWindow::parallelStream()
{
    // frame index
    static int frameIndex = 0;

		std::vector<args*> tasks;
		for (long unsigned i = 0; i < segments_.size(); i++)
		{
			tasks.push_back(new args(&image_, &segments_[i], frameIndex));
		}

		for (auto t : tasks)
		{
#if USE_THREADING
			pthread_join(t->tid, NULL);
#endif
		
			// send the parameters and image data
			MessageHeader mh;
			mh.size = sizeof(ParallelPixelStreamSegmentParameters) + t->size;
			mh.type = MESSAGE_TYPE_PARALLEL_PIXELSTREAM;

			// add the truncated URI to the header
			size_t len = uri_.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
			mh.uri[len] = '\0';

			// send the header
			unsigned long sent = tcpSocket_.write((const char *)&mh, sizeof(MessageHeader));

			while(sent < (int)sizeof(MessageHeader))
			{
					sent += tcpSocket_.write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
			}

			// send the message

			// part 1: parameters
			sent = tcpSocket_.write((const char *)&(t->segment->parameters), sizeof(ParallelPixelStreamSegmentParameters));

			while(sent < (int)sizeof(ParallelPixelStreamSegmentParameters))
			{
					sent += tcpSocket_.write((const char *)&(t->segment->parameters) + sent, sizeof(ParallelPixelStreamSegmentParameters) - sent);
			}

			// part 2: image data
			sent = tcpSocket_.write((const char *)t->buf, t->size);

			while(sent < t->size)
			{
					sent += tcpSocket_.write((const char *)t->buf + sent, t->size - sent);
			}

			// wait for acknowledgment
			while(tcpSocket_.waitForReadyRead() && tcpSocket_.bytesAvailable() < 3)
			{
#ifndef _WIN32
            std::this_thread::sleep_for(std::chrono::microseconds(10));
#endif
			}

			tcpSocket_.read(3);
			delete t;
    }

    // increment frame index
    frameIndex++;

    return true;
}
