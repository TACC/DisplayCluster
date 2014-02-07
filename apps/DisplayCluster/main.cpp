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

#include <QApplication>

#include "globals.h"
#include "config.h"
#include "configuration/MasterConfiguration.h"
#include "configuration/WallConfiguration.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "NetworkListener.h"
#include "log.h"
#include "localstreamer/PixelStreamerLauncher.h"
#include "StateSerializationHelper.h"

#include "CommandHandler.h"
#include "SessionCommandHandler.h"
#include "FileCommandHandler.h"
#include "WebbrowserCommandHandler.h"

#include "ws/WebServiceServer.h"
#include "ws/TextInputDispatcher.h"
#include "ws/TextInputHandler.h"
#include "ws/DisplayGroupManagerAdapter.h"

#include <mpi.h>
#include <unistd.h>

#if ENABLE_TUIO_TOUCH_LISTENER
    #include <X11/Xlib.h>
#endif

#if ENABLE_JOYSTICK_SUPPORT
    #include "JoystickThread.h"
#endif

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonThread.h"
#endif

#define CONFIGURATION_FILENAME "configuration.xml"
#define DISPLAYCLUSTER_DIR "DISPLAYCLUSTER_DIR"

int main(int argc, char * argv[])
{
    // get base directory
    if( !getenv( DISPLAYCLUSTER_DIR ))
    {
        put_flog(LOG_FATAL, "DISPLAYCLUSTER_DIR environment variable must be set");
        return EXIT_FAILURE;
    }

    // get configuration file name
    const QString displayClusterDir = QString(getenv( DISPLAYCLUSTER_DIR ));
    put_flog(LOG_DEBUG, "base directory is %s", displayClusterDir.toLatin1().constData());
    const QString configFilename = QString( "%1/%2" ).arg( displayClusterDir )
                                                     .arg( CONFIGURATION_FILENAME );

#if ENABLE_TUIO_TOUCH_LISTENER
    // we need X multithreading support if we're running the TouchListener thread and creating X events
    XInitThreads();
#endif

    QApplication app(argc, argv);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_mpiSize);
    MPI_Comm_split(MPI_COMM_WORLD, g_mpiRank != 0, g_mpiRank, &g_mpiRenderComm);

    g_displayGroupManager.reset( new DisplayGroupManager );

    // Load configuration
    if(g_mpiRank == 0)
        g_configuration = new MasterConfiguration(configFilename,
                                                  g_displayGroupManager->getOptions());
    else
        g_configuration = new WallConfiguration(configFilename,
                                                g_displayGroupManager->getOptions(), g_mpiRank);

    // calibrate timestamp offset between rank 0 and rank 1 clocks
    g_displayGroupManager->calibrateTimestampOffset();

    g_mainWindow = new MainWindow();

#if ENABLE_JOYSTICK_SUPPORT
    if(g_mpiRank == 0)
    {
        // do this before the thread starts to avoid X callback race conditions
        // we need SDL_INIT_VIDEO for events to work
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        {
            put_flog(LOG_ERROR, "could not initial SDL joystick subsystem");
            return -2;
        }

        // create thread to monitor joystick events (all joysticks handled in same event queue)
        JoystickThread * joystickThread = new JoystickThread();
        joystickThread->start();

        // wait for thread to start
        while(joystickThread->isRunning() == false || joystickThread->isFinished() == true)
        {
            usleep(1000);
        }
    }
#endif

#if ENABLE_SKELETON_SUPPORT
    SkeletonThread* skeletonThread = 0;

    if(g_mpiRank == 0)
    {
        skeletonThread = new SkeletonThread();
        skeletonThread->start();

        // wait for thread to start
        while( !skeletonThread->isRunning() || skeletonThread->isFinished() )
        {
            usleep(1000);
        }
    }

    connect(g_mainWindow, SIGNAL(enableSkeletonTracking()), skeletonThread, SLOT(startTimer()));
    connect(g_mainWindow, SIGNAL(disableSkeletonTracking()), skeletonThread, SLOT(stopTimer()));

    connect(skeletonThread, SIGNAL(skeletonsUpdated(std::vector< boost::shared_ptr<SkeletonState> >)),
            g_displayGroupManager.get(), SLOT(setSkeletons(std::vector<boost::shared_ptr<SkeletonState> >)),
            Qt::QueuedConnection);
#endif

    NetworkListener* networkListener = 0;
    PixelStreamerLauncher* pixelStreamerLauncher = 0;
    WebServiceServer* webServiceServer = 0;
    TextInputDispatcher* textInputDispatcher = 0;

    if(g_mpiRank == 0)
    {
        pixelStreamerLauncher = new PixelStreamerLauncher(g_displayGroupManager.get());

        pixelStreamerLauncher->connect(g_mainWindow, SIGNAL(openWebBrowser(QPointF,QSize,QString)),
                                           SLOT(openWebBrowser(QPointF,QSize,QString)));
        pixelStreamerLauncher->connect(g_mainWindow, SIGNAL(openDock(QPointF,QSize,QString)),
                                           SLOT(openDock(QPointF,QSize,QString)));
        pixelStreamerLauncher->connect(g_mainWindow, SIGNAL(hideDock()),
                                           SLOT(hideDock()));

        networkListener = new NetworkListener(*g_displayGroupManager);

        CommandHandler& handler = networkListener->getCommandHandler();
        handler.registerCommandHandler(new FileCommandHandler(g_displayGroupManager));
        handler.registerCommandHandler(new SessionCommandHandler(*g_displayGroupManager));
        handler.registerCommandHandler(new WebbrowserCommandHandler(*g_displayGroupManager,
                                                                    *pixelStreamerLauncher));

        // FastCGI WebService Server
        const int webServicePort =
                static_cast<MasterConfiguration*>(g_configuration)->getWebServicePort();
        webServiceServer = new WebServiceServer(webServicePort);

        DisplayGroupManagerAdapterPtr adapter(new DisplayGroupManagerAdapter(g_displayGroupManager));
        TextInputHandler* textInputHandler = new TextInputHandler(adapter);
        webServiceServer->addHandler("/dcapi/textinput", dcWebservice::HandlerPtr(textInputHandler));

        textInputHandler->moveToThread(webServiceServer);
        textInputDispatcher = new TextInputDispatcher(g_displayGroupManager);
        textInputDispatcher->connect(textInputHandler, SIGNAL(receivedKeyInput(char)),
                             SLOT(sendKeyEventToActiveWindow(char)));

        webServiceServer->start();
    }


    // wait for render comms to be ready for receiving and rendering background
    MPI_Barrier(MPI_COMM_WORLD);

    if(g_mpiRank == 0)
    {
        // Must be done after everything else is setup (or in the MainWindow constructor)
        g_displayGroupManager->setBackgroundColor( g_configuration->getBackgroundColor( ));
        g_displayGroupManager->setBackgroundContentFromUri( g_configuration->getBackgroundUri( ));

        if( argc == 2 )
            StateSerializationHelper(g_displayGroupManager).load( argv[1] );
    }

    // enter Qt event loop
    app.exec();

    put_flog(LOG_DEBUG, "quitting");

    // wait for all threads to finish
    QThreadPool::globalInstance()->waitForDone();

    if(g_mpiRank != 0)
        g_displayGroupManager->deleteMarkers();

#if ENABLE_SKELETON_SUPPORT
    delete skeletonThread;
    skeletonThread = 0;
#endif

#if ENABLE_JOYSTICK_SUPPORT
    delete joystickThread;
    joystickThread = 0;
#endif

    // call finalize cleanup actions
    g_mainWindow->finalize();

    // destruct the main window
    delete g_mainWindow;
    g_mainWindow = 0;

    if(g_mpiRank == 0)
    {
        g_displayGroupManager->sendQuit();
        delete pixelStreamerLauncher;
        pixelStreamerLauncher = 0;
        delete networkListener;
        networkListener = 0;

        delete textInputDispatcher;
        textInputDispatcher = 0;
        webServiceServer->stop();
        webServiceServer->wait();
        delete webServiceServer;
        webServiceServer = 0;
    }

    delete g_configuration;
    g_configuration = 0;
    g_displayGroupManager.reset();

    // clean up the MPI environment after the Qt event loop exits
    MPI_Comm_free(&g_mpiRenderComm);
    MPI_Finalize();

    return EXIT_SUCCESS;
}
