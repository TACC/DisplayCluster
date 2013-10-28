/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>     */
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
#ifndef STATE_H
#define STATE_H

#include "types.h"

class QDomDocument;
class QDomElement;
class QString;
class QXmlQuery;

/**
 * A state is the collection of opened contents which can be saved and
 * restored using this class. It will save positions and dimensions of each
 * content and also content-specific information which is required to restore
 * a previous state saved by the user.
 */
class State
{
public:
    State();

    /**
     * Save the given content windows with their positions, dimensions, etc. to
     * the given file in XML format.
     */
    bool saveXML( const QString& filename,
                  const ContentWindowManagerPtrs& contentWindowManagers );

    /**
     * Load content windows stored in the given XML file and return them in the
     * given list of contentWindowManagers.
     */
    bool loadXML( const QString& filename,
                  ContentWindowManagerPtrs& contentWindowManagers );
private:
    void saveVersion_( QDomDocument& doc, QDomElement& root );
    void saveContentWindow_( QDomDocument& doc, QDomElement& root,
                             const ContentWindowManagerPtr contentWindowManager );

    bool checkVersion_( QXmlQuery& query ) const;
    ContentPtr loadContent_( QXmlQuery& query, const int index ) const;
    ContentWindowManagerPtr restoreContent_( QXmlQuery& query,
                                             ContentPtr content, const int index ) const;
};

#endif
