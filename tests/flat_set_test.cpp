#include "catch/catch.hpp"

#include "flat_set.h"

#if 0
// Uncomment this to check container concepts if Boost is installed
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

BOOST_CONCEPT_ASSERT( ( boost::SimpleAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::SortedAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::UniqueAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::ReversibleContainer<cata::flat_set<int>> ) );
#endif

TEST_CASE( "flat_set_insertion", "[flat_set]" )
{
    cata::flat_set<int> s;
    s.insert( 2 );
    s.insert( 1 );
    s.insert( 4 );
    s.insert( 3 );
    std::vector<int> ref{ 1, 2, 3, 4 };
    REQUIRE( s.size() == ref.size() );
    CHECK( std::equal( s.begin(), s.end(), ref.begin() ) );
    CHECK( s.count( 0 ) == 0 );
    CHECK( s.count( 1 ) == 1 );
    CHECK( s.count( 2 ) == 1 );
    CHECK( s.count( 3 ) == 1 );
    CHECK( s.count( 4 ) == 1 );
    CHECK( s.count( 5 ) == 0 );
}
