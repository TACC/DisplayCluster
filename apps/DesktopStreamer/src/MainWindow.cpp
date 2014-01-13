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

#include "DesktopSelectionWindow.h"
#include "DesktopSelectionView.h"
#include "DesktopSelectionRectangle.h"

#include "dcstream/Stream.h"

#ifdef _WIN32
    typedef __int32 int32_t;
    #include <windows.h>
#else
    #include <stdint.h>
#endif

#include <unistd.h>
#include <iostream>

#define SHARE_DESKTOP_UPDATE_DELAY      1
#define FRAME_RATE_AVERAGE_NUM_FRAMES  10

#define DEFAULT_HOST_ADDRESS  "bbplxviz03.epfl.ch"
#define CURSOR_IMAGE_FILE     ":/cursor.png"

MainWindow::MainWindow()
    : dcStream_(0)
    , desktopSelectionWindow_(new DesktopSelectionWindow())
    , x_(0)
    , y_(0)
    , width_(0)
    , height_(0)
    , deviceScale_(1.f)
{
    generateCursorImage();
    setupUI();

    // Receive changes from the selection rectangle
    connect(desktopSelectionWindow_->getDesktopSelectionView()->getDesktopSelectionRectangle(),
            SIGNAL(coordinatesChanged(int,int,int,int)),
            this, SLOT(setCoordinates(int,int,int,int)));

    connect(desktopSelectionWindow_, SIGNAL(windowVisible(bool)), showDesktopSelectionWindowAction_, SLOT(setChecked(bool)));
}

void MainWindow::generateCursorImage()
{
    cursor_ = QImage( CURSOR_IMAGE_FILE ).scaled( 20 * deviceScale_,
                                                  20 * deviceScale_,
                                                  Qt::KeepAspectRatio );
}

void MainWindow::setupUI()
{
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

    hostnameLineEdit_.setText( DEFAULT_HOST_ADDRESS );

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
    connect(shareDesktopAction_, SIGNAL(triggered(bool)), this, SLOT(shareDesktop(bool))); // Only user actions
    connect(this, SIGNAL(streaming(bool)), shareDesktopAction_, SLOT(setChecked(bool)));

    // show desktop selection window action
    showDesktopSelectionWindowAction_ = new QAction("Show Rectangle", this);
    showDesktopSelectionWindowAction_->setStatusTip("Show desktop selection rectangle");
    showDesktopSelectionWindowAction_->setCheckable(true);
    showDesktopSelectionWindowAction_->setChecked(false);
    connect(showDesktopSelectionWindowAction_, SIGNAL(triggered(bool)), this, SLOT(showDesktopSelectionWindow(bool))); // Only user actions

    // create toolbar
    QToolBar * toolbar = addToolBar("toolbar");

    // add buttons to toolbar
    toolbar->addAction(shareDesktopAction_);
    toolbar->addAction(showDesktopSelectionWindowAction_);

    // Update timer
    connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));
}

void MainWindow::startStreaming()
{
    if( dcStream_ )
        return;

    dcStream_ = new dc::Stream( uriLineEdit_.text().toStdString(),
                                hostnameLineEdit_.text().toStdString( ));
    if (!dcStream_->isConnected())
    {
        handleStreamingError("Could not connect to host!");
        return;
    }

    shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
}

void MainWindow::stopStreaming()
{
    shareDesktopUpdateTimer_.stop();
    frameRateLabel_.setText("");

    delete dcStream_;
    dcStream_ = 0;

    emit streaming(false);
}

void MainWindow::handleStreamingError(const QString& errorMessage)
{
    std::cerr << errorMessage.toStdString() << std::endl;
    QMessageBox::warning(this, "Error", errorMessage, QMessageBox::Ok, QMessageBox::Ok);

    stopStreaming();

}

void MainWindow::closeEvent( QCloseEvent* event )
{
    delete desktopSelectionWindow_;
    desktopSelectionWindow_ = 0;

    stopStreaming();

    QMainWindow::closeEvent( event );
}

void MainWindow::setCoordinates(int x, int y, int width, int height)
{
    xSpinBox_.setValue(x);
    ySpinBox_.setValue(y);
    widthSpinBox_.setValue(width);
    heightSpinBox_.setValue(height);

    // the spinboxes only update the UI; we must update the actual values too
    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();
}

void MainWindow::shareDesktop(bool set)
{
    if( set )
    {
        startStreaming();
    }
    else
    {
        stopStreaming();
    }
}

void MainWindow::showDesktopSelectionWindow(bool set)
{
    if( set )
    {
        desktopSelectionWindow_->showFullScreen();
    }
    else
    {
        desktopSelectionWindow_->hide();
    }
}

void MainWindow::shareDesktopUpdate()
{
    // time the frame
    QTime frameTime;
    frameTime.start();

    const int w = width_ * deviceScale_;
    const int h = height_ * deviceScale_;

    // take screenshot
    QPixmap desktopPixmap =
        QPixmap::grabWindow( QApplication::desktop()->winId(), x_, y_, w, h );

    if( desktopPixmap.isNull( ))
    {
        handleStreamingError("Got NULL desktop pixmap");
        return;
    }

    QImage image = desktopPixmap.toImage();

    // render mouse cursor
    QPoint mousePos = ( QCursor::pos() - QPoint( x_, y_ )) * deviceScale_ -
                        QPoint( cursor_.width()/2, cursor_.height()/2);

    QPainter painter( &image );
    painter.drawImage( mousePos, cursor_ );
    painter.end(); // Make sure to release the QImage before using it to update the segements

    // QImage Format_RGB32 (0xffRRGGBB) corresponds in fact to GL_BGRA == dc::BGRA
    dc::ImageWrapper dcImage((const void*)image.bits(), image.width(), image.height(), dc::BGRA);
    dcImage.compressionPolicy = dc::COMPRESSION_ON;

    bool success = dcStream_->send(dcImage) && dcStream_->finishFrame();

    if( !success )
    {
        handleStreamingError("Streaming failure, connection closed.");
        return;
    }

    regulateFrameRate(frameTime.elapsed());
}

void MainWindow::regulateFrameRate(const int elapsedFrameTime)
{
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

void MainWindow::updateCoordinates()
{
    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();
    deviceScale_ = retinaBox_.checkState() ? 2.f : 1.f;

    generateCursorImage();

    desktopSelectionWindow_->getDesktopSelectionView()->getDesktopSelectionRectangle()->setCoordinates( x_, y_, width_, height_ );
}
