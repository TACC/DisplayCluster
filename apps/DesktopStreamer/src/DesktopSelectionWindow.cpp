#include "DesktopSelectionWindow.h"
#include "main.h"

DesktopSelectionWindow::DesktopSelectionWindow()
{
    // make window transparent
    setStyleSheet("background:transparent;");
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

    // window stays on top
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);

    // add the view after showing the window to avoid shadow artifacts on Mac
    setCentralWidget(&desktopSelectionView_);

    // button to hide the window
    QPushButton * hideWindowButton = new QPushButton("Exit selection mode");
    connect(hideWindowButton, SIGNAL(pressed()), this, SLOT(hide()));

    // makes the button square so the background doesn't look bad
    hideWindowButton->setFlat(true);

    // add it to the scene
    QGraphicsProxyWidget * hideWindowProxy = desktopSelectionView_.scene()->addWidget(hideWindowButton);
}

DesktopSelectionView * DesktopSelectionWindow::getDesktopSelectionView()
{
    return &desktopSelectionView_;
}

void DesktopSelectionWindow::hideEvent(QHideEvent * event)
{
    QWidget::hideEvent(event);

    g_mainWindow->showDesktopSelectionWindow(false);
}
