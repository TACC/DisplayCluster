#include "MainWindow.h"
#include "main.h"
#include "Content.h"
#include "ContentWindowManager.h"
#include "log.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupListWidgetProxy.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <fstream>

MainWindow::MainWindow()
{
    // defaults
    constrainAspectRatio_ = true;

    // make application quit when last window is closed
    QObject::connect(g_app, SIGNAL(lastWindowClosed()), g_app, SLOT(quit()));

    if(g_mpiRank == 0)
    {
        // rank 0 window setup
        resize(800,600);

        // create menus in menu bar
        QMenu * fileMenu = menuBar()->addMenu("&File");
        QMenu * viewMenu = menuBar()->addMenu("&View");

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

        // save contents action
        QAction * saveContentsAction = new QAction("Save State", this);
        saveContentsAction->setStatusTip("Save state");
        connect(saveContentsAction, SIGNAL(triggered()), this, SLOT(saveContents()));

        // load contents action
        QAction * loadContentsAction = new QAction("Load State", this);
        loadContentsAction->setStatusTip("Load state");
        connect(loadContentsAction, SIGNAL(triggered()), this, SLOT(loadContents()));

        // compute image pyramid action
        QAction * computeImagePyramidAction = new QAction("Compute Image Pyramid", this);
        computeImagePyramidAction->setStatusTip("Compute image pyramid");
        connect(computeImagePyramidAction, SIGNAL(triggered()), this, SLOT(computeImagePyramid()));

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

        // add actions to menus
        fileMenu->addAction(openContentAction);
        fileMenu->addAction(openContentsDirectoryAction);
        fileMenu->addAction(clearContentsAction);
        fileMenu->addAction(saveContentsAction);
        fileMenu->addAction(loadContentsAction);
        fileMenu->addAction(computeImagePyramidAction);
        fileMenu->addAction(quitAction);
        viewMenu->addAction(constrainAspectRatioAction);
        viewMenu->addAction(showWindowBordersAction);

        // add actions to toolbar
        toolbar->addAction(openContentAction);
        toolbar->addAction(openContentsDirectoryAction);
        toolbar->addAction(clearContentsAction);
        toolbar->addAction(saveContentsAction);
        toolbar->addAction(loadContentsAction);
        toolbar->addAction(computeImagePyramidAction);

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
            for(int i=0; i<g_configuration->getMyNumTiles(); i++)
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

void MainWindow::saveContents()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save State", "", "State files (*.dcs)");

    if(!filename.isEmpty())
    {
        // make sure filename has .dcs extension
        if(filename.endsWith(".dcs") != true)
        {
            put_flog(LOG_DEBUG, "appended .dcs filename extension");
            filename.append(".dcs");
        }

        // get contents vector
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

        // serialize state
        std::ofstream ofs(filename.toStdString().c_str());

        // version number
        int version = CONTENTS_FILE_VERSION_NUMBER;

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(ofs);

            oa << version;
            oa << contentWindowManagers;
        }
    }
}

void MainWindow::loadContents()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcs)");

    if(!filename.isEmpty())
    {
        // new contents vector
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers;

        // serialize state
        std::ifstream ifs(filename.toStdString().c_str());

        int version = -1;

        // brace this so destructor is called on archive before we use the stream
        // also, catch exceptions that boost serialization may throw
        try
        {
            boost::archive::binary_iarchive ia(ifs);

            ia >> version;

            put_flog(LOG_DEBUG, "state file version %i, current version %i", version, CONTENTS_FILE_VERSION_NUMBER);

            // make sure we have a compatible state file
            if(version != CONTENTS_FILE_VERSION_NUMBER)
            {
                QMessageBox::warning(this, "Error", "The state file you attempted to load is not compatible with this version of DisplayCluster.", QMessageBox::Ok, QMessageBox::Ok);

                return;
            }

            ia >> contentWindowManagers;
        }
        catch(...)
        {
            put_flog(LOG_DEBUG, "caught exception");

            QMessageBox::warning(this, "Error", "The state file you attempted to load is not compatible with this version of DisplayCluster.", QMessageBox::Ok, QMessageBox::Ok);

            return;
        }

        // assign new contents vector to display group
        g_displayGroupManager->setContentWindowManagers(contentWindowManagers);
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
