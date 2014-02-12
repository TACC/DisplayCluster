/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Julio Delgado <julio.delgadomangas@epfl.ch>   */
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

#ifndef MAPPER_H
#define MAPPER_H

#include <utility>
#include <list>

#include <boost/regex.hpp>

#include "types.h"

namespace dcWebservice
{

typedef boost::shared_ptr<boost::regex> RegexPtr;
typedef std::pair<RegexPtr, HandlerPtr> MappingPair;

/**
 * Maps regular expressions to Request Handlers.
 *
 * A mapper keeps a map of regular expressions to Handlers. A Handler can be
 * retrieved by providing a string that matches one of the regexes
 * present in the map. The handler returned is the first one for which a regex
 * match is found, therefore attention must be put in the order in which
 * Handlers are registered, since some regexes may define a subset of other
 * regexes.
 */
class Mapper
{
public:
    /**
     * Constructor
     */
    Mapper();

    /**
     * Register a handler with a regex defined by the pattern string passed
     * as parameter. If this method is called several times with the same
     * string pattern, previous mappings will be overwriten.
     *
     * @param pattern A string representing a valid regular expression.
     * @param handler A request handler.
     * @returns true if the mapper was added succesfuly, false otherwise.
     *
     */
    bool addHandler(const std::string& pattern, HandlerPtr handler);

    /**
     * Given a string, it returns the first handler for which positive regex
     * match is found.
     *
     * @param url A string that will be matched agains the different regexes
     * registered with the mapper.
     *
     * @returns The first handler whose regex matches the input string, or a
     * default handler if not match is found. The default handler always
     * returns a dcWebservice::Response::NotFound response, when its handle method is
     * invoked.
     */
    const Handler& getHandler(const std::string& url) const;

private:
    std::list<MappingPair> mappings;
    HandlerPtr _defaultHandler;
};

}

#endif // MAPPER_H

