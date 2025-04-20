#include <utility>

#include "cata_catch.h"
#include "value_ptr.h"

TEST_CASE( "value_ptr_copy_constructor", "[value_ptr]" )
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

TEST_CASE( "value_ptr_copy_assignment", "[value_ptr]" )
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

TEST_CASE( "value_ptr_move_constructor", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    REQUIRE( !!a );
    cata::value_ptr<int> b( std::move( a ) );
    CHECK( !a ); // NOLINT(bugprone-use-after-move)
    CHECK( !!b );
}

TEST_CASE( "value_ptr_move_assignment", "[value_ptr]" )
{
    cata::value_ptr<int> a = cata::make_value<int>( 7 );
    cata::value_ptr<int> b = std::move( a );
    CHECK( !a ); // NOLINT(bugprone-use-after-move)
    CHECK( !!b );
}
