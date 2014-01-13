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

#include "MessageHeader.h"

#include <QDataStream>

const size_t MessageHeader::serializedSize = sizeof(quint32) + sizeof(qint32) + MESSAGE_HEADER_URI_LENGTH;

MessageHeader::MessageHeader()
    : type(MESSAGE_TYPE_NONE)
    , size(0)
{
    memset(uri, '\0', MESSAGE_HEADER_URI_LENGTH);
}

MessageHeader::MessageHeader(MessageType type, uint32_t size, const std::string& streamUri)
    : type(type)
    , size(size)
{
    memset(uri, '\0', MESSAGE_HEADER_URI_LENGTH);

    // add the truncated URI to the header
    const size_t len = streamUri.copy(uri, MESSAGE_HEADER_URI_LENGTH - 1);
    uri[len] = '\0';
}

QDataStream& operator<<(QDataStream& out, const MessageHeader& header)
{
    out << (qint32)header.type << (quint32)header.size;

    for(size_t i = 0; i < MESSAGE_HEADER_URI_LENGTH; i++)
        out << (quint8)header.uri[i];

    return out;
}

QDataStream& operator>>(QDataStream& in, MessageHeader& header)
{
    qint32 type;
    quint32 size;

    in >> type;
    header.type = (MessageType)type;
    in >> size;
    header.size = size;

    quint8 character;
    for(size_t i = 0; i < MESSAGE_HEADER_URI_LENGTH; i++)
    {
        in >> character;
        header.uri[i] = (char)character;
    }

    return in;
}

