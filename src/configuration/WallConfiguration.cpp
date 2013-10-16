/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "WallConfiguration.h"

#include <QtXmlPatterns>

#include "../log.h"

WallConfiguration::WallConfiguration(const QString &filename, OptionsPtr options, int processIndex)
    : Configuration(filename, options)
    , screenCountForCurrentProcess_(0)
{
    loadWallSettings(processIndex);
}

void WallConfiguration::loadWallSettings(int processIndex)
{
    QXmlQuery query;
    if(!query.setFocus(QUrl(filename_)))
    {
        put_flog(LOG_FATAL, "failed to load %s", filename_.toLatin1().constData());
        exit(-1);
    }

    // TODO Remove the use of C-style char[] and sprintf
    char string[1024];
    QString queryResult;

    // get host
    sprintf(string, "string(//process[%i]/@host)", processIndex);
    query.setQuery(string);
    query.evaluateTo(&queryResult);
    host_ = queryResult;

    // get display (optional attribute)
    sprintf(string, "string(//process[%i]/@display)", processIndex);
    query.setQuery(string);

    if(query.evaluateTo(&queryResult))
    {
        display_ = queryResult;
    }
    else
    {
        display_ = QString("default (:0)"); // the default
    }

    // get number of tiles for my process
    sprintf(string, "string(count(//process[%i]/screen))", processIndex);
    query.setQuery(string);
    query.evaluateTo(&queryResult);
    screenCountForCurrentProcess_ = queryResult.toInt();

    put_flog(LOG_INFO, "rank %i: %i screens", processIndex, screenCountForCurrentProcess_);

    // populate parameters for each screen
    for(int i=1; i<=screenCountForCurrentProcess_; i++)
    {
        QPoint screenPosition;
        sprintf(string, "string(//process[%i]/screen[%i]/@x)", processIndex, i);
        query.setQuery(string);
        query.evaluateTo(&queryResult);
        screenPosition.setX(queryResult.toInt());

        sprintf(string, "string(//process[%i]/screen[%i]/@y)", processIndex, i);
        query.setQuery(string);
        query.evaluateTo(&queryResult);
        screenPosition.setY(queryResult.toInt());

        screenPosition_.push_back(screenPosition);

        QPoint screenIndex;
        sprintf(string, "string(//process[%i]/screen[%i]/@i)", processIndex, i);
        query.setQuery(string);
        query.evaluateTo(&queryResult);
        screenIndex.setX(queryResult.toInt());

        sprintf(string, "string(//process[%i]/screen[%i]/@j)", processIndex, i);
        query.setQuery(string);
        query.evaluateTo(&queryResult);
        screenIndex.setY(queryResult.toInt());

        screenGlobalIndex_.push_back(screenIndex);

        put_flog(LOG_INFO, "  screen parameters: posX = %i, posY = %i, indexX = %i, indexY = %i", screenPosition.x(), screenPosition.y(), screenIndex.x(), screenIndex.y());
    }
}

const QString &WallConfiguration::getHost() const
{
    return host_;
}

const QString &WallConfiguration::getDisplay() const
{
    return display_;
}

int WallConfiguration::getScreenCount() const
{
    return screenCountForCurrentProcess_;
}

const QPoint &WallConfiguration::getScreenPosition(int screenIndex) const
{
    return screenPosition_.at(screenIndex);
}

const QPoint &WallConfiguration::getGlobalScreenIndex(int screenIndex) const
{
    return screenGlobalIndex_.at(screenIndex);
}
