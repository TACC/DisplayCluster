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

#include "DynamicTextureContent.h"
#include "globals.h"
#include "DynamicTexture.h"
#include "MainWindow.h"
#include <boost/serialization/export.hpp>
#include "serializationHelpers.h"

BOOST_CLASS_EXPORT_GUID(DynamicTextureContent, "DynamicTextureContent")

CONTENT_TYPE DynamicTextureContent::getType()
{
    return CONTENT_TYPE_DYNAMIC_TEXTURE;
}

const QStringList& DynamicTextureContent::getSupportedExtensions()
{
    static QStringList extensions;

    if (extensions.empty())
    {
        extensions << "pyr";

        const QList<QByteArray>& imageFormats = QImageReader::supportedImageFormats();
        foreach( const QByteArray entry, imageFormats )
            extensions << entry;
    }

    return extensions;
}

void DynamicTextureContent::advance(boost::shared_ptr<ContentWindowManager> window)
{
    if( blockAdvance_ )
        return;

    // recall that advance() is called after rendering and before g_frameCount is incremented for the current frame
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->clearOldChildren(g_frameCount);
}

void DynamicTextureContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->getDimensions(width, height);
}

void DynamicTextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getDynamicTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
