#include "catch/catch.hpp"

#include "flat_set.h"
#include "assertion_helpers.h"

#if 0
// Uncomment this to check container concepts if Boost is installed
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

BOOST_CONCEPT_ASSERT( ( boost::SimpleAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::SortedAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::UniqueAssociativeContainer<cata::flat_set<int>> ) );
BOOST_CONCEPT_ASSERT( ( boost::ReversibleContainer<cata::flat_set<int>> ) );
#endif

TEST_CASE( "flat_set", "[flat_set]" )
{
    cata::flat_set<int> s;
    s.insert( 2 );
    s.insert( 1 );
    s.insert( 4 );
    s.insert( 3 );
    std::vector<int> ref{ 1, 2, 3, 4 };
    check_containers_equal( s, ref );
    CHECK( s.count( 0 ) == 0 );
    CHECK( s.count( 1 ) == 1 );
    CHECK( s.count( 2 ) == 1 );
    CHECK( s.count( 3 ) == 1 );
    CHECK( s.count( 4 ) == 1 );
    CHECK( s.count( 5 ) == 0 );

    CHECK( s.equal_range( 0 ) == std::make_pair( s.begin(), s.begin() ) );
    CHECK( s.equal_range( 1 ) == std::make_pair( s.begin(), s.begin() + 1 ) );
    CHECK( s.equal_range( 2 ) == std::make_pair( s.begin() + 1, s.begin() + 2 ) );
    CHECK( s.equal_range( 5 ) == std::make_pair( s.end(), s.end() ) );

    CHECK( s.find( 0 ) == s.end() );
    CHECK( s.find( 1 ) == s.begin() );
    CHECK( s.find( 2 ) == s.begin() + 1 );
    CHECK( s.find( 5 ) == s.end() );
}

TEST_CASE( "reversed_flat_set_insertion", "[flat_set]" )
{
    cata::flat_set<int, std::greater<int>> s;
    s.insert( 2 );
    s.insert( 1 );
    s.insert( 4 );
    s.insert( 3 );
    std::vector<int> ref{ 4, 3, 2, 1 };
    check_containers_equal( s, ref );
    CHECK( s.count( 0 ) == 0 );
    CHECK( s.count( 1 ) == 1 );
    CHECK( s.count( 2 ) == 1 );
    CHECK( s.count( 3 ) == 1 );
    CHECK( s.count( 4 ) == 1 );
    CHECK( s.count( 5 ) == 0 );
}
