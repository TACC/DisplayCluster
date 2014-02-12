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

#ifndef REQUEST_H
#define REQUEST_H

#include <map>
#include <string>

namespace dcWebservice
{

struct Request
{
    /**
     * HTTP headers table
     */
    std::map<std::string, std::string> httpHeaders;

    /**
     * Originally requested URL
     */
    std::string url;

    /**
     * HTTP request method (GET, POST, PUT, DELETE, ...)
     */
    std::string method;

    /**
     * The query string, if one is present in the url. Given
     *
     * http://bbpteam.epfl.ch/dc/video&file=f.mp4&type=thumbnail
     *
     * queryString is equals to file=f.mp4&type=thumbnail
     */
    std::string queryString;

    /**
     * The path to the resource indicated in the url. This is the part
     * of the URL used for mapping a Handler. Given
     *
     * http://bbpteam.epfl.ch/dc/video&file=f.mp4&type=thumbnail
     *
     * resource is equals to /dc/video
     */
    std::string resource;

    /**
     * If a query string is present this map contains the pairs name, value
     * for each of the parameters in the query string.
     */
    std::map<std::string, std::string> parameters;

    /**
     * If the request contains data in its body the data is stored here. Normally
     * data is only inspected/used in POST requests
     */
    std::string data;
};

}

#endif // REQUEST_H
