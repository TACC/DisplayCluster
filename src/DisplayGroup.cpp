#include "DisplayGroup.h"
#include "DisplayGroupGraphicsView.h"
#include "Content.h"
#include "main.h"
#include "log.h"
#include "PixelStream.h"
#include "PixelStreamSource.h"
#include <sstream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#ifdef __APPLE__
    #include <mpi.h>
#else
    #include <mpi/mpi.h>
#endif

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

void DisplayGroup::addContent(boost::shared_ptr<Content> content)
{
    contents_.push_back(content);

    // set display group in content object
    content->setDisplayGroup(shared_from_this());

    getGraphicsView()->scene()->addItem((QGraphicsRectItem *)content->getGraphicsItem().get());

    g_mainWindow->refreshContentsList();
}

void DisplayGroup::removeContent(boost::shared_ptr<Content> content)
{
    // find vector entry for content
    std::vector<boost::shared_ptr<Content> >::iterator it;

    it = find(contents_.begin(), contents_.end(), content);

    if(it != contents_.end())
    {
        // we found the entry
        // now, remove it
        contents_.erase(it);
    }

    // set null display group in content object
    content->setDisplayGroup(boost::shared_ptr<DisplayGroup>());

    // remove from scene
    getGraphicsView()->scene()->removeItem((QGraphicsRectItem *)content->getGraphicsItem().get());

    g_mainWindow->refreshContentsList();
}

void DisplayGroup::removeContent(std::string uri)
{
    for(unsigned int i=0; i<contents_.size(); i++)
    {
        if(contents_[i]->getURI() == uri)
        {
            removeContent(contents_[i]);
            return;
        }
    }
}

bool DisplayGroup::hasContent(std::string uri)
{
    for(unsigned int i=0; i<contents_.size(); i++)
    {
        if(contents_[i]->getURI() == uri)
        {
            return true;
        }
    }

    return false;
}

std::vector<boost::shared_ptr<Content> > DisplayGroup::getContents()
{
    return contents_;
}

void DisplayGroup::moveContentToFront(boost::shared_ptr<Content> content)
{
    // find vector entry for content
    std::vector<boost::shared_ptr<Content> >::iterator it;

    it = find(contents_.begin(), contents_.end(), content);

    if(it != contents_.end())
    {
        // we found the entry
        // now, move it to the front
        contents_.erase(it);
        contents_.insert(contents_.begin(), content);
    }

    g_mainWindow->refreshContentsList();
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
        boost::archive::binary_oarchive oa(oss);
        oa << g_displayGroup;
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
