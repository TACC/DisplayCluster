#ifndef THUMBNAIL_CACHE_H
#define THUMBNAIL_CACHE_H

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <string>

// Small JPEG previews of content files, for the remote UI. Made on demand,
// on whichever (HTTP) thread asks - never the GUI thread - and kept, keyed
// by file and modification time, so each is made once per version of the
// file. Safe to use from any thread.
//
// Images and SVGs only, for now: movies, image pyramids and streams have no
// thumbnail, and neither do images too big to decode cheaply (see
// MAX_DECODE_PIXELS) - the UI shows a placeholder for those.
class ThumbnailCache
{
    public:

        static const int SIZE = 256;    // longest side, in pixels

        // the version of uri's thumbnail (its modification time), or "" if it
        // can't have one; cheap after the first call for each file version
        std::string version(std::string uri);

        // the thumbnail as JPEG bytes, made now if need be; empty if it can't
        // have one
        QByteArray get(std::string uri);

    private:

        struct Entry
        {
            QDateTime modified;
            bool possible;          // whether this file can have a thumbnail
            QByteArray jpeg;        // made on first get()
        };

        std::mutex mutex_;
        std::map<std::string, Entry> entries_;
        std::list<std::string> recent_;    // most recently used first, for eviction

        // how many thumbnails are being made right now, to bound memory use
        std::condition_variable slotFree_;
        int making_ = 0;

        // looks up (or starts) the entry for uri's current version; false if
        // the file's gone
        bool entry(std::string uri, Entry & out);

        bool possible(QString path);
        QByteArray make(QString path);
        void touch(std::string uri);
};

#endif
