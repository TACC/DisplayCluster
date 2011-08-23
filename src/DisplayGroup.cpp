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

DisplayGroup::DisplayGroup()
{
    // defaults
    graphicsView_ = NULL;
}

DisplayGroup::~DisplayGroup()
{
    delete graphicsView_;
}

DisplayGroupGraphicsView * DisplayGroup::getGraphicsView()
{
    if(graphicsView_ == NULL)
    {
        graphicsView_ = new DisplayGroupGraphicsView();
    }

    return graphicsView_;
}

void DisplayGroup::addContent(boost::shared_ptr<Content> content)
{
    contents_.push_back(content);

    getGraphicsView()->scene()->addItem((QGraphicsRectItem *)content->getGraphicsItem().get());
}

std::vector<boost::shared_ptr<Content> > DisplayGroup::getContents()
{
    return contents_;
}

void DisplayGroup::synchronizeContents()
{
    if(g_mpiRank == 0)
    {
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
        MPI_Bcast((void *)&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast((void *)serializedString.data(), serializedString.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        // receive serialized data

        // first, get size of data
        int count = 0;
        MPI_Bcast((void *)&count, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // we have a message

        // read it
        char * buf = (char *)malloc(count);

        if(buf == NULL)
        {
            put_flog(LOG_FATAL, "rank %i: error allocating memory", g_mpiRank);
            exit(-1);
        }

        MPI_Bcast((void *)buf, count, MPI_BYTE, 0, MPI_COMM_WORLD);

        std::istringstream iss(std::istringstream::binary);

        if(iss.rdbuf()->pubsetbuf(buf, count) == NULL)
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
        free(buf);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
}
