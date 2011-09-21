#ifndef MOVIE_FACTORY_H
#define MOVIE_FACTORY_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

class Movie;

class MovieFactory {

    public:

        boost::shared_ptr<Movie> getMovie(std::string uri);

    private:

        // all existing movies
        std::map<std::string, boost::shared_ptr<Movie> > map_;
};

#endif
