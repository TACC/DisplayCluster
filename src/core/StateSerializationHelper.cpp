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

#include "StateSerializationHelper.h"

#include "DisplayGroupManager.h"
#include "State.h"
#include "StatePreview.h"
#include "globals.h"
#include "configuration/Configuration.h"

StateSerializationHelper::StateSerializationHelper(DisplayGroupManagerPtr displayGroupManager)
    : displayGroupManager_(displayGroupManager)
{
}

bool StateSerializationHelper::save(const QString& filename, const bool generatePreview)
{
    ContentWindowManagerPtrs contentWindowManagers = displayGroupManager_->getContentWindowManagers();

    if (generatePreview)
    {
        const QSize wallDimensions(g_configuration->getTotalWidth(), g_configuration->getTotalHeight());
        StatePreview filePreview(filename);
        filePreview.generateImage( wallDimensions, contentWindowManagers );
        filePreview.saveToFile();
    }

    State state;
    return state.saveXML( filename, contentWindowManagers );
}

bool StateSerializationHelper::load(const QString& filename)
{
    ContentWindowManagerPtrs contentWindowManagers;

    State state;
    if( !state.loadXML( filename, contentWindowManagers ))
        return false;

    // assign new contents vector to display group
    displayGroupManager_->setContentWindowManagers(contentWindowManagers);
    return true;
}
