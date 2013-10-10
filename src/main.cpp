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
#include "Configuration.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "NetworkListener.h"
#include "log.h"
#include "LocalPixelStreamerManager.h"
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

    SkeletonThread * g_skeletonThread = NULL;
#endif


int main(int argc, char * argv[])
{
    put_flog(LOG_INFO, "");

    // get base directory
    if(getenv("DISPLAYCLUSTER_DIR") == NULL)
    {
        put_flog(LOG_FATAL, "DISPLAYCLUSTER_DIR environment variable must be set");
        return -1;
    }

    g_displayClusterDir = std::string(getenv("DISPLAYCLUSTER_DIR"));

    put_flog(LOG_DEBUG, "base directory is %s", g_displayClusterDir.c_str());

#if ENABLE_TUIO_TOUCH_LISTENER
    // we need X multithreading support if we're running the TouchListener thread and creating X events
    XInitThreads();
#endif

    QApplication app(argc, argv);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_mpiSize);
    MPI_Comm_split(MPI_COMM_WORLD, g_mpiRank != 0, g_mpiRank, &g_mpiRenderComm);

    g_configuration = new Configuration((std::string(g_displayClusterDir) + std::string("/configuration.xml")).c_str());

    boost::shared_ptr<DisplayGroupManager> dgm(new DisplayGroupManager);
    g_displayGroupManager = dgm;

    // calibrate timestamp offset between rank 0 and rank 1 clocks
    g_displayGroupManager->calibrateTimestampOffset();

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
    if(g_mpiRank == 0)
    {
        g_skeletonThread = new SkeletonThread();
        g_skeletonThread->start();

        // wait for thread to start
        while(g_skeletonThread->isRunning() == false || g_skeletonThread->isFinished() == true)
        {
            usleep(1000);
        }
    }
#endif

    if(g_mpiRank == 0)
    {
        g_networkListener = new NetworkListener();
        g_localPixelStreamers = new LocalPixelStreamerManager(g_displayGroupManager.get());
    }

    g_mainWindow = new MainWindow();

    // wait for render comms to be ready for receiving and rendering background
    MPI_Barrier(MPI_COMM_WORLD);

    if(g_mpiRank == 0)
    {
        g_displayGroupManager->initBackground(); // Must be done after everything else is setup (or in the MainWindow constructor)
        if( argc == 2 )
            g_displayGroupManager->loadStateXMLFile( argv[1] );
    }

    // enter Qt event loop
    app.exec();

    put_flog(LOG_DEBUG, "quitting");

    // wait for all threads to finish
    QThreadPool::globalInstance()->waitForDone();

    // call finalize cleanup actions
    g_mainWindow->finalize();

    // destruct the main window
    delete g_mainWindow;

    if(g_mpiRank == 0)
    {
        g_displayGroupManager->sendQuit();
        delete g_networkListener;
        delete g_localPixelStreamers;
    }

    delete g_configuration;

    // clean up the MPI environment after the Qt event loop exits
    MPI_Finalize();

    return 0;
}
