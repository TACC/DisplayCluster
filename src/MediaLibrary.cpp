#include "MediaLibrary.h"
#include "log.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <algorithm>

namespace
{
    // how long a directory listing is reused, unless the directory changes
    const std::chrono::seconds CACHE_LIFETIME(10);
    const size_t CACHE_SIZE = 16;

    const int DEFAULT_PAGE = 200;
    const int MAX_PAGE = 1000;

    WallController::Result error(int status, std::string message)
    {
        return { status, { { "error", message } } };
    }

    // what kind of content a file is, from its extension alone (so listing
    // never has to open files); "" if the wall can't open it. Mirrors the
    // checks in Content::getContent().
    std::string contentType(const QFileInfo & info)
    {
        static const QStringList movieSuffixes = { "mov", "avi", "mp4", "mkv", "mpg", "flv", "wmv" };

        QString suffix = info.suffix().toLower();

        if(suffix == "svg") return "svg";
        if(suffix == "pyr") return "pyramid";
        if(movieSuffixes.contains(suffix)) return "movie";
        if(QImageReader::supportedImageFormats().contains(suffix.toLatin1())) return "image";

        return "";
    }
}

MediaLibrary::MediaLibrary()
{
    const char * env = getenv("DISPLAYCLUSTER_MEDIA_DIRS");
    QString spec = (env != NULL && env[0] != '\0') ? QString(env) : QDir::homePath();

    for(QString item : spec.split(QDir::listSeparator(), Qt::SkipEmptyParts))
    {
        QString name;
        int equals = item.indexOf('=');

        if(equals > 0)
        {
            name = item.left(equals);
            item = item.mid(equals + 1);
        }

        if(item.startsWith("~"))
        {
            item = QDir::homePath() + item.mid(1);
        }

        QFileInfo info(item);

        if(!info.isDir())
        {
            put_flog(LOG_WARN, "media directory %s doesn't exist; ignoring it", item.toStdString().c_str());
            continue;
        }

        Root root;
        root.path = QDir::cleanPath(info.absoluteFilePath());
        root.canonicalPath = info.canonicalFilePath();

        if(name.isEmpty())
        {
            name = QFileInfo(root.path).fileName();

            if(name.isEmpty())
            {
                name = "root";   // i.e. "/"
            }
        }

        // names identify roots in URLs, so they have to be unique
        std::string unique = name.toStdString();

        for(int n=2; findRoot(unique) != NULL; n++)
        {
            unique = name.toStdString() + "-" + std::to_string(n);
        }

        root.name = unique;
        roots_.push_back(root);

        put_flog(LOG_INFO, "media root '%s' = %s", root.name.c_str(), root.path.toStdString().c_str());
    }
}

const MediaLibrary::Root * MediaLibrary::findRoot(std::string name)
{
    for(const Root & root : roots_)
    {
        if(root.name == name)
        {
            return &root;
        }
    }

    return NULL;
}

bool MediaLibrary::insideSomeRoot(QString canonicalPath)
{
    for(const Root & root : roots_)
    {
        if(canonicalPath == root.canonicalPath || canonicalPath.startsWith(root.canonicalPath + "/") ||
           root.canonicalPath == "/")
        {
            return true;
        }
    }

    return false;
}

WallController::Result MediaLibrary::listRoots()
{
    json roots = json::array();

    for(const Root & root : roots_)
    {
        roots.push_back({ { "name", root.name }, { "path", root.path.toStdString() } });
    }

    return { 200, roots };
}

bool MediaLibrary::resolveDirectory(std::string rootName, std::string dir, QString & path, WallController::Result & err)
{
    const Root * root = findRoot(rootName);

    if(root == NULL)
    {
        err = error(404, "no media root named '" + rootName + "'");
        return false;
    }

    QString relative = QString::fromStdString(dir);

    if(QDir::isAbsolutePath(relative))
    {
        err = error(400, "'dir' must be relative to the media root");
        return false;
    }

    QString resolved = QDir::cleanPath(root->path + "/" + relative);

    if(resolved != root->path && !resolved.startsWith(root->path + "/"))
    {
        err = error(400, "'dir' must be inside the media root");
        return false;
    }

    QFileInfo info(resolved);

    if(!info.isDir())
    {
        err = error(404, "no such directory: " + dir);
        return false;
    }

    // a symlink can point anywhere; follow it only into another root
    if(!insideSomeRoot(info.canonicalFilePath()))
    {
        err = error(403, "'" + dir + "' leads outside the media roots");
        return false;
    }

    path = resolved;
    return true;
}

std::vector<MediaLibrary::Entry> MediaLibrary::readDirectory(QString path, bool all)
{
    QString key = path + (all ? "|all" : "");
    QDateTime dirModified = QFileInfo(path).lastModified();

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto cached = cache_.find(key);

        if(cached != cache_.end() && cached->second.dirModified == dirModified &&
           std::chrono::steady_clock::now() - cached->second.readAt < CACHE_LIFETIME)
        {
            return cached->second.entries;
        }
    }

    std::vector<Entry> entries;

    QFileInfoList infos = QDir(path).entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    for(const QFileInfo & info : infos)
    {
        Entry entry;
        entry.name = info.fileName();
        entry.isDir = info.isDir();
        entry.size = entry.isDir ? 0 : info.size();
        entry.modified = info.lastModified();

        if(entry.isDir)
        {
            // symlinked directories only if they stay inside the roots
            if(info.isSymLink() && !insideSomeRoot(info.canonicalFilePath()))
            {
                continue;
            }

            entry.type = "directory";
        }
        else
        {
            entry.type = contentType(info);

            if(entry.type.empty() && !all)
            {
                continue;
            }
        }

        entries.push_back(entry);
    }

    std::lock_guard<std::mutex> lock(cacheMutex_);

    if(cache_.size() >= CACHE_SIZE)
    {
        // evict the oldest
        auto oldest = std::min_element(cache_.begin(), cache_.end(), [](const auto & a, const auto & b)
        {
            return a.second.readAt < b.second.readAt;
        });

        cache_.erase(oldest);
    }

    cache_[key] = { entries, dirModified, std::chrono::steady_clock::now() };

    return entries;
}

WallController::Result MediaLibrary::list(std::string rootName, std::string dir, std::string sort,
                                          bool descending, int offset, int limit, bool all)
{
    if(sort.empty())
    {
        sort = "name";
    }

    if(sort != "name" && sort != "modified" && sort != "size")
    {
        return error(400, "'sort' must be name, modified or size");
    }

    if(offset < 0)
    {
        return error(400, "'offset' can't be negative");
    }

    if(limit <= 0)
    {
        limit = DEFAULT_PAGE;
    }

    limit = std::min(limit, MAX_PAGE);

    QString path;
    WallController::Result err;

    if(!resolveDirectory(rootName, dir, path, err))
    {
        return err;
    }

    std::vector<Entry> entries = readDirectory(path, all);

    // directories always come first, whatever the order
    std::stable_sort(entries.begin(), entries.end(), [&](const Entry & a, const Entry & b)
    {
        if(a.isDir != b.isDir)
        {
            return a.isDir;
        }

        int c = 0;

        if(sort == "modified")
        {
            c = (a.modified < b.modified) ? -1 : (b.modified < a.modified) ? 1 : 0;
        }
        else if(sort == "size")
        {
            c = (a.size < b.size) ? -1 : (b.size < a.size) ? 1 : 0;
        }

        if(c == 0)
        {
            c = QString::compare(a.name, b.name, Qt::CaseInsensitive);
        }

        return descending ? c > 0 : c < 0;
    });

    // the relative path of this directory, normalized
    QString rootPath = findRoot(rootName)->path;
    QString relativeDir = QDir(rootPath).relativeFilePath(path);

    if(relativeDir == ".")
    {
        relativeDir = "";
    }

    json page = json::array();

    for(int i = offset; i < (int)entries.size() && i < offset + limit; i++)
    {
        const Entry & e = entries[i];
        QString relative = relativeDir.isEmpty() ? e.name : relativeDir + "/" + e.name;

        json item = { { "name", e.name.toStdString() }, { "type", e.type.empty() ? "other" : e.type },
                      { "modified", e.modified.toUTC().toString(Qt::ISODate).toStdString() } };

        if(e.isDir)
        {
            item["dir"] = relative.toStdString();
        }
        else
        {
            item["size"] = e.size;

            if(!e.type.empty())
            {
                item["uri"] = QDir::cleanPath(path + "/" + e.name).toStdString();
            }
        }

        page.push_back(item);
    }

    json parent = nullptr;

    if(!relativeDir.isEmpty())
    {
        int slash = relativeDir.lastIndexOf('/');
        parent = (slash < 0) ? std::string("") : relativeDir.left(slash).toStdString();
    }

    return { 200, {
        { "root", rootName },
        { "dir", relativeDir.toStdString() },
        { "parent", parent },
        { "total", entries.size() },
        { "offset", offset },
        { "entries", page }
    } };
}

int MediaLibrary::countOpenable(QString path)
{
    int n = 0;

    for(const QFileInfo & info : QDir(path).entryInfoList(QDir::Files))
    {
        if(!contentType(info).empty())
        {
            n++;
        }
    }

    return n;
}
