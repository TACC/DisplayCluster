#include "WallController.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "Content.h"
#include "config.h"
#include "main.h"
#include "log.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>
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

    bool isOpenable(const QFileInfo & info)
    {
        static const QStringList movieSuffixes = { "mov", "avi", "mp4", "mkv", "mpg", "flv", "wmv" };

        QString suffix = info.suffix().toLower();

        return suffix == "svg" || suffix == "pyr" || movieSuffixes.contains(suffix)
            || QImageReader::supportedImageFormats().contains(suffix.toLatin1());
    }
}

WallController::WallController(std::string stateDir, std::string mediaDir) : stateDir_(stateDir), mediaDir_(mediaDir)
{
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
        { "zoom", cwm->getZoom() },
        { "centerX", centerX },
        { "centerY", centerY },
        { "z", z },
        { "contentWidth", contentWidth },
        { "contentHeight", contentHeight }
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
    std::vector<boost::shared_ptr<ContentWindowManager> > cwms = g_displayGroupManager->getContentWindowManagers();

    // the vector is in stacking order, back to front, so z is the index
    json windows = json::array();

    for(unsigned int i=0; i<cwms.size(); i++)
    {
        windows.push_back(describe(cwms[i], i));
    }

    return ok(windows);
}

WallController::Result WallController::getWindow(std::string name)
{
    std::vector<boost::shared_ptr<ContentWindowManager> > cwms = g_displayGroupManager->getContentWindowManagers();

    for(unsigned int i=0; i<cwms.size(); i++)
    {
        if(cwms[i]->getName() == name)
        {
            return ok(describe(cwms[i], i));
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
    bool hasX, hasY, hasW, hasH, hasZoom, hasCenterX, hasCenterY, hasHidden, hasFront;
    double x = 0, y = 0, w = 0, h = 0, zoom = 0, centerX = 0, centerY = 0;
    bool hidden = false, front = false;

    if(!optionalNumber(params, "x", hasX, x, err) || !optionalNumber(params, "y", hasY, y, err) ||
       !optionalNumber(params, "w", hasW, w, err) || !optionalNumber(params, "h", hasH, h, err) ||
       !optionalNumber(params, "zoom", hasZoom, zoom, err) ||
       !optionalNumber(params, "centerX", hasCenterX, centerX, err) ||
       !optionalNumber(params, "centerY", hasCenterY, centerY, err) ||
       !optionalBool(params, "hidden", hasHidden, hidden, err) ||
       !optionalBool(params, "front", hasFront, front, err))
    {
        return err;
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
        cwm->setName(newName);

        // nothing else about the window changed, so push the new label out explicitly
        g_displayGroupManager->sendDisplayGroup();
    }

    return getWindow(cwm->getName());
}

WallController::Result WallController::closeWindow(std::string name)
{
    boost::shared_ptr<ContentWindowManager> cwm = g_displayGroupManager->getContentWindowManagerByName(name);

    if(cwm == NULL)
    {
        return error(404, "no window named '" + name + "'");
    }

    g_displayGroupManager->removeContentWindowManager(cwm);

    return ok({ { "closed", name } });
}

WallController::Result WallController::clearWindows()
{
    g_displayGroupManager->setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> >());

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

WallController::Result WallController::listMedia(std::string dir)
{
    std::string path;

    if(!resolveUnder(mediaDir_, dir, path))
    {
        return error(400, "directories must be given relative to the media directory, " + mediaDir_);
    }

    QDir qdir(QString::fromStdString(path));

    if(!qdir.exists())
    {
        return error(404, "no such directory: " + dir);
    }

    json entries = json::array();

    // directories first, then the files the wall can open
    QFileInfoList infos = qdir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    for(const QFileInfo & info : infos)
    {
        std::string relative = QDir(QString::fromStdString(mediaDir_)).relativeFilePath(info.absoluteFilePath()).toStdString();

        if(info.isDir())
        {
            entries.push_back({ { "name", info.fileName().toStdString() }, { "type", "directory" }, { "dir", relative } });
        }
        else if(isOpenable(info))
        {
            entries.push_back({ { "name", info.fileName().toStdString() }, { "type", "file" }, { "uri", info.absoluteFilePath().toStdString() } });
        }
    }

    return ok({ { "dir", dir }, { "entries", entries } });
}
