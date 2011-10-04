#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#define SHARE_DESKTOP_UPDATE_DELAY 25

#include <QtGui>
#include <QtNetwork/QTcpSocket>
#include <string>

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

    public slots:

        void shareDesktop(bool set);
        void shareDesktopUpdate();

    private:

        QLineEdit hostnameLineEdit_;
        QLineEdit uriLineEdit_;
        QSpinBox xSpinBox_;
        QSpinBox ySpinBox_;
        QSpinBox widthSpinBox_;
        QSpinBox heightSpinBox_;

        std::string hostname_;
        std::string uri_;
        int x_;
        int y_;
        int width_;
        int height_;

        QTimer shareDesktopUpdateTimer_;

        QTcpSocket tcpSocket_;
};

#endif
