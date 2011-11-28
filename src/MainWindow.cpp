#include "MainWindow.h"
#include "main.h"
#include "PixelStreamContent.h"
#include "PixelStreamSource.h"
#include "ContentWindow.h"
#include "PythonConsole.h"
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
	PythonConsole::self()->init();

        // rank 0 window setup
        resize(800,600);

        // create menus in menu bar
        QMenu * fileMenu = menuBar()->addMenu("&File");
        QMenu * viewMenu = menuBar()->addMenu("&View");
	QMenu * windowMenu = menuBar()->addMenu("&Window"); 

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

        // share desktop action
        QAction * shareDesktopAction = new QAction("Share Desktop", this);
        shareDesktopAction->setStatusTip("Share desktop");
        shareDesktopAction->setCheckable(true);
        shareDesktopAction->setChecked(false);
        connect(shareDesktopAction, SIGNAL(toggled(bool)), this, SLOT(shareDesktop(bool)));

        // compute image pyramid action
        QAction * computeImagePyramidAction = new QAction("Compute Image Pyramid", this);
        computeImagePyramidAction->setStatusTip("Compute image pyramid");
        connect(computeImagePyramidAction, SIGNAL(triggered()), this, SLOT(computeImagePyramid()));

        // quit action
        QAction * quitAction = new QAction("Quit", this);
        quitAction->setStatusTip("Quit application");
        connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

        QAction *pythonAction = new QAction("Open Python Console", this); 
        pythonAction->setStatusTip("Open Python console"); 
        connect(pythonAction, SIGNAL(triggered()), PythonConsole::self(), SLOT(show())); 
        windowMenu->addAction(pythonAction); 

	QAction *xyzzyAction = new QAction("Xyzzy", this);
        xyzzyAction->setStatusTip("Run Xyzzy "); 
	connect(xyzzyAction, SIGNAL(triggered()), this, SLOT(xyzzy()));
        windowMenu->addAction(xyzzyAction); 

        // constrain aspect ratio action
        QAction * constrainAspectRatioAction = new QAction("Constrain Aspect Ratio", this);
        constrainAspectRatioAction->setStatusTip("Constrain aspect ratio");
        constrainAspectRatioAction->setCheckable(true);
        constrainAspectRatioAction->setChecked(constrainAspectRatio_);
        connect(constrainAspectRatioAction, SIGNAL(toggled(bool)), this, SLOT(constrainAspectRatio(bool)));

        // add actions to menus
        fileMenu->addAction(openContentAction);
        fileMenu->addAction(openContentsDirectoryAction);
        fileMenu->addAction(clearContentsAction);
        fileMenu->addAction(saveContentsAction);
        fileMenu->addAction(loadContentsAction);
        fileMenu->addAction(shareDesktopAction);
        fileMenu->addAction(computeImagePyramidAction);
        fileMenu->addAction(quitAction);
        viewMenu->addAction(constrainAspectRatioAction);

        // add actions to toolbar
        toolbar->addAction(openContentAction);
        toolbar->addAction(openContentsDirectoryAction);
        toolbar->addAction(clearContentsAction);
        toolbar->addAction(saveContentsAction);
        toolbar->addAction(loadContentsAction);
        toolbar->addAction(shareDesktopAction);
        toolbar->addAction(computeImagePyramidAction);

        // main widget / layout area
        QTabWidget * mainWidget = new QTabWidget();
        setCentralWidget(mainWidget);

        // add the local renderer group
        DisplayGroupGraphicsViewProxy * dggv = new DisplayGroupGraphicsViewProxy(g_displayGroup);
        mainWidget->addTab((QWidget *)dggv->getGraphicsView(), "Display group 0");

        // create contents dock widget
        QDockWidget * contentsDockWidget = new QDockWidget("Contents", this);
        QWidget * contentsWidget = new QWidget();
        QVBoxLayout * contentsLayout = new QVBoxLayout();
        contentsWidget->setLayout(contentsLayout);
        contentsDockWidget->setWidget(contentsWidget);
        addDockWidget(Qt::LeftDockWidgetArea, contentsDockWidget);

        // add the list widget
        DisplayGroupListWidgetProxy * dglwp = new DisplayGroupListWidgetProxy(g_displayGroup);
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
            boost::shared_ptr<ContentWindow> cw(new ContentWindow(c));

            g_displayGroup->addContentWindow(cw);
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
                boost::shared_ptr<ContentWindow> cw(new ContentWindow(c));

                g_displayGroup->addContentWindow(cw);

                int x = contentIndex % gridX;
                int y = contentIndex / gridX;

                cw->setCoordinates(x*w, y*h, w, h);

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
    g_displayGroup->setContentWindows(std::vector<boost::shared_ptr<ContentWindow> >());
}

void MainWindow::saveContents()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save State", "", "State files (*.dcs)");

    if(!filename.isEmpty())
    {
        // get contents vector
        std::vector<boost::shared_ptr<ContentWindow> > contentWindows = g_displayGroup->getContentWindows();

        // serialize state
        std::ofstream ofs(filename.toStdString().c_str());

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(ofs);
            oa << contentWindows;
        }
    }
}

void MainWindow::loadContents()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcs)");

    if(!filename.isEmpty())
    {
        // new contents vector
        std::vector<boost::shared_ptr<ContentWindow> > contentWindows;

        // serialize state
        std::ifstream ifs(filename.toStdString().c_str());

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_iarchive ia(ifs);
            ia >> contentWindows;
        }

        // assign new contents vector to display group
        g_displayGroup->setContentWindows(contentWindows);
    }
}

void MainWindow::shareDesktop(bool set)
{
    if(set == true)
    {
        // get width and height of area at top-left corner of desktop to share
        shareDesktopWidth_ = QInputDialog::getInt(this, "Width", "Width");
        shareDesktopHeight_ = QInputDialog::getInt(this, "Height", "Height");

        // add the content window if we don't already have one
        if(g_displayGroup->hasContent("desktop") != true)
        {
            boost::shared_ptr<Content> c(new PixelStreamContent("desktop"));
            boost::shared_ptr<ContentWindow> cw(new ContentWindow(c));

            g_displayGroup->addContentWindow(cw);
        }

        // setup timer to repeatedly update the shared desktop image
        connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));
        shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
    }
    else
    {
        shareDesktopUpdateTimer_.stop();
    }

    g_displayGroup->sendDisplayGroup();
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
        std::vector<boost::shared_ptr<ContentWindow> > contentWindows = g_displayGroup->getContentWindows();

        for(unsigned int i=0; i<contentWindows.size(); i++)
        {
            contentWindows[i]->fixAspectRatio();
        }

        // force a display group synchronization
        g_displayGroup->sendDisplayGroup();
    }
}

void MainWindow::shareDesktopUpdate()
{
    // take screenshot
    QPixmap desktopPixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), 0, 0, shareDesktopWidth_, shareDesktopHeight_);

    // save it to buffer
    QBuffer buffer;
    desktopPixmap.save(&buffer, "JPEG");

    // update pixel stream source
    g_pixelStreamSourceFactory.getObject("desktop")->setImageData(buffer.data());

    // send out updated pixel stream
    g_displayGroup->sendPixelStreams();
}

void MainWindow::updateGLWindows()
{
    // receive any waiting messages
    g_displayGroup->receiveMessages();

    // render all GLWindows
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->updateGL();
    }

    // all render processes render simultaneously
    // todo: sync swapbuffers
    MPI_Barrier(g_mpiRenderComm);

    // synchronize clock
    if(g_mpiRank == 1)
    {
        g_displayGroup->sendFrameClockUpdate();
    }
    else
    {
        g_displayGroup->receiveFrameClockUpdate();
    }

    // advance all contents
    g_displayGroup->advanceContents();

    // increment frame counter
    g_frameCount = g_frameCount + 1;

    emit(updateGLWindowsFinished());
}

#include "DisplayGroupPython.h"

boost::shared_ptr<DisplayGroupPython> pyDG = boost::shared_ptr<DisplayGroupPython>();

void MainWindow::xyzzy()
{
  if (pyDG == boost::shared_ptr<DisplayGroupPython>())
    pyDG = boost::shared_ptr<DisplayGroupPython>(new DisplayGroupPython(g_displayGroup));

  boost::shared_ptr<Content> c = Content::getContent("examples/grandpa.jpg");
  std::cerr << "content: " << c->getURI() << "\n";

  boost::shared_ptr<ContentWindow> cw = boost::shared_ptr<ContentWindow>(new ContentWindow(c));
  std::cerr << "content window: ";
  cw->dump();

  pyDG->addContentWindow(cw, NULL);

  std::cerr << "g_displayGroup contents:\n";
  g_displayGroup->dump();

  std::cerr << "DisplayGroupPython contents:\n";
  pyDG->dump();
}
