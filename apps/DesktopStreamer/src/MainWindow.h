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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#define SUPPORTED_NETWORK_PROTOCOL_VERSION 5

#define SHARE_DESKTOP_UPDATE_DELAY 1

#define FRAME_RATE_AVERAGE_NUM_FRAMES 10

#define JPEG_QUALITY 75

#include "../../../src/ParallelPixelStream.h"
#include <QtGui>
#include <QtNetwork/QTcpSocket>
#include <string>

ParallelPixelStreamSegment computeSegmentJpeg(const ParallelPixelStreamSegment & segment);

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:

        MainWindow();

        void getCoordinates(int &x, int &y, int &width, int &height);
        void setCoordinates(int x, int y, int width, int height);

        QImage getImage();

    public slots:

        void shareDesktop(bool set);
        void showDesktopSelectionWindow(bool set);
        void setParallelStreaming(bool set);
        void shareDesktopUpdate();
        void updateCoordinates();

    private:

        virtual void closeEvent( QCloseEvent* event );

        bool updatedDimensions_;

        QLineEdit hostnameLineEdit_;
        QLineEdit uriLineEdit_;
        QSpinBox xSpinBox_;
        QSpinBox ySpinBox_;
        QSpinBox widthSpinBox_;
        QSpinBox heightSpinBox_;
        QCheckBox retinaBox_;
        QSpinBox frameRateSpinBox_;
        QLabel frameRateLabel_;

        QAction * shareDesktopAction_;
        QAction * showDesktopSelectionWindowAction_;

        std::string hostname_;
        std::string uri_;
        int x_;
        int y_;
        int width_;
        int height_;
        float deviceScale_;

        bool parallelStreaming_;

        // full image
        QImage image_;

        // mouse cursor pixmap
        QImage cursor_;

        // for regular pixel streaming
        QByteArray previousImageData_;

        // for parallel pixel streaming
        std::vector<ParallelPixelStreamSegment> segments_;

        QTimer shareDesktopUpdateTimer_;

        // used for frame rate calculations
        std::vector<QTime> frameSentTimes_;

        QTcpSocket tcpSocket_;

        bool serialStream();
        bool parallelStream();
        void sendDimensions();
        void sendQuit();
};

#endif
