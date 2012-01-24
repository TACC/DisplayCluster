#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#define SUPPORTED_NETWORK_PROTOCOL_VERSION 1

#define SHARE_DESKTOP_UPDATE_DELAY 1

#include <QtGui>
#include <QtNetwork/QTcpSocket>
#include <string>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        void getCoordinates(int &x, int &y, int &width, int &height);
        void setCoordinates(int x, int y, int width, int height);

    public slots:

        void shareDesktop(bool set);
        void showDesktopSelectionWindow(bool set);
        void shareDesktopUpdate();
        void updateCoordinates();

    private:

        bool updatedDimensions_;

        QLineEdit hostnameLineEdit_;
        QLineEdit uriLineEdit_;
        QSpinBox xSpinBox_;
        QSpinBox ySpinBox_;
        QSpinBox widthSpinBox_;
        QSpinBox heightSpinBox_;
        QSpinBox frameRateSpinBox_;

        QAction * shareDesktopAction_;
        QAction * showDesktopSelectionWindowAction_;

        std::string hostname_;
        std::string uri_;
        int x_;
        int y_;
        int width_;
        int height_;

        QByteArray previousImageData_;

        QTimer shareDesktopUpdateTimer_;

        QTcpSocket tcpSocket_;
};

#endif
