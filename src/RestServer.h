#ifndef REST_SERVER_H
#define REST_SERVER_H

#include "WallController.h"
#include <memory>
#include <string>
#include <thread>

namespace httplib { class Server; }
class MediaLibrary;
class RemoteSession;
class StreamReceiver;
class ThumbnailCache;

// HTTP/JSON remote-control API for the wall, run on rank 0 only. Requests are
// served on cpp-httplib's own threads, so a slow or stalled client can't block
// the GUI; each one hands its work to the GUI thread (where all display-group
// state lives) and waits for the result.
//
// Configured from the environment:
//   DISPLAYCLUSTER_API_PORT    port to listen on (default 1910)
//   DISPLAYCLUSTER_API_TOKEN   bearer token every request must carry; if unset,
//                              read from ~/.displaycluster/api_token if present
//   DISPLAYCLUSTER_API_BIND    address to listen on (default: all interfaces if
//                              a token is configured, otherwise 127.0.0.1 only)
//   DISPLAYCLUSTER_STATE_DIR   where state files are loaded/saved (default
//                              ~/.displaycluster/states)
//   DISPLAYCLUSTER_MEDIA_DIRS  directories /media browses; see MediaLibrary.h
//   DISPLAYCLUSTER_UI_DIR      where the web UI's files are (default: <prefix>/ui)
class RestServer
{
    public:

        RestServer();
        ~RestServer();

        void start();
        void stop();

    private:

        std::unique_ptr<httplib::Server> server_;
        std::unique_ptr<WallController> controller_;
        std::unique_ptr<MediaLibrary> media_;
        std::unique_ptr<RemoteSession> session_;
        std::unique_ptr<ThumbnailCache> thumbnails_;
        std::unique_ptr<StreamReceiver> streams_;
        std::thread thread_;

        std::string bindAddress_;
        int port_;
        std::string token_;

        void setupRoutes();
        void mountUi();
};

#endif
