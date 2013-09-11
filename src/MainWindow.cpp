/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "MainWindow.h"
#include "main.h"
#include "Content.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"
#include "log.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupListWidgetProxy.h"
#include "BackgroundWidget.h"
#include "LocalPixelStreamerManager.h"

#if ENABLE_PYTHON_SUPPORT
    #include "PythonConsole.h"
#endif

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonThread.h"
#endif

#if ENABLE_TUIO_TOUCH_LISTENER
    #include "MultiTouchListener.h"
#endif

MainWindow::MainWindow()
: backgroundWidget_(0)
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
        setAcceptDrops(true);

        // create menus in menu bar
        QMenu * fileMenu = menuBar()->addMenu("&File");
        QMenu * viewMenu = menuBar()->addMenu("&View");
        QMenu * viewStreamingMenu = viewMenu->addMenu("&Streaming");
#if ENABLE_PYTHON_SUPPORT
        // add Window menu for Python console. if we add any other entries to it we'll need to remove the #if
        QMenu * windowMenu = menuBar()->addMenu("&Window");
#endif

#if ENABLE_SKELETON_SUPPORT
        QMenu * skeletonMenu = menuBar()->addMenu("&Skeleton Tracking");
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

        // load background content action
        QAction * backgroundAction = new QAction("Background", this);
        backgroundAction->setStatusTip("Select the background color and content");
        connect(backgroundAction, SIGNAL(triggered()), this, SLOT(showBackgroundWidget()));

        // Open webbrowser action
        QAction * webbrowserAction = new QAction("Web Browser", this);
        webbrowserAction->setStatusTip("Open a web browser");
        connect(webbrowserAction, SIGNAL(triggered()), this, SLOT(openWebBrowser()));

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

        // show mouse cursor action
        QAction * showMouseCursorAction = new QAction("Show Mouse Cursor", this);
        showMouseCursorAction->setStatusTip("Show mouse cursor");
        showMouseCursorAction->setCheckable(true);
        showMouseCursorAction->setChecked(g_displayGroupManager->getOptions()->getShowMouseCursor());
        connect(showMouseCursorAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowMouseCursor(bool)));

        // show touch points action
        QAction * showTouchPoints = new QAction("Show Touch Points", this);
        showTouchPoints->setStatusTip("Show touch points");
        showTouchPoints->setCheckable(true);
        showTouchPoints->setChecked(g_displayGroupManager->getOptions()->getShowTouchPoints());
        connect(showTouchPoints, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowTouchPoints(bool)));

        // show movie controls action
        QAction * showMovieControlsAction = new QAction("Show Movie Controls", this);
        showMovieControlsAction->setStatusTip("Show movie controls");
        showMovieControlsAction->setCheckable(true);
        showMovieControlsAction->setChecked(g_displayGroupManager->getOptions()->getShowMovieControls());
        connect(showMovieControlsAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowMovieControls(bool)));

        // show test pattern action
        QAction * showTestPatternAction = new QAction("Show Test Pattern", this);
        showTestPatternAction->setStatusTip("Show test pattern");
        showTestPatternAction->setCheckable(true);
        showTestPatternAction->setChecked(g_displayGroupManager->getOptions()->getShowTestPattern());
        connect(showTestPatternAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowTestPattern(bool)));

        // enable mullion compensation action
        QAction * enableMullionCompensationAction = new QAction("Enable Mullion Compensation", this);
        enableMullionCompensationAction->setStatusTip("Enable mullion compensation");
        enableMullionCompensationAction->setCheckable(true);
        enableMullionCompensationAction->setChecked(g_displayGroupManager->getOptions()->getEnableMullionCompensation());
        connect(enableMullionCompensationAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setEnableMullionCompensation(bool)));

        // show zoom context action
        QAction * showZoomContextAction = new QAction("Show Zoom Context", this);
        showZoomContextAction->setStatusTip("Show zoom context");
        showZoomContextAction->setCheckable(true);
        showZoomContextAction->setChecked(g_displayGroupManager->getOptions()->getShowZoomContext());
        connect(showZoomContextAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowZoomContext(bool)));

        // enable streaming synchronization action
        QAction * enableStreamingSynchronizationAction = new QAction("Enable Streaming Synchronization", this);
        enableStreamingSynchronizationAction->setStatusTip("Enable streaming synchronization");
        enableStreamingSynchronizationAction->setCheckable(true);
        enableStreamingSynchronizationAction->setChecked(g_displayGroupManager->getOptions()->getEnableStreamingSynchronization());
        connect(enableStreamingSynchronizationAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setEnableStreamingSynchronization(bool)));

        // show streaming segments action
        QAction * showStreamingSegmentsAction = new QAction("Show Segments", this);
        showStreamingSegmentsAction->setStatusTip("Show segments");
        showStreamingSegmentsAction->setCheckable(true);
        showStreamingSegmentsAction->setChecked(g_displayGroupManager->getOptions()->getShowStreamingSegments());
        connect(showStreamingSegmentsAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowStreamingSegments(bool)));

        // show streaming statistics action
        QAction * showStreamingStatisticsAction = new QAction("Show Statistics", this);
        showStreamingStatisticsAction->setStatusTip("Show statistics");
        showStreamingStatisticsAction->setCheckable(true);
        showStreamingStatisticsAction->setChecked(g_displayGroupManager->getOptions()->getShowStreamingStatistics());
        connect(showStreamingStatisticsAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowStreamingStatistics(bool)));

#if ENABLE_SKELETON_SUPPORT
        // enable skeleton tracking action
        QAction * enableSkeletonTrackingAction = new QAction("Enable Skeleton Tracking", this);
        enableSkeletonTrackingAction->setStatusTip("Enable skeleton tracking");
        enableSkeletonTrackingAction->setCheckable(true);
        enableSkeletonTrackingAction->setChecked(true); // timer is started by default
        connect(enableSkeletonTrackingAction, SIGNAL(toggled(bool)), this, SLOT(setEnableSkeletonTracking(bool)));

        connect(this, SIGNAL(enableSkeletonTracking()), g_skeletonThread, SLOT(startTimer()));
        connect(this, SIGNAL(disableSkeletonTracking()), g_skeletonThread, SLOT(stopTimer()));

        // show skeletons action
        QAction * showSkeletonsAction = new QAction("Show Skeletons", this);
        showSkeletonsAction->setStatusTip("Show skeletons");
        showSkeletonsAction->setCheckable(true);
        showSkeletonsAction->setChecked(g_displayGroupManager->getOptions()->getShowSkeletons());
        connect(showSkeletonsAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowSkeletons(bool)));
#endif

        // add actions to menus
        fileMenu->addAction(openContentAction);
        fileMenu->addAction(openContentsDirectoryAction);
        fileMenu->addAction(webbrowserAction);
        fileMenu->addAction(clearContentsAction);
        fileMenu->addAction(saveStateAction);
        fileMenu->addAction(loadStateAction);
        fileMenu->addAction(computeImagePyramidAction);
        fileMenu->addAction(quitAction);
        viewMenu->addAction(backgroundAction);
        viewMenu->addAction(constrainAspectRatioAction);
        viewMenu->addAction(showWindowBordersAction);
        viewMenu->addAction(showMouseCursorAction);
        viewMenu->addAction(showTouchPoints);
        viewMenu->addAction(showMovieControlsAction);
        viewMenu->addAction(showTestPatternAction);
        viewMenu->addAction(enableMullionCompensationAction);
        viewMenu->addAction(showZoomContextAction);
        viewStreamingMenu->addAction(enableStreamingSynchronizationAction);
        viewStreamingMenu->addAction(showStreamingSegmentsAction);
        viewStreamingMenu->addAction(showStreamingStatisticsAction);

#if ENABLE_PYTHON_SUPPORT
        windowMenu->addAction(pythonConsoleAction);
#endif

#if ENABLE_SKELETON_SUPPORT
        skeletonMenu->addAction(enableSkeletonTrackingAction);
        skeletonMenu->addAction(showSkeletonsAction);
#endif

        // add actions to toolbar
        toolbar->addAction(openContentAction);
        toolbar->addAction(openContentsDirectoryAction);
        toolbar->addAction(clearContentsAction);
        toolbar->addAction(saveStateAction);
        toolbar->addAction(loadStateAction);
        toolbar->addAction(computeImagePyramidAction);
        toolbar->addAction(backgroundAction);
        toolbar->addAction(webbrowserAction);
#if ENABLE_PYTHON_SUPPORT
        toolbar->addAction(pythonConsoleAction);
#endif
        // main widget / layout area
        QTabWidget * mainWidget = new QTabWidget();
        setCentralWidget(mainWidget);

        // add the local renderer group
        DisplayGroupGraphicsViewProxy * dggv = new DisplayGroupGraphicsViewProxy(g_displayGroupManager);
        mainWidget->addTab((QWidget *)dggv->getGraphicsView(), "Display group 0");

#if ENABLE_TUIO_TOUCH_LISTENER
        touchListener_ = new MultiTouchListener( dggv );
#endif

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

        // timer will trigger polling of PixelStreams
        connect(&pixelStreamTimer_, SIGNAL(timeout()), g_displayGroupManager.get(), SLOT(sendPixelStreams()));

        // start the timer
        pixelStreamTimer_.start(1000 / 30); // 30 fps

        show();
    }
    else
    {
#if ENABLE_TUIO_TOUCH_LISTENER
        touchListener_ = 0;
#endif
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

MainWindow::~MainWindow()
{
#if ENABLE_TUIO_TOUCH_LISTENER
    delete touchListener_;
#endif
}

bool MainWindow::getConstrainAspectRatio()
{
    return constrainAspectRatio_;
}

boost::shared_ptr<GLWindow> MainWindow::getGLWindow(int index)
{
    return glWindows_[index];
}

boost::shared_ptr<GLWindow> MainWindow::getActiveGLWindow()
{
    return activeGLWindow_;
}

std::vector<boost::shared_ptr<GLWindow> > MainWindow::getGLWindows()
{
    return glWindows_;
}

void MainWindow::addContent(const QString& filename)
{
    boost::shared_ptr<Content> c = ContentFactory::getContent(filename);

    if(c != NULL)
    {
        boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

        g_displayGroupManager->addContentWindowManager(cwm);

        cwm->adjustSize( SIZE_1TO1 );
    }
    else
    {
        QMessageBox messageBox;
        messageBox.setText("Unsupported file format.");
        messageBox.exec();
    }
}

void MainWindow::openContent()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose content"), QString(), ContentFactory::getSupportedFilesFilterAsString());

    if(!filename.isEmpty())
    {
        addContent(filename);
    }
}

void MainWindow::estimateGridSize(unsigned int numElem, int &gridX, int &gridY)
{
    gridX = (int)(ceil(sqrt(numElem)));
    gridY = (gridX*(gridX-1)>=(int)numElem) ? gridX-1 : gridX;
}

void MainWindow::addContentDirectory(const QString& directoryName, int gridX, int gridY)
{
    QDir directory(directoryName);
    directory.setFilter(QDir::Files);
    directory.setNameFilters( ContentFactory::getSupportedFilesFilter() );

    QFileInfoList list = directory.entryInfoList();

    // Prevent opening of folders with an excessively large number of items
    if (list.size() > 16)
    {
        QString msg = "Opening this folder will create " + QString::number(list.size()) + " content elements. Are you sure you want to continue?";
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Warning", msg, QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;
    }

    // If the grid size is unspecified, compute one large enough to hold all the elements
    if (gridX <= 0 || gridY <= 0)
    {
        estimateGridSize(list.size(), gridX, gridY);
    }

    float w = 1./(float)gridX;
    float h = 1./(float)gridY;

    int contentIndex = 0;

    for(int i=0; i<list.size() && contentIndex < gridX*gridY; i++)
    {
        QFileInfo fileInfo = list.at(i);

        boost::shared_ptr<Content> c = ContentFactory::getContent(fileInfo.absoluteFilePath());

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

void MainWindow::openContentsDirectory()
{
    QString directoryName = QFileDialog::getExistingDirectory(this);

    if(!directoryName.isEmpty())
    {
        int gridX = QInputDialog::getInt(this, "Grid X dimension", "Grid X dimension");
        int gridY = QInputDialog::getInt(this, "Grid Y dimension", "Grid Y dimension");

        addContentDirectory(directoryName, gridX, gridY);
    }
}

void MainWindow::showBackgroundWidget()
{
    assert(g_mpiRank == 0 && "Background widget is intended only for rank 0 application");

    if(!backgroundWidget_)
    {
        backgroundWidget_ = new BackgroundWidget(this);
        backgroundWidget_->setModal(true);
    }

    backgroundWidget_->show();
}

void MainWindow::openWebBrowser()
{
    static int webbrowserCounter = 0;
    bool ok;
    QString url = QInputDialog::getText(this, tr("New WebBrowser Content"),
                                         tr("URL:"), QLineEdit::Normal,
                                         "http://www.google.ch", &ok);
    if (ok && !url.isEmpty())
    {
        bool success = g_localPixelStreamers->createWebBrowser("WebBrowser"+QString::number(webbrowserCounter++), url);
        if(!success)
        {
            QMessageBox::warning(this, "Error", "A WebBrowser with this uri already exists.", QMessageBox::Ok, QMessageBox::Ok);
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
        loadState(filename);
    }
}

void MainWindow::loadState(const QString& filename)
{
    bool success = g_displayGroupManager->loadStateXMLFile(filename.toStdString());

    if(!success)
    {
        QMessageBox::warning(this, "Error", "Could not load state file.", QMessageBox::Ok, QMessageBox::Ok);
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

        boost::shared_ptr<DynamicTexture> dt(new DynamicTexture(imageFilename));
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

#if ENABLE_SKELETON_SUPPORT
void MainWindow::setEnableSkeletonTracking(bool enable)
{
    if(enable == true)
    {
        emit(enableSkeletonTracking());
    }
    else
    {
        emit(disableSkeletonTracking());
    }
}
#endif

QStringList MainWindow::extractValidContentUrls(const QMimeData* data)
{
    QStringList pathList;

    if (data->hasUrls())
    {
        QList<QUrl> urlList = data->urls();

        foreach (QUrl url, urlList)
        {
            QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
            if (ContentFactory::getSupportedExtensions().contains(extension))
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QStringList MainWindow::extractFolderUrls(const QMimeData* data)
{
    QStringList pathList;

    if (data->hasUrls())
    {
        QList<QUrl> urlList = data->urls();

        foreach (QUrl url, urlList)
        {
            if (QDir(url.toLocalFile()).exists())
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QString MainWindow::extractStateFile(const QMimeData* data)
{
    QList<QUrl> urlList = data->urls();
    if (urlList.size() == 1)
    {
        QUrl url = urlList[0];
        QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
        if (extension == "dcx")
            return url.toLocalFile();
    }
    return QString();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    QStringList pathList = extractValidContentUrls(event->mimeData());
    QStringList dirList = extractFolderUrls(event->mimeData());
    QString stateFile = extractStateFile(event->mimeData());

    if (!pathList.empty() || !dirList.empty() || !stateFile.isNull())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QStringList pathList = extractValidContentUrls(event->mimeData());
    foreach (QString url, pathList)
    {
        addContent(url);
    }

    QStringList dirList = extractFolderUrls(event->mimeData());
    if (dirList.size() > 0)
    {
        QString url = dirList[0]; // Only one directory at a time

        addContentDirectory(url);
    }

    QString stateFile = extractStateFile(event->mimeData());
    if (!stateFile.isNull())
    {
        loadState(stateFile);
    }

    event->acceptProposedAction();
}

void MainWindow::updateGLWindows()
{
    if( g_displayGroupManager->getOptions()->getShowMouseCursor( ))
        unsetCursor();
    else
        setCursor( QCursor( Qt::BlankCursor ));

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
        activeGLWindow_ = glWindows_[i];
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

    // clear old factory objects and purge any textures
    if(glWindows_.size() > 0)
    {
        glWindows_[0]->getTextureFactory().clearStaleObjects();
        glWindows_[0]->getDynamicTextureFactory().clearStaleObjects();
        glWindows_[0]->getSVGFactory().clearStaleObjects();
        glWindows_[0]->getMovieFactory().clearStaleObjects();
        glWindows_[0]->getPixelStreamFactory().clearStaleObjects();

        glWindows_[0]->purgeTextures();
    }

    // increment frame counter
    g_frameCount = g_frameCount + 1;

    emit(updateGLWindowsFinished());
}

void MainWindow::finalize()
{
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->finalize();
    }
}
