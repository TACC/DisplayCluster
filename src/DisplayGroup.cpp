#include "DisplayGroup.h"
#include "DisplayGroupGraphicsView.h"
#include "Content.h"
#include "main.h"
#include "log.h"
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

void DisplayGroup::addContent(boost::shared_ptr<Content> content)
{
    contents_.push_back(content);

    // set display group in content object
    content->setDisplayGroup(shared_from_this());

    getGraphicsView()->scene()->addItem((QGraphicsRectItem *)content->getGraphicsItem().get());
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
}

void DisplayGroup::synchronizeContents()
{
    if(g_mpiRank == 0)
    {
        // rate limit
        if(timer_.elapsed() < 1000/24)
        {
            return;
        }
        else
        {
            timer_.restart();
        }

        // serialize contents
        std::ostringstream oss(std::ostringstream::binary);

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(oss);
            oa << contents_;
        }

        // serialized data to string
        std::string serializedString = oss.str();
        int size = serializedString.size();

        // send the size and the message

        // the size is sent via a send, so that we can probe it on the render processes
        for(int i=1; i<g_mpiSize; i++)
        {
            MPI_Send((void *)&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // broadcast the message
        MPI_Bcast((void *)serializedString.data(), serializedString.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        // receive serialized data

        // check to see if we have a message (non-blocking)
        int flag;
        MPI_Status status;
        MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);

        // check to see if all render processes have a message
        int allFlag;
        MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);

        // buffer size and buffer
        int bufSize = 0;
        char * buf = NULL;

        // if all render processes have a message...
        if(allFlag != 0)
        {
            // continue receiving messages until we get to the last one which all render processes have
            // this will "drop frames" and keep all processes synchronized
            while(allFlag)
            {
                // first, get buffer size
                MPI_Recv((void *)&bufSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

                // allocate buffer
                if(buf != NULL)
                {
                    delete [] buf;
                }

                buf = new char[bufSize];

                // read message into the buffer
                MPI_Bcast((void *)buf, bufSize, MPI_BYTE, 0, MPI_COMM_WORLD);

                // check to see if we have another message waiting, for this process and for all render processes
                MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
                MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, g_mpiRenderComm);
            }

            // at this point, we've received the last message available for all render processes
            // de-serialize...
            std::istringstream iss(std::istringstream::binary);

            if(iss.rdbuf()->pubsetbuf(buf, bufSize) == NULL)
            {
                put_flog(LOG_FATAL, "rank %i: error setting stream buffer", g_mpiRank);
                exit(-1);
            }

            // read to a new vector
            std::vector<boost::shared_ptr<Content> > newContents;

            boost::archive::binary_iarchive ia(iss);
            ia >> newContents;

            // overwrite old contents
            contents_ = newContents;

            // free mpi buffer
            delete [] buf;
        }
    }
}
