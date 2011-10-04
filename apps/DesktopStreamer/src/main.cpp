#include "main.h"
#include "../../../src/log.h"

MainWindow * g_mainWindow = NULL;

int main(int argc, char * argv[])
{
    put_flog(LOG_INFO, "");

    QApplication * app = new QApplication(argc, argv);

    g_mainWindow = new MainWindow();

    // enter Qt event loop
    app->exec();
}
