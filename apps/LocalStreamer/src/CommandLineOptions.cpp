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

#include "CommandLineOptions.h"

#include <iostream>
#include <boost/program_options.hpp>

#define DEFAULT_CONFIG_FILENAME    "configuration.xml"

CommandLineOptions::CommandLineOptions(int &argc, char **argv)
    : getHelp_(false)
    , configFileName_(DEFAULT_CONFIG_FILENAME)
    , streamerType_(PS_UNKNOWN)
    , desc_("Allowed options")
{
    desc_.add_options()
        ("help", "produce help message")
        ("type", boost::program_options::value<std::string>()->default_value(""), "streamer type [webkit | dock]")
        ("config", boost::program_options::value<std::string>()->default_value(DEFAULT_CONFIG_FILENAME), "configuration xml file")
        ("url", boost::program_options::value<std::string>()->default_value(""), "webkit only: url")
    ;

    parseCommandLineArguments(argc, argv);
}

bool CommandLineOptions::getHelp() const
{
    return getHelp_;
}

const QString &CommandLineOptions::getConfigFilename() const
{
    return configFileName_;
}

PixelStreamerType CommandLineOptions::getPixelStreamerType() const
{
    return streamerType_;
}

const QString &CommandLineOptions::getUrl() const
{
    return url_;
}

void CommandLineOptions::parseCommandLineArguments(int &argc, char **argv)
{
    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc_), vm);
        boost::program_options::notify(vm);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    getHelp_ = vm.count("help");
    streamerType_ = getStreamerType(vm["type"].as<std::string>().c_str());
    configFileName_ = vm["config"].as<std::string>().c_str();
    url_ = vm["url"].as<std::string>().c_str();
}

void CommandLineOptions::showSyntax() const
{
    std::cout << desc_;
}

