#include "catch/catch.hpp"
#include "value_ptr.h"

TEST_CASE( "value_ptr copy constructor", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    REQUIRE( !!a );
    cata::value_ptr<int> b( a );
    CHECK( !!a );
    CHECK( !!b );
    a.reset( nullptr );
    CHECK( !a );
    CHECK( !!b );
}

TEST_CASE( "value_ptr copy assignment", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    REQUIRE( !!a );
    cata::value_ptr<int> b;
    REQUIRE( !b );
    b = a;
    CHECK( !!a );
    CHECK( !!b );
    a.reset( nullptr );
    CHECK( !a );
    CHECK( !!b );
}

TEST_CASE( "value_ptr move constructor", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    REQUIRE( !!a );
    cata::value_ptr<int> b( std::move( a ) );
    CHECK( !a ); // NOLINT(bugprone-use-after-move)
    CHECK( !!b );
}

TEST_CASE( "value_ptr move assignment", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    cata::value_ptr<int> b = std::move( a );
    CHECK( !a ); // NOLINT(bugprone-use-after-move)
    CHECK( !!b );
}
