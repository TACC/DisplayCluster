#ifndef CONTENTFACTORY_H
#define CONTENTFACTORY_H

#include <QStringList>
#include <boost/shared_ptr.hpp>

class Content;

class ContentFactory
{
public:

    // get a Content object of the appropriate derived type based on the URI given
    static boost::shared_ptr<Content> getContent(const QString& uri);

    static const QStringList& getSupportedExtensions();
    static const QStringList& getSupportedFilesFilter();
    static QString getSupportedFilesFilterAsString();

};

#endif // CONTENTFACTORY_H
