#include "MainWindow.h"
#include "main.h"
#include "Content.h"

MainWindow::MainWindow()
{
    if(g_mpiRank == 0)
    {
        // rank 0 window setup
        resize(800,600);

        // create tool bar
        QToolBar * toolbar = addToolBar("toolbar");

        // open content action
        QAction * openContentAction = new QAction("Open Content", this);
        openContentAction->setStatusTip("Open content");
        connect(openContentAction, SIGNAL(triggered()), this, SLOT(openContent()));

        // add actions to toolbar
        toolbar->addAction(openContentAction);

        // main widget / layout area
        QTabWidget * mainWidget = new QTabWidget();
        setCentralWidget(mainWidget);

        // add the local renderer group
        mainWidget->addTab((QWidget *)g_displayGroup.getGraphicsView().get(), "Display group 0");

        // create contents dock widget
        QDockWidget * contentsDockWidget = new QDockWidget("Contents", this);
        QWidget * contentsWidget = new QWidget();
        QVBoxLayout * contentsLayout = new QVBoxLayout();
        contentsWidget->setLayout(contentsLayout);
        contentsDockWidget->setWidget(contentsWidget);
        addDockWidget(Qt::LeftDockWidgetArea, contentsDockWidget);

        // add the list widget
        contentsListWidget_ = new QListWidget();    
        contentsLayout->addWidget(contentsListWidget_);

        show();
    }
    else
    {
        move(QPoint(g_configuration->getTileX(), g_configuration->getTileY()));
        resize(g_configuration->getScreenWidth(), g_configuration->getScreenHeight());

        boost::shared_ptr<GLWindow> glw(new GLWindow());
        glWindow_ = glw;

        showFullScreen();
        setCentralWidget(glWindow_.get());

    }
}

boost::shared_ptr<GLWindow> MainWindow::getGLWindow()
{
    return glWindow_;
}

void MainWindow::openContent()
{
    QString filename = QFileDialog::getOpenFileName(this);

    if(!filename.isEmpty())
    {
        boost::shared_ptr<Content> c(new Content(filename.toStdString()));

        g_displayGroup.addContent(c);

        // add to list view
        QListWidgetItem * newItem = new QListWidgetItem(contentsListWidget_);
        newItem->setText(filename);     
    }
}
