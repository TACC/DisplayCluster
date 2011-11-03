#ifndef DISPLAY_GROUP_H
#define DISPLAY_GROUP_H

#include "Marker.h"
#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class ContentWindow;
class DisplayGroupGraphicsView;

enum MESSAGE_TYPE { MESSAGE_TYPE_CONTENTS, MESSAGE_TYPE_CONTENTS_DIMENSIONS, MESSAGE_TYPE_PIXELSTREAM, MESSAGE_TYPE_QUIT };

#define MESSAGE_HEADER_URI_LENGTH 64

struct MessageHeader {
    int size;
    MESSAGE_TYPE type;
    char uri[MESSAGE_HEADER_URI_LENGTH]; // optional URI related to message. needs to be a fixed size so sizeof(MessageHeader) is constant
};

class DisplayGroup : public QObject, public boost::enable_shared_from_this<DisplayGroup> {
    Q_OBJECT

    public:

        boost::shared_ptr<DisplayGroupGraphicsView> getGraphicsView();

        Marker & getMarker();

        void addContentWindow(boost::shared_ptr<ContentWindow> contentWindow);
        void removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow);
        bool hasContent(std::string uri);
        void setContentWindows(std::vector<boost::shared_ptr<ContentWindow> > contentWindows);
        std::vector<boost::shared_ptr<ContentWindow> > getContentWindows();

        void moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow);

    public slots:

        void synchronize();

        void sendDisplayGroup();
        void sendContentsDimensionsRequest();
        void sendPixelStreams();
        void sendQuit();

        void advanceContents();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & marker_;
            ar & contentWindows_;
        }

        // marker
        Marker marker_;

        // vector of all of its content windows
        std::vector<boost::shared_ptr<ContentWindow> > contentWindows_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;

        void receiveDisplayGroup(MessageHeader messageHeader);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);
};

#endif
