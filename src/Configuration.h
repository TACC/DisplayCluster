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

        int getTileX();
        int getTileY();
        int getTileI();
        int getTileJ();

    private:

        QXmlQuery query_;

        int numTilesWidth_;
        int numTilesHeight_;
        int screenWidth_;
        int screenHeight_;
        int mullionWidth_;
        int mullionHeight_;

        int tileX_;
        int tileY_;
        int tileI_;
        int tileJ_;
};

#endif
