#ifndef MAIN_H
#define MAIN_H

#include "Configuration.h"
#include "MainWindow.h"
#include "DisplayGroup.h"

extern int g_mpiRank;
extern int g_mpiSize;
extern Configuration * g_configuration;
extern MainWindow * g_mainWindow;

extern DisplayGroup g_displayGroup;

#endif
