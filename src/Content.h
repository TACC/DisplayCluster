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

#ifndef CONTENT_H
#define CONTENT_H

#include "ContentFactory.h"
#include "ContentType.h"

#include <string>
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class ContentWindowManager;

class Content : public QObject {
    Q_OBJECT

    public:

        Content(QString uri = "");

        const QString& getURI() const;

        virtual CONTENT_TYPE getType() = 0;

        void getDimensions(int &width, int &height);
        void setDimensions(int width, int height);
        virtual void getFactoryObjectDimensions(int &width, int &height) = 0;
        void render(boost::shared_ptr<ContentWindowManager> window);
        void blockAdvance( bool block ) { blockAdvance_ = block; }

        // virtual method for implementing actions on advancing to a new frame
        // useful when a process has multiple GLWindows
        virtual void advance(boost::shared_ptr<ContentWindowManager>) { }

    signals:

        void dimensionsChanged(int width, int height);

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & uri_;
            ar & width_;
            ar & height_;
            ar & blockAdvance_;
        }

        QString uri_;
        int width_;
        int height_;
        bool blockAdvance_;

        virtual void renderFactoryObject(float tX, float tY, float tW, float tH) = 0;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Content)

// typedef needed for SIP
typedef boost::shared_ptr<Content> pContent;

class pyContent {

    public:

        pyContent(const char * str)
        {
            boost::shared_ptr<Content> c(ContentFactory::getContent(QString::fromAscii(str)));
            ptr_ = c;
        }

        pyContent(boost::shared_ptr<Content> c)
        {
            ptr_ = c;
        }

        boost::shared_ptr<Content> get()
        {
            return ptr_;
        }

        const char * getURI()
        {
            return ptr_->getURI().toAscii();
        }

    protected:

        boost::shared_ptr<Content> ptr_;
};

#endif
