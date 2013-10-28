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

#ifndef SERIALIZATION_HELPERS_H
#define SERIALIZATION_HELPERS_H

#include <QtGui/QColor>
#include <QtCore/QString>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost
{
namespace serialization
{

template< class Archive >
void serialize( Archive &ar, QColor& color, const unsigned int )
{
    unsigned char t;
    t = color.red(); ar & t; color.setRed(t);
    t = color.green(); ar & t; color.setGreen(t);
    t = color.blue(); ar & t; color.setBlue(t);
    t = color.alpha(); ar & t; color.setAlpha(t);
}

template< class Archive >
void save( Archive& ar, const QString& s, const unsigned int )
{
    std::string stdStr = s.toStdString();
    ar << boost::serialization::make_nvp( "value", stdStr );
}

template< class Archive >
void load( Archive& ar, QString& s, const unsigned int )
{
    std::string stdStr;
    ar >> make_nvp( "value", stdStr );
    s = QString::fromStdString(stdStr);
}

template< class Archive >
void serialize( Archive& ar, QString& s, const unsigned int version )
{
    boost::serialization::split_free( ar, s, version );
}

}
}

#endif
