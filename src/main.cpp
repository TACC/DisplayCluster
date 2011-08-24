#include "main.h"
#include "log.h"

#ifdef __APPLE__
    #include <mpi.h>
#else
    #include <mpi/mpi.h>
#endif

int g_mpiRank = -1;
int g_mpiSize = -1;
Configuration * g_configuration = NULL;
MainWindow * g_mainWindow = NULL;

DisplayGroup g_displayGroup;

int main(int argc, char * argv[])
{
    put_flog(LOG_INFO, "");

    QApplication * app = new QApplication(argc, argv);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &g_mpiSize);

    g_configuration = new Configuration("configuration.xml");

    g_mainWindow = new MainWindow();

    // enter Qt event loop
    app->exec();

    // clean up the MPI environment after the Qt event loop exits
    MPI_Finalize();
}
