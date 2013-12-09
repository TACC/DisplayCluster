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

#include "ContentLoader.h"

#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "log.h"

ContentLoader::ContentLoader(DisplayGroupManagerPtr displayGroupManager)
    : displayGroupManager_(displayGroupManager)
{
}

bool ContentLoader::load(const QString& filename, const QPointF& windowCenterPosition, const QSizeF& windowSize)
{
    ContentPtr content = ContentFactory::getContent( filename );
    if( !content )
    {
        return false;
    }

    ContentWindowManagerPtr contentWindow( new ContentWindowManager( content ));
    displayGroupManager_->addContentWindowManager( contentWindow );

    // TODO (DISCL-21) Remove this when content dimensions request is no longer needed
    contentWindow->adjustSize( SIZE_1TO1 );

    if (!windowSize.isNull())
        contentWindow->setSize(windowSize.width(), windowSize.height());

    if (!windowCenterPosition.isNull())
        contentWindow->centerPositionAround(windowCenterPosition, true);

    return true;
}

bool ContentLoader::load(const QString& filename, const QString& parentWindowUri)
{
    // Center the new content where the dock is
    ContentWindowManagerPtr parentWindow = displayGroupManager_->getContentWindowManager(parentWindowUri);
    if (parentWindow)
    {
        return load(filename, parentWindow->getWindowCenterPosition());
    }

    put_flog(LOG_WARN, "Could not find window: ", parentWindowUri.toStdString().c_str());
    return load(filename);
}
