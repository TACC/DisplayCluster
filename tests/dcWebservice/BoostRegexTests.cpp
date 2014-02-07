
#define BOOST_TEST_MODULE CommandTests
#include <boost/test/unit_test.hpp>
#include <boost/regex.hpp>
namespace ut = boost::unit_test;

BOOST_AUTO_TEST_CASE( testMatchingRegex )
{ 
    boost::regex pattern("/video");
    boost::cmatch what;
    BOOST_CHECK_EQUAL(true, boost::regex_match("/video", what, pattern));
    BOOST_CHECK_EQUAL(false, boost::regex_match("/video/", what, pattern));
}

BOOST_AUTO_TEST_CASE( testMatchingRegex2 )
{
    boost::regex pattern("/video/(.*)");
    boost::cmatch what;
    BOOST_CHECK_EQUAL(false, boost::regex_match("/video", what, pattern));
    BOOST_CHECK_EQUAL(true, boost::regex_match("/video/", what, pattern));
    BOOST_CHECK_EQUAL(true, boost::regex_match("/video/12345", what, pattern));
}
