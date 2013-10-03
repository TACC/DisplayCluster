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

#include "State.h"
#include "ContentWindowManager.h"
#include "log.h"

#include <QtXml/QtXml>
#include <QtXmlPatterns/QXmlQuery>

#include <fstream>
#include <vector>

// increment this whenever when serialized state information changes
#define CONTENTS_FILE_VERSION_NUMBER 1


State::State()
{
}

bool State::saveXML( const QString& filename,
                       const ContentWindowManagerPtrs& contentWindowManagers )
{
    QDomDocument doc("state");
    QDomElement root = doc.createElement("state");
    doc.appendChild(root);

    saveVersion_(doc, root);

    for(size_t i=0; i<contentWindowManagers.size(); i++)
    {
        saveContentWindow_(doc, root, contentWindowManagers[i]);
    }

    QString xml = doc.toString();

    std::ofstream ofs(filename.toStdString().c_str());

    if(ofs.good( ))
    {
        ofs << xml.toStdString();
        return true;
    }
    put_flog(LOG_ERROR, "could not write state file");
    return false;
}

bool State::loadXML( const QString& filename,
                       ContentWindowManagerPtrs& contentWindowManagers )
{
    contentWindowManagers.clear();

    QXmlQuery query;

    if(!query.setFocus(QUrl(filename)))
    {
        put_flog(LOG_ERROR, "failed to load %s", filename.toLocal8Bit().constData( ));
        return false;
    }

    if( !checkVersion_( query ))
        return false;

    int numContentWindows = 0;
    query.setQuery("string(count(//state/ContentWindow))");

    QString qstring;
    if(query.evaluateTo(&qstring))
    {
        numContentWindows = qstring.toInt();
    }

    put_flog(LOG_INFO, "%i content windows", numContentWindows);
    contentWindowManagers.reserve( numContentWindows );

    for(int i=1; i<=numContentWindows; i++)
    {
        ContentPtr content = loadContent_( query, i );
        if( !content )
            continue;

        ContentWindowManagerPtr contentWindowManager = restoreContent_( query, content, i );
        if( contentWindowManager )
            contentWindowManagers.push_back( contentWindowManager );
    }

    return true;
}

void State::saveVersion_( QDomDocument& doc, QDomElement& root )
{
    int version = CONTENTS_FILE_VERSION_NUMBER;
    QDomElement v = doc.createElement("version");
    v.appendChild(doc.createTextNode(QString::number(version)));
    root.appendChild(v);
}

void State::saveContentWindow_( QDomDocument& doc, QDomElement& root,
                                const ContentWindowManagerPtr contentWindowManager )
{
    const QString& uri = contentWindowManager->getContent()->getURI();

    double x, y, w, h;
    contentWindowManager->getCoordinates(x, y, w, h);

    double centerX, centerY;
    contentWindowManager->getCenter(centerX, centerY);

    double zoom = contentWindowManager->getZoom();

    QDomElement cwmNode = doc.createElement("ContentWindow");
    root.appendChild(cwmNode);

    QDomElement n = doc.createElement("URI");
    n.appendChild(doc.createTextNode(uri));
    cwmNode.appendChild(n);

    n = doc.createElement("x");
    n.appendChild(doc.createTextNode(QString::number(x)));
    cwmNode.appendChild(n);

    n = doc.createElement("y");
    n.appendChild(doc.createTextNode(QString::number(y)));
    cwmNode.appendChild(n);

    n = doc.createElement("w");
    n.appendChild(doc.createTextNode(QString::number(w)));
    cwmNode.appendChild(n);

    n = doc.createElement("h");
    n.appendChild(doc.createTextNode(QString::number(h)));
    cwmNode.appendChild(n);

    n = doc.createElement("centerX");
    n.appendChild(doc.createTextNode(QString::number(centerX)));
    cwmNode.appendChild(n);

    n = doc.createElement("centerY");
    n.appendChild(doc.createTextNode(QString::number(centerY)));
    cwmNode.appendChild(n);

    n = doc.createElement("zoom");
    n.appendChild(doc.createTextNode(QString::number(zoom)));
    cwmNode.appendChild(n);
}

bool State::checkVersion_( QXmlQuery& query ) const
{
    QString qstring;

    int version = -1;
    query.setQuery("string(/state/version)");

    if(query.evaluateTo(&qstring))
    {
        version = qstring.toInt();
    }

    if( version != CONTENTS_FILE_VERSION_NUMBER )
    {
        put_flog( LOG_ERROR, "could not load state file with version %i, expected version %i",
                  version, CONTENTS_FILE_VERSION_NUMBER );
        return false;
    }
    return true;
}

ContentPtr State::loadContent_( QXmlQuery& query, const int index ) const
{
    char string[1024];

    QString uri;
    sprintf(string, "string(//state/ContentWindow[%i]/URI)", index);
    query.setQuery(string);

    QString qstring;
    if(query.evaluateTo(&qstring))
    {
        // remove any whitespace
        uri = qstring.trimmed();

        put_flog(LOG_DEBUG, "found content window with URI %s", uri.toLocal8Bit().constData());
    }

    if(uri.isEmpty())
        return ContentPtr();

    return ContentFactory::getContent(uri);
}

ContentWindowManagerPtr State::restoreContent_( QXmlQuery& query,
                                                ContentPtr content,
                                                const int index ) const
{
    double x, y, w, h, centerX, centerY, zoom;
    x = y = w = h = centerX = centerY = zoom = -1.;

    char string[1024];
    sprintf(string, "string(//state/ContentWindow[%i]/x)", index);
    query.setQuery(string);

    QString qstring;
    if(query.evaluateTo(&qstring))
    {
        x = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/y)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        y = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/w)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        w = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/h)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        h = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/centerX)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        centerX = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/centerY)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        centerY = qstring.toDouble();
    }

    sprintf(string, "string(//state/ContentWindow[%i]/zoom)", index);
    query.setQuery(string);

    if(query.evaluateTo(&qstring))
    {
        zoom = qstring.toDouble();
    }

    ContentWindowManagerPtr contentWindowManager(new ContentWindowManager(content));

    // now, apply settings if we got them from the XML file
    if(x != -1. || y != -1.)
    {
        contentWindowManager->setPosition(x, y);
    }

    if(w != -1. || h != -1.)
    {
        contentWindowManager->setSize(w, h);
    }

    // zoom needs to be set before center because of clamping
    if(zoom != -1.)
    {
        contentWindowManager->setZoom(zoom);
    }

    if(centerX != -1. || centerY != -1.)
    {
        contentWindowManager->setCenter(centerX, centerY);
    }
    return contentWindowManager;
}
