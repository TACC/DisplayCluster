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
#include "ContentWindowManager.h"
#include "log.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupListWidgetProxy.h"

#include <set>

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
        QMenu * viewStreamingMenu = viewMenu->addMenu("&Streaming");

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

        // show content labels action
        QAction * showContentLabelsAction = new QAction("Show Content Labels", this);
        showContentLabelsAction->setStatusTip("Show identifying label overlays on content windows");
        showContentLabelsAction->setCheckable(true);
        showContentLabelsAction->setChecked(g_displayGroupManager->getOptions()->getShowContentLabels());
        connect(showContentLabelsAction, SIGNAL(toggled(bool)), g_displayGroupManager->getOptions().get(), SLOT(setShowContentLabels(bool)));

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
        viewMenu->addAction(showContentLabelsAction);
        viewMenu->addAction(showTestPatternAction);
        viewMenu->addAction(enableMullionCompensationAction);
        viewMenu->addAction(showZoomContextAction);
        viewStreamingMenu->addAction(enableStreamingSynchronizationAction);
        viewStreamingMenu->addAction(showStreamingSegmentsAction);
        viewStreamingMenu->addAction(showStreamingStatisticsAction);

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

        // version label in the menu bar's corner - lets someone looking at
        // a running wall confirm which build it's actually running, without
        // having to go dig through logs. the corner (rather than the
        // toolbar) is deliberate: a toolbar widget gets silently swallowed
        // into the ">>" overflow chevron on a narrow window, the corner
        // widget doesn't have that failure mode
        QLabel * versionLabel = new QLabel(QString("DisplayCluster ") + DISPLAYCLUSTER_GIT_VERSION);
        versionLabel->setContentsMargins(0, 0, 8, 0);
        menuBar()->setCornerWidget(versionLabel, Qt::TopRightCorner);

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

        // timer will trigger polling of ParallelPixelStreams
        connect(&parallelPixelStreamTimer_, SIGNAL(timeout()), g_displayGroupManager.get(), SLOT(sendParallelPixelStreams()));

        // start the timer
        parallelPixelStreamTimer_.start(1000 / 30); // 30 fps

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

            // GLWindow is a QWindow, not a QWidget; wrap it to embed as the central widget
            setCentralWidget(QWidget::createWindowContainer(glw.get()));

            if(g_configuration->getFullscreen() == true)
            {
                showFullScreen();
            }
            else
            {
                int x = g_configuration->getTileX(0);
                int y = g_configuration->getTileY(0);
                setGeometry(x, y, g_configuration->getScreenWidth(), g_configuration->getScreenHeight());
                setWindowFlags(Qt::FramelessWindowHint);
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
                    int x = g_configuration->getTileX(i);
                    int y = g_configuration->getTileY(i);
                    glw->setGeometry(x, y, g_configuration->getScreenWidth(), g_configuration->getScreenHeight());
                    glw->setFlags(Qt::FramelessWindowHint);
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

boost::shared_ptr<GLWindow> MainWindow::getActiveGLWindow()
{
    return activeGLWindow_;
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

    int gridX = QInputDialog::getInt(this, "Grid X dimension", "Grid X dimension", 2);
    int gridY = QInputDialog::getInt(this, "Grid Y dimension", "Grid Y dimension", 2);
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

void MainWindow::loadState(QString *filename)
{
        bool success = g_displayGroupManager->loadStateXMLFile(filename->toStdString());
        if(success != true)
            QMessageBox::warning(this, "Error", "Could not load state file.", QMessageBox::Ok, QMessageBox::Ok);
}

void MainWindow::loadState()
{
				QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcx)");

				if(!filename.isEmpty())
						loadState(&filename);
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
    {
        auto t0 = high_resolution_clock::now();

        if(g_mpiRank == 1)
        {
            g_displayGroupManager->sendFrameClockUpdate();
        }
        else
        {
            g_displayGroupManager->receiveFrameClockUpdate();
        }

        duration<double, std::ratio<1>> elapsed = high_resolution_clock::now() - t0;
        if (elapsed.count() > 0.1)
            put_flog(LOG_WARN, "frame clock send/receive took %.3fs", elapsed.count());
    }

    // every rank participates in a per-movie "is everyone caught up"
    // reduction, every render frame, so a straggler holds the whole
    // display on its last agreed-good frame instead of the rest advancing
    // without it (see Movie::setHold()/render()). This has to iterate
    // movie URIs in an order every rank is guaranteed to agree on -
    // g_displayGroupManager->getContentWindowManagers()'s own order isn't
    // safe for that, since it changes on moveToFront() (every drag starts
    // with one, via ContentWindowGraphicsItem::mousePressEvent()), and
    // that reordering broadcast can reach different ranks on different
    // frames. MPI_Allreduce calls are matched by their position in the
    // call sequence, not by which movie they're actually about, so a
    // transient order disagreement pairs one rank's vote for movie A with
    // another rank's vote for movie B, corrupting the hold decision for
    // both - a std::set of URIs is naturally sorted and so stays
    // identically ordered across ranks regardless of Z-order, as long as
    // the set of open movies itself agrees (which a pure reorder doesn't
    // change).
    //
    // The reduction's outcome also decides how this movie's clock advances
    // this frame: Resync() (re-anchor to the shared wall clock, so
    // cross-host clock-rate drift never gets more than one frame's worth
    // of time to accumulate) when everyone's caught up, or Freeze() (pin
    // the target frame, advancing it not at all) while anyone's still
    // behind. Without that, a still-behind decoder would be chasing a
    // target that keeps moving in real time even though the whole display
    // is frozen for viewers - if its decode throughput can't beat
    // real-time (contention from another movie decoding concurrently,
    // e.g.), that target recedes as fast as or faster than it can close
    // the gap, and the freeze never ends. This used to be Resync()
    // unconditionally, on its own 300s timer, guarded by an
    // MPI_Barrier(g_mpiRenderComm) that only whichever ranks happened to
    // have a live Decoder for a given movie ever called - piggybacking on
    // the same per-movie reduction that already runs every frame, on every
    // rank regardless of whether it has this movie, avoids that mismatch
    // the same way the reduction itself does.
    //
    // All movies are reduced in ONE MPI_Allreduce call, not one call per
    // movie: MPI_Allreduce reduces arrays element-wise, so packing every
    // movie's vote into one int array and reducing the whole array at once
    // keeps this at a single blocking round-trip per frame regardless of
    // how many movies are open. One call per movie was N blocking
    // round-trips every render frame - enough, with several movies open,
    // to measurably slow the whole loop down, which meant Resync() ran
    // less often for every movie (not just ones actually behind), and once
    // the elapsed time folded into each less-frequent Resync() exceeded
    // the small-gap catch-up threshold, even untouched playback started
    // tripping the catch-up path.
    {
        std::vector<boost::shared_ptr<ContentWindowManager> > windows = g_displayGroupManager->getContentWindowManagers();
        std::set<std::string> movieURIs;

        for (unsigned int i = 0; i < windows.size(); i++)
            if (windows[i]->getContent()->getType() == CONTENT_TYPE_MOVIE)
                movieURIs.insert(windows[i]->getContent()->getURI());

        std::vector<boost::shared_ptr<Movie> > movies;
        std::vector<int> localSynced;

        for (std::set<std::string>::iterator it = movieURIs.begin(); it != movieURIs.end(); ++it)
        {
            boost::shared_ptr<Movie> movie = (glWindows_.size() > 0) ? glWindows_[0]->getMovieFactory().findObject(*it) : boost::shared_ptr<Movie>();

            movies.push_back(movie);

            // a paused movie's isSynced() is frozen at whatever it was the
            // instant it got paused, since nothing updates it while its
            // decoder thread is asleep - if that happened to be false
            // (mid-catch-up right when this tile stopped overlapping the
            // content), it would vote "unsynced" for as long as it stays
            // paused, holding every other tile hostage on behalf of a rank
            // that currently has nothing to show and isn't even trying to
            // catch up. Same situation as having no movie here at all -
            // vote vacuously synced.
            localSynced.push_back((!movie || movie->isPaused() || movie->isSynced()) ? 1 : 0);
        }

        std::vector<int> allSynced(localSynced.size());

        {
            auto t0 = high_resolution_clock::now();

            MPI_Allreduce(localSynced.data(), allSynced.data(), (int)localSynced.size(), MPI_INT, MPI_LAND, g_mpiRenderComm);

            duration<double, std::ratio<1>> elapsed = high_resolution_clock::now() - t0;
            if (elapsed.count() > 0.1)
                put_flog(LOG_WARN, "per-movie sync Allreduce (%d movies) took %.3fs", (int)localSynced.size(), elapsed.count());
        }

        auto now = high_resolution_clock::now();

        for (size_t i = 0; i < movies.size(); i++)
        {
            if (!movies[i])
                continue;

            movies[i]->setHold(!allSynced[i]);

            if (allSynced[i])
                movies[i]->Resync(now);
            else
                movies[i]->Freeze(now);
        }
    }

    // render all GLWindows
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        activeGLWindow_ = glWindows_[i];
        glWindows_[i]->updateGL();
    }

    // all render processes render simultaneously
    {
        auto t0 = high_resolution_clock::now();

        MPI_Barrier(g_mpiRenderComm);

        duration<double, std::ratio<1>> elapsed = high_resolution_clock::now() - t0;
        if (elapsed.count() > 0.1)
            put_flog(LOG_WARN, "post-render MPI_Barrier took %.3fs", elapsed.count());
    }

    // swap buffers on all windows
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->swapBuffers();
    }

    // advance all contents
    g_displayGroupManager->advanceContents();

    // The above will identify the movies that need to be decoding...

    for (auto& item : glWindows_[0]->getMovieFactory())
        if (item.second->getLastRenderedFrame() == g_frameCount && item.second->isPaused())
            item.second->Resume();
        else if (item.second->getLastRenderedFrame() < g_frameCount && !item.second->isPaused())
            item.second->Pause();

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

    // every render rank learns "shutdown requested" via the same
    // frame-clock broadcast every one of them already participates in
    // identically, every frame (see DisplayGroupManager::
    // isShutdownRequested()'s comment) - so by this point, every render
    // rank has uniformly finished this frame's per-frame collectives (the
    // per-movie sync Allreduce, the post-render Barrier) together,
    // regardless of which of them reacted to the shutdown request
    // fastest. Stopping here rather than re-queuing the next iteration is
    // what keeps everyone leaving on the same logical frame, instead of
    // one rank abandoning a collective another rank is still relying on
    // it for.
    if (g_displayGroupManager->isShutdownRequested())
    {
        g_app->quit();
        return;
    }

    emit(updateGLWindowsFinished());
}

void MainWindow::finalize()
{
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->finalize();
    }
}
