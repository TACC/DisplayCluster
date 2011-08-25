#ifndef MAIN_H
#define MAIN_H

#include "Configuration.h"
#include "MainWindow.h"
#include "DisplayGroup.h"
#include <boost/shared_ptr.hpp>

extern int g_mpiRank;
extern int g_mpiSize;
extern Configuration * g_configuration;
extern boost::shared_ptr<DisplayGroup> g_displayGroup;
extern MainWindow * g_mainWindow;

#endif
