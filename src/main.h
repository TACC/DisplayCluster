#ifndef MAIN_H
#define MAIN_H

#include "Configuration.h"
#include "MainWindow.h"
#include "DisplayGroup.h"
#include <boost/shared_ptr.hpp>
#include <mpi.h>

extern int g_mpiRank;
extern int g_mpiSize;
extern MPI_Comm g_mpiRenderComm;
extern Configuration * g_configuration;
extern boost::shared_ptr<DisplayGroup> g_displayGroup;
extern MainWindow * g_mainWindow;

#endif
