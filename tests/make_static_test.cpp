#include <string>
#include <type_traits>

#include "cata_catch.h"
#include "make_static.h"
#include "string_id.h"

TEST_CASE( "make_static_macro_test", "[make_static_macro]" )
{
    using test_id_type = string_id<int>;

    CHECK( 1 == STATIC( 1 ) );
    CHECK_FALSE( 2 == STATIC( 1 ) );

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK( test_id_type( "test1" ) == STATIC( test_id_type( "test1" ) ) );
    // NOLINTNEXTLINE(cata-static-string_id-constants)
    CHECK_FALSE( test_id_type( "test1" ) == STATIC( test_id_type( "test2" ) ) );

    // check the static scope
    auto get_static = []() -> const auto & {
        return STATIC( std::string( "test" ) );
    };

    // entity with the same address should be returned
    CHECK( &get_static() == &get_static() );

    // NOLINTNEXTLINE(cata-almost-never-auto)
    const auto test11 = STATIC( "test11" );
    static_assert( std::is_same_v<std::decay_t<decltype( test11 )>, std::string>,
                   "type must be std::string" );

    CHECK( test11 == "test11" );
    CHECK( STATIC( "test11" ) == "test11" );
    CHECK( STATIC( "test11" ) == STATIC( "test11" ) );
    CHECK_FALSE( STATIC( "test11" ) == STATIC( "test12" ) );
}

TEST_CASE( "make_static_macro_benchmark_string_id", "[.][make_static_macro][benchmark]" )
{
    using test_id_type = string_id<int>;

    // NOLINTNEXTLINE(cata-static-string_id-constants)
    static const test_id_type test_id( "test" );
    BENCHMARK( "static variable outside" ) {
        return test_id.is_empty();
    };

    BENCHMARK( "static variable inside" ) {
        // NOLINTNEXTLINE(cata-static-string_id-constants)
        static const test_id_type test_id( "test" );
        return test_id.is_empty();
    };

    BENCHMARK( "inline const" ) {
        // NOLINTNEXTLINE(cata-static-string_id-constants)
        return test_id_type( "test" ).is_empty();
    };

    BENCHMARK( "static in a lambda" ) {
        return ( []() -> const auto & {
            // NOLINTNEXTLINE(cata-static-string_id-constants)
            static const test_id_type test_id( "test" );
            return test_id;
        } )().is_empty();
    };

    BENCHMARK( "make_static macro" ) {
        return STATIC( test_id_type( "test" ) ).is_empty();
    };
}
