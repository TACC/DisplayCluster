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

#ifndef FACTORY_HPP
#define FACTORY_HPP

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <QtGui>

extern long g_frameCount;

template <class T>
class Factory {

    public:

        boost::shared_ptr<T> getObject(const QString& uri)
        {
            QMutexLocker locker(&mapMutex_);

            // see if we need to create the object
            if(map_.count(uri) == 0)
            {
                boost::shared_ptr<T> t(new T(uri));

                map_[uri] = t;
            }

            return map_[uri];
        }

        void removeObject(const QString& uri)
        {
            QMutexLocker locker(&mapMutex_);

            map_.erase(uri);
        }

        std::map<QString, boost::shared_ptr<T> > getMap()
        {
            QMutexLocker locker(&mapMutex_);

            return map_;
        }

        void clear()
        {
            QMutexLocker locker(&mapMutex_);

            map_.clear();
        }

        bool contains(const QString& uri) const
        {
            return map_.count(uri);
        }

        void clearStaleObjects()
        {
            QMutexLocker locker(&mapMutex_);

            typename std::map<QString, boost::shared_ptr<T> >::iterator it = map_.begin();

            while(it != map_.end())
            {
                if(g_frameCount - it->second->getRenderedFrameCount() > 1)
                {
                    map_.erase(it++);  // note the post increment; increments the iterator but returns original value for erase
                }
                else
                {
                    it++;
                }
            }
        }

    private:

        // mutex for thread-safe access to map
        QMutex mapMutex_;

        // all existing objects
        std::map<QString, boost::shared_ptr<T> > map_;
};

#endif
