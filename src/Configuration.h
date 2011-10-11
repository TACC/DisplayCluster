#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QtGui>
#include <QtXmlPatterns>

class Configuration {

    public:

        Configuration(const char * filename);

        int getNumTilesWidth();
        int getNumTilesHeight();
        int getScreenWidth();
        int getScreenHeight();
        int getMullionWidth();
        int getMullionHeight();

        int getMyNumTiles();
        int getTileX(int i);
        int getTileY(int i);
        int getTileI(int i);
        int getTileJ(int i);

    private:

        QXmlQuery query_;

        int numTilesWidth_;
        int numTilesHeight_;
        int screenWidth_;
        int screenHeight_;
        int mullionWidth_;
        int mullionHeight_;

        int myNumTiles_;
        std::vector<int> tileX_;
        std::vector<int> tileY_;
        std::vector<int> tileI_;
        std::vector<int> tileJ_;
};

#endif
