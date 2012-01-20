#include "MainWindow.h"
#include "main.h"
#include "../../../src/log.h"
#include "../../../src/DisplayGroupManager.h" // for MessageHeader
#include "DesktopSelectionRectangle.h"
#include <turbojpeg.h>

MainWindow::MainWindow()
{
    // defaults
    updatedCoordinates_ = true;

    QWidget * widget = new QWidget();
    QFormLayout * layout = new QFormLayout();
    widget->setLayout(layout);

    setCentralWidget(widget);

    // connect widget signals / slots
    connect(&xSpinBox_, SIGNAL(valueChanged(int)), this, SLOT(updateCoordinates()));
    connect(&ySpinBox_, SIGNAL(valueChanged(int)), this, SLOT(updateCoordinates()));
    connect(&widthSpinBox_, SIGNAL(valueChanged(int)), this, SLOT(updateCoordinates()));
    connect(&heightSpinBox_, SIGNAL(valueChanged(int)), this, SLOT(updateCoordinates()));

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

    // add widgets to UI
    layout->addRow("Hostname", &hostnameLineEdit_);
    layout->addRow("Stream name", &uriLineEdit_);
    layout->addRow("X", &xSpinBox_);
    layout->addRow("Y", &ySpinBox_);
    layout->addRow("Width", &widthSpinBox_);
    layout->addRow("Height", &heightSpinBox_);

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
    xSpinBox_.setValue(x);
    ySpinBox_.setValue(y);
    widthSpinBox_.setValue(width);
    heightSpinBox_.setValue(height);

    updatedCoordinates_ = true;
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
            shareDesktopAction_->setChecked(false);
            return;
        }

        shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
    }
    else
    {
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
    if(tcpSocket_.state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");
        shareDesktopAction_->setChecked(false);
        return;
    }

    // take screenshot
    QPixmap desktopPixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), x_,y_,width_,height_);

    if(desktopPixmap.isNull() == true)
    {
        put_flog(LOG_ERROR, "got NULL desktop pixmap");
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

        // check if we updated coordinates
        if(updatedCoordinates_ == true)
        {
            MessageHeader mh;
            mh.size = 0;
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

            updatedCoordinates_ = false;
        }

        // wait for data to be completely sent
        while(tcpSocket_.bytesToWrite() > 0 && tcpSocket_.waitForBytesWritten())
        {
            usleep(10);
        }
    }
}

void MainWindow::updateCoordinates()
{
    x_ = xSpinBox_.value();
    y_ = ySpinBox_.value();
    width_ = widthSpinBox_.value();
    height_ = heightSpinBox_.value();

    // update DesktopSelectionRectangle
    if(g_desktopSelectionWindow != NULL)
    {
        g_desktopSelectionWindow->getDesktopSelectionView()->getDesktopSelectionRectangle()->setCoordinates(x_, y_, width_, height_);
    }

    updatedCoordinates_ = true;
}
