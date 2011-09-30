#include "MainWindow.h"
#include "main.h"
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "MovieContent.h"
#include "PixelStreamContent.h"
#include "PixelStreamSource.h"

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

        // share desktop action
        QAction * shareDesktopAction = new QAction("Share Desktop", this);
        shareDesktopAction->setStatusTip("Share desktop");
        shareDesktopAction->setCheckable(true);
        shareDesktopAction->setChecked(false);
        connect(shareDesktopAction, SIGNAL(toggled(bool)), this, SLOT(shareDesktop(bool)));

        // add actions to toolbar
        toolbar->addAction(openContentAction);
        toolbar->addAction(shareDesktopAction);

        // main widget / layout area
        QTabWidget * mainWidget = new QTabWidget();
        setCentralWidget(mainWidget);

        // add the local renderer group
        mainWidget->addTab((QWidget *)g_displayGroup->getGraphicsView().get(), "Display group 0");

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
        // see if this is an image
        QImageReader imageReader(filename);

        if(imageReader.canRead() == true)
        {
            // get its size
            QSize size = imageReader.size();

            // small images will use Texture; larger images will use DynamicTexture
            boost::shared_ptr<Content> c;

            if(size.width() <= 4096 && size.height() <= 4096)
            {
                boost::shared_ptr<Content> temp(new TextureContent(filename.toStdString()));
                c = temp;
            }
            else
            {
                boost::shared_ptr<Content> temp(new DynamicTextureContent(filename.toStdString()));
                c = temp;
            }

            g_displayGroup->addContent(c);
        }
        // see if this is a movie
        // todo: need a better way to determine file type
        else if(filename.endsWith(".mov") || filename.endsWith(".avi") || filename.endsWith(".mp4") || filename.endsWith(".mkv"))
        {
            boost::shared_ptr<Content> c(new MovieContent(filename.toStdString()));

            g_displayGroup->addContent(c);
        }
        else
        {
            QMessageBox messageBox;
            messageBox.setText("Unsupported file format.");
            messageBox.exec();
        }
    }
}

void MainWindow::refreshContentsList()
{
    // clear contents list widget
    contentsListWidget_->clear();

    std::vector<boost::shared_ptr<Content> > contents = g_displayGroup->getContents();

    for(unsigned int i=0; i<contents.size(); i++)
    {
        // add to list view
        QListWidgetItem * newItem = new QListWidgetItem(contentsListWidget_);
        newItem->setText(contents[i]->getURI().c_str());
    }
}

void MainWindow::shareDesktop(bool set)
{
    if(set == true)
    {
        // get width and height of area at top-left corner of desktop to share
        shareDesktopWidth_ = QInputDialog::getInt(this, "Width", "Width");
        shareDesktopHeight_ = QInputDialog::getInt(this, "Height", "Height");

        // add the content object
        boost::shared_ptr<Content> c(new PixelStreamContent("desktop"));
        g_displayGroup->addContent(c);

        // setup timer to repeatedly update the shared desktop image
        connect(&shareDesktopUpdateTimer_, SIGNAL(timeout()), this, SLOT(shareDesktopUpdate()));
        shareDesktopUpdateTimer_.start(SHARE_DESKTOP_UPDATE_DELAY);
    }
    else
    {
        shareDesktopUpdateTimer_.stop();
        g_displayGroup->removeContent("desktop");
    }

    g_displayGroup->sendDisplayGroup();
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
