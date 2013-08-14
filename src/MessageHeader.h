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

#ifndef MESSAGE_HEADER_H
#define MESSAGE_HEADER_H

#ifdef _WIN32
    typedef __int32 int32_t;
#else
    #include <stdint.h>
#endif

enum MESSAGE_TYPE
{
    MESSAGE_TYPE_CONTENTS,
    MESSAGE_TYPE_CONTENTS_DIMENSIONS,
    MESSAGE_TYPE_PIXELSTREAM,
    MESSAGE_TYPE_PIXELSTREAM_DIMENSIONS_CHANGED,
    MESSAGE_TYPE_PARALLEL_PIXELSTREAM,
    MESSAGE_TYPE_SVG_STREAM,
    MESSAGE_TYPE_BIND_INTERACTION,
    MESSAGE_TYPE_BIND_INTERACTION_EX,
    MESSAGE_TYPE_BIND_INTERACTION_REPLY,
    MESSAGE_TYPE_INTERACTION,
    MESSAGE_TYPE_FRAME_CLOCK,
    MESSAGE_TYPE_QUIT,
    MESSAGE_TYPE_ACK,
    MESSAGE_TYPE_NONE
};

#define MESSAGE_HEADER_URI_LENGTH 64

struct MessageHeader {
    int32_t size;
    MESSAGE_TYPE type;
    char uri[MESSAGE_HEADER_URI_LENGTH]; // optional URI related to message. needs to be a fixed size so sizeof(MessageHeader) is constant
};

#endif
