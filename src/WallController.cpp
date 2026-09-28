#include "WallController.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "Content.h"
#include "config.h"
#include "main.h"
#include "log.h"
#include "QSSApp.h"
#include "ThumbnailCache.h"

#include <QDir>
#include <QFileInfo>
#include <QDomDocument>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <functional>

namespace
{
    WallController::Result ok(json body)
    {
        return { 200, body };
    }

    WallController::Result error(int status, std::string message)
    {
        return { status, { { "error", message } } };
    }

    // reads an optional number from params; false (with err set) if present but not a number
    bool optionalNumber(const json & params, const char * key, bool & present, double & value, WallController::Result & err)
    {
        present = params.contains(key);

        if(present)
        {
            if(!params[key].is_number())
            {
                err = error(400, std::string("'") + key + "' must be a number");
                return false;
            }

            value = params[key].get<double>();
        }

        return true;
    }

    bool optionalBool(const json & params, const char * key, bool & present, bool & value, WallController::Result & err)
    {
        present = params.contains(key);

        if(present)
        {
            if(!params[key].is_boolean())
            {
                err = error(400, std::string("'") + key + "' must be true or false");
                return false;
            }

            value = params[key].get<bool>();
        }

        return true;
    }

    // conversions between normalized wall coordinates and tile units
    double tilesWide() { return g_configuration->getNumTilesWidth() > 0 ? g_configuration->getNumTilesWidth() : 1; }
    double tilesHigh() { return g_configuration->getNumTilesHeight() > 0 ? g_configuration->getNumTilesHeight() : 1; }
}

WallController::WallController(std::string stateDir, ThumbnailCache * thumbnails) : stateDir_(stateDir), thumbnails_(thumbnails)
{
}

json WallController::thumbnailUrl(std::string name, std::string uri)
{
    std::string version = thumbnails_->version(uri);

    if(version.empty())
    {
        return nullptr;
    }

    return "/windows/" + QUrl::toPercentEncoding(QString::fromStdString(name)).toStdString() + "/thumbnail?v=" + version;
}

json WallController::describe(boost::shared_ptr<ContentWindowManager> cwm, int z)
{
    double x, y, w, h;
    cwm->getCoordinates(x, y, w, h);

    double centerX, centerY;
    cwm->getCenter(centerX, centerY);

    int contentWidth, contentHeight;
    cwm->getContentDimensions(contentWidth, contentHeight);

    return {
        { "name", cwm->getName() },
        { "uri", cwm->getContent()->getURI() },
        { "x", x * tilesWide() },
        { "y", y * tilesHigh() },
        { "w", w * tilesWide() },
        { "h", h * tilesHigh() },
        { "hidden", cwm->getHidden() },
        { "filled", isFilled(cwm) },
        { "zoom", cwm->getZoom() },
        { "centerX", centerX },
        { "centerY", centerY },
        { "z", z },
        { "contentWidth", contentWidth },
        { "contentHeight", contentHeight },
        { "thumbnail", thumbnailUrl(cwm->getName(), cwm->getContent()->getURI()) }
    };
}

WallController::Result WallController::getConfiguration()
{
    return ok({
        { "tilesWide", g_configuration->getNumTilesWidth() },
        { "tilesHigh", g_configuration->getNumTilesHeight() },
        { "pixelsWide", g_configuration->getTotalWidth() },
        { "pixelsHigh", g_configuration->getTotalHeight() },
        { "screenWidth", g_configuration->getScreenWidth() },
        { "screenHeight", g_configuration->getScreenHeight() },
        { "mullionWidth", g_configuration->getMullionWidth() },
        { "mullionHeight", g_configuration->getMullionHeight() },
        { "version", DISPLAYCLUSTER_GIT_VERSION }
    });
}

WallController::Result WallController::listWindows()
{
    QSSApplication * app = (QSSApplication *)g_app;

    // asleep, the display group holds just the screensaver; report the layout
    // it stashed, which is what waking will bring back
    if(app->isAsleep() && !g_displayGroupManager->state_stack.empty())
    {
        return ok(stashedWindows(g_displayGroupManager->state_stack.top()));
    }

    std::vector<boost::shared_ptr<ContentWindowManager> > cwms = g_displayGroupManager->getContentWindowManagers();

    // the vector is in stacking order, back to front, so z is the index
    json windows = json::array();

    for(unsigned int i=0; i<cwms.size(); i++)
    {
        json window = describe(cwms[i], i);

        contentDimensions_[window["name"]] = { window["contentWidth"], window["contentHeight"] };

        windows.push_back(window);
    }

    return ok(windows);
}

json WallController::stashedWindows(QString xml)
{
    json windows = json::array();

    QDomDocument doc;

    if(!doc.setContent(xml))
    {
        return windows;
    }

    QDomNodeList nodes = doc.documentElement().elementsByTagName("ContentWindow");

    auto number = [](QDomElement parent, const char * tag, double fallback)
    {
        QDomElement e = parent.firstChildElement(tag);
        return e.isNull() ? fallback : e.text().toDouble();
    };

    for(int i=0; i<nodes.size(); i++)
    {
        QDomElement node = nodes.at(i).toElement();

        std::string uri = node.firstChildElement("URI").text().trimmed().toStdString();
        QDomElement nameElem = node.firstChildElement("name");
        std::string name = nameElem.isNull() ? uri : nameElem.text().trimmed().toStdString();

        // the state XML doesn't record content dimensions; use what they were
        // when last seen awake, if known
        auto dims = contentDimensions_.find(name);

        windows.push_back({
            { "name", name },
            { "uri", uri },
            { "x", number(node, "x", 0) * tilesWide() },
            { "y", number(node, "y", 0) * tilesHigh() },
            { "w", number(node, "w", 0) * tilesWide() },
            { "h", number(node, "h", 0) * tilesHigh() },
            { "hidden", number(node, "hidden", 0) != 0 },
            { "filled", fills_.count(name) > 0 },
            { "zoom", number(node, "zoom", 1) },
            { "centerX", number(node, "centerX", 0.5) },
            { "centerY", number(node, "centerY", 0.5) },
            { "z", i },
            { "contentWidth", dims != contentDimensions_.end() ? dims->second.first : 0 },
            { "contentHeight", dims != contentDimensions_.end() ? dims->second.second : 0 },
            { "thumbnail", thumbnailUrl(name, uri) }
        });
    }

    return windows;
}

WallController::Result WallController::getWindow(std::string name)
{
    for(const json & window : listWindows().body)
    {
        if(window["name"] == name)
        {
            return ok(window);
        }
    }

    return error(404, "no window named '" + name + "'");
}

WallController::Result WallController::openWindow(const json & params)
{
    if(!params.contains("uri") || !params["uri"].is_string() || params["uri"].get<std::string>().empty())
    {
        return error(400, "'uri' is required");
    }

    std::string uri = params["uri"];

    std::string name;

    if(params.contains("name"))
    {
        if(!params["name"].is_string() || params["name"].get<std::string>().empty())
        {
            return error(400, "'name' must be a non-empty string");
        }

        name = params["name"];

        if(g_displayGroupManager->getContentWindowManagerByName(name) != NULL)
        {
            return error(409, "a window named '" + name + "' already exists");
        }
    }

    Result err;
    bool hasX, hasY, hasW, hasH;
    double x = 0, y = 0, w = 0, h = 0;

    if(!optionalNumber(params, "x", hasX, x, err) || !optionalNumber(params, "y", hasY, y, err) ||
       !optionalNumber(params, "w", hasW, w, err) || !optionalNumber(params, "h", hasH, h, err))
    {
        return err;
    }

    if((hasW && w <= 0) || (hasH && h <= 0))
    {
        return error(400, "'w' and 'h' must be positive");
    }

    // Content::getContent() would substitute an error image for a missing file
    if(!QFileInfo::exists(QString::fromStdString(uri)))
    {
        return error(404, "no such file: " + uri);
    }

    boost::shared_ptr<Content> c = Content::getContent(uri);

    if(c == NULL)
    {
        return error(415, "unsupported content type: " + uri);
    }

    boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(c));
    cwm->setName(name);

    // anything not given keeps ContentWindowManager's default placement
    double cx, cy, cw, ch;
    cwm->getCoordinates(cx, cy, cw, ch);

    cwm->setCoordinates(hasX ? x / tilesWide() : cx, hasY ? y / tilesHigh() : cy,
                        hasW ? w / tilesWide() : cw, hasH ? h / tilesHigh() : ch);

    put_flog(LOG_INFO, "opening %s", uri.c_str());

    g_displayGroupManager->addContentWindowManager(cwm);

    Result r = ok(describe(cwm, g_displayGroupManager->getContentWindowManagers().size() - 1));
    r.status = 201;
    return r;
}

WallController::Result WallController::updateWindow(std::string name, const json & params)
{
    boost::shared_ptr<ContentWindowManager> cwm = g_displayGroupManager->getContentWindowManagerByName(name);

    if(cwm == NULL)
    {
        return error(404, "no window named '" + name + "'");
    }

    // validate everything before changing anything
    Result err;
    bool hasX, hasY, hasW, hasH, hasZoom, hasCenterX, hasCenterY, hasHidden, hasFront, hasFilled;
    double x = 0, y = 0, w = 0, h = 0, zoom = 0, centerX = 0, centerY = 0;
    bool hidden = false, front = false, filled = false;

    if(!optionalNumber(params, "x", hasX, x, err) || !optionalNumber(params, "y", hasY, y, err) ||
       !optionalNumber(params, "w", hasW, w, err) || !optionalNumber(params, "h", hasH, h, err) ||
       !optionalNumber(params, "zoom", hasZoom, zoom, err) ||
       !optionalNumber(params, "centerX", hasCenterX, centerX, err) ||
       !optionalNumber(params, "centerY", hasCenterY, centerY, err) ||
       !optionalBool(params, "hidden", hasHidden, hidden, err) ||
       !optionalBool(params, "front", hasFront, front, err) ||
       !optionalBool(params, "filled", hasFilled, filled, err))
    {
        return err;
    }

    if(hasFilled && (hasX || hasY || hasW || hasH))
    {
        return error(400, "'filled' can't be combined with x, y, w or h");
    }

    if((hasW && w <= 0) || (hasH && h <= 0))
    {
        return error(400, "'w' and 'h' must be positive");
    }

    if(hasZoom && zoom <= 0)
    {
        return error(400, "'zoom' must be positive");
    }

    std::string newName;

    if(params.contains("name"))
    {
        if(!params["name"].is_string() || params["name"].get<std::string>().empty())
        {
            return error(400, "'name' must be a non-empty string");
        }

        newName = params["name"];

        boost::shared_ptr<ContentWindowManager> other = g_displayGroupManager->getContentWindowManagerByName(newName);

        if(other != NULL && other != cwm)
        {
            return error(409, "a window named '" + newName + "' already exists");
        }
    }

    if(hasX || hasY || hasW || hasH)
    {
        double cx, cy, cw, ch;
        cwm->getCoordinates(cx, cy, cw, ch);

        cwm->setCoordinates(hasX ? x / tilesWide() : cx, hasY ? y / tilesHigh() : cy,
                            hasW ? w / tilesWide() : cw, hasH ? h / tilesHigh() : ch);

        // placed explicitly, so no longer filling the wall; keep this placement
        fills_.erase(cwm->getName());
    }

    if(hasFilled && filled && !isFilled(cwm))
    {
        Fill fill;
        cwm->getCoordinates(fill.restore[0], fill.restore[1], fill.restore[2], fill.restore[3]);

        std::vector<boost::shared_ptr<ContentWindowManager> > order = g_displayGroupManager->getContentWindowManagers();
        fill.z = std::find(order.begin(), order.end(), cwm) - order.begin();

        // as big as fits - setCoordinates() keeps the content's shape if the
        // wall constrains it - and centered
        double fw, fh;
        cwm->setCoordinates(0., 0., 1., 1.);
        cwm->getSize(fw, fh);
        cwm->setPosition((1. - fw) / 2., (1. - fh) / 2.);

        cwm->getCoordinates(fill.filled[0], fill.filled[1], fill.filled[2], fill.filled[3]);
        fills_[cwm->getName()] = fill;

        g_displayGroupManager->moveContentWindowManagerToFront(cwm);
    }
    else if(hasFilled && !filled && isFilled(cwm))
    {
        const Fill & fill = fills_[cwm->getName()];
        cwm->setCoordinates(fill.restore[0], fill.restore[1], fill.restore[2], fill.restore[3]);

        // back to its old place in the stack: raise it, then everything that
        // belongs above it, in order (only moves to the front, which the
        // control window and render processes already follow)
        std::vector<boost::shared_ptr<ContentWindowManager> > others = g_displayGroupManager->getContentWindowManagers();
        others.erase(std::remove(others.begin(), others.end(), cwm), others.end());

        g_displayGroupManager->moveContentWindowManagerToFront(cwm);

        for(size_t i = std::min(fill.z, others.size()); i < others.size(); i++)
        {
            g_displayGroupManager->moveContentWindowManagerToFront(others[i]);
        }

        fills_.erase(cwm->getName());
    }

    // zoom needs to be set before center because of clamping
    if(hasZoom)
    {
        cwm->setZoom(zoom);
    }

    if(hasCenterX || hasCenterY)
    {
        double cx, cy;
        cwm->getCenter(cx, cy);

        cwm->setCenter(hasCenterX ? centerX : cx, hasCenterY ? centerY : cy);
    }

    if(hasHidden)
    {
        cwm->setHidden(hidden);
    }

    if(hasFront && front)
    {
        g_displayGroupManager->moveContentWindowManagerToFront(cwm);
    }

    if(!newName.empty() && newName != cwm->getName())
    {
        auto fill = fills_.find(cwm->getName());

        if(fill != fills_.end())
        {
            fills_[newName] = fill->second;
            fills_.erase(fill);
        }

        cwm->setName(newName);

        // nothing else about the window changed, so push the new label out explicitly
        g_displayGroupManager->sendDisplayGroup();
    }

    return getWindow(cwm->getName());
}

bool WallController::isFilled(boost::shared_ptr<ContentWindowManager> cwm)
{
    auto fill = fills_.find(cwm->getName());

    if(fill == fills_.end())
    {
        return false;
    }

    // moved or resized since - by anyone, the control window included - so
    // that placement stands and there's nothing to restore
    double x, y, w, h;
    cwm->getCoordinates(x, y, w, h);

    const double * f = fill->second.filled;

    if(std::abs(x - f[0]) > 1e-9 || std::abs(y - f[1]) > 1e-9 || std::abs(w - f[2]) > 1e-9 || std::abs(h - f[3]) > 1e-9)
    {
        fills_.erase(fill);
        return false;
    }

    return true;
}

WallController::Result WallController::closeWindow(std::string name)
{
    boost::shared_ptr<ContentWindowManager> cwm = g_displayGroupManager->getContentWindowManagerByName(name);

    if(cwm == NULL)
    {
        return error(404, "no window named '" + name + "'");
    }

    g_displayGroupManager->removeContentWindowManager(cwm);
    fills_.erase(name);

    return ok({ { "closed", name } });
}

WallController::Result WallController::clearWindows()
{
    g_displayGroupManager->setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> >());
    fills_.clear();

    return ok(json::object());
}

namespace
{
    // every display option the API exposes; all but constrainAspectRatio live in Options
    struct OptionAccess
    {
        const char * name;
        std::function<bool()> get;
        std::function<void(bool)> set;
    };

    std::vector<OptionAccess> optionAccessors()
    {
        boost::shared_ptr<Options> o = g_displayGroupManager->getOptions();

        return {
            { "constrainAspectRatio", []() { return g_mainWindow->getConstrainAspectRatio(); }, [](bool b) { g_mainWindow->constrainAspectRatio(b); } },
            { "showWindowBorders", [o]() { return o->getShowWindowBorders(); }, [o](bool b) { o->setShowWindowBorders(b); } },
            { "showContentLabels", [o]() { return o->getShowContentLabels(); }, [o](bool b) { o->setShowContentLabels(b); } },
            { "showTestPattern", [o]() { return o->getShowTestPattern(); }, [o](bool b) { o->setShowTestPattern(b); } },
            { "enableMullionCompensation", [o]() { return o->getEnableMullionCompensation(); }, [o](bool b) { o->setEnableMullionCompensation(b); } },
            { "showZoomContext", [o]() { return o->getShowZoomContext(); }, [o](bool b) { o->setShowZoomContext(b); } },
            { "enableStreamingSynchronization", [o]() { return o->getEnableStreamingSynchronization(); }, [o](bool b) { o->setEnableStreamingSynchronization(b); } },
            { "showStreamingSegments", [o]() { return o->getShowStreamingSegments(); }, [o](bool b) { o->setShowStreamingSegments(b); } },
            { "showStreamingStatistics", [o]() { return o->getShowStreamingStatistics(); }, [o](bool b) { o->setShowStreamingStatistics(b); } }
        };
    }
}

WallController::Result WallController::getOptions()
{
    json options = json::object();

    for(const OptionAccess & option : optionAccessors())
    {
        options[option.name] = option.get();
    }

    return ok(options);
}

WallController::Result WallController::setOptions(const json & params)
{
    std::vector<OptionAccess> options = optionAccessors();

    // validate everything before changing anything
    for(auto it = params.begin(); it != params.end(); ++it)
    {
        auto known = std::find_if(options.begin(), options.end(), [&](const OptionAccess & o) { return it.key() == o.name; });

        if(known == options.end())
        {
            return error(400, "unknown option '" + it.key() + "'");
        }

        if(!it.value().is_boolean())
        {
            return error(400, "'" + it.key() + "' must be true or false");
        }
    }

    for(const OptionAccess & option : options)
    {
        if(params.contains(option.name))
        {
            option.set(params[option.name].get<bool>());
        }
    }

    return getOptions();
}

bool WallController::resolveUnder(std::string root, std::string relative, std::string & resolved)
{
    QString rootPath = QDir::cleanPath(QDir(QString::fromStdString(root)).absolutePath());

    if(QDir::isAbsolutePath(QString::fromStdString(relative)))
    {
        return false;
    }

    QString path = QDir::cleanPath(rootPath + "/" + QString::fromStdString(relative));

    if(path != rootPath && !path.startsWith(rootPath + "/"))
    {
        return false;
    }

    // a symlink inside root could still point outside it
    QFileInfo info(path);

    if(info.exists())
    {
        QString canonicalRoot = QFileInfo(rootPath).canonicalFilePath();
        QString canonicalPath = info.canonicalFilePath();

        if(canonicalPath != canonicalRoot && !canonicalPath.startsWith(canonicalRoot + "/"))
        {
            return false;
        }
    }

    resolved = path.toStdString();
    return true;
}

WallController::Result WallController::listStates()
{
    json states = json::array();

    QDir dir(QString::fromStdString(stateDir_));

    // newest first, since that's usually the one wanted
    QFileInfoList infos = dir.entryInfoList(QStringList() << "*.dcx", QDir::Files, QDir::Time);

    for(const QFileInfo & info : infos)
    {
        states.push_back({
            { "file", info.fileName().toStdString() },
            { "modified", info.lastModified().toUTC().toString(Qt::ISODate).toStdString() }
        });
    }

    return ok(states);
}

WallController::Result WallController::loadState(std::string file)
{
    std::string path;

    if(file.empty() || !resolveUnder(stateDir_, file, path))
    {
        return error(400, "state files must be given relative to the state directory, " + stateDir_);
    }

    if(!QFileInfo(QString::fromStdString(path)).isFile())
    {
        return error(404, "no such state file: " + file);
    }

    fills_.clear();

    if(!g_displayGroupManager->loadStateXMLFile(path))
    {
        return error(400, "could not load state file: " + file);
    }

    return listWindows();
}

WallController::Result WallController::saveState(std::string file)
{
    // match the control window's Save State
    if(QFileInfo(QString::fromStdString(file)).suffix() != "dcx")
    {
        file += ".dcx";
    }

    std::string path;

    if(!resolveUnder(stateDir_, file, path))
    {
        return error(400, "state files must be given relative to the state directory, " + stateDir_);
    }

    QDir().mkpath(QFileInfo(QString::fromStdString(path)).absolutePath());

    if(!g_displayGroupManager->saveStateXMLFile(path))
    {
        return error(500, "could not write state file: " + file);
    }

    return ok({ { "saved", file } });
}

WallController::Result WallController::openDirectory(std::string dir, int cols, int rows)
{
    int opened = g_mainWindow->openContentsGrid(QString::fromStdString(dir), cols, rows);

    Result r = ok({ { "opened", opened }, { "cols", cols }, { "rows", rows } });
    r.status = 201;
    return r;
}
