#include "main.h"
#include "config.h"
#include "log.h"
#include <mpi.h>

#if ENABLE_TUIO_TOUCH_LISTENER
    #include "TouchListener.h"
    #include <X11/Xlib.h>
#endif

#if ENABLE_JOYSTICK_SUPPORT
    #include "JoystickThread.h"
#endif

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonThread.h"
    SkeletonThread * g_skeletonThread;
#endif

QApplication * g_app = NULL;
int g_mpiRank = -1;
int g_mpiSize = -1;
MPI_Comm g_mpiRenderComm;
Configuration * g_configuration = NULL;
boost::shared_ptr<DisplayGroupManager> g_displayGroupManager;
MainWindow * g_mainWindow = NULL;
NetworkListener * g_networkListener = NULL;
long g_frameCount = 0;

int main(int argc, char * argv[])
{
    put_flog(LOG_INFO, "");

#if ENABLE_TUIO_TOUCH_LISTENER
    // we need X multithreading support if we're running the TouchListener thread and creating X events
    XInitThreads();
#endif

    g_app = new QApplication(argc, argv);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_mpiSize);
    MPI_Comm_split(MPI_COMM_WORLD, g_mpiRank != 0, g_mpiRank, &g_mpiRenderComm);

    g_configuration = new Configuration("configuration.xml");

    boost::shared_ptr<DisplayGroupManager> dgm(new DisplayGroupManager);
    g_displayGroupManager = dgm;

    // calibrate timestamp offset between rank 0 and rank 1 clocks
    g_displayGroupManager->calibrateTimestampOffset();

    g_mainWindow = new MainWindow();

#if ENABLE_TUIO_TOUCH_LISTENER
    if(g_mpiRank == 0)
    {
        new TouchListener();
    }
#endif

#if ENABLE_JOYSTICK_SUPPORT
    if(g_mpiRank == 0)
    {
        // create thread to monitor joystick events (all joysticks handled in same event queue)
        JoystickThread * joystickThread = new JoystickThread();
        joystickThread->start();
    }
#endif

# if ENABLE_SKELETON_SUPPORT
    if(g_mpiRank == 0)
    {
        g_skeletonThread = new SkeletonThread();
        g_skeletonThread->start();
    }
#endif

    if(g_mpiRank == 0)
    {
        g_networkListener = new NetworkListener();
    }

    // enter Qt event loop
    g_app->exec();

    put_flog(LOG_DEBUG, "quitting");

    if(g_mpiRank == 0)
    {
        g_displayGroupManager->sendQuit();
    }

    // clean up the MPI environment after the Qt event loop exits
    MPI_Finalize();

    return 0;
}
