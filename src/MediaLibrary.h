#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include "WallController.h"
#include <QDateTime>
#include <QString>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Browses the wall host's filesystem for the remote API, confined to a set
// of named root directories. Safe to call from several HTTP threads at once;
// it only touches the filesystem, never display-group state, so it doesn't
// need the GUI thread.
//
// Roots come from DISPLAYCLUSTER_MEDIA_DIRS, a list separated like PATH (':',
// or ';' on Windows) of directories, each optionally prefixed "name=" - e.g.
// "content=/data/content:/scratch/movies". An unnamed root is named after its
// last path component. Defaults to the home directory.
class MediaLibrary
{
    public:

        MediaLibrary();

        // [{name, path}]
        WallController::Result listRoots();

        // one page of a directory: subdirectories first, then files the wall
        // can open (or all files, with all=true); sort is name, modified or size
        WallController::Result list(std::string root, std::string dir, std::string sort,
                                    bool descending, int offset, int limit, bool all);

        // resolves root + dir (relative to it) to an absolute directory path
        bool resolveDirectory(std::string root, std::string dir, QString & path, WallController::Result & err);

        // the number of files in dir the wall can open, without opening them
        int countOpenable(QString path);

    private:

        struct Root
        {
            std::string name;
            QString path;
            QString canonicalPath;
        };

        struct Entry
        {
            QString name;
            bool isDir;
            std::string type;
            qint64 size;
            QDateTime modified;
        };

        struct CachedListing
        {
            std::vector<Entry> entries;
            QDateTime dirModified;
            std::chrono::steady_clock::time_point readAt;
        };

        std::vector<Root> roots_;

        // recent directory listings, so paging and re-sorting a big directory
        // doesn't re-read it each time
        std::mutex cacheMutex_;
        std::map<QString, CachedListing> cache_;

        const Root * findRoot(std::string name);
        bool insideSomeRoot(QString canonicalPath);
        std::vector<Entry> readDirectory(QString path, bool all);
};

#endif
