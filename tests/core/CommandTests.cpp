/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
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


#define BOOST_TEST_MODULE CommandTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "Command.h"
#include "CommandType.h"

BOOST_AUTO_TEST_CASE( testCommandTypeToStringConversion )
{
    BOOST_CHECK_EQUAL( getCommandTypeString(COMMAND_TYPE_UNKNOWN).toStdString(), "unknown" );
    BOOST_CHECK_EQUAL( getCommandTypeString(COMMAND_TYPE_FILE).toStdString(), "file" );
    BOOST_CHECK_EQUAL( getCommandTypeString(COMMAND_TYPE_SESSION).toStdString(), "session" );
    BOOST_CHECK_EQUAL( getCommandTypeString(COMMAND_TYPE_WEBBROWSER).toStdString(), "webbrowser" );

    BOOST_CHECK_EQUAL( getCommandType(""), COMMAND_TYPE_UNKNOWN );
    BOOST_CHECK_EQUAL( getCommandType("zorglump"), COMMAND_TYPE_UNKNOWN );
    BOOST_CHECK_EQUAL( getCommandType("unknown"), COMMAND_TYPE_UNKNOWN );
    BOOST_CHECK_EQUAL( getCommandType("file"), COMMAND_TYPE_FILE );
    BOOST_CHECK_EQUAL( getCommandType("session"), COMMAND_TYPE_SESSION );
    BOOST_CHECK_EQUAL( getCommandType("webbrowser"), COMMAND_TYPE_WEBBROWSER );
}

BOOST_AUTO_TEST_CASE( testCommandConstruction )
{
    Command command(COMMAND_TYPE_WEBBROWSER, "http://www.google.com");

    BOOST_CHECK_EQUAL( command.getType(), COMMAND_TYPE_WEBBROWSER );
    BOOST_CHECK_EQUAL( command.getArguments().toStdString(), "http://www.google.com");
    BOOST_CHECK_EQUAL( command.getCommand().toStdString(), "webbrowser::http://www.google.com");
    BOOST_CHECK( command.isValid( ));
}

BOOST_AUTO_TEST_CASE( testCommandValidDeconstruction )
{
    Command command("webbrowser::http://www.google.com");

    BOOST_CHECK_EQUAL( command.getType(), COMMAND_TYPE_WEBBROWSER );
    BOOST_CHECK_EQUAL( command.getArguments().toStdString(), "http://www.google.com");
    BOOST_CHECK( command.isValid( ));
}

BOOST_AUTO_TEST_CASE( testCommandInvalidDeconstruction )
{
    {
        Command command("iruegfn09::83r(*RY$r4//froif");

        BOOST_CHECK_EQUAL( command.getType(), COMMAND_TYPE_UNKNOWN );
        BOOST_CHECK_EQUAL( command.getArguments().toStdString(), "");
        BOOST_CHECK( !command.isValid( ));
    }
    {
        Command command("otgninh");

        BOOST_CHECK_EQUAL( command.getType(), COMMAND_TYPE_UNKNOWN );
        BOOST_CHECK_EQUAL( command.getArguments().toStdString(), "");
        BOOST_CHECK( !command.isValid( ));
    }
}

