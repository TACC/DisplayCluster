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
#include "main.h"
#include "log.h"
#include "PixelStreamContent.h"
#include "SVGStreamSource.h"
#include "SVGContent.h"
#include <sstream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/algorithm/string.hpp>
#include <mpi.h>
#include <QDomDocument>
#include <fstream>

DisplayGroupManager::DisplayGroupManager()
{
    // create new Options object
    boost::shared_ptr<Options> options(new Options());
    options_ = options;

    // make Options trigger sendDisplayGroup() when it is updated
    connect(options_.get(), SIGNAL(updated()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    // register Interactionstate in Qt
    qRegisterMetaType<InteractionState>("InteractionState");

    // register types for use in signals/slots
    qRegisterMetaType<boost::shared_ptr<ContentWindowManager> >("boost::shared_ptr<ContentWindowManager>");

    // serialization support for the vector of skeleton states
#if ENABLE_SKELETON_SUPPORT
    qRegisterMetaType<std::vector< boost::shared_ptr<SkeletonState> > >("std::vector< boost::shared_ptr<SkeletonState> >");
#endif
    // For using PixelStreamSegment in QSignals
    qRegisterMetaType<PixelStreamSegment>("PixelStreamSegment");
}

boost::shared_ptr<Options> DisplayGroupManager::getOptions() const
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

const std::vector<boost::shared_ptr<Marker> >& DisplayGroupManager::getMarkers() const
{
    return markers_;
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

void DisplayGroupManager::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
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

void DisplayGroupManager::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // Notify the (local) pixel stream source of the deletion of the window so the source can be removed too
        if (contentWindowManager->getContent()->getType() == CONTENT_TYPE_PIXEL_STREAM)
        {
            QString uri = contentWindowManager->getContent()->getURI();
            deletePixelStream(uri);
            emit(pixelStreamViewClosed(uri));
        }

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

void DisplayGroupManager::setBackgroundContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager)
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

boost::shared_ptr<ContentWindowManager> DisplayGroupManager::getBackgroundContentWindowManager() const
{
    return backgroundContent_;
}

void DisplayGroupManager::initBackground()
{
    assert(g_configuration != NULL && "initBackground() needs a valid configuration file loaded");

    backgroundColor_ = g_configuration->getBackgroundColor();

    if(!g_configuration->getBackgroundUri().isEmpty())
    {
        boost::shared_ptr<Content> c = ContentFactory::getContent(g_configuration->getBackgroundUri());

        if(c != NULL)
        {
            boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));
            g_displayGroupManager->setBackgroundContentWindowManager(cwm);
        }
    }
    else
        sendDisplayGroup();
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

bool DisplayGroupManager::saveStateXMLFile(std::string filename)
{
    // get contents vector
    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers = getContentWindowManagers();

    // save as XML
    QDomDocument doc("state");
    QDomElement root = doc.createElement("state");
    doc.appendChild(root);

    // version number
    int version = CONTENTS_FILE_VERSION_NUMBER;

    QDomElement v = doc.createElement("version");
    v.appendChild(doc.createTextNode(QString::number(version)));
    root.appendChild(v);

    for(unsigned int i=0; i<contentWindowManagers.size(); i++)
    {
        // get values
        const QString& uri = contentWindowManagers[i]->getContent()->getURI();

        double x, y, w, h;
        contentWindowManagers[i]->getCoordinates(x, y, w, h);

        double centerX, centerY;
        contentWindowManagers[i]->getCenter(centerX, centerY);

        double zoom = contentWindowManagers[i]->getZoom();

        // add the XML node with these values
        QDomElement cwmNode = doc.createElement("ContentWindow");
        root.appendChild(cwmNode);

        QDomElement n = doc.createElement("URI");
        n.appendChild(doc.createTextNode(uri));
        cwmNode.appendChild(n);

        n = doc.createElement("x");
        n.appendChild(doc.createTextNode(QString::number(x)));
        cwmNode.appendChild(n);

        n = doc.createElement("y");
        n.appendChild(doc.createTextNode(QString::number(y)));
        cwmNode.appendChild(n);

        n = doc.createElement("w");
        n.appendChild(doc.createTextNode(QString::number(w)));
        cwmNode.appendChild(n);

        n = doc.createElement("h");
        n.appendChild(doc.createTextNode(QString::number(h)));
        cwmNode.appendChild(n);

        n = doc.createElement("centerX");
        n.appendChild(doc.createTextNode(QString::number(centerX)));
        cwmNode.appendChild(n);

        n = doc.createElement("centerY");
        n.appendChild(doc.createTextNode(QString::number(centerY)));
        cwmNode.appendChild(n);

        n = doc.createElement("zoom");
        n.appendChild(doc.createTextNode(QString::number(zoom)));
        cwmNode.appendChild(n);
    }

    QString xml = doc.toString();

    std::ofstream ofs(filename.c_str());

    if(ofs.good() == true)
    {
        ofs << xml.toStdString();
        return true;
    }
    else
    {
        put_flog(LOG_ERROR, "could not write state file");
        return false;
    }
}

bool DisplayGroupManager::loadStateXMLFile(std::string filename)
{
    QXmlQuery query;

    if(query.setFocus(QUrl(filename.c_str())) == false)
    {
        put_flog(LOG_ERROR, "failed to load %s", filename.c_str());
        return false;
    }

    // temp
    QString qstring;

#if 0
    // get version; we don't do anything with it now but may in the future
    int version = -1;
    query.setQuery("string(/state/version)");

    if(query.evaluateTo(&qstring) == true)
    {
        version = qstring.toInt();
    }
#endif

    // get number of content windows
    int numContentWindows = 0;
    query.setQuery("string(count(//state/ContentWindow))");

    if(query.evaluateTo(&qstring) == true)
    {
        numContentWindows = qstring.toInt();
    }

    put_flog(LOG_INFO, "%i content windows", numContentWindows);

    // new contents vector
    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers;

    for(int i=1; i<=numContentWindows; i++)
    {
        char string[1024];

        QString uri;
        sprintf(string, "string(//state/ContentWindow[%i]/URI)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            // remove any whitespace
            uri = qstring.trimmed();

            put_flog(LOG_DEBUG, "found content window with URI %s", uri.toLocal8Bit().constData());
        }

        if(uri.isEmpty())
            continue;

        double x, y, w, h, centerX, centerY, zoom;
        x = y = w = h = centerX = centerY = zoom = -1.;

        sprintf(string, "string(//state/ContentWindow[%i]/x)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            x = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/y)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            y = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/w)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            w = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/h)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            h = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/centerX)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            centerX = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/centerY)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            centerY = qstring.toDouble();
        }

        sprintf(string, "string(//state/ContentWindow[%i]/zoom)", i);
        query.setQuery(string);

        if(query.evaluateTo(&qstring) == true)
        {
            zoom = qstring.toDouble();
        }

        boost::shared_ptr<Content> c = ContentFactory::getContent(uri);

        if(c != NULL)
        {
            boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

            contentWindowManagers.push_back(cwm);

            // now, apply settings if we got them from the XML file
            if(x != -1. || y != -1.)
            {
                cwm->setPosition(x, y);
            }

            if(w != -1. || h != -1.)
            {
                cwm->setSize(w, h);
            }

            // zoom needs to be set before center because of clamping
            if(zoom != -1.)
            {
                cwm->setZoom(zoom);
            }

            if(centerX != -1. || centerY != -1.)
            {
                cwm->setCenter(centerX, centerY);
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
            else if(mh.type == MESSAGE_TYPE_SVG_STREAM)
            {
                receiveSVGStreams(mh);
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

void DisplayGroupManager::sendDisplayGroup()
{
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

void DisplayGroupManager::sendPixelStreams()
{
    // iterate through all pixel streams and send updates if needed
    std::map<QString, boost::shared_ptr<PixelStream> > map = pixelStreamSourceFactory_.getMap();

    for(std::map<QString, boost::shared_ptr<PixelStream> >::iterator it = map.begin(); it != map.end(); it++)
    {
        const QString& uri = (*it).first;
        boost::shared_ptr<PixelStream> pixelStreamSource = (*it).second;

        // get updated segments
        // if streaming synchronization is enabled, we need to send all segments; otherwise just the latest segments
        std::vector<PixelStreamSegment> segments;

        if(options_->getEnableStreamingSynchronization())
        {
            segments = pixelStreamSource->getAndPopAllSegments();
        }
        else
        {
            segments = pixelStreamSource->getAndPopLatestSegments();
        }

        if(segments.empty())
            continue;

        int width, height;
        pixelStreamSource->getDimensions(width, height);
        bool changeViewSize = pixelStreamSource->changeViewDimensionsRequested();

        adjustPixelStreamContentDimensions(uri, width, height, changeViewSize);

        sendPixelStreamSegments(segments, uri);
    }
}

void DisplayGroupManager::adjustPixelStreamContentDimensions(QString uri, int width, int height, bool changeViewSize)
{
    boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);
    if(cwm)
    {
        // check for updated dimensions
        boost::shared_ptr<Content> c = cwm->getContent();

        int oldWidth, oldHeight;
        c->getDimensions(oldWidth, oldHeight);

        if(width != oldWidth || height != oldHeight)
        {
            c->setDimensions(width, height);
            if (changeViewSize)
            {
                cwm->adjustSize( SIZE_1TO1 );
            }
        }
    }
}

void DisplayGroupManager::sendPixelStreamSegments(const std::vector<PixelStreamSegment> & segments, const QString& uri)
{
    assert(!segments.empty() && "sendPixelStreamSegments() received an empty vector");

    // serialize the vector
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::archive::binary_oarchive oa(oss);
        oa << segments;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_PIXELSTREAM;

    // add the truncated URI to the header
    strncpy(mh.uri, uri.toLocal8Bit().constData(), MESSAGE_HEADER_URI_LENGTH-1);

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<g_mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // broadcast the message
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void DisplayGroupManager::sendSVGStreams()
{
    // iterate through all SVG streams and send updates if needed
    std::map<QString, boost::shared_ptr<SVGStreamSource> > map = g_SVGStreamSourceFactory.getMap();

    for(std::map<QString, boost::shared_ptr<SVGStreamSource> >::iterator it = map.begin(); it != map.end(); it++)
    {
        const QString& uri = (*it).first;
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
                put_flog(LOG_DEBUG, "adding SVG stream: %s", uri.toLocal8Bit().constData());

                boost::shared_ptr<Content> c(new SVGContent(uri));
                boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));

                addContentWindowManager(cwm);
            }

            // check for updated dimensions
            QSvgRenderer svgRenderer;

            if(svgRenderer.load(imageData) != true || svgRenderer.isValid() == false)
            {
                put_flog(LOG_ERROR, "error loading %s", uri.toLocal8Bit().constData());
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
                    cwm->adjustSize( SIZE_1TO1 );
                }
            }

            int size = imageData.size();

            // send the header and the message
            MessageHeader mh;
            mh.size = size;
            mh.type = MESSAGE_TYPE_SVG_STREAM;

            // add the truncated URI to the header
            strncpy(mh.uri, uri.toLocal8Bit().constData(), MESSAGE_HEADER_URI_LENGTH-1);

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

    // will send EVT_CLOSE through InteractionState
    std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers;
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

void DisplayGroupManager::processPixelStreamSegment(QString uri, PixelStreamSegment segment)
{
    // Only accept segments if we already have a pixelstream
    if (pixelStreamSourceFactory_.contains(uri))
        pixelStreamSourceFactory_.getObject(uri)->insertSegment(segment);
}

void DisplayGroupManager::openPixelStream(QString uri, int width, int height)
{
    // Create a pixel stream receiver
    pixelStreamSourceFactory_.getObject(uri);

    // add a Content/ContentWindowManager for this URI
    boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);

    if(!cwm)
    {
        put_flog(LOG_DEBUG, "adding pixel stream: %s", uri.toLocal8Bit().constData());

        boost::shared_ptr<Content> c(new PixelStreamContent(uri));
        c->setDimensions(width, height);
        cwm = boost::shared_ptr<ContentWindowManager>(new ContentWindowManager(c));
        addContentWindowManager(cwm);
    }
}

void DisplayGroupManager::deletePixelStream(QString uri)
{
    pixelStreamSourceFactory_.removeObject(uri);

    put_flog(LOG_DEBUG, "deleting pixel stream: %s", uri.toLocal8Bit().constData());

    boost::shared_ptr<ContentWindowManager> cwm = getContentWindowManager(uri, CONTENT_TYPE_PIXEL_STREAM);
    if( cwm )
        removeContentWindowManager( cwm );
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::setSkeletons(std::vector< boost::shared_ptr<SkeletonState> > skeletons)
{
    skeletons_ = skeletons;

    sendDisplayGroup();
}
#endif

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

    boost::archive::binary_iarchive ia(iss);
    ia >> g_displayGroupManager;

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

    // now, insert all segments
    for(unsigned int i=0; i<segments.size(); i++)
    {
        g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(uri)->insertSegment(segments[i]);
    }

    // update pixel streams corresponding to new segments
    g_mainWindow->getGLWindow()->getPixelStreamFactory().getObject(uri)->updateSegmentRenderers();

    // free mpi buffer
    delete [] buf;
}

void DisplayGroupManager::receiveSVGStreams(MessageHeader messageHeader)
{
    // receive serialized data
    char * buf = new char[messageHeader.size];

    // read message into the buffer
    MPI_Bcast((void *)buf, messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // URI
    const QString uri(messageHeader.uri);

    // de-serialize...
    g_mainWindow->getGLWindow()->getSVGFactory().getObject(uri)->setImageData(QByteArray(buf, messageHeader.size));

    // free mpi buffer
    delete [] buf;
}
