/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "Application.h"

#include "localstreamer/LocalPixelStreamer.h"
#include "localstreamer/LocalPixelStreamerFactory.h"

#include "localstreamer/CommandLineOptions.h"
#include "dcstream/StreamPrivate.h"

#include <QTimer>

#define DC_STREAM_HOST_ADDRESS "localhost"
//#define COMPRESS_IMAGES

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , streamer_(0)
    , dcStream_(0)
{
}

Application::~Application()
{
    delete dcStream_;
    delete streamer_;
}

bool Application::initalize(const CommandLineOptions& options)
{
    // Create the streamer
    streamer_ = LocalPixelStreamerFactory::create(options);
    if (!streamer_)
        return false;
    connect(streamer_, SIGNAL(imageUpdated(QImage)), this, SLOT(sendImage(QImage)));
    connect(streamer_, SIGNAL(openContent(QString)), this, SLOT(sendOpenContent(QString)));

    // Connect to DisplayCluster
    dcStream_ = new dc::Stream(options.getName().toStdString(), DC_STREAM_HOST_ADDRESS);
    if (!dcStream_->isConnected())
    {
        std::cerr << "Could not connect to host!" << std::endl;
        delete dcStream_;
        dcStream_ = 0;
        return false;
    }

    // Use a timer to process Event received from the dc::Stream
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(processPendingEvents()));
    timer->start(1);

    return true;
}

void Application::sendImage(QImage image)
{
#ifdef COMPRESS_IMAGES
    // QImage Format_RGB32 (0xffRRGGBB) corresponds in fact to GL_BGRA == dc::BGRA
    dc::ImageWrapper dcImage((const void*)image.constBits(), image.width(), image.height(), dc::BGRA);
    dcImage.compressionPolicy = dc::COMPRESSION_ON;
#else
    // This conversion is suboptimal, but the only solution until we send the PixelFormat with the PixelStreamSegment
    image = image.rgbSwapped();
    dc::ImageWrapper dcImage((const void*)image.constBits(), image.width(), image.height(), dc::RGBA);
    dcImage.compressionPolicy = dc::COMPRESSION_OFF;
#endif
    bool success = dcStream_->send(dcImage) && dcStream_->finishFrame();

    if(!success)
    {
        QApplication::quit();
        return;
    }
}

void Application::processPendingEvents()
{
    checkConnection();

    if (!dcStream_->isRegisteredForEvents())
    {
        dcStream_->registerForEvents();
    }
    else
    {
        while(dcStream_->hasEvent())
        {
            streamer_->processEvent(dcStream_->getEvent());
        }
    }
}

void Application::sendOpenContent(QString uri)
{
    dcStream_->getPrivateImpl()->sendOpenContent(uri.toStdString());
}

void Application::checkConnection()
{
    if( !dcStream_->isConnected() )
    {
        QApplication::quit();
        return;
    }
}

