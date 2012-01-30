/*********************************************************************/
/* Copyright 2011 - 2012  The University of Texas at Austin.         */
/* All rights reserved.                                              */
/*                                                                   */
/* This is a pre-release version of DisplayCluster. All rights are   */
/* reserved by the University of Texas at Austin. You may not modify */
/* or distribute this software without permission from the authors.  */
/* Refer to the LICENSE file distributed with the software for       */
/* details.                                                          */
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
