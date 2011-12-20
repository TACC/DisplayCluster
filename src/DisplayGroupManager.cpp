#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "Content.h"
#include "main.h"
#include "log.h"
#include "PixelStream.h"
#include "PixelStreamSource.h"
#include "PixelStreamContent.h"
#include <sstream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <mpi.h>

DisplayGroupManager::DisplayGroupManager()
{
    // create new Options object
    boost::shared_ptr<Options> options(new Options());
    options_ = options;

    // make Options trigger sendDisplayGroup() when it is updated
    connect(options_.get(), SIGNAL(updated()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    // register types for use in signals/slots
    qRegisterMetaType<boost::shared_ptr<ContentWindowManager> >("boost::shared_ptr<ContentWindowManager>");

    // serialization support for the vector of skeleton states
#if ENABLE_SKELETON_SUPPORT
    qRegisterMetaType<std::vector<SkeletonState> >("std::vector<SkeletonState>");
#endif
}

boost::shared_ptr<Options> DisplayGroupManager::getOptions()
{
    return options_;
}

boost::shared_ptr<Marker> DisplayGroupManager::getNewMarker()
{
    QMutexLocker locker(&markersMutex_);

    boost::shared_ptr<Marker> marker(new Marker());
    markers_.push_back(marker);

    // the marker needs to be owned by the main thread for queued connections to work properly
    marker->moveToThread(QApplication::instance()->thread());

    // make marker trigger sendDisplayGroup() when it is updated
    connect(marker.get(), SIGNAL(positionChanged()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    return marker;
}

std::vector<boost::shared_ptr<Marker> > DisplayGroupManager::getMarkers()
{
    return markers_;
}

boost::shared_ptr<boost::posix_time::ptime> DisplayGroupManager::getTimestamp()
{
    // rank 0 will return a timestamp calibrated to rank 1's clock
    if(g_mpiRank == 0)
    {
        boost::shared_ptr<boost::posix_time::ptime> timestamp(new boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time() + timestampOffset_));

        return timestamp;
    }
    else
    {
        return timestamp_;
    }
}

#if ENABLE_SKELETON_SUPPORT
std::vector<SkeletonState> DisplayGroupManager::getSkeletons()
{
    return skeletons_;
}
#endif

void DisplayGroupManager::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // set display group in content window manager object
        contentWindowManager->setDisplayGroupManager(shared_from_this());

        sendDisplayGroup();

        // make sure we have its dimensions so we can constrain its aspect ratio
        sendContentsDimensionsRequest();
    }
}

void DisplayGroupManager::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // set null display group in content window manager object
        contentWindowManager->setDisplayGroupManager(boost::shared_ptr<DisplayGroupManager>());

        sendDisplayGroup();
    }
}

void DisplayGroupManager::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
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
                g_app->quit();
                return;
            }
#if ENABLE_SKELETON_SUPPORT
            else if(mh.type == MESSAGE_TYPE_SKELETONS)
            {
                receiveSkeletons(mh);
            }
#endif

            // check to see if we have another message waiting, for this process and for all render processes
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
            MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);
        }

        // at this point, we've received the last message available for all render processes
    }
}

void DisplayGroupManager::sendDisplayGroup()
{

    QMutexLocker locker(&markersMutex_);

    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::shared_ptr<DisplayGroupManager> dgm = shared_from_this();

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

        if(g_mainWindow->getConstrainAspectRatio() == true)
        {
            contentWindowManagers_[i]->fixAspectRatio();
        }
    }

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::sendPixelStreams()
{
    // iterate through all pixel streams and send updates if needed
    std::map<std::string, boost::shared_ptr<PixelStreamSource> > map = g_pixelStreamSourceFactory.getMap();

    for(std::map<std::string, boost::shared_ptr<PixelStreamSource> >::iterator it = map.begin(); it != map.end(); it++)
    {
        std::string uri = (*it).first;
        boost::shared_ptr<PixelStreamSource> pixelStreamSource = (*it).second;

        // get buffer
        bool updated;
        QByteArray imageData = pixelStreamSource->getImageData(updated);

        if(updated == true)
        {
            // make sure Content/ContentWindowManager exists for the URI

            // todo: this means as long as the pixel stream is updating, we'll have a window for it
            // closing a window therefore will not terminate the pixel stream
            if(hasContent(uri) == false)
            {
                put_flog(LOG_DEBUG, "adding pixel stream: %s", uri.c_str());

                boost::shared_ptr<Content> c(new PixelStreamContent(uri));
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                addContentWindowManager(cwm);
            }

            int size = imageData.size();

            // send the header and the message
            MessageHeader mh;
            mh.size = size;
            mh.type = MESSAGE_TYPE_PIXELSTREAM;

            // add the truncated URI to the header
            size_t len = uri.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
            mh.uri[len] = '\0';

            // the header is sent via a send, so that we can probe it on the render processes
            for(int i=1; i<g_mpiSize; i++)
            {
                MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
            }

            // broadcast the message
            MPI_Bcast((void *)imageData.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
        }
    }
}

void DisplayGroupManager::sendFrameClockUpdate()
{
    // this should only be called by the rank 1 process
    if(g_mpiRank != 1)
    {
        put_flog(LOG_WARN, "called by rank %i != 1", g_mpiRank);
        return;
    }

    boost::shared_ptr<boost::posix_time::ptime> timestamp(new boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()));

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

    // read to a new timestamp
    boost::shared_ptr<boost::posix_time::ptime> timestamp;

    boost::archive::binary_iarchive ia(iss);
    ia >> timestamp;

    // free mpi buffer
    delete [] buf;

    // update timestamp
    timestamp_ = timestamp;
}

void DisplayGroupManager::sendQuit()
{
    // send the header and the message
    MessageHeader mh;
    mh.type = MESSAGE_TYPE_QUIT;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::setSkeletons(std::vector<SkeletonState> skeletons)
{
    skeletons_ = skeletons;

    sendDisplayGroup();
}
#endif

void DisplayGroupManager::advanceContents()
{
    // note that if we have multiple ContentWindowManagers corresponding to a single Content object,
    // we will call advance() multiple times per frame on that Content object...
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        contentWindowManagers_[i]->getContent()->advance(contentWindowManagers_[i]);
    }
}

void DisplayGroupManager::receiveDisplayGroup(MessageHeader messageHeader)
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

    // read to a new display group
    boost::shared_ptr<DisplayGroupManager> displayGroupManager;

    boost::archive::binary_iarchive ia(iss);
    ia >> displayGroupManager;

    // overwrite old display group
    g_displayGroupManager = displayGroupManager;

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::receiveContentsDimensionsRequest(MessageHeader messageHeader)
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

void DisplayGroupManager::receivePixelStreams(MessageHeader messageHeader)
{
    // receive serialized data
    char * buf = new char[messageHeader.size];

    // read message into the buffer
    MPI_Bcast((void *)buf, messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // URI
    std::string uri = std::string(messageHeader.uri);

    // de-serialize...
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(uri)->setImageData(QByteArray(buf, messageHeader.size));

    // free mpi buffer
    delete [] buf;
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::receiveSkeletons(MessageHeader messageHeader)
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

    // read to skeletons vector
    std::vector<SkeletonState> skeletons;

    boost::archive::binary_iarchive ia(iss);
    ia >> skeletons_;

    put_flog(LOG_DEBUG, "deserialized %i skeletons", skeletons_.size());

    // overwrite old skeletons vector
    ///////skeletons_ = skeletons;

    // free mpi buffer
    delete [] buf;
}
#endif
