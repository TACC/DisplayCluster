#include "DesktopSelectionView.h"
#include "DesktopSelectionRectangle.h"

DesktopSelectionView::DesktopSelectionView()
{
    // create and set scene for the view
    setScene(new QGraphicsScene());

    // force scene to be anchored at top left
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // set attributes of the view
    setInteractive(true);

    // disable scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // create and add the rectangle for the selection area
    desktopSelectionRectangle_ = new DesktopSelectionRectangle();
    scene()->addItem(desktopSelectionRectangle_);
}

DesktopSelectionRectangle * DesktopSelectionView::getDesktopSelectionRectangle()
{
    return desktopSelectionRectangle_;
}

void DesktopSelectionView::resizeEvent(QResizeEvent * event)
{
    // scene rectangle matches viewport rectangle
    setSceneRect(rect());

    QGraphicsView::resizeEvent(event);
}
