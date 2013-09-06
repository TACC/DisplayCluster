#include "ContentFactory.h"

#include "log.h"
#include "main.h"

#include "Content.h"
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "SVGContent.h"
#include "MovieContent.h"

boost::shared_ptr<Content> ContentFactory::getContent(const QString& uri)
{
    // make sure file exists; otherwise use error image
    if( !QFile::exists( uri ))
    {
        put_flog(LOG_ERROR, "could not find file %s", uri.toLocal8Bit().constData());

        const QString errorImageFilename = QString( "%1%2%3" ).arg( QString::fromStdString( g_displayClusterDir ))
                                                              .arg( "/data/" )
                                                              .arg( ERROR_IMAGE_FILENAME );
        boost::shared_ptr<Content> c(new TextureContent(errorImageFilename));

        return c;
    }

    // convert to lower case for case-insensitivity in determining file type
    const QString extension = QFileInfo(uri).suffix().toLower();

    // see if this is an SVG image (must do this first, since SVG can also be read as an image directly)
    if(SVGContent::getSupportedExtensions().contains(extension))
    {
        boost::shared_ptr<Content> c(new SVGContent(uri));

        return c;
    }

    // see if this is an image
    QImageReader imageReader(uri);
    if(imageReader.canRead())
    {
        // get its size
        QSize size = imageReader.size();

        // small images will use Texture; larger images will use DynamicTexture
        boost::shared_ptr<Content> c;

        if(size.width() <= 4096 && size.height() <= 4096)
        {
            boost::shared_ptr<Content> temp(new TextureContent(uri));
            c = temp;
        }
        else
        {
            boost::shared_ptr<Content> temp(new DynamicTextureContent(uri));
            c = temp;
        }

        // set the size if valid
        if(size.isValid() == true)
        {
            c->setDimensions(size.width(), size.height());
        }

        return c;
    }

    // see if this is an image pyramid
    if(extension == "pyr")
    {
        boost::shared_ptr<Content> c(new DynamicTextureContent(uri));

        return c;
    }

    // see if this is a movie
    if(MovieContent::getSupportedExtensions().contains(extension))
    {
        boost::shared_ptr<Content> c(new MovieContent(uri));

        return c;
    }

    put_flog(LOG_ERROR, "Unsupported or invalid file %s", uri.toLocal8Bit().constData());
    const QString errorImageFilename = QString( "%1%2%3" ).arg( QString::fromStdString( g_displayClusterDir ))
                                                          .arg( "/data/" )
                                                          .arg( ERROR_IMAGE_FILENAME );
    boost::shared_ptr<Content> c(new TextureContent(errorImageFilename));
    return c;
}

const QStringList& ContentFactory::getSupportedExtensions()
{
    static QStringList extensions;

    if (extensions.empty())
    {
        extensions.append(SVGContent::getSupportedExtensions());
        extensions.append(TextureContent::getSupportedExtensions());
        extensions.append(DynamicTextureContent::getSupportedExtensions());
        extensions.append(MovieContent::getSupportedExtensions());
        extensions.removeDuplicates();
    }

    return extensions;
}

const QStringList& ContentFactory::getSupportedFilesFilter()
{
    static QStringList filters;

    if (filters.empty())
    {
        const QStringList& extensions = getSupportedExtensions();
        foreach( const QString ext, extensions )
            filters.append( "*." + ext );
    }

    return filters;
}

QString ContentFactory::getSupportedFilesFilterAsString()
{
    const QStringList& extensions = getSupportedFilesFilter();

    QString s;
    QTextStream out(&s);

    out << "Content files (";
    foreach( const QString ext, extensions )
        out << ext << " ";
    out << ")";

    return s;
}
