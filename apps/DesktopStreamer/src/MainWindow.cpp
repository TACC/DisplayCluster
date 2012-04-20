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

MainWindow::MainWindow()
{
    // defaults
    updatedDimensions_ = true;

    QWidget * widget = new QWidget();
    QFormLayout * layout = new QFormLayout();
    widget->setLayout(layout);

    setCentralWidget(widget);

    // connect widget signals / slots
    connect(&xSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&ySpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&widthSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));
    connect(&heightSpinBox_, SIGNAL(editingFinished()), this, SLOT(updateCoordinates()));

    // constrain valid range and default to a quarter of the desktop, centered
    QRect desktopRect = QApplication::desktop()->screenGeometry();

    xSpinBox_.setRange(0, desktopRect.width());
    ySpinBox_.setRange(0, desktopRect.height());
    widthSpinBox_.setRange(1, desktopRect.width());
    heightSpinBox_.setRange(1, desktopRect.height());

    xSpinBox_.setValue(desktopRect.width() / 4);
    ySpinBox_.setValue(desktopRect.height() / 4);
    widthSpinBox_.setValue(desktopRect.width() / 2);
    heightSpinBox_.setValue(desktopRect.height() / 2);

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
    layout->addRow("Max frame rate", &frameRateSpinBox_);

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

    // create toolbar
    QToolBar * toolbar = addToolBar("toolbar");

    // add buttons to toolbar
    toolbar->addAction(shareDesktopAction_);
    toolbar->addAction(showDesktopSelectionWindowAction_);

    // timer will trigger updating of the desktop image
    connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));

    show();
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
    else
    {
        tcpSocket_.disconnectFromHost();

        shareDesktopUpdateTimer_.stop();
    }
}

void MainWindow::showDesktopSelectionWindow(bool set)
{
    if(set == true)
    {
        g_desktopSelectionWindow->showFullScreen();
    }
    else
    {
        g_desktopSelectionWindow->hide();
    }

    // this slot may be called externally, so make sure the action checked value matches the set argument
    if(showDesktopSelectionWindowAction_->isChecked() != set)
    {
        showDesktopSelectionWindowAction_->setChecked(set);
    }
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
    QPixmap desktopPixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), x_,y_,width_,height_);

    if(desktopPixmap.isNull() == true)
    {
        put_flog(LOG_ERROR, "got NULL desktop pixmap");
        QMessageBox::warning(this, "Error", "Got NULL desktop pixmap.", QMessageBox::Ok, QMessageBox::Ok);

        tcpSocket_.disconnectFromHost();
        shareDesktopAction_->setChecked(false);
        return;
    }

    // convert to QImage
    QImage image = desktopPixmap.toImage();

    // use libjpeg-turbo for JPEG conversion
    tjhandle handle = tjInitCompress();
    int pixelFormat = TJPF_BGRX;
    unsigned char ** jpegBuf;
    unsigned char * jpegBufPtr = NULL;
    jpegBuf = &jpegBufPtr;
    unsigned long jpegSize = 0;
    int jpegSubsamp = TJSAMP_444;
    int jpegQual = 75;
    int flags = 0;

    int success = tjCompress2(handle, image.scanLine(0), image.width(), image.bytesPerLine(), image.height(), pixelFormat, jpegBuf, &jpegSize, jpegSubsamp, jpegQual, flags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");
        QMessageBox::warning(this, "Error", "Image conversion failure.", QMessageBox::Ok, QMessageBox::Ok);

        tcpSocket_.disconnectFromHost();
        shareDesktopAction_->setChecked(false);
        return;
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
            usleep(10);
#endif
        }

        tcpSocket_.read(3);
    }

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
            usleep(10);
#endif
        }

        tcpSocket_.read(3);
    }

    // elapsed time (milliseconds)
    int elapsedFrameTime = frameTime.elapsed();

    // frame rate limiting
    int maxFrameRate = frameRateSpinBox_.value();

    int desiredFrameTime = (int)(1000. * 1. / (float)maxFrameRate);

    int sleepTime = desiredFrameTime - elapsedFrameTime;

    if(sleepTime > 0)
    {
#ifdef _WIN32
        Sleep(sleepTime);
#else
        usleep(1000 * sleepTime);
#endif
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
}
