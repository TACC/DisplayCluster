// httplib.h must come before anything that might pull in <windows.h>, since
// it needs winsock2.h first
#include "httplib.h"

#include "RestServer.h"
#include "MediaLibrary.h"
#include "RemoteSession.h"
#include "Configuration.h"
#include "QSSApp.h"
#include "main.h"
#include "log.h"

#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <chrono>
#include <cmath>
#include <future>

namespace
{
    // the most windows POST /windows/directory opens unless asked for more
    const int MAX_DEFAULT_GRID = 36;

    // the header a request carries its control lease in
    const char * LEASE_HEADER = "X-DC-Lease";

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

    // runs f on the GUI thread, where all display-group state lives, and returns its result
    WallController::Result onGuiThread(std::function<WallController::Result()> f)
    {
        auto promise = std::make_shared<std::promise<WallController::Result> >();
        std::future<WallController::Result> future = promise->get_future();

        QMetaObject::invokeMethod(g_app, [promise, f]()
        {
            try
            {
                promise->set_value(f());
            }
            catch(const std::exception & e)
            {
                promise->set_value({ 500, { { "error", e.what() } } });
            }
        }, Qt::QueuedConnection);

        if(future.wait_for(GUI_TIMEOUT) != std::future_status::ready)
        {
            return { 503, { { "error", "timed out waiting for the display" } } };
        }

        return future.get();
    }

    // like onGuiThread(), for a change to the wall: it wakes the wall first (so
    // the change applies to the real layout, not the screensaver) and restarts
    // the idle countdown after, as input at the control window would. Reads
    // don't, so a client that's only watching doesn't keep the wall awake.
    WallController::Result changeOnGuiThread(std::function<WallController::Result()> f)
    {
        return onGuiThread([f]()
        {
            QSSApplication * app = (QSSApplication *)g_app;

            app->wakeNow();
            WallController::Result result = f();
            app->restartIdleTimer();

            return result;
        });
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
    stateDir = QDir(QString::fromStdString(stateDir)).absolutePath().toStdString();

    controller_.reset(new WallController(stateDir));
    session_.reset(new RemoteSession(controller_.get()));
    media_.reset(new MediaLibrary());
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
    session_->attach();

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
        // ends the event streams, which would otherwise hold their threads open
        session_->shutdown();
        server_->stop();
        thread_.join();
    }
}

void RestServer::setupRoutes()
{
    httplib::Server & s = *server_;
    WallController * c = controller_.get();
    RemoteSession * session = session_.get();

    // runs a change to the wall, if the request holds control (see RemoteSession)
    auto change = [session](const httplib::Request & req, httplib::Response & res, std::function<WallController::Result()> f)
    {
        if(!session->renew(req.get_header_value(LEASE_HEADER)))
        {
            json holder = session->holder();

            respond(res, { 423, {
                { "error", holder.is_null() ? "take control of the wall first (POST /control)"
                                            : "the wall is controlled by " + holder["label"].get<std::string>() },
                { "controller", holder } } });
            return;
        }

        respond(res, changeOnGuiThread(f));
    };

    s.set_pre_routing_handler([this](const httplib::Request & req, httplib::Response & res)
    {
        put_flog(LOG_DEBUG, "remote API: %s %s", req.method.c_str(), req.path.c_str());

        // the UI's own files load without a token; it asks for one
        if(req.path == "/ui" || req.path.rfind("/ui/", 0) == 0)
        {
            return httplib::Server::HandlerResponse::Unhandled;
        }

        // browsers' EventSource can't send headers, so /events may carry the
        // token as ?access_token= instead
        bool authorized = token_.empty() ||
                          tokensMatch(req.get_header_value("Authorization"), "Bearer " + token_) ||
                          (req.has_param("access_token") && tokensMatch(req.get_param_value("access_token"), token_));

        if(!authorized)
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
            "GET /status", "GET /events",
            "POST /control", "POST /control/heartbeat", "DELETE /control",
            "POST /sleep", "POST /wake",
            "GET /config",
            "GET /windows", "POST /windows", "DELETE /windows",
            "GET /windows/{name}", "PATCH /windows/{name}", "DELETE /windows/{name}",
            "GET /options", "PATCH /options",
            "GET /state", "POST /state/load", "POST /state/save",
            "POST /windows/directory",
            "GET /media", "GET /media/{root}?dir=&sort=name|modified|size&order=asc|desc&offset=&limit=&all="
        } } } });
    });

    s.Get("/status", [session](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([session]() { return WallController::Result { 200, session->status() }; }));
    });

    s.Post("/control", [session](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(!parseBody(req, res, body))
        {
            return;
        }

        if((body.contains("label") && !body["label"].is_string()) || (body.contains("force") && !body["force"].is_boolean()))
        {
            respond(res, { 400, { { "error", "'label' must be a string and 'force' true or false" } } });
            return;
        }

        json lease, holder;
        if(session->acquire(body.value("label", ""), body.value("force", false), lease, holder))
        {
            respond(res, { 200, lease });
        }
        else
        {
            respond(res, { 423, { { "error", "the wall is controlled by " + holder["label"].get<std::string>() }, { "controller", holder } } });
        }
    });

    s.Post("/control/heartbeat", [session](const httplib::Request & req, httplib::Response & res)
    {
        if(session->renew(req.get_header_value(LEASE_HEADER)))
        {
            respond(res, { 200, { { "controller", session->holder() } } });
        }
        else
        {
            respond(res, { 423, { { "error", "you don't have control" }, { "controller", session->holder() } } });
        }
    });

    s.Delete("/control", [session](const httplib::Request & req, httplib::Response & res)
    {
        if(session->release(req.get_header_value(LEASE_HEADER)))
        {
            respond(res, { 200, { { "controller", nullptr } } });
        }
        else
        {
            respond(res, { 423, { { "error", "you don't have control" }, { "controller", session->holder() } } });
        }
    });

    s.Post("/sleep", [change](const httplib::Request & req, httplib::Response & res)
    {
        change(req, res, []()
        {
            ((QSSApplication *)g_app)->sleepNow();
            return WallController::Result { 200, { { "asleep", true } } };
        });
    });

    // anyone may wake the wall, controller or not: it brings back what was
    // there, changing nothing
    s.Post("/wake", [](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([]()
        {
            QSSApplication * app = (QSSApplication *)g_app;

            app->wakeNow();
            app->restartIdleTimer();

            return WallController::Result { 200, { { "asleep", false } } };
        }));
    });

    // server-sent events: the wall's current state now, then again each time it
    // changes; a comment line every 10 s otherwise. A controller that passes
    // ?lease= keeps its control alive for as long as it has the stream open.
    s.Get("/events", [session](const httplib::Request & req, httplib::Response & res)
    {
        std::string lease = req.get_param_value("lease");
        auto version = std::make_shared<uint64_t>(0);

        res.set_header("Cache-Control", "no-cache");
        res.set_header("X-Accel-Buffering", "no");

        res.set_chunked_content_provider("text/event-stream", [session, lease, version](size_t, httplib::DataSink & sink)
        {
            if(session->isShutDown())
            {
                return false;
            }

            std::string snapshot, message;

            if(session->waitForSnapshot(*version, snapshot, std::chrono::seconds(10)))
            {
                message = "data: " + snapshot + "\n\n";
            }
            else if(session->isShutDown())
            {
                return false;
            }
            else
            {
                message = ": ping\n\n";
            }

            // only a stream that's still reaching its client keeps control alive
            if(!sink.write(message.data(), message.size()))
            {
                return false;
            }

            if(!lease.empty())
            {
                session->renew(lease);
            }

            return true;
        });
    });

    s.Get("/config", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->getConfiguration(); }));
    });

    s.Get("/windows", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->listWindows(); }));
    });

    s.Post("/windows", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            change(req, res, [c, body]() { return c->openWindow(body); });
        }
    });

    s.Delete("/windows", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        change(req, res, [c]() { return c->clearWindows(); });
    });

    // window names default to the content's path, so {name} can contain slashes
    s.Get(R"(/windows/(.+))", [c](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        respond(res, onGuiThread([c, name]() { return c->getWindow(name); }));
    });

    s.Patch(R"(/windows/(.+))", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        json body;
        if(parseBody(req, res, body))
        {
            change(req, res, [c, name, body]() { return c->updateWindow(name, body); });
        }
    });

    s.Delete(R"(/windows/(.+))", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        std::string name = req.matches[1];
        change(req, res, [c, name]() { return c->closeWindow(name); });
    });

    s.Get("/options", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->getOptions(); }));
    });

    s.Patch("/options", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            change(req, res, [c, body]() { return c->setOptions(body); });
        }
    });

    s.Get("/state", [c](const httplib::Request &, httplib::Response & res)
    {
        respond(res, onGuiThread([c]() { return c->listStates(); }));
    });

    s.Post("/state/load", [c, change](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        if(parseBody(req, res, body))
        {
            std::string file;
            if(stringParam(body, "file", file, res))
            {
                change(req, res, [c, file]() { return c->loadState(file); });
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

    // browsing only touches the filesystem, so there's no need to involve the GUI thread
    MediaLibrary * m = media_.get();

    s.Get("/media", [m](const httplib::Request &, httplib::Response & res)
    {
        respond(res, m->listRoots());
    });

    s.Get(R"(/media/([^/]+))", [m](const httplib::Request & req, httplib::Response & res)
    {
        respond(res, m->list(req.matches[1], req.get_param_value("dir"), req.get_param_value("sort"),
                             req.get_param_value("order") == "desc",
                             atoi(req.get_param_value("offset").c_str()), atoi(req.get_param_value("limit").c_str()),
                             req.get_param_value("all") == "true"));
    });

    s.Post("/windows/directory", [c, m, change](const httplib::Request & req, httplib::Response & res)
    {
        json body;
        std::string root;
        if(!parseBody(req, res, body) || !stringParam(body, "root", root, res))
        {
            return;
        }

        if(body.contains("dir") && !body["dir"].is_string())
        {
            respond(res, { 400, { { "error", "'dir' must be a string" } } });
            return;
        }

        QString path;
        WallController::Result err;
        if(!m->resolveDirectory(root, body.value("dir", ""), path, err))
        {
            respond(res, err);
            return;
        }

        for(const char * key : { "cols", "rows" })
        {
            if(body.contains(key) && !body[key].is_number_integer())
            {
                respond(res, { 400, { { "error", std::string("'") + key + "' must be an integer" } } });
                return;
            }
        }

        int cols = body.value("cols", 0), rows = body.value("rows", 0);

        if(cols <= 0 || rows <= 0)
        {
            // enough cells for every file (up to a limit), shaped so each cell
            // is roughly 16:9 on this wall
            int n = std::max(1, std::min(m->countOpenable(path), MAX_DEFAULT_GRID));
            double wallAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

            cols = std::max(1, (int)std::lround(std::sqrt(n * wallAspect / (16. / 9.))));
            cols = std::min(cols, n);
            rows = (n + cols - 1) / cols;
        }

        std::string dir = path.toStdString();
        change(req, res, [c, dir, cols, rows]() { return c->openDirectory(dir, cols, rows); });
    });
}
