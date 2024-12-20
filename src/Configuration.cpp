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
#include "DisplayGroupManager.h"
#include "log.h"

#include "XmlConfig.hpp"

Configuration::Configuration(const char * filename)
{
    put_flog(LOG_INFO, "loading %s", filename);

    XmlConfigReader reader(&xmlcfg);
    if (! reader.Read(filename))
    {
        put_flog(LOG_FATAL, "failed to load %s", filename);
        exit(-1);
    }

    put_flog(LOG_INFO, "dimensions: numTilesWidth = %i, numTilesHeight = %i, screenWidth = %i, screenHeight = %i, mullionWidth = %i, mullionHeight = %i. fullscreen = %i", getNumTilesWidth(), getNumTilesHeight(), getScreenWidth(), getScreenHeight(), getMullionWidth(), getMullionHeight(), getFullscreen());
}

int Configuration::getNumTilesWidth()
{
    return xmlcfg.numTilesWidth;
}

int Configuration::getNumTilesHeight()
{
    return xmlcfg.numTilesHeight;
}

int Configuration::getScreenWidth()
{
    return xmlcfg.screenWidth;
}

int Configuration::getScreenHeight()
{
    return xmlcfg.screenHeight;
}

int Configuration::getMullionWidth()
{
    if(g_displayGroupManager->getOptions()->getEnableMullionCompensation() == true)
    {
        return xmlcfg.mullionWidth;
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
        return xmlcfg.mullionHeight;
    }
    else
    {
        return 0;
    }
}

bool Configuration::getFullscreen()
{
    return (xmlcfg.fullscreen != 0);
}

int Configuration::getTotalWidth()
{
    return getNumTilesWidth() * getScreenWidth() + (getNumTilesWidth() - 1) * getMullionWidth();
}

int Configuration::getTotalHeight()
{
    return getNumTilesHeight() * getScreenHeight() + (getNumTilesHeight() - 1) * getMullionHeight();
}

std::string Configuration::getMyHost()
{
    return xmlcfg.processes[g_mpiRank + 1].host;
}

std::string Configuration::getMyDisplay()
{
    return xmlcfg.processes[g_mpiRank + 1].display;
}

int Configuration::getMyNumTiles()
{
    return xmlcfg.processes[g_mpiRank + 1].screens.size();
}

int Configuration::getTileX(int i)
{
    return xmlcfg.processes[g_mpiRank + 1].screens[i].x;
}

int Configuration::getTileY(int i)
{
    return xmlcfg.processes[g_mpiRank + 1].screens[i].y;
}

int Configuration::getTileI(int i)
{
    return xmlcfg.processes[g_mpiRank + 1].screens[i].i;
}

int Configuration::getTileJ(int i)
{
    return xmlcfg.processes[g_mpiRank + 1].screens[i].j;
}
