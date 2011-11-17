#include "DisplayGroup.h"
#include "DisplayGroupGraphicsView.h"
#include "ContentWindow.h"
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

boost::shared_ptr<DisplayGroupGraphicsView> DisplayGroup::getGraphicsView()
{
    if(graphicsView_ == NULL)
    {
        boost::shared_ptr<DisplayGroupGraphicsView> dggv(new DisplayGroupGraphicsView());
        graphicsView_ = dggv;
    }

    return graphicsView_;
}

Marker & DisplayGroup::getMarker()
{
    return marker_;
}

void DisplayGroup::addContentWindow(boost::shared_ptr<ContentWindow> contentWindow)
{
    contentWindows_.push_back(contentWindow);

    // set display group in content window object
    contentWindow->setDisplayGroup(shared_from_this());

    // add the window to the display group and to the GUI scene
    sendDisplayGroup();
    getGraphicsView()->scene()->addItem((QGraphicsItem *)(contentWindow->getContentWindowGraphicsItem()));

    // make sure we have its dimensions so we can constrain its aspect ratio
    sendContentsDimensionsRequest();

    g_mainWindow->refreshContentsList();
}

void DisplayGroup::removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow)
{
    // find vector entry for content window
    std::vector<boost::shared_ptr<ContentWindow> >::iterator it;

    it = find(contentWindows_.begin(), contentWindows_.end(), contentWindow);

    if(it != contentWindows_.end())
    {
        // we found the entry
        // now, remove it
        contentWindows_.erase(it);
    }

    // set null display group in content window object
    contentWindow->setDisplayGroup(boost::shared_ptr<DisplayGroup>());

    g_mainWindow->refreshContentsList();
}

bool DisplayGroup::hasContent(std::string uri)
{
    for(unsigned int i=0; i<contentWindows_.size(); i++)
    {
        if(contentWindows_[i]->getContent()->getURI() == uri)
        {
            return true;
        }
    }

    return false;
}

void DisplayGroup::setContentWindows(std::vector<boost::shared_ptr<ContentWindow> > contentWindows)
{
    // clear existing content windows
    contentWindows_.clear();

    // add new content windows
    for(unsigned int i=0; i<contentWindows.size(); i++)
    {
        addContentWindow(contentWindows[i]);
    }

    g_mainWindow->refreshContentsList();

    sendDisplayGroup();
}

std::vector<boost::shared_ptr<ContentWindow> > DisplayGroup::getContentWindows()
{
    return contentWindows_;
}

void DisplayGroup::moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow)
{
    // find vector entry for content window
    std::vector<boost::shared_ptr<ContentWindow> >::iterator it;

    it = find(contentWindows_.begin(), contentWindows_.end(), contentWindow);

    if(it != contentWindows_.end())
    {
        // we found the entry
        // now, move it to end of the list (last item rendered is on top)
        contentWindows_.erase(it);
        contentWindows_.push_back(contentWindow);
    }

    g_mainWindow->refreshContentsList();

    sendDisplayGroup();
}

boost::shared_ptr<boost::posix_time::ptime> DisplayGroup::getTimestamp()
{
    return timestamp_;
}

void DisplayGroup::handleMessage(MessageHeader messageHeader, QByteArray byteArray)
{
    if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM)
    {
        // check to see if Content/ContentWindow exists for the URI
        std::string uri = std::string(messageHeader.uri);

        if(hasContent(uri) == false)
        {
            put_flog(LOG_DEBUG, "adding pixel stream: %s", uri.c_str());

            boost::shared_ptr<Content> c(new PixelStreamContent(uri));
            boost::shared_ptr<ContentWindow> cw(new ContentWindow(c));

            addContentWindow(cw);
        }

        // update pixel stream source
        g_pixelStreamSourceFactory.getObject(uri)->setImageData(byteArray);

        // send updated pixelstream
        sendPixelStreams();
    }
}

void DisplayGroup::synchronize()
{
    // rank 0: send out display group and pixel streams
    if(g_mpiRank == 0)
    {
        sendDisplayGroup();
        sendPixelStreams();
    }
    else
    {
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

                // check to see if we have another message waiting, for this process and for all render processes
                MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
                MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);
            }

            // at this point, we've received the last message available for all render processes
        }
    }
}

void DisplayGroup::sendDisplayGroup()
{
    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::shared_ptr<DisplayGroup> dg = shared_from_this();

        boost::archive::binary_oarchive oa(oss);
        oa << dg;
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

void DisplayGroup::sendContentsDimensionsRequest()
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
    for(unsigned int i=0; i<dimensions.size() && i<contentWindows_.size(); i++)
    {
        contentWindows_[i]->getContent()->setDimensions(dimensions[i].first, dimensions[i].second);

        if(g_mainWindow->getConstrainAspectRatio() == true)
        {
            contentWindows_[i]->fixAspectRatio();
        }
    }

    // free mpi buffer
    delete [] buf;
}

void DisplayGroup::sendPixelStreams()
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

void DisplayGroup::sendFrameClockUpdate()
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

void DisplayGroup::receiveFrameClockUpdate()
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

void DisplayGroup::sendQuit()
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

void DisplayGroup::advanceContents()
{
    // note that if we have multiple ContentWindows corresponding to a single Content object,
    // we will call advance() multiple times per frame on that Content object...
    for(unsigned int i=0; i<contentWindows_.size(); i++)
    {
        contentWindows_[i]->getContent()->advance(contentWindows_[i]);
    }
}

void DisplayGroup::receiveDisplayGroup(MessageHeader messageHeader)
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
    boost::shared_ptr<DisplayGroup> displayGroup;

    boost::archive::binary_iarchive ia(iss);
    ia >> displayGroup;

    // overwrite old display group
    g_displayGroup = displayGroup;

    // free mpi buffer
    delete [] buf;
}

void DisplayGroup::receiveContentsDimensionsRequest(MessageHeader messageHeader)
{
    if(g_mpiRank == 1)
    {
        // get dimensions of Content objects associated with each ContentWindow
        // note that we must use g_displayGroup to access content windows since earlier updates (in the same frame)
        // of this display group may have occurred, and g_displayGroup would have then been replaced
        std::vector<std::pair<int, int> > dimensions;

        for(unsigned int i=0; i<g_displayGroup->contentWindows_.size(); i++)
        {
            int w,h;
            g_displayGroup->contentWindows_[i]->getContent()->getFactoryObjectDimensions(w, h);

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

void DisplayGroup::receivePixelStreams(MessageHeader messageHeader)
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
