#include "MovieFactory.h"
#include "Movie.h"

boost::shared_ptr<Movie> MovieFactory::getMovie(std::string uri)
{
    // see if we need to create the movie
    if(map_.count(uri) == 0)
    {
        boost::shared_ptr<Movie> t(new Movie(uri));

        map_[uri] = t;
    }

    return map_[uri];
}
