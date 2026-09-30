#ifndef STREAM_RECEIVER_H
#define STREAM_RECEIVER_H

#include <atomic>
#include <mutex>
#include <set>
#include <string>

namespace httplib { namespace ws { class WebSocket; } }

// Pixel streams sent over a WebSocket (GET /stream/{name} on the remote API's
// port), for senders that can't open the raw TCP socket DesktopStreamer uses:
// a browser extension, or a web page sharing a screen. Each binary message is
// one whole frame, as JPEG; the wall shows it in a window named {name}, just
// as it would a DesktopStreamer stream, and the window closes when the
// connection does. The server answers each frame with a text message "ack",
// so a sender can keep one frame in flight rather than queueing them.
//
// A connection keeps the wall awake: it wakes the wall when it opens and
// counts as activity for as long as it stays open, frames or not, since a
// shared page that isn't changing is still being shown.
class StreamReceiver
{
    public:

        StreamReceiver();

        // serves one connection until it closes; runs on its HTTP thread
        void serve(httplib::ws::WebSocket & ws, std::string name);

        // ends every connection's serve() within a couple of seconds
        void shutdown();

    private:

        // names streaming over this receiver, so a second sender can't take
        // over a window the first one is still filling
        std::mutex mutex_;
        std::set<std::string> names_;

        std::atomic<bool> shutDown_;

        bool claim(const std::string & name);
        void release(const std::string & name);
};

#endif
