#include "config.h"
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
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <QDomDocument>

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
        fileMenu->addAction(saveContentsAction);
        fileMenu->addAction(loadContentsAction);
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
        toolbar->addAction(saveContentsAction);
        toolbar->addAction(loadContentsAction);
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
    QString filename = QFileDialog::getSaveFileName(this, "Save State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        // make sure filename has .dcx extension
        if(filename.endsWith(".dcx") != true)
        {
            put_flog(LOG_DEBUG, "appended .dcx filename extension");
            filename.append(".dcx");
        }

        // get contents vector
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = g_displayGroupManager->getContentWindowManagers();

        // save as XML
        QDomDocument doc("state");
        QDomElement root = doc.createElement("state");
        doc.appendChild(root);

        // version number
        int version = CONTENTS_FILE_VERSION_NUMBER;

        QDomElement v = doc.createElement("version");
        v.appendChild(doc.createTextNode(QString::number(version)));
        root.appendChild(v);

        for(unsigned int i=0; i<contentWindowManagers.size(); i++)
        {
            // get values
            std::string uri = contentWindowManagers[i]->getContent()->getURI();

            double x, y, w, h;
            contentWindowManagers[i]->getCoordinates(x, y, w, h);

            double centerX, centerY;
            contentWindowManagers[i]->getCenter(centerX, centerY);

            double zoom = contentWindowManagers[i]->getZoom();

            bool selected = contentWindowManagers[i]->getSelected();

            // add the XML node with these values
            QDomElement cwmNode = doc.createElement("ContentWindow");
            root.appendChild(cwmNode);

            QDomElement n = doc.createElement("URI");
            n.appendChild(doc.createTextNode(QString(uri.c_str())));
            cwmNode.appendChild(n);

            n = doc.createElement("x");
            n.appendChild(doc.createTextNode(QString::number(x)));
            cwmNode.appendChild(n);

            n = doc.createElement("y");
            n.appendChild(doc.createTextNode(QString::number(y)));
            cwmNode.appendChild(n);

            n = doc.createElement("w");
            n.appendChild(doc.createTextNode(QString::number(w)));
            cwmNode.appendChild(n);

            n = doc.createElement("h");
            n.appendChild(doc.createTextNode(QString::number(h)));
            cwmNode.appendChild(n);

            n = doc.createElement("centerX");
            n.appendChild(doc.createTextNode(QString::number(centerX)));
            cwmNode.appendChild(n);

            n = doc.createElement("centerY");
            n.appendChild(doc.createTextNode(QString::number(centerY)));
            cwmNode.appendChild(n);

            n = doc.createElement("zoom");
            n.appendChild(doc.createTextNode(QString::number(zoom)));
            cwmNode.appendChild(n);

            n = doc.createElement("selected");
            n.appendChild(doc.createTextNode(QString::number(selected)));
            cwmNode.appendChild(n);
        }

        QString xml = doc.toString();

        std::ofstream ofs(filename.toStdString().c_str());

        ofs << xml.toStdString();
    }
}

void MainWindow::loadContents()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        QXmlQuery query;

        if(query.setFocus(QUrl(filename)) == false)
        {
            put_flog(LOG_FATAL, "failed to load %s", filename.toStdString().c_str());
            exit(-1);
        }

        // temp
        QString qstring;

        // get version; we don't do anything with it now but may in the future
        int version = -1;
        query.setQuery("string(/state/version)");

        if(query.evaluateTo(&qstring) == true)
        {
            version = qstring.toInt();
        }

        // get number of content windows
        int numContentWindows = 0;
        query.setQuery("string(count(//state/ContentWindow))");

        if(query.evaluateTo(&qstring) == true)
        {
            numContentWindows = qstring.toInt();
        }

        put_flog(LOG_INFO, "%i content windows", numContentWindows);

        // new contents vector
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers;

        for(int i=1; i<=numContentWindows; i++)
        {
            char string[1024];

            std::string uri;
            sprintf(string, "string(//state/ContentWindow[%i]/URI)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                uri = qstring.toStdString();

                // remove any whitespace
                boost::trim(uri);

                put_flog(LOG_DEBUG, "found content window with URI %s", uri.c_str());
            }

            double x, y, w, h, centerX, centerY, zoom;
            x = y = w = h = centerX = centerY = zoom = -1.;

            bool selected = false;

            sprintf(string, "string(//state/ContentWindow[%i]/x)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                x = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/y)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                y = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/w)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                w = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/h)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                h = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/centerX)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                centerX = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/centerY)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                centerY = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/zoom)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                zoom = qstring.toDouble();
            }

            sprintf(string, "string(//state/ContentWindow[%i]/selected)", i);
            query.setQuery(string);

            if(query.evaluateTo(&qstring) == true)
            {
                selected = (bool)qstring.toInt();
            }

            // add the window if we have a valid URI
            if(uri.empty() == false)
            {
                boost::shared_ptr<Content> c = Content::getContent(uri);

                if(c != NULL)
                {
                    boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                    contentWindowManagers.push_back(cwm);

                    // now, apply settings if we got them from the XML file
                    if(x != -1. || y != -1.)
                    {
                        cwm->setPosition(x, y);
                    }

                    if(w != -1. || h != -1.)
                    {
                        cwm->setSize(w, h);
                    }

                    // zoom needs to be set before center because of clamping
                    if(zoom != -1.)
                    {
                        cwm->setZoom(zoom);
                    }

                    if(centerX != -1. || centerY != -1.)
                    {
                        cwm->setCenter(centerX, centerY);
                    }

                    cwm->setSelected(selected);
                }
            }
        }

        if(contentWindowManagers.size() > 0)
        {
            // assign new contents vector to display group
            g_displayGroupManager->setContentWindowManagers(contentWindowManagers);
        }
        else
        {
            QMessageBox::warning(this, "Error", "No content windows specified in the state file.", QMessageBox::Ok, QMessageBox::Ok);
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
