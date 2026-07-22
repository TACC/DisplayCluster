/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "Configuration.h"
#include "log.h"
#include "main.h"

#include <fstream>
#include "json.hpp"
using json = nlohmann::json;

Configuration::Configuration(const char * filename)
{
    put_flog(LOG_INFO, "loading %s", filename);

    std::ifstream ifs(filename);

    if(ifs.good() == false)
    {
        put_flog(LOG_FATAL, "failed to load %s", filename);
        exit(-1);
    }

    json root;

    try
    {
        ifs >> root;
    }
    catch(const std::exception & e)
    {
        put_flog(LOG_FATAL, "failed to parse %s: %s", filename, e.what());
        exit(-1);
    }

    const json & dimensions = root.at("dimensions");

    // get screen / mullion dimensions
    numTilesWidth_ = dimensions.at("numTilesWidth").get<int>();
    numTilesHeight_ = dimensions.at("numTilesHeight").get<int>();
    screenWidth_ = dimensions.at("screenWidth").get<int>();
    screenHeight_ = dimensions.at("screenHeight").get<int>();
    mullionWidth_ = dimensions.at("mullionWidth").get<int>();
    mullionHeight_ = dimensions.at("mullionHeight").get<int>();

    // fullscreen mode flag (optional, defaults to disabled)
    fullscreen_ = dimensions.value("fullscreen", 0);

    put_flog(LOG_INFO, "dimensions: numTilesWidth = %i, numTilesHeight = %i, screenWidth = %i, screenHeight = %i, mullionWidth = %i, mullionHeight = %i. fullscreen = %i", numTilesWidth_, numTilesHeight_, screenWidth_, screenHeight_, mullionWidth_, mullionHeight_, fullscreen_);

    // get tile parameters (if we're not rank 0)
    if(g_mpiRank > 0)
    {
        // processes[] is 0-indexed; rank 0 is the master and isn't listed, so
        // rank N corresponds to processes[N-1], matching the old XML
        // //process[N] 1-indexed XPath convention (N there also skipped rank 0)
        int processIndex = g_mpiRank - 1;

        const json & process = root.at("processes").at(processIndex);

        host_ = process.at("host").get<std::string>();

        // display (optional)
        display_ = process.value("display", std::string("default (:0)"));

        const json & screens = process.at("screens");

        myNumTiles_ = (int)screens.size();

        put_flog(LOG_INFO, "rank %i: %i tiles", g_mpiRank, myNumTiles_);

        // populate parameters for each tile
        for(int i=0; i<myNumTiles_; i++)
        {
            const json & screen = screens.at(i);

            tileX_.push_back(screen.at("x").get<int>());
            tileY_.push_back(screen.at("y").get<int>());

            // local pixel offsets on display
            tileI_.push_back(screen.at("i").get<int>());
            tileJ_.push_back(screen.at("j").get<int>());

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
    if(g_displayGroupManager->getOptions()->getEnableMullionCompensation() == true)
    {
        return mullionWidth_;
    }
    else
    {
        return 0;
    }
}

int Configuration::getMullionHeight()
{
    if(g_displayGroupManager->getOptions()->getEnableMullionCompensation() == true)
    {
        return mullionHeight_;
    }
    else
    {
        return 0;
    }
}

bool Configuration::getFullscreen()
{
    return (fullscreen_ != 0);
}

int Configuration::getTotalWidth()
{
    return numTilesWidth_ * screenWidth_ + (numTilesWidth_ - 1) * getMullionWidth();
}

int Configuration::getTotalHeight()
{
    return numTilesHeight_ * screenHeight_ + (numTilesHeight_ - 1) * getMullionHeight();
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
