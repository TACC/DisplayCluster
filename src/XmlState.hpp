#include <iostream>
#include <sstream>
#include <QFile>
#include <QBuffer>
#include <QXmlStreamReader>
#include <vector>


class XmlContentWindow
{
public:
    XmlContentWindow() {}

    XmlContentWindow(boost::shared_ptr<ContentWindowManager> cwm)
    {
        cwm->getCoordinates(x, y, w, h);
        cwm->getCenter(centerX, centerY);
        uri = cwm->getContent()->getURI();
        zoom = cwm->getZoom();
        selected = cwm->getSelected();
    }

    XmlContentWindow(QXmlStreamReader *rdr)
    {
        rdr->readNext();
        auto a = rdr->name().toString().toStdString();
        while (!(a == "ContentWindow" && rdr->isEndElement()) || rdr->atEnd())
        {
            if (a == "URI")
            {
                auto a = rdr->readElementText();
                uri = a.toStdString();
            }
            else if (a == "x")
            {
                auto a = rdr->readElementText();
                x = a.toFloat();
            }
            else if (a == "y")
            {
                auto a = rdr->readElementText();
                y = a.toFloat();
            }
            else if (a == "w")
            {
                auto a = rdr->readElementText();
                w = a.toFloat();
            }
            else if (a == "h")
            {
                auto a = rdr->readElementText();
                h = a.toFloat();
            }
            else if (a == "centerX")
            {
                auto a = rdr->readElementText();
                centerX = a.toFloat();
            }
            else if (a == "centerY")
            {
                auto a = rdr->readElementText();
                centerY = a.toFloat();
            }
            else if (a == "zoom")
            {
                auto a = rdr->readElementText();
                zoom = a.toFloat();
            }
            else if (a == "selected")
            {
                auto a = rdr->readElementText();
                selected = a.toUInt();
            }
            
            rdr->readNext();
            a = rdr->name().toString().toStdString();
        }
    }

    bool write(QXmlStreamWriter *wrtr)
    {
        wrtr->writeStartElement("ContentWindow");
        
        wrtr->writeTextElement("URI", uri.c_str());

#define W(a) { std::stringstream ss; ss << a;  wrtr->writeTextElement(#a, ss.str().c_str()); }

        W(x)
        W(y)
        W(w)
        W(h)
        W(centerX)
        W(centerY)
        W(zoom)
        W(selected)

        wrtr->writeEndElement();
        return true;
    }


    double w, h;
    double x, y;
    double centerX, centerY;
    double zoom;
    int   selected;
    std::string uri;
};

class XmlState
{
public:

    XmlState() {}

    XmlState(DisplayGroupManager *dgm)
    {
        version = 1;
        for (auto cwm : dgm->getContentWindowManagers())
            contentWindows.push_back(new XmlContentWindow(cwm));
    }

    XmlState(QString& qstr)
    {
        QBuffer buffer;

        buffer.open(QIODevice::ReadWrite);
        buffer.write(qstr.toUtf8());

        Read(&buffer);
    }

    XmlState(std::string filename)
    {
      QFile file(filename.c_str());

      if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        std::cerr << "Error opening file";
        exit(1);
      }

      Read(&file);
    }
        
    int version;
    std::vector<XmlContentWindow *> contentWindows;

    bool Read(QIODevice *dev)
    {
        QXmlStreamReader rdr(dev);

        while (! rdr.atEnd())
        {
            rdr.readNextStartElement();
            auto a = rdr.name().toString().toStdString();

            if (a == "state")
               return read(&rdr);
        }

        return true;
    }

    bool Write(QIODevice *dev)
    {
        QXmlStreamWriter wrtr(dev);
        wrtr.setAutoFormatting(true);

        wrtr.writeStartDocument();

        if (!  write(&wrtr))
            return false;

        wrtr.writeEndDocument();
        return true;
    }

    bool Write(QString& qstr)
    {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        
        return Write(&buffer);
    }


    bool Write(std::string filename)
    {
        QFile file(filename.c_str());

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
          std::cerr << "Error opening file";
          exit(1);
        }

        return Write(&file);
    }

private:
    bool read(QXmlStreamReader *rdr)
    {
        while (! rdr->atEnd())
        {
            rdr->readNext();
            auto a = rdr->name().toString().toStdString();
            if (a == "state" && rdr->isEndElement())
                break;

            if (a == "version")
            {
                auto a = rdr->readElementText();
                version = a.toUInt();
            }
            if (a == "ContentWindow")
            {
                auto b = new XmlContentWindow(rdr);
                contentWindows.push_back(b);
            }
        }

        return true;
    }

    bool write(QXmlStreamWriter *wrtr)
    {
        std::stringstream ss;

        wrtr->writeStartElement("state");

        ss << version;
        wrtr->writeTextElement("version", ss.str().c_str());

        for (auto c : contentWindows)
            if (! c->write(wrtr))
                return false;

        wrtr->writeEndElement();

        return true;
    }
};

