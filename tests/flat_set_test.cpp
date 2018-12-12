#include "catch/catch.hpp"

#include "flat_set.h"
#include "assertion_helpers.h"

#if 0
// Uncomment this to check container concepts if Boost is installed
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

BOOST_CONCEPT_ASSERT( ( boost::RandomAccessContainer<cata::flat_set<int>> ) );
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

TEST_CASE( "flat_set_ranged_operations", "[flat_set]" )
{
    std::vector<int> in1{ 0, 0, 2, 4, 6 };
    cata::flat_set<int> s( in1.begin(), in1.end() );
    {
        INFO( "constructing" );
        std::vector<int> ref1{ 0, 2, 4, 6 };
        check_containers_equal( s, ref1 );
    }
    {
        INFO( "inserting range" );
        std::vector<int> in2{ 1, 2, 3, 4 };
        s.insert( in2.begin(), in2.end() );
        std::vector<int> ref2{ 0, 1, 2, 3, 4, 6 };
        check_containers_equal( s, ref2 );
    }
    {
        INFO( "erasing range" );
        s.erase( s.lower_bound( 2 ), s.lower_bound( 4 ) );
        std::vector<int> ref3{ 0, 1, 4, 6 };
        check_containers_equal( s, ref3 );
    }
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
