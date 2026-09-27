#ifndef WALL_CONTROLLER_H
#define WALL_CONTROLLER_H

#include "json.hpp"
#include <string>
#include <boost/shared_ptr.hpp>

using json = nlohmann::json;

class ContentWindowManager;

// The operations the remote API can perform on the wall. Every method must be
// called on the GUI thread (RestServer arranges that), and returns an HTTP-style
// status plus a JSON body - the result on success, {"error": "..."} otherwise.
//
// Windows are addressed by name (see ContentWindowManager::getName()), and
// coordinates are in tile units: (0, 0) is the top-left of the wall, and
// (tilesWide, tilesHigh) the bottom-right.
class WallController
{
    public:

        struct Result
        {
            int status;
            json body;
        };

        // stateDir: directory that state files are loaded from and saved to
        // mediaDir: directory that listMedia() browses
        WallController(std::string stateDir, std::string mediaDir);

        Result getConfiguration();

        Result listWindows();
        Result getWindow(std::string name);

        // params: uri (required), name, x, y, w, h
        Result openWindow(const json & params);

        // params: any of x, y, w, h, hidden, front, zoom, centerX, centerY, name
        Result updateWindow(std::string name, const json & params);

        Result closeWindow(std::string name);
        Result clearWindows();

        Result getOptions();

        // params: any of the options getOptions() reports, as booleans
        Result setOptions(const json & params);

        // the state files in the state directory, newest first
        Result listStates();

        // file is relative to the state directory
        Result loadState(std::string file);
        Result saveState(std::string file);

        // dir is relative to the media directory; "" lists the media directory itself
        Result listMedia(std::string dir);

    private:

        std::string stateDir_;
        std::string mediaDir_;

        json describe(boost::shared_ptr<ContentWindowManager> cwm, int z);

        // resolves a client-supplied path under root, refusing any that escape it
        bool resolveUnder(std::string root, std::string relative, std::string & resolved);
};

#endif
