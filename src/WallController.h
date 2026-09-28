#ifndef WALL_CONTROLLER_H
#define WALL_CONTROLLER_H

#include "json.hpp"
#include <string>
#include <map>
#include <QString>
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
        WallController(std::string stateDir);

        Result getConfiguration();

        Result listWindows();
        Result getWindow(std::string name);

        // params: uri (required), name, x, y, w, h
        Result openWindow(const json & params);

        // params: any of x, y, w, h, hidden, front, zoom, centerX, centerY, name,
        // filled - true makes the window as big as fits, centered and in front,
        // remembering where it was; false puts it back
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

        // opens up to cols x rows of the openable files in dir (an absolute
        // path, already vetted by MediaLibrary) tiled across the wall
        Result openDirectory(std::string dir, int cols, int rows);

    private:

        std::string stateDir_;

        json describe(boost::shared_ptr<ContentWindowManager> cwm, int z);

        // the windows in state XML, described as listWindows() does
        json stashedWindows(QString xml);

        // windows filling the wall, by name: where each was, and where filling put it
        struct Fill
        {
            double restore[4];
            double filled[4];
        };

        std::map<std::string, Fill> fills_;

        // whether the window is filling the wall (and hasn't been moved since)
        bool isFilled(boost::shared_ptr<ContentWindowManager> cwm);

        // each window's content dimensions when last listed awake, by name
        std::map<std::string, std::pair<int, int> > contentDimensions_;

        // resolves a client-supplied path under root, refusing any that escape it
        bool resolveUnder(std::string root, std::string relative, std::string & resolved);
};

#endif
