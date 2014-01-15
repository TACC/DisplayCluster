/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
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

#include "CommandHandler.h"

#include "Command.h"
#include "log.h"

#include "ContentLoader.h"
#include "StateSerializationHelper.h"

CommandHandler::CommandHandler(DisplayGroupManager& displayGroupManager)
    : displayGroupManager_(displayGroupManager)
{
}

void CommandHandler::process(const QString command, const QString parentWindowUri)
{
    Command commandObject(command);

    switch(commandObject.getType())
    {
    case COMMAND_TYPE_FILE:
        handleFileCommand(commandObject.getArguments(), parentWindowUri);
        break;
    case COMMAND_TYPE_WEBBROWSER:
        handleWebbrowserCommand(commandObject.getArguments());
        break;
    case COMMAND_TYPE_UNKNOWN:
    default:
        put_flog( LOG_ERROR, "Invalid command received: '%s'",
                  command.toStdString().c_str());
        return;
    }
}

void CommandHandler::handleFileCommand(const QString& uri, const QString& parentWindowUri)
{
    const QString& extension = QFileInfo(uri).suffix().toLower();

    if( extension == "dcx" )
    {
        StateSerializationHelper(displayGroupManager_.shared_from_this()).load(uri);
    }
    else if ( ContentFactory::getSupportedExtensions().contains( extension ))
    {
        ContentLoader loader(displayGroupManager_.shared_from_this());
        loader.load(uri, parentWindowUri);
    }
    else
    {
        put_flog(LOG_WARN, "Received uri with unsupported extension: '%s'",
                 uri.toStdString().c_str());
    }
}

void CommandHandler::handleWebbrowserCommand(const QString &url)
{
    emit openWebBrowser(QPointF(), QSize(), url);
}
