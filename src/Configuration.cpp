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

    // check for fullscreen mode flag
    query_.setQuery("string(/configuration/dimensions/@fullscreen)");

    if(query_.evaluateTo(&qstring) == true)
    {
        fullscreen_ = qstring.toInt();
    }
    else
    {
        // default to fullscreen disabled
        fullscreen_ = 0;
    }

    put_flog(LOG_INFO, "dimensions: numTilesWidth = %i, numTilesHeight = %i, screenWidth = %i, screenHeight = %i, mullionWidth = %i, mullionHeight = %i. fullscreen = %i", numTilesWidth_, numTilesHeight_, screenWidth_, screenHeight_, mullionWidth_, mullionHeight_, fullscreen_);

    // get tile parameters (if we're not rank 0)
    if(g_mpiRank > 0)
    {
        int processIndex = g_mpiRank;

        // get host
        sprintf(string, "string(//process[%i]/@host)", processIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        host_ = qstring.toStdString();

        // get display (optional attribute)
        sprintf(string, "string(//process[%i]/@display)", processIndex);
        query_.setQuery(string);

        if(query_.evaluateTo(&qstring) == true)
        {
            display_ = qstring.toStdString();
        }
        else
        {
            display_ = std::string("default (:0)"); // the default
        }

        // get number of tiles for my process
        sprintf(string, "string(count(//process[%i]/screen))", processIndex);
        query_.setQuery(string);
        query_.evaluateTo(&qstring);
        myNumTiles_ = qstring.toInt();

        put_flog(LOG_INFO, "rank %i: %i tiles", processIndex, myNumTiles_);

        // populate parameters for each tile
        for(int i=1; i<=myNumTiles_; i++)
        {
            sprintf(string, "string(//process[%i]/screen[%i]/@x)", processIndex, i);
            query_.setQuery(string);
            query_.evaluateTo(&qstring);
            tileX_.push_back(qstring.toInt());

            sprintf(string, "string(//process[%i]/screen[%i]/@y)", processIndex, i);
            query_.setQuery(string);
            query_.evaluateTo(&qstring);
            tileY_.push_back(qstring.toInt());

            // local pixel offsets on display
            sprintf(string, "string(//process[%i]/screen[%i]/@i)", processIndex, i);
            query_.setQuery(string);
            query_.evaluateTo(&qstring);
            tileI_.push_back(qstring.toInt());

            sprintf(string, "string(//process[%i]/screen[%i]/@j)", processIndex, i);
            query_.setQuery(string);
            query_.evaluateTo(&qstring);
            tileJ_.push_back(qstring.toInt());

            put_flog(LOG_INFO, "tile parameters: tileX = %i, tileY = %i, tileI = %i, tileJ = %i", tileX_.back(), tileY_.back(), tileI_.back(), tileJ_.back());
        }
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

bool Configuration::getFullscreen()
{
    return (fullscreen_ != 0);
}

int Configuration::getTotalWidth()
{
    return numTilesWidth_ * screenWidth_ + (numTilesWidth_ - 1) * mullionWidth_;
}

int Configuration::getTotalHeight()
{
    return numTilesHeight_ * screenHeight_ + (numTilesHeight_ - 1) * mullionHeight_;
}

std::string Configuration::getMyHost()
{
    return host_;
}

std::string Configuration::getMyDisplay()
{
    return display_;
}

int Configuration::getMyNumTiles()
{
    return myNumTiles_;
}

int Configuration::getTileX(int i)
{
    return tileX_[i];
}

int Configuration::getTileY(int i)
{
    return tileY_[i];
}

int Configuration::getTileI(int i)
{
    return tileI_[i];
}

int Configuration::getTileJ(int i)
{
    return tileJ_[i];
}
