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

#ifndef XMLCONFIG_HPP
#define XMLCONFIG_HPP

#include <iostream>
#include <QFile>
#include <QXmlStreamReader>
#include <vector>


class Screen
{
public:
    int i, j;
    int x, y;
    bool read(QXmlStreamReader *rdr)
    {
        auto attrs = rdr->attributes();

        if (attrs.hasAttribute("i"))
            i = attrs.value("i").toString().toUInt();
        else
            return false;
  
        if (attrs.hasAttribute("j"))
            j = attrs.value("j").toString().toUInt();
        else
            return false;
        
        if (attrs.hasAttribute("x"))
            x = attrs.value("x").toString().toUInt();
        else
            return false;
        
        if (attrs.hasAttribute("y"))
            y = attrs.value("y").toString().toUInt();
        else
            return false;
        
        while (! rdr->atEnd())
        {
            rdr->readNext();
            auto a = rdr->name().toString().toStdString();
            if (a == "screen" && rdr->isEndElement())
                break;
        }

        return true;
    }
};


class Process
{
public:
    std::string host;
    std::string display;
    std::vector<Screen> screens;
    bool read(QXmlStreamReader *rdr)
    {
        auto attrs = rdr->attributes();

        if (attrs.hasAttribute("host"))
            host = attrs.value("host").toString().toStdString();
        else
            return false;
      
        if (attrs.hasAttribute("display"))
            display = attrs.value("display").toString().toStdString();
        else
            return false;
      
        while (! rdr->atEnd())
        {
            rdr->readNext();
            auto a = rdr->name().toString().toStdString();
            if (a == "process" && rdr->isEndElement())
                break;

            if (a == "screen")
            {
                auto b = Screen();
                b.read(rdr);
                screens.push_back(b);
            }
        }

        return true;
    }
};

class XmlConfig
{
public:
    int numTilesWidth;
    int numTilesHeight;
    int screenWidth;
    int screenHeight;
    int mullionWidth;
    int mullionHeight;
    int fullscreen;
    std::vector<Process> processes;
    bool read(QXmlStreamReader *rdr)
    {
        while (!rdr->atEnd())
        {
            rdr->readNextStartElement();
            auto a = rdr->name().toString().toStdString();
            if (a == "dimensions" && rdr->isStartElement())
                break;
        }

        if (rdr->atEnd())
            return false;

        auto attrs = rdr->attributes();

        if (attrs.hasAttribute("numTilesWidth"))
            numTilesWidth = attrs.value("numTilesWidth").toUInt();
        else
            return false;
      
        if (attrs.hasAttribute("numTilesHeight"))
            numTilesHeight = attrs.value("numTilesHeight").toUInt();
        else
            return false;
      
        if (attrs.hasAttribute("screenHeight"))
            screenHeight = attrs.value("screenHeight").toUInt();
        else
            return false;
      
        if (attrs.hasAttribute("screenWidth"))
            screenWidth = attrs.value("screenWidth").toUInt();
        else
            return false;

        if (attrs.hasAttribute("mullionWidth"))
            mullionWidth = attrs.value("mullionWidth").toUInt();
        else
            return false;
      
        if (attrs.hasAttribute("mullionHeight"))
            mullionHeight = attrs.value("mullionHeight").toUInt();
        else
            return false;
      
        if (attrs.hasAttribute("fullscreen"))
            fullscreen = attrs.value("fullscreen").toUInt();
        else
            return false;

        while (! rdr->atEnd())
        {
            rdr->readNext();
            auto a = rdr->name().toString().toStdString();
            if (a == "configuration" && rdr->isEndElement())
                break;

            if (a == "process")
            {
                auto b = Process();
                b.read(rdr);
                processes.push_back(b);
            }
        }

        return true;
    }
};

class XmlConfigReader
{
public:
    XmlConfigReader(XmlConfig *c) : config(c) {}
  
    bool Read(std::string filename)
    {
      QFile file(filename.c_str());

      if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        std::cerr << "Error opening file";
        exit(1);
      }

      QXmlStreamReader rdr(&file);

      while (! rdr.atEnd())
      {
        rdr.readNextStartElement();
        auto a = rdr.name().toString().toStdString();

        if (a == "configuration")
           return config->read(&rdr);
      }

      return true;
    }
    
private:
    XmlConfig *config;
};

#if 0
int main(int argc, char *argv[])
{
  XmlConfig config;
  XmlConfigReader reader(&config);

  if (! reader.Read(argv[1]))
      std::cerr << "error reading xml\n";

  std::cerr << "numTilesWidth" << ": " << config.numTilesWidth << "\n";
  std::cerr << "numTilesHeight" << ": " << config.numTilesHeight << "\n";
  std::cerr << "screenWidth" << ": " << config.screenWidth << "\n";
  std::cerr << "screenHeight" << ": " << config.screenHeight << "\n";
  std::cerr << "mullionWidth" << ": " << config.mullionWidth << "\n";
  std::cerr << "mullionHeight" << ": " << config.mullionHeight << "\n";
  std::cerr << "fullscreen" << ": " << config.fullscreen << "\n";

  for (auto a : config.processes)
  {
      std::cerr << a.host << " " << a.display << "\n";
      for (auto b : a.screens)
          std::cerr << "    " << b.i << " " << b.j << " " << b.x << " " << b.y << "\n";
  }
}
#endif

#endif
