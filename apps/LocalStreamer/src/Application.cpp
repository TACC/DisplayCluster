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

#include "LocalPixelStreamer.h"
#include "WebkitPixelStreamer.h"
#include "DockPixelStreamer.h"

#include "LocalPixelStreamerFactory.h"
#include "LocalPixelStreamerType.h"

#include "dcstream/DcSocket.h"

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , streamer_(0)
    , dcSocket(0)
{
}

Application::~Application()
{
    dcStreamDisconnect(dcSocket);

    delete streamer_;
}

QString getUriForStreamer(PixelStreamerType type)
{
    qint64 pid = QCoreApplication::applicationPid();
    switch(type)
    {
    case PS_WEBKIT:
        return QString("WebBrowser_%1").arg(pid);
    default:
        return "";
    }
}

bool Application::initalize(const CommandLineOptions& options)
{
    // Connect via DcStream to Master application
    dcSocket = dcStreamConnect("localhost");
    if (!dcSocket)
        return false;

    // Create the streamer
    QString uri = getUriForStreamer(options.getPixelStreamerType());
    streamer_ = LocalPixelStreamerFactory::create(options.getPixelStreamerType(), uri);
    if (!streamer_)
        return false;
    connect(streamer_, SIGNAL(segmentUpdated(QString,PixelStreamSegment)), this, SLOT(processPixelStreamSegment(QString,PixelStreamSegment)));

    // Forward InteractionState updates to the pixel streamer
    connect(dcSocket, SIGNAL(received(InteractionState)), streamer_, SLOT(updateInteractionState(InteractionState)), Qt::QueuedConnection);
    if (!dcStreamBindInteraction(dcSocket, streamer_->getUri().toStdString()))
        return false;

    if(options.getPixelStreamerType() == PS_WEBKIT)
    {
        static_cast<WebkitPixelStreamer*>(streamer_)->setUrl(options.getUrl());
    }

    return true;
}

void Application::processPixelStreamSegment(QString uri, PixelStreamSegment segment)
{
    bool success = dcStreamSendPixelStreamSegment(dcSocket, segment, streamer_->getUri().toStdString());

    if(!success)
    {
        QApplication::quit();
        return;
    }

    dcStreamIncrementFrameIndex();
}

