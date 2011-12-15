#ifndef FACTORY_HPP
#define FACTORY_HPP

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <QtGui>

template <class T>
class Factory {

    public:

        boost::shared_ptr<T> getObject(std::string uri)
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

        std::map<std::string, boost::shared_ptr<T> > getMap()
        {
            QMutexLocker locker(&mapMutex_);

            return map_;
        }

    private:

        // mutex for thread-safe access to map
        QMutex mapMutex_;

        // all existing objects
        std::map<std::string, boost::shared_ptr<T> > map_;
};

#endif
