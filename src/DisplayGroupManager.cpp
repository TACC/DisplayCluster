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
#include "Content.h"
#include "log.h"
#include "PixelStream.h"
#include "PixelStreamSource.h"
#include "PixelStreamContent.h"
#include "ParallelPixelStreamContent.h"
#include "SVGStreamSource.h"
#include "SVGContent.h"
#include "MainWindow.h"
#include <sstream>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/algorithm/string.hpp>
#include <mpi.h>
#include <fstream>
#include <cstring>
#include <pthread.h>

#include "XmlState.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int 
ptid()
{
	return (((long long)pthread_self()) & 0xfffffff);
}


DisplayGroupManager::DisplayGroupManager()
{
		synchronization_suspended = false;

    // create new Options object
    boost::shared_ptr<Options> options(new Options());
    options_ = options;

    // make Options trigger sendDisplayGroup() when it is updated
    connect(options_.get(), SIGNAL(updated()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    // register types for use in signals/slots
    qRegisterMetaType<boost::shared_ptr<ContentWindowManager> >("boost::shared_ptr<ContentWindowManager>");

    // serialization support for the vector of skeleton states
#if ENABLE_SKELETON_SUPPORT
    qRegisterMetaType<std::vector< boost::shared_ptr<SkeletonState> > >("std::vector< boost::shared_ptr<SkeletonState> >");
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
        boost::posix_time::ptime xx = *timestamp_;
        std::string stime2 = boost::posix_time::to_simple_string(xx);

        return timestamp_;
    }
}

#if ENABLE_SKELETON_SUPPORT
std::vector<boost::shared_ptr<SkeletonState> > DisplayGroupManager::getSkeletons()
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

bool DisplayGroupManager::saveStateXML(QString& xml)
{
    XmlState s(this);
    s.Write(xml);
		return true;
}

bool DisplayGroupManager::saveStateXMLFile(std::string filename)
{
    XmlState s(this);
    s.Write(filename);
		return true;
}

bool DisplayGroupManager::loadStateXML(QString xml)
{
    XmlState s(xml);

    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers;

    for (auto xcw : s.contentWindows)
    {
        if(xcw->uri.empty() == false)
        {
            boost::shared_ptr<Content> c = Content::getContent(xcw->uri);

            if(c != NULL)
            {
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                contentWindowManagers.push_back(cwm);

                cwm->setPosition(xcw->x, xcw->y);
                cwm->setSize(xcw->w, xcw->h);
                cwm->setZoom(xcw->zoom);
                cwm->setCenter(xcw->centerX, xcw->centerY);
                cwm->setSelected(xcw->selected);
            }
        }
    }

    if(contentWindowManagers.size() > 0)
    {
        // assign new contents vector to display group
        setContentWindowManagers(contentWindowManagers);
    }
    else
    {
        put_flog(LOG_WARN, "no content windows specified in the state file");
    }

    return true;
}

bool DisplayGroupManager::loadStateXMLFile(std::string filename)
{
		QFile file(filename.c_str());
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
				std::cerr << "error\n";
				exit(1);
		}

		QByteArray barray = file.readAll();
		QString str(barray);

		loadStateXML(str);

    return true;
}

void DisplayGroupManager::sendMessage(MESSAGE_TYPE type, std::string uri, char *bytes, int size)
{
    MessageHeader mh;

    mh.size = size;
    mh.type = type;

    size_t len = uri.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
    mh.uri[len] = '\0';

    mh.time = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time());
    timestamp_ = boost::shared_ptr<boost::posix_time::ptime>(new boost::posix_time::ptime(mh.time));
    timestampOffset_ = boost::posix_time::microsec_clock::universal_time() - mh.time;

    MPI_Bcast(&mh, sizeof(MessageHeader), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (size > 0)
        MPI_Bcast(bytes, size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

static void received_message() {}

void DisplayGroupManager::receiveMessage()
{
    MessageHeader mh;

    MPI_Bcast(&mh, sizeof(MessageHeader), MPI_BYTE, 0, MPI_COMM_WORLD);
    received_message();

    g_mainWindow->Lock();

    std::string stime1 = boost::posix_time::to_simple_string(mh.time);

    timestamp_ = boost::shared_ptr<boost::posix_time::ptime>(new boost::posix_time::ptime(mh.time));

    boost::posix_time::ptime xx = *timestamp_;

    std::string stime2 = boost::posix_time::to_simple_string(xx);

    timestampOffset_ = boost::posix_time::microsec_clock::universal_time() - mh.time;

    put_flog(999, "timestamp %s", boost::posix_time::to_simple_string(*timestamp_.get()));

    char *bytes = NULL;
    if (mh.size > 0) 
    {
        bytes = new char[mh.size];
        MPI_Bcast((void *)bytes, mh.size, MPI_BYTE, 0, MPI_COMM_WORLD);
    }

    if(mh.type == MESSAGE_TYPE_CONTENTS)
    {
        receiveDisplayGroup(mh, bytes);
    }
    else if(mh.type == MESSAGE_TYPE_CONTENTS_DIMENSIONS)
    {
        receiveContentsDimensionsRequest(mh, bytes);
    }
    else if(mh.type == MESSAGE_TYPE_PIXELSTREAM)
    {
        receivePixelStreams(mh, bytes);
    }
    else if(mh.type == MESSAGE_TYPE_PARALLEL_PIXELSTREAM)
    {
        receiveParallelPixelStreams(mh, bytes);
    }
    else if(mh.type == MESSAGE_TYPE_PING)
    {
    }
    else if(mh.type == MESSAGE_TYPE_QUIT)
    {
        g_app->quit();
        return;
    }

    g_mainWindow->Unlock();

    if (bytes) delete[] bytes;
}

void DisplayGroupManager::sendDisplayGroup()
{
		if (synchronization_suspended)
			return;

// std::cerr << "SDG going for lock " << ptid() << "\n";
		pthread_mutex_lock(&lock);
// std::cerr << "SDG got lock " << ptid() << "\n";

    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        QMutexLocker locker(&markersMutex_);

        boost::shared_ptr<DisplayGroupManager> dgm = shared_from_this();

        boost::archive::binary_oarchive oa(oss);
        oa << dgm;
    }

    // serialized data to string
    std::string serializedString = oss.str();

    sendMessage(MESSAGE_TYPE_CONTENTS, "", serializedString.data(), serializedString.size());

		pthread_mutex_unlock(&lock);
}

void DisplayGroupManager::sendContentsDimensionsRequest()
{
    if(g_mpiSize < 2)
    {
        put_flog(LOG_WARN, "cannot get contents dimension update for g_mpiSize == %i", g_mpiSize);
        return;
    }

    sendMessage(MESSAGE_TYPE_CONTENTS_DIMENSIONS, "", NULL, 0);

    int size; MPI_Status status;

    MPI_Recv((void *)&size, sizeof(int), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    char *buf = new char[size];
    MPI_Recv((void *)buf, size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    // de-serialize...

		boost::iostreams::basic_array_source<char> device(buf, size);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > iss(device);

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
            if(getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM) == NULL)
            {
                put_flog(LOG_DEBUG, "adding pixel stream: %s", uri.c_str());

                boost::shared_ptr<Content> c(new PixelStreamContent(uri));
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                addContentWindowManager(cwm);
            }

            sendMessage(MESSAGE_TYPE_PIXELSTREAM, uri, imageData.data(), imageData.size());
        }

        // check for updated dimensions
        int pixelStreamWidth, pixelStreamHeight;

        pixelStreamSource->getDimensions(pixelStreamWidth, pixelStreamHeight, updated);

        if(updated == true)
        {
            boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);

            if(cwm != NULL)
            {
                boost::shared_ptr<Content> c = cwm->getContent();

                c->setDimensions(pixelStreamWidth, pixelStreamHeight);
            }
        }
    }
}

void DisplayGroupManager::sendParallelPixelStreams()
{
    // iterate through all parallel pixel streams and send updates if needed
    std::map<std::string, boost::shared_ptr<ParallelPixelStream> > map = g_parallelPixelStreamSourceFactory.getMap();

    for(std::map<std::string, boost::shared_ptr<ParallelPixelStream> >::iterator it = map.begin(); it != map.end(); it++)
    {
        std::string uri = (*it).first;
        boost::shared_ptr<ParallelPixelStream> parallelPixelStreamSource = (*it).second;

        // get updated segments
        // if streaming synchronization is enabled, we need to send all segments; otherwise just the latest segments
        std::vector<ParallelPixelStreamSegment> segments;

        if(options_->getEnableStreamingSynchronization() == true)
        {
            segments = parallelPixelStreamSource->getAndPopAllSegments();
        }
        else
        {
            segments = parallelPixelStreamSource->getAndPopLatestSegments();
        }

        if(segments.size() > 0)
        {
            // make sure Content/ContentWindowManager exists for the URI

            // todo: this means as long as the parallel pixel stream is updating, we'll have a window for it
            // closing a window therefore will not terminate the parallel pixel stream
            if(getContentWindowManager(uri, CONTENT_TYPE_PARALLEL_PIXEL_STREAM) == NULL)
            {
                put_flog(LOG_DEBUG, "adding parallel pixel stream: %s", uri.c_str());

                boost::shared_ptr<Content> c(new ParallelPixelStreamContent(uri));
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                addContentWindowManager(cwm);
            }

            // serialize the vector
            std::ostringstream oss(std::ostringstream::binary);

            // brace this so destructor is called on archive before we use the stream
            {
                boost::archive::binary_oarchive oa(oss);
                oa << segments;
            }

            // serialized data to string
            std::string serializedString = oss.str();

            sendMessage(MESSAGE_TYPE_PARALLEL_PIXELSTREAM, uri, serializedString.data(), serializedString.size());

            // check for updated dimensions
            int newWidth = segments[0].parameters.totalWidth;
            int newHeight = segments[0].parameters.totalHeight;

            boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_PARALLEL_PIXEL_STREAM);

            if(cwm != NULL)
            {
                boost::shared_ptr<Content> c = cwm->getContent();

                int oldWidth, oldHeight;
                c->getDimensions(oldWidth, oldHeight);

                if(newWidth != oldWidth || newHeight != oldHeight)
                {
                    c->setDimensions(newWidth, newHeight);
                }
            }
        }
    }
}

void DisplayGroupManager::sendSVGStreams()
{
    // iterate through all SVG streams and send updates if needed
    std::map<std::string, boost::shared_ptr<SVGStreamSource> > map = g_SVGStreamSourceFactory.getMap();

    for(std::map<std::string, boost::shared_ptr<SVGStreamSource> >::iterator it = map.begin(); it != map.end(); it++)
    {
        std::string uri = (*it).first;
        boost::shared_ptr<SVGStreamSource> svgStreamSource = (*it).second;

        // get buffer
        bool updated;
        QByteArray imageData = svgStreamSource->getImageData(updated);

        if(updated == true)
        {
            // make sure Content/ContentWindowManager exists for the URI

            // todo: this means as long as the SVG stream is updating, we'll have a window for it
            // closing a window therefore will not terminate the SVG stream
            if(getContentWindowManager(uri, CONTENT_TYPE_SVG) == NULL)
            {
                put_flog(LOG_DEBUG, "adding SVG stream: %s", uri.c_str());

                boost::shared_ptr<Content> c(new SVGContent(uri));
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                addContentWindowManager(cwm);
            }

            // check for updated dimensions
            QSvgRenderer svgRenderer;

            if(svgRenderer.load(imageData) != true || svgRenderer.isValid() == false)
            {
                put_flog(LOG_ERROR, "error loading %s", uri.c_str());
                continue;
            }

            int newWidth = svgRenderer.defaultSize().width();
            int newHeight = svgRenderer.defaultSize().height();

            boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_SVG);

            if(cwm != NULL)
            {
                boost::shared_ptr<Content> c = cwm->getContent();

                int oldWidth, oldHeight;
                c->getDimensions(oldWidth, oldHeight);

                if(newWidth != oldWidth || newHeight != oldHeight)
                {
                    c->setDimensions(newWidth, newHeight);
                }
            }

            sendMessage(MESSAGE_TYPE_SVG_STREAM, uri, imageData.data(), imageData.size());
        }
    }
}

void DisplayGroupManager::sendQuit()
{
    sendMessage(MESSAGE_TYPE_QUIT, "", NULL, 0);
}

void DisplayGroupManager::advanceContents()
{
    // note that if we have multiple ContentWindowManagers corresponding to a single Content object,
    // we will call advance() multiple times per frame on that Content object...
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        contentWindowManagers_[i]->getContent()->advance(contentWindowManagers_[i]);
    }
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::setSkeletons(std::vector< boost::shared_ptr<SkeletonState> > skeletons)
{
    skeletons_ = skeletons;

    sendDisplayGroup();
}
#endif

void DisplayGroupManager::receiveDisplayGroup(MessageHeader messageHeader, char *bytes)
{
		pthread_mutex_lock(&lock);

    // de-serialize...

		boost::iostreams::basic_array_source<char> device(bytes, messageHeader.size);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > iss(device);

    // read to a new display group
    boost::shared_ptr<DisplayGroupManager> displayGroupManager;

    boost::archive::binary_iarchive ia(iss);
    ia >> displayGroupManager;

    // overwrite old display group
    displayGroupManager->timestamp_ = g_displayGroupManager->timestamp_;
    displayGroupManager->timestampOffset_ = g_displayGroupManager->timestampOffset_;
    g_displayGroupManager = displayGroupManager;
  

		pthread_mutex_unlock(&lock);
}

void DisplayGroupManager::receiveContentsDimensionsRequest(MessageHeader messageHeader, char *bytes)
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

        MPI_Send((void *)&size, sizeof(size), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        MPI_Send((void *)serializedString.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
}

void DisplayGroupManager::receivePixelStreams(MessageHeader messageHeader, char *bytes)
{
    // URI
    std::string uri = std::string(messageHeader.uri);

    // de-serialize...
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(uri)->setImageData(QByteArray(bytes, messageHeader.size));
}

void DisplayGroupManager::receiveParallelPixelStreams(MessageHeader messageHeader, char *bytes)
{
    // URI
    std::string uri = std::string(messageHeader.uri);

    // de-serialize...

		boost::iostreams::basic_array_source<char> device(bytes, messageHeader.size);
		boost::iostreams::stream<boost::iostreams::basic_array_source<char> > iss(device);

    // read to a new segments vector
    std::vector<ParallelPixelStreamSegment> segments;

    boost::archive::binary_iarchive ia(iss);
    ia >> segments;

    // now, insert all segments
    for(unsigned int i=0; i<segments.size(); i++)
    {
        g_mainWindow->getGLWindow()->getParallelPixelStreamFactory().getObject(uri)->insertSegment(segments[i]);
    }

    // update pixel streams corresponding to new segments
    g_mainWindow->getGLWindow()->getParallelPixelStreamFactory().getObject(uri)->updatePixelStreams();
}

void DisplayGroupManager::receiveSVGStreams(MessageHeader messageHeader, char *bytes)
{
    // URI
    std::string uri = std::string(messageHeader.uri);

    // de-serialize...
    g_mainWindow->getGLWindow()->getSVGFactory().getObject(uri)->setImageData(QByteArray(bytes, messageHeader.size));
}


void DisplayGroupManager::pushState()
{
	QString s;
	saveStateXML(s);
	state_stack.push(s);
}

void DisplayGroupManager::popState()
{
	loadStateXML(state_stack.top());
	state_stack.pop();
}

void DisplayGroupManager::suspendSynchronization()
{
	// std::cerr << "SUSPEND\n";
	if (synchronization_suspended)
		put_flog(LOG_DEBUG, "DisplayGroupManager::suspendSynchronization() while synchronization is suspended\n");
	else
		synchronization_suspended = true;
}
	

void DisplayGroupManager::resumeSynchronization()
{
	// std::cerr << "RESUME\n";
	if (! synchronization_suspended)
		put_flog(LOG_DEBUG, "DisplayGroupManager::resumeSynchronization() while synchronization is NOT suspended\n");
	else
	{
		synchronization_suspended = false;
		sendDisplayGroup();
	}
}
	
