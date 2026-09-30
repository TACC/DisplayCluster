// httplib.h must come before anything that might pull in <windows.h>, since
// it needs winsock2.h first
#include "httplib.h"

#include "StreamReceiver.h"
#include "ParallelPixelStream.h"
#include "MessageHeader.h"
#include "QSSApp.h"
#include "main.h"
#include "log.h"

#include <QMetaObject>
#include <turbojpeg.h>
#include <chrono>
#include <future>

namespace
{
    // how often read() gives up waiting, to notice shutdown and keep the wall awake
    const time_t POLL_SECONDS = 2;

    // how often an open stream counts as activity at the wall
    const std::chrono::seconds KEEP_AWAKE_INTERVAL(10);

    // how long closing a stream's window waits for the GUI thread
    const std::chrono::seconds GUI_TIMEOUT(30);

    // stream names travel to the wall processes in a MessageHeader's fixed-size
    // uri, so longer ones are cut, at a character boundary, to fit
    std::string fitName(std::string name)
    {
        size_t max = MESSAGE_HEADER_URI_LENGTH - 1;

        if(name.size() > max)
        {
            size_t end = max;

            // back up over UTF-8 continuation bytes (10xxxxxx)
            while(end > 0 && ((unsigned char)name[end] & 0xC0) == 0x80)
            {
                end--;
            }

            name.resize(end);
        }

        return name;
    }

    // a JPEG's dimensions, from its header; false if it isn't a JPEG
    bool jpegSize(const std::string & jpeg, int & width, int & height)
    {
        tjhandle handle = tjInitDecompress();
        int subsamp;

        bool ok = handle != NULL &&
                  tjDecompressHeader2(handle, (unsigned char *)jpeg.data(), (unsigned long)jpeg.size(), &width, &height, &subsamp) == 0 &&
                  width > 0 && height > 0;

        if(handle != NULL)
        {
            tjDestroy(handle);
        }

        return ok;
    }

    // postpones the wall's sleep, waking it if asked; doesn't wait for the GUI thread
    void keepAwake(bool wake)
    {
        QMetaObject::invokeMethod(g_app, [wake]()
        {
            QSSApplication * app = (QSSApplication *)g_app;

            if(wake)
            {
                app->wakeNow();
            }

            app->restartIdleTimer();
        }, Qt::QueuedConnection);
    }

    // closes a stream's window and drops its source, as DesktopStreamer's
    // disconnecting does; waits, so the name isn't reused before it's done
    void closeStream(const std::string & name)
    {
        auto done = std::make_shared<std::promise<void> >();
        std::future<void> future = done->get_future();

        QMetaObject::invokeMethod(g_displayGroupManager.get(), [name, done]()
        {
            g_displayGroupManager->closeStream(QString::fromStdString(name), CONTENT_TYPE_PARALLEL_PIXEL_STREAM);
            done->set_value();
        }, Qt::QueuedConnection);

        if(future.wait_for(GUI_TIMEOUT) != std::future_status::ready)
        {
            put_flog(LOG_WARN, "timed out closing stream %s", name.c_str());
        }
    }
}

StreamReceiver::StreamReceiver() : shutDown_(false)
{
}

void StreamReceiver::serve(httplib::ws::WebSocket & ws, std::string name)
{
    name = fitName(name);

    if(name.empty())
    {
        ws.close(httplib::ws::CloseStatus::PolicyViolation, "the stream needs a name");
        return;
    }

    if(!claim(name))
    {
        ws.close(httplib::ws::CloseStatus::PolicyViolation, "a stream by that name is already on the wall");
        return;
    }

    put_flog(LOG_INFO, "WebSocket stream opened: %s", name.c_str());

    keepAwake(true);
    auto lastKeptAwake = std::chrono::steady_clock::now();

    ws.set_read_timeout(POLL_SECONDS);

    int frameIndex = 0;
    std::string message;

    while(!shutDown_)
    {
        httplib::ws::ReadResult result = ws.read(message);

        if(result == httplib::ws::Fail)
        {
            break;
        }

        if(result == httplib::ws::Binary)
        {
            int width, height;

            if(!jpegSize(message, width, height))
            {
                ws.close(httplib::ws::CloseStatus::InvalidPayload, "frames must be JPEG images");
                break;
            }

            // the whole frame is one segment of a one-source parallel pixel stream
            ParallelPixelStreamSegment segment;
            segment.parameters.sourceIndex = 0;
            segment.parameters.frameIndex = frameIndex++;
            segment.parameters.x = 0;
            segment.parameters.y = 0;
            segment.parameters.width = width;
            segment.parameters.height = height;
            segment.parameters.totalWidth = width;
            segment.parameters.totalHeight = height;
            segment.imageData = QByteArray(message.data(), (int)message.size());

            // MainWindow's polling of parallel pixel streams sends it to the wall
            g_parallelPixelStreamSourceFactory.getObject(name)->insertSegment(segment);

            ws.send(std::string("ack"));
        }

        // text messages are reserved; a timeout just means no new frame
        if(std::chrono::steady_clock::now() - lastKeptAwake >= KEEP_AWAKE_INTERVAL)
        {
            keepAwake(false);
            lastKeptAwake = std::chrono::steady_clock::now();
        }
    }

    // at shutdown the wall is going away anyway, and the GUI thread may be
    // the one waiting for this thread to finish
    if(!shutDown_)
    {
        closeStream(name);
    }

    release(name);

    put_flog(LOG_INFO, "WebSocket stream closed: %s (%d frames)", name.c_str(), frameIndex);
}

void StreamReceiver::shutdown()
{
    shutDown_ = true;
}

bool StreamReceiver::claim(const std::string & name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // taken by another WebSocket, or by a DesktopStreamer stream
    if(names_.count(name) != 0 || g_parallelPixelStreamSourceFactory.findObject(name) != NULL)
    {
        return false;
    }

    names_.insert(name);
    return true;
}

void StreamReceiver::release(const std::string & name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    names_.erase(name);
}
