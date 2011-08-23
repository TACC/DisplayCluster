#include "Configuration.h"
#include "log.h"
#include "main.h"

Configuration::Configuration(const char * filename)
{
    put_flog(LOG_INFO, "loading %s", filename);

    if(query_.setFocus(QUrl(filename)) == false)
    {
        put_flog(LOG_FATAL, "failed to load %s", filename);
        exit(-1);
    }

    // temp strings
    char string[1024];
    QString qstring;

    // get screen / mullion dimensions
    query_.setQuery("string(/configuration/dimensions/@numTilesWidth)");
    query_.evaluateTo(&qstring);
    numTilesWidth_ = qstring.toInt();

    query_.setQuery("string(/configuration/dimensions/@numTilesHeight)");
    query_.evaluateTo(&qstring);
    numTilesHeight_ = qstring.toInt();

    query_.setQuery("string(/configuration/dimensions/@screenWidth)");
    query_.evaluateTo(&qstring);
    screenWidth_ = qstring.toInt();

    query_.setQuery("string(/configuration/dimensions/@screenHeight)");
    query_.evaluateTo(&qstring);
    screenHeight_ = qstring.toInt();

    query_.setQuery("string(/configuration/dimensions/@mullionWidth)");
    query_.evaluateTo(&qstring);
    mullionWidth_ = qstring.toInt();

    query_.setQuery("string(/configuration/dimensions/@mullionHeight)");
    query_.evaluateTo(&qstring);
    mullionHeight_ = qstring.toInt();

    put_flog(LOG_INFO, "dimensions: numTilesWidth = %i, numTilesHeight = %i, screenWidth = %i, screenHeight = %i, mullionWidth = %i, mullionHeight = %i", numTilesWidth_, numTilesHeight_, screenWidth_, screenHeight_, mullionWidth_, mullionHeight_);

    // get tile dimension (if we're not rank 0)
    if(g_mpiRank > 0)
    {
        int tileIndex = g_mpiRank;

        sprintf(string, "string(//tile[%i]/screen/@x)", tileIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        tileX_ = qstring.toInt();

        sprintf(string, "string(//tile[%i]/screen/@y)", tileIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        tileY_ = qstring.toInt();

        // local pixel offsets on display
        sprintf(string, "string(//tile[%i]/screen/@i)", tileIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        tileI_ = qstring.toInt();

        sprintf(string, "string(//tile[%i]/screen/@j)", tileIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        tileJ_ = qstring.toInt();

        put_flog(LOG_INFO, "tile parameters: tileX = %i, tileY = %i, tileI = %i, tileJ = %i", tileX_, tileY_, tileI_, tileJ_);
    }
}

int Configuration::getNumTilesWidth()
{
    return numTilesWidth_;
}

int Configuration::getNumTilesHeight()
{
    return numTilesHeight_;
}

int Configuration::getScreenWidth()
{
    return screenWidth_;
}

int Configuration::getScreenHeight()
{
    return screenHeight_;
}

int Configuration::getMullionWidth()
{
    return mullionWidth_;
}

int Configuration::getMullionHeight()
{
    return mullionHeight_;
}

int Configuration::getTileX()
{
    return tileX_;
}

int Configuration::getTileY()
{
    return tileY_;
}

int Configuration::getTileI()
{
    return tileI_;
}

int Configuration::getTileJ()
{
    return tileJ_;
}
