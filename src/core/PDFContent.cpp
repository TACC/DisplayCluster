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

#include "PDFContent.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "globals.h"
#include <boost/serialization/export.hpp>
#include "serializationHelpers.h"

BOOST_CLASS_EXPORT_GUID(PDFContent, "PDFContent")

CONTENT_TYPE PDFContent::getType()
{
    return CONTENT_TYPE_PDF;
}

const QStringList& PDFContent::getSupportedExtensions()
{
    static QStringList extensions;

    if (extensions.empty())
    {
        extensions << "pdf";
    }

    return extensions;
}

void PDFContent::setPageCount(int count)
{
    pageCount_ = count;
}

void PDFContent::nextPage()
{
    if (pageNumber_ < pageCount_-1)
    {
        ++pageNumber_;
        emit(pageChanged());
    }
}

void PDFContent::previousPage()
{
    if (pageNumber_ > 0)
    {
        --pageNumber_;
        emit(pageChanged());
    }
}

void PDFContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getPDFFactory().getObject(getURI())->getDimensions(width, height);
}

void PDFContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getPDFFactory().getObject(getURI())->setPage(pageNumber_);
    g_mainWindow->getGLWindow()->getPDFFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
