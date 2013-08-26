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

#include <QDomElement>

Configuration::Configuration(const char * filename)
: filename_(filename)
, backgroundColor_(Qt::black)
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

    // dock start directory
    query_.setQuery("string(/configuration/dock/@directory)");
    query_.evaluateTo(&dockStartDir_);
    dockStartDir_.remove(QRegExp("[\\n\\t\\r]"));
    if( dockStartDir_.isEmpty( ))
        dockStartDir_ = QDir::homePath();

    // Background content URI
    query_.setQuery("string(/configuration/background/@uri)");
    query_.evaluateTo(&backgroundUri_);
    backgroundUri_.remove(QRegExp("[\\n\\t\\r]"));

    // Background color
    query_.setQuery("string(/configuration/background/@color)");
    if (query_.evaluateTo(&qstring))
    {
        qstring.remove(QRegExp("[\\n\\t\\r]"));

        if(QColor::isValidColor(qstring))
        {
            backgroundColor_.setNamedColor(qstring);
        }
    }

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

QString Configuration::getDockStartDir() const
{
    return dockStartDir_;
}

QString Configuration::getBackgroundUri() const
{
    return backgroundUri_;
}

QColor Configuration::getBackgroundColor() const
{
    return backgroundColor_;
}

void Configuration::setBackgroundColor(const QColor &color)
{
    backgroundColor_ = color;
}

void Configuration::setBackgroundUri(const QString& uri)
{
    backgroundUri_ = uri;
}

bool Configuration::save()
{
    QDomDocument doc("XmlDoc");
    QFile file(filename_);
    if (!file.open(QIODevice::ReadOnly))
    {
        put_flog(LOG_ERROR, "could not open configuration xml file for saving");
        return false;
    }
    doc.setContent(&file);
    file.close();

    QDomElement root = doc.documentElement();

    QDomElement background = root.firstChildElement("background");
    if (background.isNull())
    {
        background = doc.createElement("background");
        root.appendChild(background);
    }
    background.setAttribute("uri", backgroundUri_);
    background.setAttribute("color", backgroundColor_.name());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        put_flog(LOG_ERROR, "could not open configuration xml file for saving");
        return false;
    }
    QTextStream out(&file);
    out << doc.toString(4);
    file.close();
    return true;
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
