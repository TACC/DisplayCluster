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
#include "globals.h"
#include "Options.h"

#include <QDomElement>
#include <QtXmlPatterns>

Configuration::Configuration(const QString &filename, OptionsPtr options)
    : filename_(filename)
    , options_(options)
    , totalScreenCountX_(0)
    , totalScreenCountY_(0)
    , screenWidth_(0)
    , screenHeight_(0)
    , mullionWidth_(0)
    , mullionHeight_(0)
    , fullscreen_(false)
    , backgroundColor_(Qt::black)
{
    load();
}

void Configuration::load()
{
    put_flog(LOG_INFO, "loading %s", filename_.toLatin1().constData());

    QXmlQuery query;

    if(!query.setFocus(QUrl(filename_)))
    {
        put_flog(LOG_FATAL, "failed to load %s", filename_.toLatin1().constData());
        exit(-1);
    }

    QString queryResult;

    // get screen / mullion dimensions
    query.setQuery("string(/configuration/dimensions/@numTilesWidth)");
    if(query.evaluateTo(&queryResult))
        totalScreenCountX_ = queryResult.toInt();

    query.setQuery("string(/configuration/dimensions/@numTilesHeight)");
    if(query.evaluateTo(&queryResult))
        totalScreenCountY_ = queryResult.toInt();

    query.setQuery("string(/configuration/dimensions/@screenWidth)");
    if(query.evaluateTo(&queryResult))
        screenWidth_ = queryResult.toInt();

    query.setQuery("string(/configuration/dimensions/@screenHeight)");
    if(query.evaluateTo(&queryResult))
        screenHeight_ = queryResult.toInt();

    query.setQuery("string(/configuration/dimensions/@mullionWidth)");
    if(query.evaluateTo(&queryResult))
        mullionWidth_ = queryResult.toInt();

    query.setQuery("string(/configuration/dimensions/@mullionHeight)");
    if(query.evaluateTo(&queryResult))
        mullionHeight_ = queryResult.toInt();

    // check for fullscreen mode flag
    query.setQuery("string(/configuration/dimensions/@fullscreen)");
    if(query.evaluateTo(&queryResult))
        fullscreen_ = queryResult.toInt() != 0;

    put_flog(LOG_INFO, "dimensions: numTilesWidth = %i, numTilesHeight = %i, screenWidth = %i, screenHeight = %i, mullionWidth = %i, mullionHeight = %i. fullscreen = %i", totalScreenCountX_, totalScreenCountY_, screenWidth_, screenHeight_, mullionWidth_, mullionHeight_, fullscreen_);

    // Background content URI
    query.setQuery("string(/configuration/background/@uri)");
    if(query.evaluateTo(&queryResult))
        backgroundUri_ = queryResult.remove(QRegExp("[\\n\\t\\r]"));

    // Background color
    query.setQuery("string(/configuration/background/@color)");
    if (query.evaluateTo(&queryResult))
    {
        queryResult.remove(QRegExp("[\\n\\t\\r]"));

        QColor newColor( queryResult );
        if( newColor.isValid( ))
            backgroundColor_ = newColor;
    }
}



int Configuration::getTotalScreenCountX() const
{
    return totalScreenCountX_;
}

int Configuration::getTotalScreenCountY() const
{
    return totalScreenCountY_;
}

int Configuration::getScreenWidth() const
{
    return screenWidth_;
}

int Configuration::getScreenHeight() const
{
    return screenHeight_;
}

int Configuration::getMullionWidth() const
{
    if(options_->getEnableMullionCompensation())
    {
        return mullionWidth_;
    }
    else
    {
        return 0;
    }
}

int Configuration::getMullionHeight() const
{
    if(options_->getEnableMullionCompensation())
    {
        return mullionHeight_;
    }
    else
    {
        return 0;
    }
}

int Configuration::getTotalWidth() const
{
    return totalScreenCountX_ * screenWidth_ + (totalScreenCountX_ - 1) * getMullionWidth();
}

int Configuration::getTotalHeight() const
{
    return totalScreenCountY_ * screenHeight_ + (totalScreenCountY_ - 1) * getMullionHeight();
}

bool Configuration::getFullscreen() const
{
    return fullscreen_;
}

const QString &Configuration::getBackgroundUri() const
{
    return backgroundUri_;
}

const QColor &Configuration::getBackgroundColor() const
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

bool Configuration::save() const
{
    return save(filename_);
}

bool Configuration::save(const QString &filename) const
{
    QDomDocument doc("XmlDoc");
    QFile infile(filename_);
    if (!infile.open(QIODevice::ReadOnly))
    {
        put_flog(LOG_ERROR, "could not open configuration xml file for saving");
        return false;
    }
    doc.setContent(&infile);
    infile.close();

    QDomElement root = doc.documentElement();

    QDomElement background = root.firstChildElement("background");
    if (background.isNull())
    {
        background = doc.createElement("background");
        root.appendChild(background);
    }
    background.setAttribute("uri", backgroundUri_);
    background.setAttribute("color", backgroundColor_.name());

    QFile outfile(filename);
    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        put_flog(LOG_ERROR, "could not open configuration xml file for saving");
        return false;
    }
    QTextStream out(&outfile);
    out << doc.toString(4);
    outfile.close();
    return true;
}
