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

#include "MovieContent.h"
#include "globals.h"
#include "Movie.h"
#include "ContentWindowManager.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include <boost/serialization/export.hpp>
#include "serializationHelpers.h"

BOOST_CLASS_EXPORT_GUID(MovieContent, "MovieContent")

CONTENT_TYPE MovieContent::getType()
{
    return CONTENT_TYPE_MOVIE;
}

const QStringList& MovieContent::getSupportedExtensions()
{
    static QStringList extensions;

    if (extensions.empty())
    {
        extensions << "mov" << "avi" << "mp4" << "mkv" << "mpg" << "mpeg" << "flv" << "wmv";
    }

    return extensions;
}

void MovieContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->getDimensions(width, height);
}

void MovieContent::advance(ContentWindowManagerPtr window)
{
    if( blockAdvance_ )
        return;

    // window parameters
    double x, y, w, h;
    window->getCoordinates(x, y, w, h);

    // skip a frame if the Content rectangle is not visible in ANY windows; otherwise decode normally
    const bool skip = !g_mainWindow->isRegionVisible(x, y, w, h);

    boost::shared_ptr< Movie > movie = g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI());
    movie->setPause( window->getControlState() & STATE_PAUSED );
    movie->setLoop( window->getControlState() & STATE_LOOP );
    movie->nextFrame(skip);
}

void MovieContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getMovieFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
