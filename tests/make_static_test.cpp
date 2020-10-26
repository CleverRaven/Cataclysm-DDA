#include "catch/catch.hpp"

#include "make_static.h"
#include "string_id.h"

TEST_CASE( "make_static_macro_test", "[make_static_macro]" )
{
    using test_id_type = string_id<int>;

    CHECK( 1 == STATIC( 1 ) );
    CHECK_FALSE( 2 == STATIC( 1 ) );

    CHECK( test_id_type( "test1" ) == STATIC( test_id_type( "test1" ) ) );
    CHECK_FALSE( test_id_type( "test1" ) == STATIC( test_id_type( "test2" ) ) );

    // check the static scope
    auto get_static = []() -> const auto & {
        return STATIC( std::string( "test" ) );
    };

    // entity with the same address should be returned
    CHECK( &get_static() == &get_static() );
}

TEST_CASE( "make_static_macro_benchmark", "[.][make_static_macro][benchmark]" )
{
    using test_id_type = string_id<int>;

    static const test_id_type test_id( "test" );
    BENCHMARK( "static variable outside" ) {
        return test_id.str()[0];
    };

    BENCHMARK( "static variable inside" ) {
        static const test_id_type test_id( "test" );
        return test_id.str()[0];
    };

    BENCHMARK( "inline const" ) {
        return test_id_type( "test" ).str()[0];
    };

    BENCHMARK( "static in a lambda" ) {
        return ( []() -> const auto & {
            static const test_id_type test_id( "test" );
            return test_id;
        } )().str()[0];
    };

    BENCHMARK( "make_static macro" ) {
        return STATIC( test_id_type( "test" ) ).str()[0];
    };
}