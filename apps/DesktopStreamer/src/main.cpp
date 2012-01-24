#include "main.h"
#include "../../../src/log.h"

MainWindow * g_mainWindow = NULL;
DesktopSelectionWindow * g_desktopSelectionWindow = NULL;

int main(int argc, char * argv[])
{
    put_flog(LOG_INFO, "");

    QApplication * app = new QApplication(argc, argv);

    g_mainWindow = new MainWindow();
    g_desktopSelectionWindow = new DesktopSelectionWindow();

    // enter Qt event loop
    return app->exec();
}
