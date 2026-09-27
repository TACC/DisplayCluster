// httplib.h must come before anything that might pull in <windows.h>, since
// it needs winsock2.h first
#include "httplib.h"

#include "RestServer.h"
#include "QSSApp.h"
#include "main.h"
#include "log.h"

#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <chrono>
#include <future>

namespace
{
    // how long a request waits for the GUI thread before giving up
    const std::chrono::seconds GUI_TIMEOUT(30);

    std::string getenvOr(const char * name, std::string fallback)
    {
        const char * value = getenv(name);
        return (value != NULL && value[0] != '\0') ? std::string(value) : fallback;
    }

    std::string displayClusterHome()
    {
        return QDir::homePath().toStdString() + "/.displaycluster";
    }

    std::string readToken()
    {
        std::string token = getenvOr("DISPLAYCLUSTER_API_TOKEN", "");

        if(token.empty())
        {
            QFile file(QString::fromStdString(displayClusterHome() + "/api_token"));

            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                token = QString(file.readAll()).trimmed().toStdString();
            }
        }

        return token;
    }

    // compares every byte regardless of where the first mismatch is, so response
    // timing doesn't reveal how much of a guessed token was right
    bool tokensMatch(const std::string & a, const std::string & b)
    {
        if(a.size() != b.size())
        {
            return false;
        }

        unsigned char diff = 0;

        for(size_t i=0; i<a.size(); i++)
        {
            diff |= (unsigned char)(a[i] ^ b[i]);
        }

        return diff == 0;
    }

    void respond(httplib::Response & res, const WallController::Result & result)
    {
        res.status = result.status;
        res.set_content(result.body.dump(), "application/json");
    }

    // runs f on the GUI thread and returns its result. Every request wakes the
    // wall from the screensaver, as activity at the control window would - even
    // reads, since while asleep the real windows are stashed away (see
    // QSSApplication::sleep_start()) and a read would see only the screensaver.
    WallController::Result onGuiThread(std::function<WallController::Result()> f)
    {
        auto promise = std::make_shared<std::promise<WallController::Result> >();
        std::future<WallController::Result> future = promise->get_future();

        QMetaObject::invokeMethod(g_app, [promise, f]()
        {
            QSSApplication * app = (QSSApplication *)g_app;

            app->pause_screensaver();

            try
            {
                promise->set_value(f());
            }
            catch(const std::exception & e)
            {
                promise->set_value({ 500, { { "error", e.what() } } });
            }

            app->resume_screensaver();
        }, Qt::QueuedConnection);

        if(future.wait_for(GUI_TIMEOUT) != std::future_status::ready)
        {
            return { 503, { { "error", "timed out waiting for the display" } } };
        }

        return future.get();
    }

    // parses a request body that must be a JSON object; an empty body counts as {}
    bool parseBody(const httplib::Request & req, httplib::Response & res, json & body)
    {
        if(req.body.empty())
        {
            body = json::object();
            return true;
        }

        body = json::parse(req.body, nullptr, false);

        if(body.is_discarded() || !body.is_object())
        {
            respond(res, { 400, { { "error", "request body must be a JSON object" } } });
            return false;
        }

        return true;
    }

    // reads a required, non-empty string member of a request body
    bool stringParam(const json & body, const char * key, std::string & value, httplib::Response & res)
    {
        if(!body.contains(key) || !body[key].is_string() || body[key].get<std::string>().empty())
        {
            respond(res, { 400, { { "error", std::string("'") + key + "' is required" } } });
            return false;
        }

        value = body[key];
        return true;
    }
}

RestServer::RestServer()
{
    port_ = atoi(getenvOr("DISPLAYCLUSTER_API_PORT", "1910").c_str());
    token_ = readToken();
    bindAddress_ = getenvOr("DISPLAYCLUSTER_API_BIND", token_.empty() ? "127.0.0.1" : "0.0.0.0");

    std::string stateDir = getenvOr("DISPLAYCLUSTER_STATE_DIR", displayClusterHome() + "/states");
    std::string mediaDir = getenvOr("DISPLAYCLUSTER_MEDIA_DIR", QDir::homePath().toStdString());

    // absolute, so paths handed back to clients can be passed straight to open
    stateDir = QDir(QString::fromStdString(stateDir)).absolutePath().toStdString();
    mediaDir = QDir(QString::fromStdString(mediaDir)).absolutePath().toStdString();

    controller_.reset(new WallController(stateDir, mediaDir));
    server_.reset(new httplib::Server());

    // requests are small JSON documents
    server_->set_payload_max_length(1024 * 1024);

    setupRoutes();
}

RestServer::~RestServer()
{
    stop();
}

void RestServer::start()
{
    if(token_.empty() && bindAddress_ != "127.0.0.1" && bindAddress_ != "localhost" && bindAddress_ != "::1")
    {
        put_flog(LOG_WARN, "remote API listening on %s with no token: anyone who can reach port %d can control the wall", bindAddress_.c_str(), port_);
    }

    if(!server_->bind_to_port(bindAddress_, port_))
    {
        put_flog(LOG_ERROR, "remote API could not listen on %s:%d", bindAddress_.c_str(), port_);
        return;
    }

    put_flog(LOG_INFO, "remote API listening on %s:%d (%s)", bindAddress_.c_str(), port_, token_.empty() ? "no token" : "token required");

    thread_ = std::thread([this]() { server_->listen_after_bind(); });
}

void RestServer::stop()
{
    if(thread_.joinable())
    {
        server_->stop();
        thread_.join();
    }
}

void RestServer::setupRoutes()
{
    httplib::Server & s = *server_;
    WallController * c = controller_.get();

    s.set_pre_routing_handler([this](const httplib::Request & req, httplib::Response & res)
    {
        put_flog(LOG_DEBUG, "remote API: %s %s", req.method.c_str(), req.path.c_str());

        if(!token_.empty() && !tokensMatch(req.get_header_value("Authorization"), "Bearer " + token_))
        {
            res.set_header("WWW-Authenticate", "Bearer");
            respond(res, { 401, { { "error", "missing or incorrect token" } } });
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    s.set_error_handler([](const httplib::Request &, httplib::Response & res)
    {
        // leave bodies our handlers already wrote alone
        if(res.body.empty())
        {
            respond(res, { res.status, { { "error", httplib::status_message(res.status) } } });
        }
    });

    s.Get("/", [](const httplib::Request &, httplib::Response & res)
    {
        respond(res, { 200, { { "endpoints", {
            "GET /config",
            "GET /windows", "POST /windows", "DELETE /windows",
            "GET /windows/{name}", "PATCH /windows/{name}", "DELETE /windows/{name}",
            "GET /options", "PATCH /options",
            "GET /state", "POST /state/load", "POST /state/save",
            "GET /media?dir={dir}"
        } } } });
    });

    s.Get("/config", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->getConfiguration(); }));
    });

    s.Get("/windows", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->listWindows(); }));
    });

    s.Post("/windows", [c](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            respond(res, onGuiThread([c, body]() { return c->openWindow(body); }));
        }
    });

    s.Delete("/windows", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->clearWindows(); }));
    });

    // window names default to the content's path, so {name} can contain slashes
    s.Get(R"(/windows/(.+))", [c](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        respond(res, onGuiThread([c, name]() { return c->getWindow(name); }));
    });

    s.Patch(R"(/windows/(.+))", [c](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        json body;
        if(parseBody(req, res, body))
        {
            respond(res, onGuiThread([c, name, body]() { return c->updateWindow(name, body); }));
        }
    });

    s.Delete(R"(/windows/(.+))", [c](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        respond(res, onGuiThread([c, name]() { return c->closeWindow(name); }));
    });

    s.Get("/options", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->getOptions(); }));
    });

    s.Patch("/options", [c](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            respond(res, onGuiThread([c, body]() { return c->setOptions(body); }));
        }
    });

    s.Get("/state", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->listStates(); }));
    });

    s.Post("/state/load", [c](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            std::string file;
            if(stringParam(body, "file", file, res))
            {
                respond(res, onGuiThread([c, file]() { return c->loadState(file); }));
            }
        }
    });

    s.Post("/state/save", [c](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            std::string file;
            if(stringParam(body, "file", file, res))
            {
                respond(res, onGuiThread([c, file]() { return c->saveState(file); }));
            }
        }
    });

    s.Get("/media", [c](const httplib::Request & req, httplib::Response & res)
    {
        // only touches the filesystem, so there's no need to involve the GUI thread
        respond(res, c->listMedia(req.get_param_value("dir")));
    });
}
