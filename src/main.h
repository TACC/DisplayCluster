#ifndef MAIN_H
#define MAIN_H

#include "Configuration.h"
#include "MainWindow.h"
#include "DisplayGroupManager.h"
#include "NetworkListener.h"
#include "config.h"
#include <boost/shared_ptr.hpp>
#include <mpi.h>

extern QApplication * g_app;
extern int g_mpiRank;
extern int g_mpiSize;
extern MPI_Comm g_mpiRenderComm;
extern Configuration * g_configuration;
extern boost::shared_ptr<DisplayGroupManager> g_displayGroupManager;
extern MainWindow * g_mainWindow;
extern NetworkListener * g_networkListener;
extern long g_frameCount;

#if ENABLE_SKELETON_SUPPORT
    class SkeletonThread;
    extern SkeletonThread * g_skeletonThread;
#endif

#endif
