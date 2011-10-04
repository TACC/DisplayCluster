#include "MainWindow.h"
#include "main.h"
#include "../../../src/log.h"
#include "../../../src/DisplayGroup.h" // for MessageHeader

MainWindow::MainWindow()
{
    QWidget * widget = new QWidget();
    QFormLayout * layout = new QFormLayout();
    widget->setLayout(layout);

    setCentralWidget(widget);

    // set widget parameters
    xSpinBox_.setRange(0, 4096);
    ySpinBox_.setRange(0, 4096);
    widthSpinBox_.setRange(1, 4096);
    heightSpinBox_.setRange(1, 4096);

    // default to entire desktop
    QRect desktopRect = QApplication::desktop()->screenGeometry();

    xSpinBox_.setValue(desktopRect.x());
    ySpinBox_.setValue(desktopRect.y());
    widthSpinBox_.setValue(desktopRect.width());
    heightSpinBox_.setValue(desktopRect.height());

    // add widgets to UI
    layout->addRow("Hostname", &hostnameLineEdit_);
    layout->addRow("Stream name", &uriLineEdit_);
    layout->addRow("X", &xSpinBox_);
    layout->addRow("Y", &ySpinBox_);
    layout->addRow("Width", &widthSpinBox_);
    layout->addRow("Height", &heightSpinBox_);

    // share desktop action
    QAction * shareDesktopAction = new QAction("Share Desktop", this);
    shareDesktopAction->setStatusTip("Share desktop");
    shareDesktopAction->setCheckable(true);
    shareDesktopAction->setChecked(false);
    connect(shareDesktopAction, SIGNAL(toggled(bool)), this, SLOT(shareDesktop(bool)));

    // create toolbar
    QToolBar * toolbar = addToolBar("toolbar");

    // add share button to toolbar
    toolbar->addAction(shareDesktopAction);

    // timer will trigger updating of the desktop image
    connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));

    show();
}

void MainWindow::shareDesktop(bool set)
{
    if(set == true)
    {
        // save values from UI
        hostname_ = hostnameLineEdit_.text().toStdString();
        uri_ = uriLineEdit_.text().toStdString();
        x_ = xSpinBox_.value();
        y_ = ySpinBox_.value();
        width_ = widthSpinBox_.value();
        height_ = heightSpinBox_.value();

        // open connection (disconnecting from an existing connection if necessary)
        tcpSocket_.disconnectFromHost();
        tcpSocket_.connectToHost(hostname_.c_str(), 1701);

        if(tcpSocket_.waitForConnected() != true)
        {
            put_flog(LOG_ERROR, "could not connect");
            return;
        }

        shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
    }
    else
    {
        shareDesktopUpdateTimer_.stop();
    }
}

void MainWindow::shareDesktopUpdate()
{
    if(tcpSocket_.state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");
        shareDesktopUpdateTimer_.stop();
        return;
    }

    while(tcpSocket_.bytesToWrite() > 0 && tcpSocket_.waitForBytesWritten())
    {
        usleep(100);
    }

    // take screenshot
    QPixmap desktopPixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), x_,y_,width_,height_);

    // save it to buffer
    QBuffer buffer;
    desktopPixmap.save(&buffer, "JPEG");

    QByteArray byteArray = buffer.data();

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
}
