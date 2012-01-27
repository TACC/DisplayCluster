#include "config.h"
#include "MainWindow.h"
#include "main.h"
#include "Content.h"
#include "ContentWindowManager.h"
#include "log.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupListWidgetProxy.h"

#if ENABLE_PYTHON_SUPPORT
    #include "PythonConsole.h"
#endif

MainWindow::MainWindow()
{
    // defaults
    constrainAspectRatio_ = true;

    // make application quit when last window is closed
    QObject::connect(g_app, SIGNAL(lastWindowClosed()), g_app, SLOT(quit()));

    if(g_mpiRank == 0)
    {
#if ENABLE_PYTHON_SUPPORT
        PythonConsole::init();
#endif

        // rank 0 window setup
        resize(800,600);

        // create menus in menu bar
        QMenu * fileMenu = menuBar()->addMenu("&File");
        QMenu * viewMenu = menuBar()->addMenu("&View");
#if ENABLE_PYTHON_SUPPORT
        // add Window menu for Python console. if we add any other entries to it we'll need to remove the #if
        QMenu * windowMenu = menuBar()->addMenu("&Window");
#endif

        // create tool bar
        QToolBar * toolbar = addToolBar("toolbar");

        // open content action
        QAction * openContentAction = new QAction("Open Content", this);
        openContentAction->setStatusTip("Open content");
        connect(openContentAction, SIGNAL(triggered()), this, SLOT(openContent()));

        // open contents directory action
        QAction * openContentsDirectoryAction = new QAction("Open Contents Directory", this);
        openContentsDirectoryAction->setStatusTip("Open contents directory");
        connect(openContentsDirectoryAction, SIGNAL(triggered()), this, SLOT(openContentsDirectory()));

        // clear contents action
        QAction * clearContentsAction = new QAction("Clear", this);
        clearContentsAction->setStatusTip("Clear");
        connect(clearContentsAction, SIGNAL(triggered()), this, SLOT(clearContents()));

        // save state action
        QAction * saveStateAction = new QAction("Save State", this);
        saveStateAction->setStatusTip("Save state");
        connect(saveStateAction, SIGNAL(triggered()), this, SLOT(saveState()));

        // load state action
        QAction * loadStateAction = new QAction("Load State", this);
        loadStateAction->setStatusTip("Load state");
        connect(loadStateAction, SIGNAL(triggered()), this, SLOT(loadState()));

        // compute image pyramid action
        QAction * computeImagePyramidAction = new QAction("Compute Image Pyramid", this);
        computeImagePyramidAction->setStatusTip("Compute image pyramid");
        connect(computeImagePyramidAction, SIGNAL(triggered()), this, SLOT(computeImagePyramid()));

#if ENABLE_PYTHON_SUPPORT
        // Python console action
        QAction * pythonConsoleAction = new QAction("Open Python Console", this);
        pythonConsoleAction->setStatusTip("Open Python console");
        connect(pythonConsoleAction, SIGNAL(triggered()), PythonConsole::self(), SLOT(show()));
#endif

        // quit action
        QAction * quitAction = new QAction("Quit", this);
        quitAction->setStatusTip("Quit application");
        connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

        // constrain aspect ratio action
        QAction * constrainAspectRatioAction = new QAction("Constrain Aspect Ratio", this);
        constrainAspectRatioAction->setStatusTip("Constrain aspect ratio");
        constrainAspectRatioAction->setCheckable(true);
        constrainAspectRatioAction->setChecked(constrainAspectRatio_);
        connect(constrainAspectRatioAction, SIGNAL(toggled(bool)), this, SLOT(constrainAspectRatio(bool)));

        // show window borders action
        QAction * showWindowBordersAction = new QAction("Show Window Borders", this);
        showWindowBordersAction->setStatusTip("Show window borders");
        showWindowBordersAction->setCheckable(true);
        showWindowBordersAction->setChecked(g_displayGroupManager->getOptions()->getShowWindowBorders());
        connect(showWindowBordersAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowWindowBorders(bool)));

        // show test pattern action
        QAction * showTestPatternAction = new QAction("Show Test Pattern", this);
        showTestPatternAction->setStatusTip("Show test pattern");
        showTestPatternAction->setCheckable(true);
        showTestPatternAction->setChecked(g_displayGroupManager->getOptions()->getShowTestPattern());
        connect(showTestPatternAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowTestPattern(bool)));

        // add actions to menus
        fileMenu->addAction(openContentAction);
        fileMenu->addAction(openContentsDirectoryAction);
        fileMenu->addAction(clearContentsAction);
        fileMenu->addAction(saveStateAction);
        fileMenu->addAction(loadStateAction);
        fileMenu->addAction(computeImagePyramidAction);
        fileMenu->addAction(quitAction);
        viewMenu->addAction(constrainAspectRatioAction);
        viewMenu->addAction(showWindowBordersAction);
        viewMenu->addAction(showTestPatternAction);
#if ENABLE_PYTHON_SUPPORT
        windowMenu->addAction(pythonConsoleAction);
#endif

        // add actions to toolbar
        toolbar->addAction(openContentAction);
        toolbar->addAction(openContentsDirectoryAction);
        toolbar->addAction(clearContentsAction);
        toolbar->addAction(saveStateAction);
        toolbar->addAction(loadStateAction);
        toolbar->addAction(computeImagePyramidAction);
#if ENABLE_PYTHON_SUPPORT
        toolbar->addAction(pythonConsoleAction);
#endif
        // main widget / layout area
        QTabWidget * mainWidget = new QTabWidget();
        setCentralWidget(mainWidget);

        // add the local renderer group
        DisplayGroupGraphicsViewProxy * dggv = new DisplayGroupGraphicsViewProxy(g_displayGroupManager);
        mainWidget->addTab((QWidget *)dggv->getGraphicsView(), "Display group 0");

        // create contents dock widget
        QDockWidget * contentsDockWidget = new QDockWidget("Contents", this);
        QWidget * contentsWidget = new QWidget();
        QVBoxLayout * contentsLayout = new QVBoxLayout();
        contentsWidget->setLayout(contentsLayout);
        contentsDockWidget->setWidget(contentsWidget);
        addDockWidget(Qt::LeftDockWidgetArea, contentsDockWidget);

        // add the list widget
        DisplayGroupListWidgetProxy * dglwp = new DisplayGroupListWidgetProxy(g_displayGroupManager);
        contentsLayout->addWidget(dglwp->getListWidget());

        show();
    }
    else
    {
        // setup OpenGL windows
        // if we have just one tile for this process, make the GL window the central widget
        // otherwise, create multiple windows
        if(g_configuration->getMyNumTiles() == 1)
        {
            move(QPoint(g_configuration->getTileX(0), g_configuration->getTileY(0)));
            resize(g_configuration->getScreenWidth(), g_configuration->getScreenHeight());

            boost::shared_ptr<GLWindow> glw(new GLWindow(0));
            glWindows_.push_back(glw);

            setCentralWidget(glw.get());

            if(g_configuration->getFullscreen() == true)
            {
                showFullScreen();
            }
            else
            {
                show();
            }
        }
        else
        {
            for(unsigned int i=0; i<g_configuration->getMyNumTiles(); i++)
            {
                QRect windowRect = QRect(g_configuration->getTileX(i), g_configuration->getTileY(i), g_configuration->getScreenWidth(), g_configuration->getScreenHeight());

                // setup shared OpenGL contexts
                GLWindow * shareWidget = NULL;

                if(i > 0)
                {
                    shareWidget = glWindows_[0].get();
                }

                boost::shared_ptr<GLWindow> glw(new GLWindow(i, windowRect, shareWidget));
                glWindows_.push_back(glw);

                if(g_configuration->getFullscreen() == true)
                {
                    glw->showFullScreen();
                }
                else
                {
                    glw->show();
                }
            }
        }

        // setup connection so updateGLWindows() will be called continuously
        // must be queued so we return to the main event loop and avoid infinite recursion
        connect(this, SIGNAL(updateGLWindowsFinished()), this, SLOT(updateGLWindows()), Qt::QueuedConnection);

        // trigger the first update
        updateGLWindows();
    }
}

bool MainWindow::getConstrainAspectRatio()
{
    return constrainAspectRatio_;
}

boost::shared_ptr<GLWindow> MainWindow::getGLWindow(int index)
{
    return glWindows_[index];
}

std::vector<boost::shared_ptr<GLWindow> > MainWindow::getGLWindows()
{
    return glWindows_;
}

void MainWindow::openContent()
{
    QString filename = QFileDialog::getOpenFileName(this);

    if(!filename.isEmpty())
    {
        boost::shared_ptr<Content> c = Content::getContent(filename.toStdString());

        if(c != NULL)
        {
            boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

            g_displayGroupManager->addContentWindowManager(cwm);
        }
        else
        {
            QMessageBox messageBox;
            messageBox.setText("Unsupported file format.");
            messageBox.exec();
        }
    }
}

void MainWindow::openContentsDirectory()
{
    QString directoryName = QFileDialog::getExistingDirectory(this);

    int gridX = QInputDialog::getInt(this, "Grid X dimension", "Grid X dimension");
    int gridY = QInputDialog::getInt(this, "Grid Y dimension", "Grid Y dimension");
    float w = 1./(float)gridX;
    float h = 1./(float)gridY;

    if(!directoryName.isEmpty())
    {
        QDir directory(directoryName);
        directory.setFilter(QDir::Files);

        QFileInfoList list = directory.entryInfoList();

        int contentIndex = 0;

        for(int i=0; i<list.size() && contentIndex < gridX*gridY; i++)
        {
            QFileInfo fileInfo = list.at(i);

            boost::shared_ptr<Content> c = Content::getContent(fileInfo.absoluteFilePath().toStdString());

            if(c != NULL)
            {
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                g_displayGroupManager->addContentWindowManager(cwm);

                int x = contentIndex % gridX;
                int y = contentIndex / gridX;

                cwm->setCoordinates(x*w, y*h, w, h);

                contentIndex++;

                put_flog(LOG_DEBUG, "added file %s", fileInfo.absoluteFilePath().toStdString().c_str());
            }
            else
            {
                put_flog(LOG_DEBUG, "ignoring unsupported file %s", fileInfo.absoluteFilePath().toStdString().c_str());
            }
        }
    }
}

void MainWindow::clearContents()
{
    g_displayGroupManager->setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> >());
}

void MainWindow::saveState()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        // make sure filename has .dcx extension
        if(filename.endsWith(".dcx") != true)
        {
            put_flog(LOG_DEBUG, "appended .dcx filename extension");
            filename.append(".dcx");
        }

        bool success = g_displayGroupManager->saveStateXMLFile(filename.toStdString());

        if(success != true)
        {
            QMessageBox::warning(this, "Error", "Could not save state file.", QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

void MainWindow::loadState()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        bool success = g_displayGroupManager->loadStateXMLFile(filename.toStdString());

        if(success != true)
        {
            QMessageBox::warning(this, "Error", "Could not load state file.", QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

void MainWindow::computeImagePyramid()
{
    // get image filename
    QString imageFilename = QFileDialog::getOpenFileName(this, "Select image");

    if(!imageFilename.isEmpty())
    {
        put_flog(LOG_DEBUG, "got image filename %s", imageFilename.toStdString().c_str());

        std::string imagePyramidPath = imageFilename.toStdString() + ".pyramid/";

        put_flog(LOG_DEBUG, "got image pyramid path %s", imagePyramidPath.c_str());

        boost::shared_ptr<DynamicTexture> dt(new DynamicTexture(imageFilename.toStdString()));
        dt->computeImagePyramid(imagePyramidPath);

        put_flog(LOG_DEBUG, "done");
    }
}

void MainWindow::constrainAspectRatio(bool set)
{
    constrainAspectRatio_ = set;

    if(constrainAspectRatio_ == true)
    {
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

        for(unsigned int i=0; i<contentWindowManagers.size(); i++)
        {
            contentWindowManagers[i]->fixAspectRatio();
        }

        // force a display group synchronization
        g_displayGroupManager->sendDisplayGroup();
    }
}

void MainWindow::updateGLWindows()
{
    // receive any waiting messages
    g_displayGroupManager->receiveMessages();

    // synchronize clock
    // do this right after receiving messages to ensure we have an accurate clock for rendering, etc. below
    if(g_mpiRank == 1)
    {
        g_displayGroupManager->sendFrameClockUpdate();
    }
    else
    {
        g_displayGroupManager->receiveFrameClockUpdate();
    }

    // render all GLWindows
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->updateGL();
    }

    // all render processes render simultaneously
    MPI_Barrier(g_mpiRenderComm);

    // swap buffers on all windows
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->swapBuffers();
    }

    // advance all contents
    g_displayGroupManager->advanceContents();

    // increment frame counter
    g_frameCount = g_frameCount + 1;

    emit(updateGLWindowsFinished());
}
