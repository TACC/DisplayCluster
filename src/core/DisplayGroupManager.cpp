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

#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "ContentFactory.h"
#include "Content.h"
#include "globals.h"
#include "log.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "MessageHeader.h"
#include "PixelStream.h"

#include <sstream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/algorithm/string.hpp>
#include <mpi.h>
#include <fstream>
#include <QSvgRenderer>


DisplayGroupManager::DisplayGroupManager()
    : options_(new Options())
{
    // make Options trigger sendDisplayGroup() when it is updated
    connect(options_.get(), SIGNAL(updated()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    // register types for use in signals/slots
    qRegisterMetaType<Event>("Event");
    qRegisterMetaType<ContentWindowManagerPtr>("ContentWindowManagerPtr");
    qRegisterMetaType<PixelStreamSegment>("PixelStreamSegment");

#if ENABLE_SKELETON_SUPPORT
    qRegisterMetaType<std::vector< boost::shared_ptr<SkeletonState> > >("std::vector< boost::shared_ptr<SkeletonState> >");
#endif
}

DisplayGroupManager::~DisplayGroupManager()
{
}

OptionsPtr DisplayGroupManager::getOptions() const
{
    return options_;
}

MarkerPtr DisplayGroupManager::getNewMarker()
{
    QMutexLocker locker(&markersMutex_);

    MarkerPtr marker(new Marker());
    markers_.push_back(marker);

    // the marker needs to be owned by the main thread for queued connections to work properly
    marker->moveToThread(QApplication::instance()->thread());

    // make marker trigger sendDisplayGroup() when it is updated
    connect(marker.get(), SIGNAL(positionChanged()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    return marker;
}

const MarkerPtrs& DisplayGroupManager::getMarkers() const
{
    return markers_;
}

void DisplayGroupManager::deleteMarkers()
{
    QMutexLocker locker(&markersMutex_);

    if( markers_.empty( ))
        return;

    markers_[0]->releaseTexture();
    markers_.clear();
}

boost::posix_time::ptime DisplayGroupManager::getTimestamp() const
{
    // rank 0 will return a timestamp calibrated to rank 1's clock
    if(g_mpiRank == 0)
    {
        return boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time() + timestampOffset_);
    }
    else
    {
        return timestamp_;
    }
}

#if ENABLE_SKELETON_SUPPORT
std::vector<boost::shared_ptr<SkeletonState> > DisplayGroupManager::getSkeletons()
{
    return skeletons_;
}
#endif

void DisplayGroupManager::addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // set display group in content window manager object
        contentWindowManager->setDisplayGroupManager(shared_from_this());

        sendDisplayGroup();

        if (contentWindowManager->getContent()->getType() != CONTENT_TYPE_PIXEL_STREAM)
        {
            // TODO initialize all content dimensions on creation so we can remove this procedure
            // make sure we have its dimensions so we can constrain its aspect ratio
            sendContentsDimensionsRequest();
        }
    }
}

void DisplayGroupManager::removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // Notify the (local) pixel stream source of the deletion of the window so the source can be removed too
        if (contentWindowManager->getContent()->getType() == CONTENT_TYPE_PIXEL_STREAM)
        {
            const QString& uri = contentWindowManager->getContent()->getURI();
            closePixelStream(uri);
            emit(pixelStreamViewClosed(uri));
        }

        // set null display group in content window manager object
        contentWindowManager->setDisplayGroupManager(DisplayGroupManagerPtr());

        sendDisplayGroup();
    }
}

void DisplayGroupManager::moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        sendDisplayGroup();
    }
}

void DisplayGroupManager::calibrateTimestampOffset()
{
    // can't calibrate timestamps unless we have at least 2 processes
    if(g_mpiSize < 2)
    {
        put_flog(LOG_DEBUG, "cannot calibrate with g_mpiSize == %i", g_mpiSize);
        return;
    }

    // synchronize all processes
    MPI_Barrier(g_mpiRenderComm);

    // get current timestamp immediately after
    boost::posix_time::ptime timestamp(boost::posix_time::microsec_clock::universal_time());

    // rank 1: send timestamp to rank 0
    if(g_mpiRank == 1)
    {
        // serialize state
        std::ostringstream oss(std::ostringstream::binary);

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(oss);
            oa << timestamp;
        }

        // serialized data to string
        std::string serializedString = oss.str();
        int size = serializedString.size();

        // send the header and the message
        MessageHeader mh;
        mh.size = size;

        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        MPI_Send((void *)serializedString.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
    // rank 0: receive timestamp from rank 1
    else if(g_mpiRank == 0)
    {
        MessageHeader messageHeader;

        MPI_Status status;
        MPI_Recv((void *)&messageHeader, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

        // receive serialized data
        char * buf = new char[messageHeader.size];

        // read message into the buffer
        MPI_Recv((void *)buf, messageHeader.size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

        // de-serialize...
        std::istringstream iss(std::istringstream::binary);

        if(iss.rdbuf()->pubsetbuf(buf, messageHeader.size) == NULL)
        {
            put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
            exit(-1);
        }

        // read to a new timestamp
        boost::posix_time::ptime rank1Timestamp;

        boost::archive::binary_iarchive ia(iss);
        ia >> rank1Timestamp;

        // free mpi buffer
        delete [] buf;

        // now, calculate and store the timestamp offset
        timestampOffset_ = rank1Timestamp - timestamp;

        put_flog(LOG_DEBUG, "timestamp offset = %s", (boost::posix_time::to_simple_string(timestampOffset_)).c_str());
    }
}

void DisplayGroupManager::setBackgroundContentWindowManager(ContentWindowManagerPtr contentWindowManager)
{
    // This method can be used to remove the background by sending a NULL ptr
    if (contentWindowManager != NULL)
    {
        // set display group in content window manager object
        contentWindowManager->setDisplayGroupManager(shared_from_this());
        contentWindowManager->adjustSize( SIZE_FULLSCREEN );
    }

    backgroundContent_ = contentWindowManager;

    sendDisplayGroup();
}

ContentWindowManagerPtr DisplayGroupManager::getBackgroundContentWindowManager() const
{
    return backgroundContent_;
}

bool DisplayGroupManager::isEmpty() const
{
    return contentWindowManagers_.empty();
}

ContentWindowManagerPtr DisplayGroupManager::getActiveWindow() const
{
    if (isEmpty())
        return ContentWindowManagerPtr();

    return contentWindowManagers_.back();
}

bool DisplayGroupManager::setBackgroundContentFromUri(const QString filename)
{
    if(!filename.isEmpty())
    {
        ContentPtr content = ContentFactory::getContent(filename);

        if( content )
        {
            ContentWindowManagerPtr contentWindow(new ContentWindowManager(content));
            setBackgroundContentWindowManager(contentWindow);
            return true;
        }
    }
    return false;
}

QColor DisplayGroupManager::getBackgroundColor() const
{
    return backgroundColor_;
}

void DisplayGroupManager::setBackgroundColor(QColor color)
{
    if(color == backgroundColor_)
        return;
    backgroundColor_ = color;
    sendDisplayGroup();
}

boost::shared_ptr<DisplayGroupInterface> DisplayGroupManager::getDisplayGroupInterface(QThread * thread)
{
    boost::shared_ptr<DisplayGroupInterface> dgi(new DisplayGroupInterface(shared_from_this()));

    // push it to the other thread
    dgi.get()->moveToThread(thread);

    return dgi;
}

void DisplayGroupManager::positionWindow(const QString uri, const QPointF position)
{
    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri, CONTENT_TYPE_ANY);
    if (contentWindow)
    {
        contentWindow->centerPositionAround(position, true);
    }
    else
    {
        // Store position for use when window actually opens
        windowPositions_[uri] = position;
    }
}

void DisplayGroupManager::hideWindow(const QString uri)
{
    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri, CONTENT_TYPE_ANY);
    if (contentWindow)
    {
        double x, y;
        contentWindow->getSize(x, y);
        contentWindow->setPosition(0,-2*y);
    }
}

void DisplayGroupManager::receiveMessages()
{
    if(g_mpiRank == 0)
    {
        put_flog(LOG_FATAL, "called on rank 0");
        exit(-1);
    }

    // check to see if we have a message (non-blocking)
    int flag;
    MPI_Status status;
    MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);

    // check to see if all render processes have a message
    int allFlag;
    MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);

    // message header
    MessageHeader mh;

    // if all render processes have a message...
    if(allFlag != 0)
    {
        // continue receiving messages until we get to the last one which all render processes have
        // this will "drop frames" and keep all processes synchronized
        while(allFlag)
        {
            // first, get message header
            MPI_Recv((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);

            if(mh.type == MESSAGE_TYPE_CONTENTS)
            {
                receiveDisplayGroup(mh);
            }
            else if(mh.type == MESSAGE_TYPE_CONTENTS_DIMENSIONS)
            {
                receiveContentsDimensionsRequest(mh);
            }
            else if(mh.type == MESSAGE_TYPE_PIXELSTREAM)
            {
                receivePixelStreams(mh);
            }
            else if(mh.type == MESSAGE_TYPE_QUIT)
            {
                QApplication::instance()->quit();
                return;
            }

            // check to see if we have another message waiting, for this process and for all render processes
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
            MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);
        }

        // at this point, we've received the last message available for all render processes
    }
}

void DisplayGroupManager::sendDisplayGroup()
{
    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        QMutexLocker locker(&markersMutex_);

        DisplayGroupManagerPtr dgm = shared_from_this();

        boost::archive::binary_oarchive oa(oss);
        oa << dgm;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_CONTENTS;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // broadcast the message
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void DisplayGroupManager::sendContentsDimensionsRequest()
{
    if(g_mpiSize < 2)
    {
        put_flog(LOG_WARN, "cannot get contents dimension update for g_mpiSize == %i", g_mpiSize);
        return;
    }

    // send the header and the message
    MessageHeader mh;
    mh.type = MESSAGE_TYPE_CONTENTS_DIMENSIONS;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // now, receive response from rank 1
    MPI_Status status;
    MPI_Recv((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    // receive serialized data
    char * buf = new char[mh.size];

    // read message into the buffer
    MPI_Recv((void *)buf, mh.size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buf, mh.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
        exit(-1);
    }

    // read to a new vector
    std::vector<std::pair<int, int> > dimensions;

    boost::archive::binary_iarchive ia(iss);
    ia >> dimensions;

    // overwrite old dimensions
    for(unsigned int i=0; i<dimensions.size() && i<contentWindowManagers_.size(); i++)
    {
        contentWindowManagers_[i]->getContent()->setDimensions(dimensions[i].first, dimensions[i].second);
    }

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::adjustPixelStreamContentDimensions(QString uri, int width, int height, bool changeViewSize)
{
    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);
    if(contentWindow)
    {
        // check for updated dimensions
        ContentPtr c = contentWindow->getContent();

        int oldWidth, oldHeight;
        c->getDimensions(oldWidth, oldHeight);

        if(width != oldWidth || height != oldHeight)
        {
            c->setDimensions(width, height);
            if (changeViewSize)
            {
                contentWindow->adjustSize( SIZE_1TO1 );
            }
        }
    }
}

void DisplayGroupManager::registerEventReceiver(QString uri, bool exclusive, EventReceiver* receiver)
{
    bool success = false;

    // Try to register with the ContentWindowManager corresponding to this stream
    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri);

    if(contentWindow)
    {
        put_flog(LOG_DEBUG, "found window: '%s'", uri.toStdString().c_str());

        // If a receiver is already registered, don't register this one if exclusive was requested
        if( !exclusive || !contentWindow->hasEventReceivers() )
        {
            success = contentWindow->registerEventReceiver( receiver );

            if (success)
                contentWindow->setWindowState(ContentWindowInterface::SELECTED);
        }
    }
    else
    {
        put_flog(LOG_DEBUG, "could not find window: '%s'", uri.toStdString().c_str());
    }

    emit eventRegistrationReply(uri, success);
}

void DisplayGroupManager::sendFrameClockUpdate()
{
    // this should only be called by the rank 1 process
    if(g_mpiRank != 1)
    {
        put_flog(LOG_WARN, "called by rank %i != 1", g_mpiRank);
        return;
    }

    boost::posix_time::ptime timestamp(boost::posix_time::microsec_clock::universal_time());

    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::archive::binary_oarchive oa(oss);
        oa << timestamp;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_FRAME_CLOCK;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=2; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // broadcast it
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, g_mpiRenderComm);

    // update timestamp
    timestamp_ = timestamp;
}

void DisplayGroupManager::receiveFrameClockUpdate()
{
    // we shouldn't run the broadcast if we're rank 1
    if(g_mpiRank == 1)
    {
        return;
    }

    // receive the message header
    MessageHeader messageHeader;
    MPI_Status status;
    MPI_Recv((void *)&messageHeader, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    if(messageHeader.type != MESSAGE_TYPE_FRAME_CLOCK)
    {
        put_flog(LOG_FATAL, "unexpected message type");
        exit(-1);
    }

    // receive serialized data
    char * buf = new char[messageHeader.size];

    // read message into the buffer
    MPI_Bcast((void *)buf, messageHeader.size, MPI_BYTE, 0, g_mpiRenderComm);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buf, messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
        exit(-1);
    }

    boost::archive::binary_iarchive ia(iss);
    ia >> timestamp_;

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::sendQuit()
{
    // send the header and the message
    MessageHeader mh;
    mh.type = MESSAGE_TYPE_QUIT;

    // will send EVT_CLOSE through Event
    ContentWindowManagerPtrs contentWindowManagers;
    setContentWindowManagers( contentWindowManagers );

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }
}

void DisplayGroupManager::advanceContents()
{
    // note that if we have multiple ContentWindowManagers corresponding to a single Content object,
    // we will call advance() multiple times per frame on that Content object...
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        contentWindowManagers_[i]->getContent()->advance(contentWindowManagers_[i]);
    }
    if (backgroundContent_)
    {
        backgroundContent_->getContent()->advance(backgroundContent_);
    }
}

void DisplayGroupManager::openPixelStream(QString uri, int width, int height)
{
    // add a Content/ContentWindowManager for this URI
    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);

    if(!contentWindow)
    {
        put_flog(LOG_DEBUG, "adding pixel stream: %s", uri.toLocal8Bit().constData());

        ContentPtr content = ContentFactory::getPixelStreamContent(uri);
        content->setDimensions(width, height);
        contentWindow = ContentWindowManagerPtr(new ContentWindowManager(content));

        // Position window if needed
        WindowPositions::iterator it = windowPositions_.find(uri);
        if (it != windowPositions_.end())
        {
            contentWindow->centerPositionAround(it->second, true);
            windowPositions_.erase(it);
        }
        addContentWindowManager(contentWindow);

        emit(pixelStreamViewAdded(uri));
    }
}

void DisplayGroupManager::closePixelStream(const QString& uri)
{
    put_flog(LOG_DEBUG, "deleting pixel stream: %s", uri.toLocal8Bit().constData());

    ContentWindowManagerPtr contentWindow = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);
    if( contentWindow )
        removeContentWindowManager( contentWindow );
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::setSkeletons(std::vector< boost::shared_ptr<SkeletonState> > skeletons)
{
    skeletons_ = skeletons;

    sendDisplayGroup();
}
#endif

void DisplayGroupManager::receiveDisplayGroup(const MessageHeader& messageHeader)
{
    // receive serialized data
    char * buf = new char[messageHeader.size];

    // read message into the buffer
    MPI_Bcast((void *)buf, messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buf, messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
        exit(-1);
    }

    boost::archive::binary_iarchive ia(iss);
    ia >> g_displayGroupManager;

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::receiveContentsDimensionsRequest(const MessageHeader& messageHeader)
{
    if(g_mpiRank == 1)
    {
        // get dimensions of Content objects associated with each ContentWindowManager
        // note that we must use g_displayGroupManager to access content window managers since earlier updates (in the same frame)
        // of this display group may have occurred, and g_displayGroupManager would have then been replaced
        std::vector<std::pair<int, int> > dimensions;

        for(unsigned int i=0; i<g_displayGroupManager->contentWindowManagers_.size(); i++)
        {
            int w,h;
            g_displayGroupManager->contentWindowManagers_[i]->getContent()->getFactoryObjectDimensions(w, h);

            dimensions.push_back(std::pair<int,int>(w,h));
        }

        // serialize
        std::ostringstream oss(std::ostringstream::binary);

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(oss);
            oa << dimensions;
        }

        // serialized data to string
        std::string serializedString = oss.str();
        int size = serializedString.size();

        // send the header and the message
        MessageHeader mh;
        mh.size = size;
        mh.type = MESSAGE_TYPE_CONTENTS_DIMENSIONS;

        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        MPI_Send((void *)serializedString.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
}

void DisplayGroupManager::receivePixelStreams(const MessageHeader& messageHeader)
{
    // receive serialized data
    char * buf = new char[messageHeader.size];

    // read message into the buffer
    MPI_Bcast((void *)buf, messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // URI
    const QString uri(messageHeader.uri);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buf, messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
        exit(-1);
    }

    // read to a new segments vector
    std::vector<PixelStreamSegment> segments;

    boost::archive::binary_iarchive ia(iss);
    ia >> segments;

    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(uri)->insertNewFrame(segments);

    // free mpi buffer
    delete [] buf;
}
